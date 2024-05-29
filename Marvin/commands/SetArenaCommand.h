#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SetArenaCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');

    if (args.empty()) {
      game.SendChatMessage(sender + " has sent me to pub.");
      game.SetArena("");
      return;
    }

    // set args[0] to empty to return to pub arena
    if ((args[0] == "?go")) {
      args[0] = "";
      game.SendChatMessage(sender + " has sent me to pub.");
    } else {
      game.SendChatMessage(sender + " has sent me to ?go " + args[0]);
    }
    // bb.Set<std::string>("Arena", args[0]);
    game.SetArena(args[0]);
    // game.SendChatMessage("?go " + args[0]);
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Invalid selection. !setarena XXXX");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"setarena"}; }
  std::string GetDescription(){return "Sends the bot to an arena. Leave argument empty or type \"go\" to return to pub (.setarena) (.setarena ?go)"; }
  int GetSecurityLevel() { return 5; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marv.