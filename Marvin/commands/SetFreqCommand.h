#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');

    if (args.empty() || args[0].empty()) {
      SendUsage(game, sender);
      return;
    }

    uint16_t number = 111;

    if (std::isdigit(args[0][0])) {
      number = atoi(args[0].c_str());
    } else {
      SendUsage(game, sender);
      return;
    }

    if (number >= 0 && number < 100) {
      bb.Set<uint16_t>("Freq", number);
      game.SendPrivateMessage(sender, "Frequency selection recieved.");
    } else {
      SendUsage(game, sender);
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Invalid selection. !setfreq [freq]");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"setfreq", "sf"}; }
  std::string GetDescription() { return "Sets the bot to a public frequency"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
