#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetShipCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');

    if (args.empty() || args[0].empty()) {
      SendUsage(game, sender);
      return;
    }

    uint16_t number = 12;

    if (std::isdigit(args[0][0])) {
      number = atoi(args[0].c_str());
    } else {
      SendUsage(game, sender);
      return;
    }

    if (number >= 1 && number <= 9) {
      bb.Set<uint16_t>("Ship", number - 1);

      if (number == 9) {
        bb.SetAllToDefault();
        bb.Set<bool>("IsAnchor", false);
        bb.Set<bool>("Swarm", false);

        game.SendPrivateMessage(sender, "My behaviors are also reset when sent to spec");
      } else {
        game.SendPrivateMessage(sender, "Ship selection recieved.");
      }
    } else {
      SendUsage(game, sender);
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Invalid selection. !setship [shipNumber]");
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"setship", "ss"}; }
  std::string GetDescription() { return "Sets the ship (9 = spec)"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
