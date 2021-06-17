#include "CommandSystem.h"

#include "Bot.h"
#include "Common.h"

namespace marvin {

constexpr int kArenaSecurityLevel = 5;
const std::unordered_map<std::string, int> kOperators = {
    {"tm_master", 10}, {"baked cake", 10}, {"x-demo", 10}, {"lyra.", 5}, {"profile", 5}, {"monkey", 5}, {"neostar", 5},
};

class CommandsCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    if (sender.empty()) return;

    std::vector<CommandExecutor*> executors;

    int requester_level = cmd.GetSecurityLevel(sender);

    CommandSystem::Commands& commands = cmd.GetCommands();
    for (auto& entry : commands) {
      CommandExecutor* executor = entry.second.get();

      if (std::find(executors.begin(), executors.end(), executor) == executors.end()) {
        std::string output;

        if (requester_level >= executor->GetSecurityLevel()) {
          std::vector<std::string> aliases = executor->GetAliases();

          for (std::size_t i = 0; i < aliases.size(); ++i) {
            if (i != 0) {
              output += "/";
            }

            output += "!" + aliases[i];
          }

          output += " - " + executor->GetDescription() + " [" + std::to_string(executor->GetSecurityLevel()) + "]";

          bot.GetGame().SendPrivateMessage(sender, output);
        }

        executors.push_back(executor);
      }
    }
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Public | CommandAccess_Private; }

  std::vector<std::string> GetAliases() { return {"commands", "c"}; }

  std::string GetDescription() { return "Shows commands"; }

  int GetSecurityLevel() { return 0; }
};

CommandSystem::CommandSystem() {
  RegisterCommand(std::make_shared<CommandsCommand>());
}

int CommandSystem::GetSecurityLevel(const std::string& player) {
  auto iter = kOperators.find(player);

  if (iter != kOperators.end()) {
    return iter->second;
  }

  return 0;
}

bool CommandSystem::ProcessMessage(Bot& bot, ChatMessage& chat) {
  std::string& msg = chat.message;

  if (msg.empty()) return false;
  if (msg[0] != '!' && msg[0] != '.') return false;

  std::size_t split = msg.find(' ');
  std::string trigger = Lowercase(msg.substr(1, split - 1));
  std::string arg;

  if (split != std::string::npos) {
    arg = msg.substr(split + 1);
  }

  auto iter = commands_.find(trigger);
  if (iter != commands_.end()) {
    CommandExecutor& command = *iter->second;
    u32 access_request = (1 << chat.type);

    if (access_request & command.GetAccess()) {
      int security_level = 0;

      if (chat.type == 0) {
        security_level = kArenaSecurityLevel;
      } else {
        auto op_iter = kOperators.find(Lowercase(chat.player));

        if (op_iter != kOperators.end()) {
          security_level = op_iter->second;
        }
      }

      if (security_level >= command.GetSecurityLevel()) {
        command.Execute(*this, bot, chat.player, arg);
        return true;
      }
    }
  }

  return false;

#if 0
  std::string msg = PrefixCheck(chat.message);


  bool mod = HasAccess(chat.player);
  bool locked = bb.ValueOr<bool>("CmdLock", false);

  bool mArena = chat.type == 0;
  bool mPublic = chat.type == 2;
  bool mPrivate = chat.type == 5;
  bool mChannel = chat.type == 9;

  if ((msg == "modlist" || msg == "ml") && mPrivate) {
    SendModList(game, chat.player);
    return behavior::ExecuteResult::Failure;
  }

  std::vector<std::string> commands = { "lockmarv", "lm", "unlockmarv", "um", "lockfreq", "lf", "unlockfreq", "uf",
                                       "anchor",   "a",  "rush",       "r",  "swarm",    "s",  "swarmoff",   "so",
                                       "multi",    "m",  "multioff",   "mo", "repel",    "r",  "repeloff",   "ro",
                                       "setship",  "ss", "setfreq",    "sf" };

  std::vector<std::string> queue;
  std::string current = "";

  for (std::size_t i = 0; i < msg.size(); i++) {
    if (msg[i] == ';') {
      queue.push_back(current);
      current = "";

    } else {
      current += msg[i];
    }
  }

  if (!current.empty()) {
    queue.push_back(current);
  }

  for (std::size_t i = 0; i < queue.size(); i++) {
    msg = queue[i];

    if ((msg == "lockmarv" || msg == "lm") && mod) {
      if (bb.ValueOr<bool>("CmdLock", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Marv was already locked.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching from unlocked to locked.");
      }
      bb.Set<bool>("CmdLock", true);
    }

    if ((msg == "unlockmarv" || msg == "um") && mod) {
      if (bb.ValueOr<bool>("CmdLock", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Marv was already unlocked.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching from locked to unlocked.");
      }
      bb.Set<bool>("CmdLock", false);
    }

    if ((msg == "lockfreq" || msg == "lf") && (mod || !locked)) {
      if (bb.ValueOr<bool>("FreqLock", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Marv was already locked.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching from unlocked to locked.");
      }
      bb.Set<bool>("FreqLock", true);
    }

    if ((msg == "unlockfreq" || msg == "uf") && (mod || !locked)) {
      if (bb.ValueOr<bool>("FreqLock", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Marv was already unlocked.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching from locked to unlocked.");
      }
      bb.Set<bool>("FreqLock", false);
    }

    if ((msg == "anchor" || msg == "a") && (mod || !locked) && mPrivate) {
      if (bb.ValueOr<bool>("IsAnchor", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Marv was already anchoring.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching to anchor mode.");
      }
      bb.Set<bool>("IsAnchor", true);
    }

    if ((msg == "rush" || msg == "r") && (mod || !locked) && mPrivate) {
      if (bb.ValueOr<bool>("IsAnchor", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Marv was already rushing.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching to rush mode.");
      }
      bb.Set<bool>("IsAnchor", false);
    }

    if ((msg == "swarm" || msg == "s") && (mod)) {
      if (bb.ValueOr<bool>("Swarm", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Marv was already swarming.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching swarm mode on.");
      }
      bb.Set<bool>("Swarm", true);
    }

    if ((msg == "swarmoff" || msg == "so") && (mod || !locked)) {
      if (bb.ValueOr<bool>("Swarm", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Swarm mode was already off.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Switching swarm mode off.");
      }
      bb.Set<bool>("Swarm", false);
    }

    if ((msg == "multi" || msg == "m") && (mod || !locked) && mPrivate) {
      if (bb.ValueOr<bool>("UseMultiFire", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Multifire is already on.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Turning on multifire.");
      }
      bb.Set<bool>("UseMultiFire", true);
    }

    if ((msg == "multioff" || msg == "mo") && (mod || !locked) && mPrivate) {
      if (bb.ValueOr<bool>("UseMultiFire", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Multifire is already off.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Turning off multifire.");
      }
      bb.Set<bool>("UseMultiFire", false);
    }

    if ((msg == "repel" || msg == "rep") && (mod || !locked)) {
      if (bb.ValueOr<bool>("UseRepel", false) == true) {
        game.SendChatMessage(":" + chat.player + ":Already using repels.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Turning repels on.");
      }
      bb.Set<bool>("UseRepel", true);
    }

    if ((msg == "repeloff" || msg == "ro") && (mod || !locked)) {
      if (bb.ValueOr<bool>("UseRepel", false) == false) {
        game.SendChatMessage(":" + chat.player + ":Repels were already off.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Turning repels off.");
      }
      bb.Set<bool>("UseRepel", false);
    }

    if ((msg.compare(0, 8, "setship ") == 0 || msg.compare(0, 3, "ss ") == 0) && (mod || !locked)) {
      uint16_t number = 12;
      if (std::isdigit(msg[msg.size() - 1])) {
        number = (std::stoi(std::string(1, msg[msg.size() - 1])) - 1);
      } else {
        game.SendChatMessage(":" + chat.player + ":Invalid selection.");
      }
      if (number >= 0 && number < 9) {
        bb.Set<uint16_t>("Ship", number);
        if (number == 8) {
          bb.Set<bool>("FreqLock", false);
          bb.Set<bool>("IsAnchor", false);
          bb.Set<bool>("Swarm", false);
          bb.Set<bool>("UseMultiFire", false);
          bb.Set<bool>("UseRepel", false);
          game.SendChatMessage(":" + chat.player + ":My behaviors are also reset when sent to spec");
        } else {
          game.SendChatMessage(":" + chat.player + ":Ship selection recieved.");
        }
      } else {
        game.SendChatMessage(":" + chat.player + ":Invalid selection.");
      }
    }

    if ((msg.compare(0, 8, "setfreq ") == 0 || msg.compare(0, 3, "sf ") == 0) && (mod || !locked)) {
      uint16_t number = 111;
      std::string freq;
      if (std::isdigit(msg[msg.size() - 1])) {
        if (std::isdigit(msg[msg.size() - 2])) {
          freq = { msg[msg.size() - 2], msg[msg.size() - 1] };
        } else {
          freq = { msg[msg.size() - 1] };
        }

        number = std::stoi(freq);
      } else {
        game.SendChatMessage(":" + chat.player + ":Invalid selection.");
      }
      if (number >= 0 && number < 100) {
        bb.Set<uint16_t>("Freq", number);
        game.SendChatMessage(":" + chat.player + ":Frequency selection recieved.");
      } else {
        game.SendChatMessage(":" + chat.player + ":Invalid selection.");
      }
    }
  }
#endif
}

// Very simple tokenizer. Doesn't treat quoted strings as one token.
std::vector<std::string> Tokenize(std::string message, char delim) {
  std::vector<std::string> tokens;

  std::size_t pos = 0;

  while ((pos = message.find(delim)) != std::string::npos) {
    // Skip over multiple delims in a row
    if (pos > 0) {
      tokens.push_back(message.substr(0, pos));
    }

    message = message.substr(pos + 1);
  }

  if (!message.empty()) {
    tokens.push_back(message);
  }

  return tokens;
}

}  // namespace marvin
