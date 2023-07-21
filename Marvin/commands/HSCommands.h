#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class HSBuyCommand : public CommandExecutor {
  public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
     Blackboard& bb = bot.GetBlackboard();
     GameProxy& game = bot.GetGame();
     
    std::vector<std::string> args = SplitString(arg, "|");
    // adding to the item list in case buy was used in a delimiter

    uint16_t ship = game.GetPlayer().ship;

    if (!args.empty() && isdigit(args[0][0])) {
      int num = args[0][0] - '0';
      if (num >= 1 && num <= 8) {
        ship = num - 1;
        args.erase(args.begin());
      } else {
        game.SendPrivateMessage(sender, "Invalid ship number (1-8 only).");
        SendUsage(game, sender);
      }
    }

     for (const std::string& item : args) {
      bb.EmplaceHSBuySellList(item);
     }

     bb.SetHSBuySellActionCount(bb.GetHSBuySellList().items.size());
     bb.SetHSBuySellTimeStamp(bot.GetTime().GetTime());
     bb.SetHSBuySellAction(ItemAction::Buy);
     bb.SetHSBuySellActionCompleted(false);
     bb.SetHSBuySellSender(sender);
     bb.SetHSBuySellShip(ship);

     if (ship == game.GetPlayer().ship) {
      game.SendPrivateMessage(sender, "Attempting to buy items for my current ship.");
     } else {
      game.SendPrivateMessage(sender, "Attempting to buy items for ship " + GetShip(ship) + ".");
     }
  }

   void SendUsage(GameProxy& game, std::string sender) {
     game.SendPrivateMessage(sender, "Example format: /.buy 2|close combat|radiating coils");
     game.SendPrivateMessage(sender,
                             "Where 2 is the ship to buy the item in, don't include the number if you want to buy "
                             "items for the current ship.");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"buy"}; }
  std::string GetDescription() { return "Tell the bot to buy items (use  a '|' delimiter to buy multiple items)."; }
  int GetSecurityLevel() { return 0; }
};

class HSSellCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
     Blackboard& bb = bot.GetBlackboard();
     GameProxy& game = bot.GetGame();

     std::vector<std::string> args = SplitString(arg, "|");
     // adding to the item list in case sell was used in a delimiter
     uint16_t ship = game.GetPlayer().ship;

     if (!args.empty() && isdigit(args[0][0])) {
      int num = args[0][0] - '0';
      if (num >= 1 && num <= 8) {
        ship = num - 1;
        args.erase(args.begin());
      } else {
        game.SendPrivateMessage(sender, "Invalid ship number (1-8 only).");
        SendUsage(game, sender);
      }
     }

     for (const std::string& item : args) {
      bb.EmplaceHSBuySellList(item);
     }

     bb.SetHSBuySellActionCount(bb.GetHSBuySellList().items.size());
     bb.SetHSBuySellTimeStamp(bot.GetTime().GetTime());
     bb.SetHSBuySellAction(ItemAction::Sell);
     bb.SetHSBuySellActionCompleted(false);
     bb.SetHSBuySellSender(sender);
     bb.SetHSBuySellShip(ship);

     if (ship == game.GetPlayer().ship) {
      game.SendPrivateMessage(sender, "Attempting to sell items for my current ship.");
     } else {
      game.SendPrivateMessage(sender, "Attempting to buy items for ship " + GetShip(ship) + ".");
     }
  }


  void SendUsage(GameProxy& game, std::string sender) {
     game.SendPrivateMessage(sender, "Example format: /.sell 2|close combat|radiating coils");
     game.SendPrivateMessage(sender, "Where 2 is the ship to sell the item in, don't include the number if you want to sell items for the current ship.");
  }
  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"sell"}; }
  std::string GetDescription() { return "Example format: /.sell 2|close combat|radiating coils"; }
  int GetSecurityLevel() { return 0; }
};

class HSFlagCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    // if (bb.ValueOr<bool>("CmdLock", false) == true) {
    if (bb.GetCanFlag()) {
      game.SendPrivateMessage(sender, "Marv was already flagging.");
    } else {
      game.SendPrivateMessage(sender, "Switching to flagging.");
    }

    // bb.Set<bool>("CmdLock", true);
    bb.SetCanFlag(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"flag"}; }
  std::string GetDescription() { return "Allows bot to jump on to a flag team"; }
  int GetSecurityLevel() { return 0; }
};

class HSFlagOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    // if (bb.ValueOr<bool>("CmdLock", false) == true) {
    if (!bb.GetCanFlag()) {
      game.SendPrivateMessage(sender, "Marv was not flagging.");
    } else {
      game.SendPrivateMessage(sender, "Marv will stop flagging.");
    }

    // bb.Set<bool>("CmdLock", true);
    bb.SetCanFlag(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"flagoff"}; }
  std::string GetDescription() { return "Disallows bot to jump on to a flag team"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin