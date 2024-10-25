
#include "AutoMode.h"
#include "Multicont.h"
#include "Process.h"
#include "Utility.h"
#include "Debug.h"




int main(int argc, char* argv[]) 
{
  std::vector<std::string> argsv = CharArrayToStringVector(argc, argv);
  std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;
  char input;
  DWORD pid = 0;
  std::vector<DWORD> continuum_pids = marvin::GetProcessIds("Continuum.exe");
  bool result = false;
  std::unique_ptr<marvin::AutoBot> bot = nullptr;
  std::unique_ptr<marvin::Process> process = nullptr;

    //marvin::debug_log.open("Injector.log", std::ios::out | std::ios::app);
    //marvin::debug_log << "List of memory errors" << std::endl << std::endl;

  if (!marvin::GetDebugPrivileges()) {
    OutputError("Failed to get debug privileges. Try running as Administrator.");
    return EXIT_FAILURE;
  }

  RemoveMatchingFiles("Marvin-");

  if (argc > 1) {
    std::string arg;
    int num_bots = 0;

    if (GetArg(argc, argsv, "-NumBots", arg)) 
    {
      if (IsDigit(arg)) {
        num_bots = std::stoi(arg);
      } else {
        OutputError("Error: Argument after -Numbots must be a number.");
        return EXIT_FAILURE;
      }
    }

    if (num_bots > 0) {
      std::cout << "This session was created with the command line arguments to run " 
          << num_bots << " bots." << std::endl;
      bot = std::make_unique<marvin::AutoBot>(num_bots);
      //while (true) 
     // {
       // bot->MonitorBots();
     // }
      return EXIT_SUCCESS;
    }
  }

   while (result == false) {
    result = true;
    ListMenu();
    std::cin >> input;

    switch (input) 
    {
      case '0': 
      {
        bot = std::make_unique<marvin::AutoBot>();
       // while (true) 
      //  {
       //   bot->MonitorBots();
      //  }
        return EXIT_SUCCESS;
        break;
      }
      case '1': 
      {
        pid = marvin::SelectPid(continuum_pids);
        break;
      }
      default: 
      {
        std::cerr << "Invalid selection." << std::endl;
        result = false;
      }
    }
   }
      
   if (pid == 0) 
   {
    return EXIT_FAILURE;
   }

  process = std::make_unique<marvin::Process>(pid);

  if (process->HasModule(INJECT_MODULE_NAME)) 
  {
    std::cerr << "Invalid selection. " << pid << " already has Marvin loaded." << std::endl;
    return EXIT_FAILURE;
  }

  if (process->InjectModule(inject_path)) 
  {
    std::cout << "Successfully loaded." << std::endl;
  } else {
    std::cerr << "Failed to load" << std::endl;
  }

  return EXIT_SUCCESS;
}
