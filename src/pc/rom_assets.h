#pragma once
#include <PR/ultratypes.h>

enum RomAssetType {
    ROM_ASSET_VTX,
    ROM_ASSET_TEXTURE,
    ROM_ASSET_SAMPLE,
    ROM_ASSET_COLLISION,
    ROM_ASSET_ANIM,
    ROM_ASSET_DIALOG,
    ROM_ASSET_DEMO,
};

#define ROM_ASSET_JOIN2(_a, _b) _a ## _b
#define ROM_ASSET_JOIN(_a, _b) ROM_ASSET_JOIN2(_a, _b)
#define ROM_ASSET_QUEUE_FUNC(_name) ROM_ASSET_JOIN(_name, _rom_assets_queue)
#define ROM_ASSET_QUEUE_INIT(_name) ROM_ASSET_JOIN(_name, _rom_assets_queue_initializer)

#ifdef _MSC_VER
#define ROM_ASSET_UNIQUE_FUNC(_id) ROM_ASSET_JOIN(rom_asset_queue_, _id)
#define ROM_ASSET_UNIQUE_INIT(_id) ROM_ASSET_JOIN(rom_asset_queue_initializer_, _id)
#define ROM_ASSET_CONSTRUCTOR_IMPL(_id) \
static void ROM_ASSET_UNIQUE_FUNC(_id)(void); \
__pragma(section(".CRT$XCU", read)) \
__declspec(allocate(".CRT$XCU")) static void (__cdecl *ROM_ASSET_UNIQUE_INIT(_id))(void) = ROM_ASSET_UNIQUE_FUNC(_id); \
static void ROM_ASSET_UNIQUE_FUNC(_id)(void)
#define ROM_ASSET_CONSTRUCTOR(_name) ROM_ASSET_CONSTRUCTOR_IMPL(__COUNTER__)
#else
#define ROM_ASSET_CONSTRUCTOR(_name) __attribute__((constructor)) static void ROM_ASSET_QUEUE_FUNC(_name)(void)
#endif

#define ROM_ASSET_LOAD_VTX(_name, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
Vtx _name[(_segmentedSize) / 16] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_name, ROM_ASSET_VTX, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_TEXTURE(_name, _filename, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
ALIGNED8 Texture _name[_segmentedSize] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_name, ROM_ASSET_TEXTURE, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_SAMPLE(_name, _ptr, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_ptr, ROM_ASSET_SAMPLE, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_SEQUENCE(_name, _ptr, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_ptr, ROM_ASSET_SAMPLE, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_COLLISION(_name, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
Collision _name[(_segmentedSize) / 2] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_name, ROM_ASSET_COLLISION, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_ANIM(_name, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
static u16 _name[(_segmentedSize) / 2] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_name, ROM_ASSET_ANIM, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_ANIM_2D(_name, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize, _secondDim) \
static u16 _name[(_segmentedSize) / (2 * _secondDim)][_secondDim] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_name, ROM_ASSET_ANIM, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_MARIO_ANIM(_name, _ptr, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
static u16 _name[(_segmentedSize) / 2] = { 0 }; \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_ptr, ROM_ASSET_ANIM, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_DIALOG(_ptr, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
ROM_ASSET_CONSTRUCTOR(_ptr) { \
    rom_assets_queue(_ptr, ROM_ASSET_DIALOG, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

#define ROM_ASSET_LOAD_DEMO(_name, _ptr, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize) \
ROM_ASSET_CONSTRUCTOR(_name) { \
    rom_assets_queue(_ptr, ROM_ASSET_DEMO, _physicalAddress, _physicalSize, _segmentedAddress, _segmentedSize); \
}

void rom_assets_load(void);
void rom_assets_queue(void* ptr, enum RomAssetType assetType, u32 physicalAddress, u32 physicalSize, u32 segmentedAddress, u32 segmentedSize);
u8* rom_assets_decompress(u32* data, u32* decompressedSize);
