#include "CommandSystem.h"

#include "../Bot.h"
#include "../Common.h"
#include "../Debug.h"
#include "BaseDuelCommands.h"
#include "CombatRoleCommands.h"
#include "CommandsCommand.h"
#include "DelimiterCommand.h"
#include "HelpCommand.h"
#include "LockCommand.h"
#include "LockFreqCommand.h"
#include "ModListCommand.h"
#include "StatusCommands.h"
#include "ConsumableCommands.h"
#include "SetFreqCommand.h"
#include "SetShipCommand.h"
#include "SetArenaCommand.h"
#include "HSCommands.h"
#include "EGCommands.h"
#include "TipCommand.h"
#include "UFOCommand.h"
#include "CommandAnnouncerCommands.h"
#include "SayCommand.h"

namespace marvin {

constexpr int kArenaSecurityLevel = 5;
// add names in lowercase
const std::unordered_map<std::string, int> kOperators = {
    {"tm_master", 10}, {"baked cake", 10}, {"x-demo", 10}, {"lyra.", 5}, {"profile", 5}, {"monkey", 10},    
    {"neostar", 5},     {"geekgrrl", 5},  {"sed", 5},   {"frog", 5},
    {"sk", 5},         {"b.o.x.", 10},     {"devastated", 5}};

CommandSystem::CommandSystem(Zone zone) {

  RegisterCommand(std::make_shared<HelpCommand>());
  RegisterCommand(std::make_shared<CommandsCommand>());
  RegisterCommand(std::make_shared<DelimiterCommand>());
  RegisterCommand(std::make_shared<ModListCommand>());
  RegisterCommand(std::make_shared<TipCommand>());
  RegisterCommand(std::make_shared<StaffChatAnnouncerCommand>());

  RegisterCommand(std::make_shared<LockCommand>());
  RegisterCommand(std::make_shared<SetShipCommand>());
  RegisterCommand(std::make_shared<SetFreqCommand>());
  RegisterCommand(std::make_shared<SetArenaCommand>());
  RegisterCommand(std::make_shared<SayCommand>());

  RegisterCommand(std::make_shared<LockFreqCommand>());

  RegisterCommand(std::make_shared<NerfCommand>());
  RegisterCommand(std::make_shared<NormalCommand>());

  RegisterCommand(std::make_shared<MultiCommand>());
  RegisterCommand(std::make_shared<CloakCommand>());
  RegisterCommand(std::make_shared<StealthCommand>());
  RegisterCommand(std::make_shared<XRadarCommand>());
  RegisterCommand(std::make_shared<AntiWarpCommand>());

  RegisterCommand(std::make_shared<RepelCommand>());
  RegisterCommand(std::make_shared<BurstCommand>());
  RegisterCommand(std::make_shared<DecoyCommand>());
  RegisterCommand(std::make_shared<RocketCommand>());
  RegisterCommand(std::make_shared<BrickCommand>());
  RegisterCommand(std::make_shared<PortalCommand>());
  RegisterCommand(std::make_shared<ThorCommand>());

   switch (zone) {
    case Zone::Devastation: {
      RegisterCommand(std::make_shared<BDPublicCommand>());
      RegisterCommand(std::make_shared<BDPrivateCommand>());
      RegisterCommand(std::make_shared<StartBDCommand>());
      RegisterCommand(std::make_shared<StopBDCommand>());
      RegisterCommand(std::make_shared<HoldBDCommand>());
      RegisterCommand(std::make_shared<ResumeBDCommand>());
      RegisterCommand(std::make_shared<AnchorCommand>());
      RegisterCommand(std::make_shared<RushCommand>());
      RegisterCommand(std::make_shared<SwarmCommand>());
      RegisterCommand(std::make_shared<LagAttachCommand>());
      RegisterCommand(std::make_shared<UFOCommand>());
      break;
    }
    case Zone::Hyperspace: {
      RegisterCommand(std::make_shared<HSFlagCommand>());
      RegisterCommand(std::make_shared<HSCenterCommand>());
      RegisterCommand(std::make_shared<HSBuyCommand>());
      RegisterCommand(std::make_shared<HSSellCommand>());
      RegisterCommand(std::make_shared<HSShipStatusCommand>());
      RegisterCommand(std::make_shared<AnchorCommand>());
      RegisterCommand(std::make_shared<RushCommand>());
      RegisterCommand(std::make_shared<UFOCommand>());
      break;
    }
    case Zone::ExtremeGames: {
      RegisterCommand(std::make_shared<EGPCommand>());
      RegisterCommand(std::make_shared<EGLCommand>());
      RegisterCommand(std::make_shared<EGRCommand>());
      break;
    }
    default: {
      break;
    }
  }
}

int CommandSystem::GetSecurityLevel(const std::string& player) {
  auto iter = kOperators.find(Lowercase(player));

  if (iter != kOperators.end()) {
    return iter->second;
  }

  return 0;
}

bool CommandSystem::ProcessMessage(Bot& bot, ChatMessage& chat) {
  bool result = false;
  std::string& msg = chat.message;

  if (msg.empty()) return false;
  if (msg[0] != '!' && msg[0] != '.') return false;

  // the chat fetcher picks up public/private commands sent from the bot to other bots in chat
  // the chat fetcher also picks up double messages when the bot pm's itself
  // the command proccessor needs to handle commands sent from itself
  // 
  // this behavior should:
  // 1. Trigger on it's own public commands
  // 2. Not trigger on private commands sent to other bots
  // 3. Trigger on private commands sent to itself one time
  if (chat.player == bot.GetGame().GetName() && chat.type == ChatType::Private) {
   // reset the flag 
   if (time_.GetTime() > self_message_cooldown_) {
      ignore_self_message_ = true;
   }

   // always throw out the first message, and allow any after if sent within a short time window
   if (ignore_self_message_) {
      ignore_self_message_ = false;
      self_message_cooldown_ = time_.GetTime() + 250;
      return false;
   } else {
      ignore_self_message_ = true;
   }
  } 

  msg.erase(0, 1);

  std::vector<std::string> tokens = Tokenize(msg, ';');

    for (std::string current_msg : tokens) {
   std::size_t split = current_msg.find(' ');
   std::string candidate_trigger = Lowercase(current_msg.substr(0, split));

   auto [trigger, iter] = GetTrigger(commands_, candidate_trigger);

   std::string arg = current_msg.substr(trigger.size());

   if (iter != commands_.end()) {
      CommandExecutor& command = *iter->second;
      u32 access_request = (1 << (u32)chat.type);

      /*
       * bitfield evaluation example (showing 4 bits):
       * chat type is 0000 for arena message
       * request_access evaluates to 0001
       * command access can be 0001 for arena or 0101 for both arena and public
       * 0001 & 0101 = 0001 which evaluates to true or non zero
       */
      if (access_request & command.GetAccess()) {
        int security_level = 0;

        if (chat.type == ChatType::Arena) {
          security_level = kArenaSecurityLevel;
        } else {
          auto op_iter = kOperators.find(Lowercase(chat.player));

          if (op_iter != kOperators.end()) {
            security_level = op_iter->second;
          }
        }

        if (security_level >= command.GetSecurityLevel()) {
          Blackboard& bb = bot.GetBlackboard();

          // If the command is lockable, bot is locked, and requester isn't an operator then ignore it.
          // if (!(command.GetFlags() & CommandFlag_Lockable) || !bb.ValueOr<bool>("CmdLock", false) ||
          if (!(command.GetFlags() & CommandFlag_Lockable) || !bb.GetCommandLock() || security_level > 0) {
            command.Execute(*this, bot, chat.player, arg);
            if (bb.ValueOr<bool>("staffchatannouncer", false) || trigger == "cmdlogger") {
              bot.GetGame().SendChatMessage("\\" + chat.player + " has sent me command: " + std::string(trigger) + " " +
                                            arg);
            }
            result = true;
          }
        }
      }
   }
  }

  return result;


  #if 0

  for (std::string current_msg : tokens) {

    // search for commands that are delimited with a space
    std::size_t split = current_msg.find(' ');
    std::string candidate_trigger = Lowercase(current_msg.substr(0, split));

    auto [trigger, iter] = GetTrigger(commands_, candidate_trigger);

    std::string arg = current_msg.substr(trigger.size());



    
    //const std::string trigger = Lowercase(current_msg.substr(0, split));
    //std::string temp_trigger = trigger;
    //std::string arg;

    //auto iter = commands_.find(trigger);

    // search for commands that have no delimiter, but contain a digit.
    if (iter == commands_.end()) {
      
      auto digit_iter = std::find_if(trigger.begin(), trigger.end(), isdigit);

      if (digit_iter != trigger.begin() && digit_iter != trigger.end()) {
        split = digit_iter - trigger.begin() - 1;
        temp_trigger = trigger.substr(0, split + 1);
        iter = commands_.find(temp_trigger);
      }
    }

    // search for commands that end with the phrase "on"
    if (iter == commands_.end()) {
      split = trigger.find_last_of("on");

      if (split != std::string::npos) {
        split--;
        temp_trigger = trigger.substr(0, split + 1);
        iter = commands_.find(temp_trigger);
      }
    }

    // search for commands that end with the phrase "off"
    if (iter == commands_.end()) {
      split = trigger.find_last_of("off");

      if (split != std::string::npos) {
        split--;
        temp_trigger = trigger.substr(0, split + 1);
        iter = commands_.find(temp_trigger);
      }
    }

    if (split != std::string::npos) {
      arg = current_msg.substr(split + 1);
    }

    //iter = commands_.find(trigger);
    if (iter != commands_.end()) {
      CommandExecutor& command = *iter->second;
      u32 access_request = (1 << (u32)chat.type);

      /* 
      * bitfield evaluation example (showing 4 bits):
      * chat type is 0000 for arena message
      * request_access evaluates to 0001
      * command access can be 0001 for arena or 0101 for both arena and public
      * 0001 & 0101 = 0001 which evaluates to true or non zero
      */
      if (access_request & command.GetAccess()) {
          int security_level = 0;

          if (chat.type == ChatType::Arena) {
            security_level = kArenaSecurityLevel;
          } else {
            auto op_iter = kOperators.find(Lowercase(chat.player));

            if (op_iter != kOperators.end()) {
              security_level = op_iter->second;
            }
          }

          if (security_level >= command.GetSecurityLevel()) {
            Blackboard& bb = bot.GetBlackboard();

            // If the command is lockable, bot is locked, and requester isn't an operator then ignore it.
            // if (!(command.GetFlags() & CommandFlag_Lockable) || !bb.ValueOr<bool>("CmdLock", false) ||
            if (!(command.GetFlags() & CommandFlag_Lockable) || !bb.GetCommandLock() || security_level > 0) {
              command.Execute(*this, bot, chat.player, arg);
              result = true;
            }
          }
      }
    }
  }

  return result;

  #endif
}

const Operators& CommandSystem::GetOperators() const {
  return kOperators;
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

// Gets a trigger by removing common suffixes.
std::pair<std::string_view, Commands::const_iterator> CommandSystem::GetTrigger(const Commands& commands,
                                                                        std::string_view candidate) {
  auto iter = commands.find(std::string(candidate));

  // This trigger is already a perfect match so use it.
  if (iter != commands.end()) return std::make_pair(candidate, iter);

  // Start by stripping digits.
  auto digit_iter = std::find_if(candidate.begin(), candidate.end(), isdigit);

  if (digit_iter != candidate.begin() && digit_iter != candidate.end()) {
    size_t split = digit_iter - candidate.begin();

    candidate = candidate.substr(0, split);
    iter = commands.find(std::string(candidate));

    if (iter != commands.end()) return std::make_pair(candidate, iter);
  }

  // Search for common endings.
  const char* kSuffixSet[] = {"on", "off"};

  for (size_t i = 0; i < sizeof(kSuffixSet) / sizeof(*kSuffixSet); ++i) {
    if (candidate.ends_with(kSuffixSet[i])) {
      candidate = candidate.substr(0, candidate.size() - strlen(kSuffixSet[i]));

      iter = commands.find(std::string(candidate));

      // Always return here even if it fails so it doesn't strip off a bunch of suffixes.
      return std::make_pair(candidate, iter);
    }
  }

  // No triggers were found.
  return std::make_pair(candidate, commands.end());
}

}  // namespace marvin
