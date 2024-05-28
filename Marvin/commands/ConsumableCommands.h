#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class RepelCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "repel") {
      if (bb.GetUseRepel()) {
        game.SendPrivateMessage(sender, "repel currently ON.");
      } else {
        game.SendPrivateMessage(sender, "repel currently OFF.");
      }
    } else {
      if (alias == "repelon" || !args.empty() && args[0] == "on") {  // could be an access violation
        game.SendPrivateMessage(sender, "Turning repel ON.");
        bb.SetUseRepel(true);
      } else if (alias == "repeloff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning repel OFF.");
        bb.SetUseRepel(false);
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
  std::vector<std::string> GetAliases() { return {"repel", "repelon", "repeloff" }; }
  std::string GetDescription() { return "Sets bot to use repel with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class BurstCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "burst") {
      if (bb.GetUseBurst()) {
        game.SendPrivateMessage(sender, "burst currently ON.");
      } else {
        game.SendPrivateMessage(sender, "burst currently OFF.");
      }
    } else {
      if (alias == "burston" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning burst ON.");
        bb.SetUseBurst(true);
      } else if (alias == "burstoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning burst OFF.");
        bb.SetUseBurst(false);
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
  std::vector<std::string> GetAliases() { return {"burst", "burston", "burstoff"}; }
  std::string GetDescription() { return "Sets bot to use burst with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class DecoyCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "decoy") {
      if (bb.GetUseDecoy()) {
        game.SendPrivateMessage(sender, "decoy currently ON.");
      } else {
        game.SendPrivateMessage(sender, "decoy currently OFF.");
      }
    } else {
      if (alias == "decoyon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning decoy ON.");
        bb.SetUseDecoy(true);
      } else if (alias == "decoyoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning decoy OFF.");
        bb.SetUseDecoy(false);
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
  std::vector<std::string> GetAliases() { return {"decoy", "decoyon", "decoyoff"}; }
  std::string GetDescription() { return "Sets bot to use decoys with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class RocketCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "rocket") {
      if (bb.GetUseRocket()) {
        game.SendPrivateMessage(sender, "rocket currently ON.");
      } else {
        game.SendPrivateMessage(sender, "rocket currently OFF.");
      }
    } else {
      if (alias == "rocketon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning rocket ON.");
        bb.SetUseRocket(true);
      } else if (alias == "rocketoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning rocket OFF.");
        bb.SetUseRocket(false);
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
  std::vector<std::string> GetAliases() { return {"rocket", "rocketon", "rocketoff"}; }
  std::string GetDescription() { return "Sets bot to use rockets with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class BrickCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "brick") {
      if (bb.GetUseBrick()) {
        game.SendPrivateMessage(sender, "brick currently ON.");
      } else {
        game.SendPrivateMessage(sender, "brick currently OFF.");
      }
    } else {
      if (alias == "brickon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning brick ON.");
        bb.SetUseBrick(true);
      } else if (alias == "brickoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning brick OFF.");
        bb.SetUseBrick(false);
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
  std::vector<std::string> GetAliases() { return {"brick", "brickon", "brickoff"}; }
  std::string GetDescription() { return "Sets bot to use bricks with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class PortalCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "portal") {
      if (bb.GetUsePortal()) {
        game.SendPrivateMessage(sender, "portal currently ON.");
      } else {
        game.SendPrivateMessage(sender, "portal currently OFF.");
      }
    } else {
      if (alias == "portalon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning portal ON.");
        bb.SetUsePortal(true);
      } else if (alias == "portaloff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning portal OFF.");
        bb.SetUsePortal(false);
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
  std::vector<std::string> GetAliases() { return {"portal", "portalon", "portaloff"}; }
  std::string GetDescription() { return "Sets bot to use portals with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

class ThorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "thor") {
      if (bb.GetUseThor()) {
        game.SendPrivateMessage(sender, "thor currently ON.");
      } else {
        game.SendPrivateMessage(sender, "thor currently OFF.");
      }
    } else {
      if (alias == "thoron" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning thor ON.");
        bb.SetUseThor(true);
      } else if (alias == "thoroff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning thor OFF.");
        bb.SetUseThor(false);
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
  std::vector<std::string> GetAliases() { return {"thor", "thoron", "thoroff"}; }
  std::string GetDescription() { return "Sets bot to use thors with arguments \"on\" \"off\""; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Consumable; };
};

}  // namespace marvin
