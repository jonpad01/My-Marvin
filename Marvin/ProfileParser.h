#pragma once

#include <cstdint>
#include <string>
#include "GameProxy.h"
#include "platform/Menu.h"

namespace marvin {

class ProfileParser {
 public:
  static bool GetProfileData(uint32_t index, ProfileData& data);
  static bool GetProfileNames(std::vector<std::string>& names);

 private:
  static std::string GetWorkingDirectory();
  static bool GetStringData(std::vector<std::string>& data);
};



}  // namespace marvin