#pragma once

#include <cstdint>
#include <string>
#include "GameProxy.h"

namespace marvin {

class ProfileParser {
 public:
  ProfileParser(uint32_t index) : profile_index(index) {}
  bool LoadProfileData();
  const ProfileData& GetProfileData() { return profile_data; }

 private:
  std::string GetWorkingDirectory();

  uint32_t profile_index;
  ProfileData profile_data;
};

}  // namespace marvin