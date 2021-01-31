#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
//#include <stdio.h>
//#include <TlHelp32.h>


namespace marvin {
	namespace memory {

		uint32_t ReadU32(HANDLE handle, std::size_t address);
		bool WriteU32(HANDLE handle, std::size_t address, uint32_t value);

		std::string ReadString(HANDLE handle, std::size_t address, std::size_t len);

		std::string ReadChatEntry(std::size_t module_base, HANDLE handle);

	} //namespace memory
} //namespace marvin