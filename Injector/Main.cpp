#include "AutoMode.h"
#include "Multicont.h"
#include "Process.h"


int main(int argc, char* argv[]) {

    if (!marvin::GetDebugPrivileges()) {
        std::cerr << "Failed to get debug privileges. Try running as Administrator."
            << std::endl;
        return EXIT_FAILURE;
    }

    auto continuum_pids = marvin::GetProcessIds("Continuum.exe");

#if 0
    if (continuum_pids.empty()) {
        std::cout << "No Continuum.exe processes found." << std::endl;
        return EXIT_FAILURE;
    }
#endif

    std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;

    DWORD pid = 0;

    if (argc > 1) {
        std::string target_player;

        for (int i = 1; i < argc; ++i) {
            if (i != 1) {
                target_player += " ";
            }

            target_player += argv[i];
        }

        if (!target_player.empty() && target_player[0] == '*') {
            marvin::InjectExcept(continuum_pids, target_player.substr(1));
            return EXIT_SUCCESS;
        }

        pid = marvin::SelectPid(continuum_pids, target_player);
    }
    else {
        pid = marvin::SelectPid(continuum_pids);
    }

    if (pid == 0) {
        return EXIT_FAILURE;
    }
    //selection route for running multiple bots or running reconect loop
    if (pid == 1) {
        auto mode = std::make_unique<marvin::AutoBot>();

        //continuous non stop party time loop
        while (true) {
            mode->MonitorBots();
        }
        //AutoMode();
        return EXIT_SUCCESS;
    }

    auto process = std::make_unique<marvin::Process>(pid);

    if (process->HasModule(INJECT_MODULE_NAME)) {
        std::cerr << "Invalid selection. " << pid << " already has Marvin loaded."
            << std::endl;
        return EXIT_FAILURE;
    }

    if (process->InjectModule(inject_path)) {
        std::cout << "Successfully loaded." << std::endl;
    }
    else {
        std::cerr << "Failed to load" << std::endl;
    }

    return EXIT_SUCCESS;
}


