#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class UFOCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    uint8_t ufo_status = game.GetPlayer().status;
    bool ufo_enabled = (ufo_status & Status_UFO) != 0;

    if (args.empty()) status = true;

    if (status && alias == "ufo") {
      if (ufo_enabled) {
        game.SendPrivateMessage(sender, "ufo currently ON.");
      } else {
        game.SendPrivateMessage(sender, "ufo currently OFF.");
      }
    } else {
      if (alias == "ufoon" || args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning ufo ON.");
        game.SetStatus(StatusFlag::Status_UFO, true);
      } else if (alias == "ufooff" || args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning ufo OFF.");
        game.SetStatus(StatusFlag::Status_UFO, false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!ufo on\" or \"!ufo off\" or \"!ufo\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"ufo", "ufoon", "ufooff"}; }
  std::string GetDescription() {
    return "Sets bot to switch ufo status with arguments \"on\" \"off\"";
  }
  int GetSecurityLevel() { return 5; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin