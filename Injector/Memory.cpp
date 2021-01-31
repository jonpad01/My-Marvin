#include "Memory.h"

namespace marvin {
    namespace memory {

        struct ChatEntry {
            char message[256];
            char player[24];
            char unknown[8];
            unsigned char type;
            char unknown2[3];
        };

        uint32_t ReadU32(HANDLE handle, std::size_t address) {
            uint32_t value = 0;
            SIZE_T num_read;

            if (ReadProcessMemory(handle, (LPVOID)address, &value, sizeof(uint32_t), &num_read)) {
                return value;
            }

            return 0;
        }

        bool WriteU32(HANDLE handle, size_t address, uint32_t value) {
   
            SIZE_T num_read;

            if (WriteProcessMemory(handle, (LPVOID)address, &value, sizeof(value), &num_read)) {
                return true;
            }
            return false;
        }

        std::string ReadString(HANDLE handle, std::size_t address, std::size_t len) {
            std::string value;
            SIZE_T read;

            value.resize(len);

            if (ReadProcessMemory(handle, (LPVOID)address, &value[0], len, &read)) {
                return value;
            }

            return "";
        }

        

        std::string ReadChatEntry(std::size_t module_base, HANDLE handle) {

            uint32_t game_addr = marvin::memory::ReadU32(handle, module_base + 0xC1AFC);
            uint32_t entry_ptr_addr = game_addr + 0x2DD08;
            uint32_t entry_count_addr = entry_ptr_addr + 8;

            uint32_t entry_ptr = marvin::memory::ReadU32(handle, entry_ptr_addr);
            uint32_t entry_count = 0;

            // Client isn't in game
            if (entry_ptr == 0) return "";

            ReadProcessMemory(handle, (LPVOID)entry_count_addr, &entry_count, sizeof(uint32_t), nullptr);

            uint32_t last_entry_addr = entry_ptr + (entry_count - 1) * sizeof(ChatEntry);
            ChatEntry entry;

            ReadProcessMemory(handle, (LPVOID)last_entry_addr, &entry, sizeof(ChatEntry), nullptr);

            return entry.message;
        }

    }  // namespace memory
} // namespace marvin