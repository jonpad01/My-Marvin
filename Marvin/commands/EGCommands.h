#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class EGPCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendChatMessage("!p");
  }

  void SendUsage(GameProxy& game, std::string sender) {
    game.SendPrivateMessage(sender, "Private command only.");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"p"}; }
  std::string GetDescription() { return "Instructs the bot to send !p to chat."; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Action; };
};

class EGLCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendChatMessage("!l");
  }

  void SendUsage(GameProxy& game, std::string sender) { game.SendPrivateMessage(sender, "Private command only."); }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"l"}; }
  std::string GetDescription() { return "Instructs the bot to send !l to chat."; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Action; };
};

class EGRCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendChatMessage("!r");
  }

  void SendUsage(GameProxy& game, std::string sender) { game.SendPrivateMessage(sender, "Private command only."); }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"r"}; }
  std::string GetDescription() { return "Instructs the bot to send !r to chat."; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Action; };
};

}  // namespace marvin