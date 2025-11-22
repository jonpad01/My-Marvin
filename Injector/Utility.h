#pragma once
#include <vector>
#include <string>

void OutputError(const std::string& msg);
void ListMenu();
bool GetArg(int argc, const std::vector<std::string>& argsv, const std::string& arg_name, std::string& arg);
std::vector<std::string> CharArrayToStringVector(int size, char* array[]);
bool IsDigit(const std::string& string);
void RemoveMatchingFiles(const std::string& substring);