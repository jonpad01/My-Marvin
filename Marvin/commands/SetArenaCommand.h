#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetArenaCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');

    if (args.empty() || args[0].empty()) {
      SendUsage(game, sender);
      return;
    }

    if (bb.GetArena() == args[0]) {
      game.SendPrivateMessage(sender, "I am already in that arena.");
    } else {
      //game.SendPrivateMessage(sender, "Arena selection recieved.");
      game.SendChatMessage(sender + " has sent me to ?go " + args[0]);
    }

      //bb.Set<std::string>("Arena", args[0]);
      bb.SetArena(args[0]);
      bb.SetCommandRequest(CommandRequestType::ArenaChange);
      //game.SendChatMessage("?go " + args[0]);
      
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Invalid selection. !setarena XXXX");
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"setarena"}; }
  std::string GetDescription() { return "Sends the bot to an arena"; }
  int GetSecurityLevel() { return 5; }
};

}  // namespace marv.