#pragma once

#include <string>
#include <unordered_map>

#include "../Common.h"
#include "../GameProxy.h"
#include "../Time.h"

namespace marvin {

class Bot;

enum class CommandType {All, Info, Behavior, Consumable, Status, Action, Hosting};

/*
  A player with a Blacklisted security level is not able to use any commands.
  All players will have access to commands with an Unrestricted security level by default.
*/
enum class SecurityLevel { Blacklisted, Unrestricted, Elevated, Maximum };
const std::vector<std::string> kCommandTypeStr {"all", "info", "behavior", "consumable", "status", "action", "hosting"}; 

// Match bits with the internal chat type numbers to simplify access check
enum {
  CommandAccess_Arena = (1 << 0),
  CommandAccess_PublicMacro = (1 << 1),
  CommandAccess_Public = (1 << 2),
  CommandAccess_Team = (1 << 3),
  CommandAccess_OtherTeam = (1 << 4),
  CommandAccess_Private = (1 << 5),
  CommandAccess_RedWarning = (1 << 6),
  CommandAccess_RemotePrivate = (1 << 7),
  CommandAccess_RedError = (1 << 8),
  CommandAccess_Chat = (1 << 9),
  CommandAccess_Fuchsia = (1 << 10),

  CommandAccess_All = (CommandAccess_PublicMacro | CommandAccess_Public | CommandAccess_Team |
       CommandAccess_OtherTeam | CommandAccess_Private | CommandAccess_RemotePrivate | CommandAccess_Chat),
};

typedef u32 CommandAccessFlags;

enum {
  CommandFlag_Lockable =  (1 << 0),
};
typedef u32 CommandFlags;

class CommandSystem;

class CommandExecutor {
 public:
  virtual void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) = 0;
  virtual CommandAccessFlags GetAccess() = 0;
  virtual void SetAccess(CommandAccessFlags flags) = 0;
  virtual CommandFlags GetFlags() { return 0; }
  virtual std::vector<std::string> GetAliases() = 0;
  virtual std::string GetDescription() = 0;
  //virtual int GetSecurityLevel() = 0;
  virtual SecurityLevel GetSecurityLevel() = 0;
  virtual CommandType GetCommandType() = 0;
};

//using Operators = std::unordered_map<std::string, int>;
using Operators = std::unordered_map<std::string, SecurityLevel>;
using Commands = std::unordered_map<std::string, std::shared_ptr<CommandExecutor>>;

class CommandSystem {
 public:
  CommandSystem(Zone zone);

  bool ProcessMessage(Bot& bot, ChatMessage& chat);

  void RegisterCommand(std::shared_ptr<CommandExecutor> executor) {
    for (std::string trigger : executor->GetAliases()) {
      commands_[Lowercase(trigger)] = executor;
    }
  }

  SecurityLevel GetSecurityLevel(const std::string& player);
  //int GetSecurityLevel(const std::string& player);

  Commands& GetCommands() { return commands_; }
  const Operators& GetOperators() const;

 private:
  static std::pair<std::string_view, Commands::const_iterator> GetTrigger(const Commands& commands,
                                                                          std::string_view candidate);

  Commands commands_;
  bool ignore_self_message_ = true;
  Time time_;
  uint64_t self_message_cooldown_ = 0;
};

std::vector<std::string> Tokenize(std::string message, char delim);

}  // namespace marvin
