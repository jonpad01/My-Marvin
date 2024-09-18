#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace fuse {

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using MemoryAddress = u32;

}  // namespace fuse

#ifdef FUSE_BUILDING
#define FUSE_EXPORT __declspec(dllexport)
#else
#define FUSE_EXPORT __declspec(dllimport)
#endif
