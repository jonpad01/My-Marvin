#pragma once

#include <algorithm>
#include <string>

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class CommandsCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    if (sender.empty()) return;
    GameProxy& game = bot.GetGame();
    Zone zone = bot.GetGame().GetZone();

    std::vector<std::string> args = Tokenize(arg, ' ');

    uint16_t number = 0;

    if (!args.empty()) {
      if (std::isdigit(args[0][0])) {
        number = atoi(args[0].c_str());
      } else {
        SendUsage(game, sender);
        return;
      }
    }

    if (number >= kCommandTypeStr.size()) {
      SendUsage(game, sender);
      return;
    }

    CommandType type = (CommandType)number;

    //int requester_level = cmd.GetSecurityLevel(sender);
    SecurityLevel requester_level = cmd.GetSecurityLevel(sender);
    Commands& commands = cmd.GetCommands();
    // Store a list of executors so multiple triggers linking to the same executor aren't displayed on different lines.
    std::vector<CommandExecutor*> executors;

    // Store the triggers so they can be sorted before output
    struct Trigger {
      CommandExecutor* executor;
      std::string triggers;

      Trigger(CommandExecutor* executor, const std::string& triggers) : executor(executor), triggers(triggers) {}
    };

    std::vector<Trigger> triggers;

    // Loop through every registered command and store the combined triggers
    for (auto& entry : commands) {
      CommandExecutor* executor = entry.second.get();

      if (type != CommandType::All && executor->GetCommandType() != type) continue;

      if (std::find(executors.begin(), executors.end(), executor) == executors.end()) {
        std::string output;

        std::vector<std::string> aliases = executor->GetAliases();

        // Combine all the aliases
        for (std::size_t i = 0; i < aliases.size(); ++i) {
          if (i == 0) output += "-- !" + aliases[i];

          if (i == 0 && aliases.size() > 1) output += " (";

          if (i != 0) {
            output += "!" + aliases[i];

            if (i != aliases.size() - 1) {
              output += ", ";
            } else {
              output += ")";
            }
          }
        }

        if (requester_level < executor->GetSecurityLevel()) {
          output += " - ACCESS DENIED";
        }

        triggers.emplace_back(executor, output);

        executors.push_back(executor);
      }
    }

    // Sort triggers alphabetically
    std::sort(triggers.begin(), triggers.end(),
              [](const Trigger& l, const Trigger& r) { return l.triggers.compare(r.triggers) < 0; });

    // Display the stored triggers
    for (Trigger& trigger : triggers) {
      std::string desc = trigger.executor->GetDescription();
      SecurityLevel security = trigger.executor->GetSecurityLevel();
      //int security = trigger.executor->GetSecurityLevel();

      std::string output = trigger.triggers + " - " + desc + " [" + std::to_string((int)security) + "]";

      bot.GetGame().SendPrivateMessage(sender, output);
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Argument must be a number.  (!commands 1)");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  std::vector<std::string> GetAliases() { return {"commands", "c"}; }
  std::string GetDescription() { return "Shows available commands"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Info; };
};

}  // namespace marvin
