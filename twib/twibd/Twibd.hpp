#pragma once

#include<list>
#include<thread>
#include<mutex>
#include<variant>
#include<map>
#include<random>

#include<libusb.h>

#include "blockingconcurrentqueue.h"

#include "USBBackend.hpp"

#include "TCPFrontend.hpp"
#ifndef _WIN32
#include "UNIXFrontend.hpp"
#endif

#include "Logger.hpp"
#include "Messages.hpp"
#include "Device.hpp"
#include "LocalClient.hpp"

namespace twili {
namespace twibd {

class Twibd {
 public:
	Twibd();
	~Twibd();

	void AddDevice(std::shared_ptr<Device> device);
	void AddClient(std::shared_ptr<Client> client);
	void PostRequest(Request &&request);
	void PostResponse(Response &&response);
	void RemoveDevice(std::shared_ptr<Device> device);
	void RemoveClient(std::shared_ptr<Client> client);
	
	void Process();
	Response HandleRequest(Request &request);
	
 private:
	backend::USBBackend usb;
	frontend::TCPFrontend tcp;
#ifndef _WIN32
	frontend::UNIXFrontend unix;
#endif

	moodycamel::BlockingConcurrentQueue<std::variant<Request, Response>> dispatch_queue;
	
	std::mutex device_map_mutex;
	std::map<uint32_t, std::weak_ptr<Device>> devices;
	
	std::mutex client_map_mutex;
	std::map<uint32_t, std::weak_ptr<Client>> clients;

	std::mt19937 rng;

	std::shared_ptr<LocalClient> local_client;
};

} // namespace twibd
} // namespace twili
