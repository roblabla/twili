#pragma once

#include<vector>

#include "RemoteObject.hpp"

namespace twili {
namespace twib {

struct ProcessListEntry {
	uint64_t process_id;
	uint32_t result;
	uint64_t title_id;
	char process_name[12];
	uint32_t mmu_flags;
};

struct AddressSpaceEntry {
	uint64_t base_addr;
	uint64_t size;
	uint32_t memory_type;
	uint32_t memory_attribute;
	uint32_t memory_permission;
	uint32_t pad;
};

class ITwibDeviceInterface {
 public:
	ITwibDeviceInterface(RemoteObject obj);

	uint64_t Run(std::vector<uint8_t> executable);
	void Reboot();
	std::vector<uint8_t> CoreDump(uint64_t process_id);
	void Terminate(uint64_t process_id);
	std::vector<ProcessListEntry> ListProcesses();
	std::vector<AddressSpaceEntry> ListAddressSpace(uint64_t process_id);
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
