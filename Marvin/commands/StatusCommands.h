
#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class MultiCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseMultiFire", false) == true) {
      game.SendPrivateMessage(sender, "Multifire is already on.");
    } else {
      game.SendPrivateMessage(sender, "Turning on multifire.");
    }

    bb.Set<bool>("UseMultiFire", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multi"}; }
  std::string GetDescription() { return "Sets multifire on"; }
  int GetSecurityLevel() { return 0; }
};

class MultiOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseMultiFire", false) == false) {
      game.SendPrivateMessage(sender, "Multifire is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off multifire.");
    }

    bb.Set<bool>("UseMultiFire", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multioff"}; }
  std::string GetDescription() { return "Sets multifire off"; }
  int GetSecurityLevel() { return 0; }
};

class CloakCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseCloak", false) == true) {
      game.SendPrivateMessage(sender, "Cloaking is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now cloaking.");
    }

    bb.Set<bool>("UseCloak", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cloak"}; }
  std::string GetDescription() { return "Sets cloak on"; }
  int GetSecurityLevel() { return 0; }
};

class CloakOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseCloak", false) == false) {
      game.SendPrivateMessage(sender, "Cloaking is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off cloaking.");
    }

    bb.Set<bool>("UseCloak", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cloakoff"}; }
  std::string GetDescription() { return "Sets cloak off"; }
  int GetSecurityLevel() { return 0; }
};

class StealthCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseStealth", false) == true) {
      game.SendPrivateMessage(sender, "Stealth is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using stealth.");
    }

    bb.Set<bool>("UseStealth", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"stealth"}; }
  std::string GetDescription() { return "Sets stealth on"; }
  int GetSecurityLevel() { return 0; }
};

class StealthOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseStealth", false) == false) {
      game.SendPrivateMessage(sender, "Stealth is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off stealth.");
    }

    bb.Set<bool>("UseStealth", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"stealthoff"}; }
  std::string GetDescription() { return "Sets stealth off"; }
  int GetSecurityLevel() { return 0; }
};

class XRadarCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseXRadar", false) == true) {
      game.SendPrivateMessage(sender, "XRadar is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using XRadar.");
    }

    bb.Set<bool>("UseXRadar", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"xradar"}; }
  std::string GetDescription() { return "Sets XRadar on"; }
  int GetSecurityLevel() { return 0; }
};

class XRadarOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseXRadar", false) == false) {
      game.SendPrivateMessage(sender, "XRadar is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off XRadar.");
    }

    bb.Set<bool>("UseXRadar", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"xradaroff"}; }
  std::string GetDescription() { return "Sets XRadar off"; }
  int GetSecurityLevel() { return 0; }
};

class AntiWarpCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseAntiWarp", false) == true) {
      game.SendPrivateMessage(sender, "AntiWarp is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using AntiWarp.");
    }

    bb.Set<bool>("UseAntiWarp", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"anti"}; }
  std::string GetDescription() { return "Sets AntiWarp on"; }
  int GetSecurityLevel() { return 0; }
};

class AntiWarpOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseAntiWarp", false) == false) {
      game.SendPrivateMessage(sender, "AntiWarp is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off AntiWarp.");
    }

    bb.Set<bool>("UseAntiWarp", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"antioff"}; }
  std::string GetDescription() { return "Sets AntiWarp off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin