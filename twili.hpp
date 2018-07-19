#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include "USBBridge.hpp"
#include "MonitoredProcess.hpp"

#include<list>

namespace twili {

class Twili {
 public:
	Twili();

	// bridge commands
	trn::Result<std::nullopt_t> Reboot(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> Run(std::vector<uint8_t> nro, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> CoreDump(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> Terminate(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> ListProcesses(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> UpgradeTwili(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> Identify(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	trn::Result<std::nullopt_t> PrintAddressSpace(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	
	std::optional<twili::MonitoredProcess*> FindMonitoredProcess(uint64_t pid);
   
	bool destroy_flag = false;
	trn::Waiter event_waiter;
	trn::ipc::server::IPCServer server;
	twili::usb::USBBridge usb_bridge;
	std::list<twili::MonitoredProcess> monitored_processes;
 private:
	std::vector<uint8_t> hbabi_shim_nro;
};

}
