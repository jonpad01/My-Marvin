#include "Utility.h"
#include <iostream>


void OutputError(const std::string& msg)
{
  std::cerr << msg << std::endl;
  system("pause");
}

void ListMenu()
{
  std::cout << "0" << ": " << "Auto Mode" << std::endl;
  std::cout << "1" << ": " << "Manual Mode" << std::endl;
}

bool GetArg(int argc, const std::vector<std::string>& argsv, const std::string& arg_name, std::string& arg) 
{
  for (int i = 1; i < argc; i++) 
  {
    std::size_t found = argsv[i].find(arg_name);
    if (found != std::string::npos) 
    {
      if (i + 1 < argc) 
      {
        arg = argsv[i + 1];
        break;
      } else {
        OutputError("Error: No value was listed after argument: " + arg_name);
        return false;
      }
    }
  }
  return true;
}

std::vector<std::string> CharArrayToStringVector(int size, char* array[]) 
{
  std::vector<std::string> output;

  for (int i = 0; i < size; i++) 
  {
    output.push_back(array[i]);
  }
  return output;
}

bool IsDigit(const std::string& string) {
  for (std::size_t i = 0; i < string.size(); i++) {
      if (!isdigit(string[i]))
      {
      return false;
    }
  }
  return true;
}