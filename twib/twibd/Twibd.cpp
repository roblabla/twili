#include "Twibd.hpp"

#include<stdio.h>
#include<stdlib.h>

#include<libusb.h>
#include<msgpack11.hpp>

#include "Protocol.hpp"
#include "config.hpp"
#include "err.hpp"

#include <iostream>
#include <ostream>
#include <string>

namespace twili {
namespace twibd {

Twibd::Twibd() : local_client(std::make_shared<LocalClient>(this)), usb(this), tcp(this)
#ifndef _WIN32
	, unix(this)
#endif
{
	AddClient(local_client);
	usb.Probe();
}

Twibd::~Twibd() {
	log(DEBUG, "destroying twibd");
}

void Twibd::AddDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	log(INFO, "adding device with id %08x", device->device_id);
	devices[device->device_id] = device;
}

void Twibd::AddClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);

	uint32_t client_id;
	do {
		client_id = rng();
	} while(clients.find(client_id) != clients.end());
	client->client_id = client_id;
	log(INFO, "adding client with newly assigned id %08x", client_id);
	
	clients[client_id] = client;
}

void Twibd::PostRequest(Request &&request) {
	dispatch_queue.enqueue(request);
}

void Twibd::PostResponse(Response &&response) {
	dispatch_queue.enqueue(response);
}

void Twibd::RemoveClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);
	clients.erase(clients.find(client->client_id));
}

void Twibd::RemoveDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	devices.erase(devices.find(device->device_id));
}

void Twibd::Process() {
	std::variant<Request, Response> v;
	dispatch_queue.wait_dequeue(v);

	if(v.index() == 0) {
		Request rq = std::get<Request>(v);
		log(DEBUG, "dispatching request");
		log(DEBUG, "  client id: %08x", rq.client_id);
		log(DEBUG, "  device id: %08x", rq.device_id);
		log(DEBUG, "  object id: %08x", rq.object_id);
		log(DEBUG, "  command id: %08x", rq.command_id);
		log(DEBUG, "  tag: %08x", rq.tag);
		
		if(rq.device_id == 0) {
			PostResponse(HandleRequest(rq));
		} else {
			std::shared_ptr<Device> device;
			{
				std::lock_guard<std::mutex> lock(device_map_mutex);
				auto i = devices.find(rq.device_id);
				if(i == devices.end()) {
					PostResponse(rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_DEVICE));
					return;
				}
				device = i->second.lock();
				if(!device || device->deletion_flag) {
					PostResponse(rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_DEVICE));
					return;
				}
			}
			device->SendRequest(std::move(rq));
		}
	} else if(v.index() == 1) {
		Response rs = std::get<Response>(v);
		log(DEBUG, "dispatching response");
		log(DEBUG, "  client id: %08x", rs.client_id);
		log(DEBUG, "  object id: %08x", rs.object_id);
		log(DEBUG, "  result code: %08x", rs.result_code);
		log(DEBUG, "  tag: %08x", rs.tag);

		std::shared_ptr<Client> client;
		{
			std::lock_guard<std::mutex> lock(client_map_mutex);
			auto i = clients.find(rs.client_id);
			if(i == clients.end()) {
				log(INFO, "dropping response destined for unknown client %08x", rs.client_id);
				return;
			}
			client = i->second.lock();
			if(!client) {
				log(INFO, "dropping response destined for destroyed client %08x", rs.client_id);
				return;
			}
			if(client->deletion_flag) {
				log(INFO, "dropping response destined for client flagged for deletion %08x", rs.client_id);
				return;
			}
		}
		client->PostResponse(rs);
	}
}

Response Twibd::HandleRequest(Request &rq) {
	switch(rq.object_id) {
	case 0:
		switch((protocol::ITwibMetaInterface::Command) rq.command_id) {
		case protocol::ITwibMetaInterface::Command::LIST_DEVICES: {
			log(DEBUG, "command 0 issued to twibd meta object: LIST_DEVICES");
			
			Response r = rq.RespondOk();
			std::vector<msgpack11::MsgPack> device_packs;
			for(auto i = devices.begin(); i != devices.end(); i++) {
				auto device = i->second.lock();
				device_packs.push_back(
					msgpack11::MsgPack::object {
						{"device_id", device->device_id},
						{"identification", device->identification}
					});
			}
			msgpack11::MsgPack array_pack(device_packs);
			std::string ser = array_pack.dump();
			r.payload = std::vector<uint8_t>(ser.begin(), ser.end());
			
			return r; }
		default:
			return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION);
		}
	default:
		return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT);
	}
}

} // namespace twibd
} // namespace twili

int main(int argc, char *argv[]) {
#ifdef _WIN32
	WSADATA wsaData;
	int err;
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		printf("WSASStartup failed with error: %d\n", err);
		return 1;
	}
#endif

	add_log(std::make_shared<twili::log::PrettyFileLogger>(stdout, twili::log::Level::DEBUG, twili::log::Level::ERR));
	add_log(std::make_shared<twili::log::PrettyFileLogger>(stderr, twili::log::Level::ERR));

	log(MSG, "starting twibd");
	twili::twibd::Twibd twibd;
	while(1) {
		twibd.Process();
	}
	return 0;
}
