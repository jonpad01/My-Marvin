#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class LockFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "lockfreq") {
      if (bb.GetFreqLock()) {
        game.SendPrivateMessage(sender, "lockfreq currently ON.");
      } else {
        game.SendPrivateMessage(sender, "lockfreq currently OFF.");
      }
    } else {
      if (alias == "lockfreqon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning lockfreq ON.");
        bb.SetFreqLock(true);
      } else if (alias == "lockfreqoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning lockfreq OFF.");
        bb.SetFreqLock(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!lockfreq on\" or \"!lockfreq off\" or \"!lockfreq\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"lockfreq", "lockfreqon", "lockfreqoff"}; }
  std::string GetDescription() { return "Locks/unlocks automatic frequency selection"; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin
