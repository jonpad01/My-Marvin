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
      std::string op = entry.first;
      int level = entry.second;

      output += op + " [" + std::to_string(level) + "] ";
    }

    bot.GetGame().SendPrivateMessage(sender, output);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  std::vector<std::string> GetAliases() { return {"modlist"}; }
  std::string GetDescription() { return "See who can lock marv"; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Info; }
};

}  // namespace marvin
