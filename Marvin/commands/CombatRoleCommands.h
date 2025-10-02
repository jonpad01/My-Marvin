#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class AnchorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    CombatRole role = bb.ValueOr<CombatRole>("combatrole", CombatRole::None);

      if (role == CombatRole::Anchor) {
      game.SendPrivateMessage(sender, "Marv was already anchoring.");
    } else {
      game.SendPrivateMessage(sender, "Switching to anchor mode.");
    }

    bb.Set<CombatRole>("combatrole", CombatRole::Anchor);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"anchor", "a"}; }
  std::string GetDescription() { return "Enable anchor behavior when basing"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Behavior; };
};

class RushCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    CombatRole role = bb.ValueOr<CombatRole>("combatrole", CombatRole::None);

    if (role == CombatRole::Rusher) {
      game.SendPrivateMessage(sender, "Marv was already rushing.");
    } else {
      game.SendPrivateMessage(sender, "Switching to rush mode.");
    }

    bb.Set<CombatRole>("combatrole", CombatRole::Rusher);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rush", "r"}; }
  std::string GetDescription() { return "Enable rush behavior when basing"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Behavior; };
};

class NerfCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendPrivateMessage(sender, "Switching to nerf mode.");

    // bb.Set<bool>("IsAnchor", false);
    bb.SetCombatDifficulty(CombatDifficulty::Nerf);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"nerf"}; }
  std::string GetDescription() { return "Make the bot easier to fight."; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Behavior; };
};

class NormalCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

   game.SendPrivateMessage(sender, "Switching to normal mode.");
   
    // bb.Set<bool>("IsAnchor", false);
    bb.SetCombatDifficulty(CombatDifficulty::Normal);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"normal"}; }
  std::string GetDescription() { return "Set bot to normal difficulty"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Behavior; };
};

}  // namespace marvin
