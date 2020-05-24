#pragma once

#include <cstdint>

#include <vector>
#include <array>

#include <inc/natives.h>

struct HashNode {
    int hash;
    UINT16 data;
    UINT16 padding;
    HashNode* next;
};

class MemoryAccess {
public:
    static uintptr_t MemoryAccess::FindPattern(const char *pattern, const char *mask, const char* startAddress, size_t size);
    static uintptr_t FindPattern(const char* pattern, const char* mask);
    static void Init();
    static char *GetVehicleMakeName(int modelHash);
};
