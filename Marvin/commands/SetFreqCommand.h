#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
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
      //bb.Set<uint16_t>("Freq", number);
      //bb.SetFreq(number);
      game.SetFreq(number);
      game.SendPrivateMessage(sender, "If you don't see me change freq, either the balancer stopped me, or I jumped back automatically.");
    } else {
      SendUsage(game, sender);
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Invalid selection. !setfreq [freq]");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"setfreq", "sf"}; }
  std::string GetDescription() { return "Sets the bot to a public frequency"; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin
