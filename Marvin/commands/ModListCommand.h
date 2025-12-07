#pragma once

#include <string>
#include <unordered_map>

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class ModListCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    if (sender.empty()) return;

    std::string output;

    for (const auto& entry : cmd.GetOperators()) {
      output = entry.first;
      SecurityLevel level = entry.second;

      switch (level) {
        case SecurityLevel::Blacklisted: {
          output += ": Blacklisted";
        } break;
        case SecurityLevel::Unrestricted: {
          output += ": Urestricted";
        } break;
        case SecurityLevel::Elevated: {
          output += ": Elevated";
        } break;
        case SecurityLevel::Maximum: {
          output += ": Maximum";
        } break;
        default:
          break;
      }
      bot.GetGame().SendPrivateMessage(sender, output);
    }
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  std::vector<std::string> GetAliases() { return {"modlist"}; }
  std::string GetDescription() { return "See who can lock marv"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Info; }
};

}  // namespace marvin
