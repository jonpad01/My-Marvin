#pragma once

#include <string>
#include <unordered_map>

#include "Common.h"
#include "GameProxy.h"

namespace marvin {

class Bot;

// Match bits with the internal chat type numbers to simplify access check
enum {
  CommandAccess_Arena = (1 << 0),
  CommandAccess_Public = (1 << 2),
  CommandAccess_Private = (1 << 5),
  CommandAccess_Chat = (1 << 9),
};
typedef u32 CommandAccessFlags;

class CommandSystem;

class CommandExecutor {
 public:
  virtual void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) = 0;
  virtual CommandAccessFlags GetAccess() = 0;
  virtual std::vector<std::string> GetAliases() = 0;
  virtual std::string GetDescription() = 0;
  virtual int GetSecurityLevel() = 0;
};

class CommandSystem {
 public:
  using Commands = std::unordered_map<std::string, std::shared_ptr<CommandExecutor>>;

  CommandSystem();

  bool ProcessMessage(Bot& bot, ChatMessage& chat);

  void RegisterCommand(std::shared_ptr<CommandExecutor> executor) {
    for (std::string trigger : executor->GetAliases()) {
      commands_[Lowercase(trigger)] = executor;
    }
  }

  int GetSecurityLevel(const std::string& player);

  Commands& GetCommands() { return commands_; }

 private:
  Commands commands_;
};

std::vector<std::string> Tokenize(std::string message, char delim);

#if 0
std::string PrefixCheck(std::string message);

bool HasAccess(std::string player);

void SendHelpMenu(GameProxy& game, std::string player);

void SendCommandsMenu(GameProxy& game, std::string player);

void SendModList(GameProxy& game, std::string player);
#endif
}  // namespace marvin
