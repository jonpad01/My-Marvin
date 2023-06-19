
#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class MultiCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseMultiFire, false) == true) {
    if (bb.GetUseMultiFire()) {
      game.SendPrivateMessage(sender, "Multifire is already on.");
    } else {
      game.SendPrivateMessage(sender, "Turning on multifire.");
    }

    //bb.Set<bool>(BB::UseMultiFire, true);
    bb.SetUseMultiFire(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multi"}; }
  std::string GetDescription() { return "Sets multifire on"; }
  int GetSecurityLevel() { return 0; }
};

class MultiOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseMultiFire, false) == false) {
      if (!bb.GetUseMultiFire()) {
      game.SendPrivateMessage(sender, "Multifire is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off multifire.");
    }

   // bb.Set<bool>(BB::UseMultiFire, false);
    bb.SetUseMultiFire(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multioff"}; }
  std::string GetDescription() { return "Sets multifire off"; }
  int GetSecurityLevel() { return 0; }
};

class CloakCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseCloak, false) == true) {
      if (bb.GetUseCloak()) {
      game.SendPrivateMessage(sender, "Cloaking is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now cloaking.");
    }

    //bb.Set<bool>(BB::UseCloak, true);
    bb.SetUseCloak(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cloak"}; }
  std::string GetDescription() { return "Sets cloak on"; }
  int GetSecurityLevel() { return 0; }
};

class CloakOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseCloak, false) == false) {
      if (!bb.GetUseCloak()) {
      game.SendPrivateMessage(sender, "Cloaking is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off cloaking.");
    }

    //bb.Set<bool>(BB::UseCloak, false);
    bb.SetUseCloak(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cloakoff"}; }
  std::string GetDescription() { return "Sets cloak off"; }
  int GetSecurityLevel() { return 0; }
};

class StealthCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseStealth, false) == true) {
      if (bb.GetUseStealth()) {
      game.SendPrivateMessage(sender, "Stealth is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using stealth.");
    }

    //bb.Set<bool>(BB::UseStealth, true);
    bb.SetUseStealth(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"stealth"}; }
  std::string GetDescription() { return "Sets stealth on"; }
  int GetSecurityLevel() { return 0; }
};

class StealthOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseStealth, false) == false) {
      if (!bb.GetUseStealth()) {
      game.SendPrivateMessage(sender, "Stealth is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off stealth.");
    }

    //bb.Set<bool>(BB::UseStealth, false);
    bb.SetUseStealth(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"stealthoff"}; }
  std::string GetDescription() { return "Sets stealth off"; }
  int GetSecurityLevel() { return 0; }
};

class XRadarCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseXRadar, false) == true) {
      if (bb.GetUseXradar()) {
      game.SendPrivateMessage(sender, "XRadar is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using XRadar.");
    }

    //bb.Set<bool>(BB::UseXRadar, true);
    bb.SetUseXradar(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"xradar"}; }
  std::string GetDescription() { return "Sets XRadar on"; }
  int GetSecurityLevel() { return 0; }
};

class XRadarOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseXRadar, false) == false) {
      if (!bb.GetUseXradar()) {
      game.SendPrivateMessage(sender, "XRadar is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off XRadar.");
    }

    //bb.Set<bool>(BB::UseXRadar, false);
    bb.SetUseXradar(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"xradaroff"}; }
  std::string GetDescription() { return "Sets XRadar off"; }
  int GetSecurityLevel() { return 0; }
};

class AntiWarpCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseAntiWarp, false) == true) {
      if (bb.GetUseAntiWarp()) {
      game.SendPrivateMessage(sender, "AntiWarp is already on.");
    } else {
      game.SendPrivateMessage(sender, "Now using AntiWarp.");
    }

    //bb.Set<bool>(BB::UseAntiWarp, true);
    bb.SetUseAntiWarp(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"anti"}; }
  std::string GetDescription() { return "Sets AntiWarp on"; }
  int GetSecurityLevel() { return 0; }
};

class AntiWarpOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseAntiWarp, false) == false) {
      if (!bb.GetUseAntiWarp()) {
      game.SendPrivateMessage(sender, "AntiWarp is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off AntiWarp.");
    }

    //bb.Set<bool>(BB::UseAntiWarp, false);
    bb.SetUseAntiWarp(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"antioff"}; }
  std::string GetDescription() { return "Sets AntiWarp off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin