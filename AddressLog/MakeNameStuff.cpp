#include <Windows.h>
#include <Psapi.h>
#include "MakeNameStuff.h"

typedef void*(*GetModelInfo_t)(unsigned int modelHash, int* index);
GetModelInfo_t GetModelInfo;

int gameVersion = getGameVersion();

void MemoryAccess::Init() {

    auto addr = FindPattern("\x0F\xB7\x05\x00\x00\x00\x00"
        "\x45\x33\xC9\x4C\x8B\xDA\x66\x85\xC0"
        "\x0F\x84\x00\x00\x00\x00"
        "\x44\x0F\xB7\xC0\x33\xD2\x8B\xC1\x41\xF7\xF0\x48"
        "\x8B\x05\x00\x00\x00\x00"
        "\x4C\x8B\x14\xD0\xEB\x09\x41\x3B\x0A\x74\x54",
        "xxx????xxxxxxxxxxx????"
        "xxxxxxxxxxxxxx????xxxxxxxxxxx");
    GetModelInfo = (GetModelInfo_t)(addr);
}

char *MemoryAccess::GetVehicleMakeName(int modelHash) {
    int index = 0xFFFF;
    void* modelInfo = GetModelInfo(modelHash, &index);
    if (gameVersion < 38) {
        return (char *)((uintptr_t)modelInfo + 0x27c);
    }
    else {
        return (char *)((uintptr_t)modelInfo + 0x2a4);
    }
}

uintptr_t MemoryAccess::FindPattern(const char *pattern, const char *mask, const char* startAddress, size_t size) {
    const char* address_end = startAddress + size;
    const auto mask_length = static_cast<size_t>(strlen(mask) - 1);

    for (size_t i = 0; startAddress < address_end; startAddress++) {
        if (*startAddress == pattern[i] || mask[i] == '?') {
            if (mask[i + 1] == '\0') {
                return reinterpret_cast<uintptr_t>(startAddress) - mask_length;
            }
            i++;
        }
        else {
            i = 0;
        }
    }
    return 0;
}

uintptr_t MemoryAccess::FindPattern(const char* pattern, const char* mask) {
    MODULEINFO modInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    return FindPattern(pattern, mask, reinterpret_cast<const char *>(modInfo.lpBaseOfDll), modInfo.SizeOfImage);
}
