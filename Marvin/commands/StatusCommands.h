
#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class MultiCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetUseMultiFire()) {
        game.SendPrivateMessage(sender, "multi currently ON.");
      } else {
        game.SendPrivateMessage(sender, "multi currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning multi ON.");
        bb.SetUseMultiFire(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning multi OFF.");
        bb.SetUseMultiFire(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!multi on\" or \"!multi off\" or \"!multi\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multi"}; }
  std::string GetDescription() { return "Sets bot to use multifire with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Status; }
};

class CloakCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetUseCloak()) {
        game.SendPrivateMessage(sender, "cloak currently ON.");
      } else {
        game.SendPrivateMessage(sender, "cloak currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning cloak ON.");
        bb.SetUseCloak(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning cloak OFF.");
        bb.SetUseCloak(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!cloak on\" or \"!cloak off\" or \"!cloak\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cloak"}; }
  std::string GetDescription() { return "Sets bot to use cloak with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Status; }
};

class StealthCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetUseStealth()) {
        game.SendPrivateMessage(sender, "stealth currently ON.");
      } else {
        game.SendPrivateMessage(sender, "stealth currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning stealth ON.");
        bb.SetUseStealth(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning stealth OFF.");
        bb.SetUseStealth(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!stealth on\" or \"!stealth off\" or \"!stealth\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"stealth", "stealthon", "stealthoff"}; }
  std::string GetDescription() { return "Sets bot to use stealth with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Status; }
};

class XRadarCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetUseXradar()) {
        game.SendPrivateMessage(sender, "xradar currently ON.");
      } else {
        game.SendPrivateMessage(sender, "xradar currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning xradar ON.");
        bb.SetUseXradar(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning xradar OFF.");
        bb.SetUseXradar(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!xradar on\" or \"!xradar off\" or \"!xradar\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"xradar"}; }
  std::string GetDescription() { return "Sets bot to use xradar with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Status; }
};

class AntiWarpCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetUseAntiWarp()) {
        game.SendPrivateMessage(sender, "anti currently ON.");
      } else {
        game.SendPrivateMessage(sender, "anti currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning anti ON.");
        bb.SetUseAntiWarp(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning anti OFF.");
        bb.SetUseAntiWarp(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!anti on\" or \"!anti off\" or \"!anti\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"anti"}; }
  std::string GetDescription() { return "Sets bot to use antiwarp with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Status; }
};

}  // namespace marvin