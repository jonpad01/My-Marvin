#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class LockFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      bool frequency_locked = bb.ValueOr<bool>(BBKey::FrequencyLocked, false);
      if (frequency_locked) {
        game.SendPrivateMessage(sender, "lockfreq currently ON.");
      } else {
        game.SendPrivateMessage(sender, "lockfreq currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning lockfreq ON.");
        bb.Set<bool>(BBKey::FrequencyLocked, true);
        //bb.SetFreqLock(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning lockfreq OFF.");
        bb.Set<bool>(BBKey::FrequencyLocked, false);
        //bb.SetFreqLock(false);
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
  std::vector<std::string> GetAliases() { return {"lockfreq"}; }
  std::string GetDescription() { return "Locks/unlocks automatic frequency selection"; }
  //int GetSecurityLevel() { return 1; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Elevated; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin
