#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

/*
  Injector injects the Loader.dll into the Continuum process.
  The loader is the one that loads the Marvin.dll so it can hot swap it.
*/

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>

#include "Multicont.h"
#include "AutoMode.h"

#define INJECT_MODULE_NAME "Loader.dll"

struct ChatEntry {
    char message[256];
    char player[24];
    char unknown[8];
    unsigned char type;
    char unknown2[3];
};

namespace marvin {
	namespace memory {

		uint32_t ReadU32(HANDLE handle, size_t address);

		std::string ReadString(HANDLE handle, size_t address, size_t len);

        ChatEntry* ReadChat(HANDLE handle, size_t address);

    } //namespace memory



    class Process {
    public:
        Process(DWORD process_id)
            : process_handle_(nullptr), process_id_(process_id) {}
        ~Process() {
            if (process_handle_) {
                CloseHandle(process_handle_);
            }
        }

        Process(const Process& other) = delete;
        Process& operator=(const Process& other) = delete;

        std::size_t GetModuleBase(const char* module_name);

        bool HasModule(const char* module_name);

        bool InjectModule(const std::string& module_path);

        HANDLE GetHandle();

        DWORD GetId() { return process_id_; }


    private:
        HANDLE process_handle_;
        DWORD process_id_;
    };


    bool GetDebugPrivileges();

    std::string GetWorkingDirectory();

    std::vector<DWORD> GetProcessIds(const char* exe_file);

    class GameProxy {
    public:
        virtual ~GameProxy() {}
        virtual std::string GetName() const = 0;
    };
	
    class ContinuumGameProxy : public GameProxy {
    public:
        ContinuumGameProxy(std::unique_ptr<Process> process) : process_(std::move(process)) {}

        std::string GetName() const override;

        Process& GetProcess() { return *process_; }


    private:
        std::unique_ptr<Process> process_;
    };

} //namespace marvin


DWORD SelectPid(const std::vector<DWORD>& pids, std::string target_player);

DWORD SelectPid(const std::vector<DWORD>& pids);

void InjectExcept(const std::vector<DWORD>& pids, const std::string& exception);


int main(int argc, char* argv[]);