#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetShipCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    uint16_t number = 12;

    // check if alias should have args
    if (alias == "setship" || alias == "ss") {

      std::vector<std::string> args = Tokenize(arg, ' ');

      if (args.empty() || args[0].empty()) {
        SendUsage(game, sender);
        return;
      }

      if (std::isdigit(args[0][0])) {
        number = atoi(args[0].c_str());
      } else {
        SendUsage(game, sender);
        return;
      }
    } else {  // alias is a compressed alias (ss2)
      std::string input = alias.substr(alias.size() - 1);
      if (!input.empty() && std::isdigit(input[0])) {
        number = atoi(input.c_str());
      } else {
        SendUsage(game, sender);
        return;
      }
    }

    if (number >= 1 && number <= 9) {
      game.SetShip(number - 1);
      bot.GetBlackboard().SetShip(Ship(number - 1));

      if (number == 9) {
        //bb.SetAllToDefault();
        bb.SetToDefaultBehavior();

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

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { 

    std::vector<std::string> alias_list{"setship", "ss"};
    std::string alias_prefix = "ss";

    // add alias for ss1 - ss9
    for (std::size_t i = 1; i <= 9; i++) {
      std::string alias = alias_prefix + std::to_string(i);
      alias_list.push_back(alias);
    }

    return alias_list;
   //return {"setship", "ss"}; 
  }
  std::string GetDescription() { return "Sets the ship (9 = spec)"; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin
