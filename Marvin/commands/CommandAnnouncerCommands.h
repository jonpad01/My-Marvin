#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class StaffChatAnnouncerCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.ValueOr<bool>("staffchatannouncer", false)) {
        game.SendPrivateMessage(sender, "Staff chat announcer currently ON.");
      } else {
        game.SendPrivateMessage(sender, "Staff chat announcer currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning staff chat announcer ON.");
        bb.Set<bool>("staffchatannouncer", true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning staff chat announcer OFF.");
        bb.Set<bool>("staffchatannouncer", false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender,
                            "Use \"!cmdlogger on\" or \"!cmdlogger off\" or \"!cmdlogger\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"cmdlogger"}; }
  std::string GetDescription() { return "Sends message to staff chat anytime a command is sent to the bot."; }
  int GetSecurityLevel() { return 5; }
  CommandType GetCommandType() { return CommandType::Info; }
};

}  // namespace marvin
