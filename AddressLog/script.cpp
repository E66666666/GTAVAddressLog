#include "script.h"

#include <windows.h>
#include <Psapi.h>
#include <sstream>
#include <iomanip>

#include <inc/natives.h>

#include "Util/Paths.h"
#include "Util/Util.hpp"
#include "Util/Logger.hpp"

Hash model;
Vehicle vehicle = 0;
Vehicle prevVehicle = 0;
Player player;
Ped playerPed;

int prevNotification;

uint64_t(*GetAddressOfEntity)(Entity entity);

int wheelsPtrOffset;
int numWheelsOffset;


uintptr_t FindPattern(const char* pattern, const char* mask) {
	MODULEINFO modInfo = { nullptr };
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	const char* start_offset = reinterpret_cast<const char *>(modInfo.lpBaseOfDll);
	const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

	intptr_t pos = 0;
	const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

	for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
		if (*retAddress == pattern[pos] || mask[pos] == '?') {
			if (mask[pos + 1] == '\0') {
				return (reinterpret_cast<uintptr_t>(retAddress) - searchLen);
			}
			pos++;
		}
		else {
			pos = 0;
		}
	}
	return 0;
}

void initOffsets() {
    auto addr = FindPattern("\x3B\xB7\x48\x0B\x00\x00\x7D\x0D", "xx????xx");
    wheelsPtrOffset = addr == 0 ? 0 : *(int*)(addr + 2) - 8;
    numWheelsOffset = addr == 0 ? 0 : *(int*)(addr + 2);
}

void cpyToClipboard(HWND hwnd, const std::string &s)
{
	OpenClipboard(hwnd);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
	if (!hg) {
		CloseClipboard();
		return;
	}
	memcpy(GlobalLock(hg), s.c_str(), s.size() + 1);
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

std::string prettyNameFromHash(Hash hash) {
	char *name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(hash);
	std::string displayName = UI::_GET_LABEL_TEXT(name);
	if (displayName == "NULL") {
		displayName = name;
	}
	return displayName;
}

uint64_t GetWheelsPtr(Vehicle handle) {
    if (wheelsPtrOffset == 0) return 0;
    auto address = GetAddressOfEntity(handle);
    return *reinterpret_cast<uint64_t *>(address + wheelsPtrOffset);
}

uint8_t GetNumWheels(Vehicle handle) {
    if (numWheelsOffset == 0) return 0;
    auto address = GetAddressOfEntity(handle);
    return *reinterpret_cast<int *>(address + numWheelsOffset);
}


std::vector<uint64_t> GetWheelPtrs(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers
	auto numWheels = GetNumWheels(handle);
	std::vector<uint64_t> wheelPtrs;
	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		wheelPtrs.push_back(wheelAddr);
	}
	return wheelPtrs;
}

void update_game() {
	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();

	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || 
		!PLAYER::IS_PLAYER_CONTROL_ON(player) || 
		ENTITY::IS_ENTITY_DEAD(playerPed) || 
		PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) {
		return;
	}

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle)) {
		return;
	}

	model = ENTITY::GET_ENTITY_MODEL(vehicle);
	std::string vehicleName = prettyNameFromHash(model);

	if (vehicle != prevVehicle) {
		auto address = GetAddressOfEntity(vehicle);
		prevVehicle = vehicle;
		showNotification("New vehicle: " + vehicleName, &prevNotification);

		std::stringstream hashAsHex;
		std::stringstream logStream;
		hashAsHex << "0x" << std::setfill('0') << std::setw(12) << std::uppercase << std::hex << address;
		logStream << std::left << std::setw(16) << std::setfill(' ') << hashAsHex.str();
		logStream << std::left << std::setw(16) << std::setfill(' ') << prettyNameFromHash(model);
		logger.Write(logStream.str());

		int i = 0;
		for (auto address : GetWheelPtrs(vehicle)) {
			std::stringstream ssWheelAddress;
			ssWheelAddress << std::hex << address;
			logger.Write("\t\tWheel " + std::to_string(i) + " address: " + ssWheelAddress.str());
			i++;
		}


		cpyToClipboard(GetDesktopWindow(), hashAsHex.str());
	}
}

void main() {
	logger.Write("Script started");
    initOffsets();
	uintptr_t GetAddressOfEntityAddress = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
												 "xxxxxxxx????xxxxxxx");

	if (GetAddressOfEntityAddress == 0) {
		logger.Write("Couldn't find GetAddressOfEntity");
		return;
	}
    if (wheelsPtrOffset == 0) {
        logger.Write("Couldn't find wheel ptr offset");
        return;
    }
    if (numWheelsOffset == 0) {
        logger.Write("Couldn't find wheel count offset");
        return;
    }

	GetAddressOfEntity = reinterpret_cast<uint64_t(*)(Entity)>(GetAddressOfEntityAddress);

	while (true) {
		update_game();
		WAIT(0);
	}
}

void ScriptMain() {
	srand(GetTickCount());
	main();
}
