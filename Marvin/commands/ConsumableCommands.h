#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class RepelCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseRepel, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "repel currently ON.");
      } else {
        game.SendPrivateMessage(sender, "repel currently OFF.");
      }
    } else {
      if (args[0] == "on") {  
        game.SendPrivateMessage(sender, "Turning repel ON.");
        //bb.SetUseRepel(true);
        bb.Set<bool>(BBKey::UseRepel, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning repel OFF.");
        //bb.SetUseRepel(false);
        bb.Set<bool>(BBKey::UseRepel, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!repel on\" or \"!repel off\" or \"!repel\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repel"}; }
  std::string GetDescription() { return "Sets bot to use repel with arguments \"on\" \"off\""; }
 // int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class BurstCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseBurst, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "burst currently ON.");
      } else {
        game.SendPrivateMessage(sender, "burst currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning burst ON.");
       // bb.SetUseBurst(true);
        bb.Set<bool>(BBKey::UseBurst, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning burst OFF.");
        //bb.SetUseBurst(false);
        bb.Set<bool>(BBKey::UseBurst, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }
  

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!burst on\" or \"!burst off\" or \"!burst\" (send current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"burst"}; }
  std::string GetDescription() { return "Sets bot to use burst with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class DecoyCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseDecoy, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "decoy currently ON.");
      } else {
        game.SendPrivateMessage(sender, "decoy currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning decoy ON.");
        //bb.SetUseDecoy(true);
        bb.Set<bool>(BBKey::UseDecoy, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning decoy OFF.");
        //bb.SetUseDecoy(false);
        bb.Set<bool>(BBKey::UseDecoy, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!decoy on\" or \"!decoy off\" or \"!decoy\" (send current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"decoy"}; }
  std::string GetDescription() { return "Sets bot to use decoys with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class RocketCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseRocket, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "rocket currently ON.");
      } else {
        game.SendPrivateMessage(sender, "rocket currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning rocket ON.");
        //bb.SetUseRocket(true);
        bb.Set<bool>(BBKey::UseRocket, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning rocket OFF.");
        //bb.SetUseRocket(false);
        bb.Set<bool>(BBKey::UseRocket, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

   void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!rocket on\" or \"!rocket off\" or \"!rocket\" (send current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rocket"}; }
  std::string GetDescription() { return "Sets bot to use rockets with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class BrickCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseBrick, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "brick currently ON.");
      } else {
        game.SendPrivateMessage(sender, "brick currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning brick ON.");
        //bb.SetUseBrick(true);
        bb.Set<bool>(BBKey::UseBrick, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning brick OFF.");
        //bb.SetUseBrick(false);
        bb.Set<bool>(BBKey::UseBrick, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!brick on\" or \"!brick off\" or \"!brick\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"brick"}; }
  std::string GetDescription() { return "Sets bot to use bricks with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class PortalCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UsePortal, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "portal currently ON.");
      } else {
        game.SendPrivateMessage(sender, "portal currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning portal ON.");
        //bb.SetUsePortal(true);
        bb.Set<bool>(BBKey::UsePortal, true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning portal OFF.");
        //bb.SetUsePortal(false);
        bb.Set<bool>(BBKey::UsePortal, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!portal on\" or \"!portal off\" or \"!portal\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"portal"}; }
  std::string GetDescription() { return "Sets bot to use portals with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class ThorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    bool enabled = bb.ValueOr<bool>(BBKey::UseThor, false);

    if (args.empty()) status = true;

    if (status) {
      if (enabled) {
        game.SendPrivateMessage(sender, "thor currently ON.");
      } else {
        game.SendPrivateMessage(sender, "thor currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning thor ON.");
        bb.Set<bool>(BBKey::UseThor, true);
        //bb.SetUseThor(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning thor OFF.");
        //bb.SetUseThor(false);
        bb.Set<bool>(BBKey::UseThor, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!thor on\" or \"!thor off\" or \"!thor\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"thor"}; }
  std::string GetDescription() { return "Sets bot to use thors with arguments \"on\" \"off\""; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

}  // namespace marvin
