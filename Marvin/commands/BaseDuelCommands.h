#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class BDPublicCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    if (game.GetZone() != Zone::Devastation) {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
      return;
    }

    const std::vector<std::string> commands = {"bdstart, bdstop, bdhold, bdresume"};

    for (std::string command : commands) {
      auto iter = cmd.GetCommands().find(command);
      if (iter != cmd.GetCommands().end()) {
        iter->second->SetAccess(CommandAccess_Public);
      }
    }
    game.SendChatMessage("This bot now only listens to public baseduel commands. (Command sent by: " + sender + ")");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdpublic"}; }
  std::string GetDescription() { return "Allow the bot to accept public baseduel commands."; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Hosting; };
};

class BDPrivateCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    if (game.GetZone() != Zone::Devastation) {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
      return;
    }

    const std::vector<std::string> commands = {"bdstart, bdstop, bdhold, bdresume"};

    for (std::string command : commands) {
      auto iter = cmd.GetCommands().find(command);
      if (iter != cmd.GetCommands().end()) {
        iter->second->SetAccess(CommandAccess_Private);
      }
    }
    game.SendChatMessage("This bot now only listens to private baseduel commands. (Command sent by: " + sender + ")");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdprivate"}; }
  std::string GetDescription() { return "Set the bot to only accept private baseduel commands."; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Hosting; };
};


class StartBDCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>("RunBD", false)) {
      if (bb.GetBDState() == BDState::Running) {
      game.SendPrivateMessage(sender, "I am currently running a baseduel game.");
      return;
      }
      if (bb.GetBDState() == BDState::Start) {
      game.SendPrivateMessage(sender, "I am currently starting a baseduel game.");
      return;
      }
      if (bb.GetBDState() == BDState::Paused) {
      game.SendPrivateMessage(sender, "I am currently paused in the middle of a game.");
      return;
      }
      if (bb.GetBDState() == BDState::Ended) {
      game.SendPrivateMessage(sender, "I am currently ending a game, please try the command again.");
      return;
      }

    if (game.GetZone() == Zone::Devastation) {
     // bb.Set<bool>("RunBD", true);
     // bb.Set<bool>("BDWarpToBase", true);
      bb.Set<BDState>("bdstate", BDState::Start);
      bb.Set<bool>("reloadbot", true);
      //bb.SetWarpToState(WarpToState::Base);
      game.SendChatMessage("Starting a base duel game. (Command sent by: " + sender + ")");
    } else {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
    }
  }

  StartBDCommand() : access(CommandAccess_Private) {}
  CommandAccessFlags GetAccess() { return access; }
  void SetAccess(CommandAccessFlags flags) { access = flags; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdstart"}; }
  std::string GetDescription() { return "Start a base duel game."; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Hosting; };

  private:
  CommandAccessFlags access;
};

class StopBDCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    if (game.GetZone() == Zone::Devastation) {
      // if (!bb.ValueOr<bool>("RunBD", false)) {
      if (bb.GetBDState() == BDState::Stopped || bb.GetBDState() == BDState::Ended) {
        game.SendPrivateMessage(sender, "I am not running a baseduel game.");
        return;
      }

      //if (bb.ValueOr<bool>("HoldBD", false) == false) {
      if (bb.GetBDState() != BDState::Paused) {
        game.SendPrivateMessage(sender, "You must hold the game first before using this command.");
      } else {
        //bb.Set<bool>("RunBD", false);
        //bb.Set<bool>("BDManualStop", true);
        //bb.Set<bool>("HoldBD", false);
        bb.SetBDState(BDState::Ended);
        game.SendChatMessage("The baseduel game has been stopped. (Command sent by: " + sender + ")");
      }
    } else {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
    }
  }

  StopBDCommand() : access(CommandAccess_Private) {}
  CommandAccessFlags GetAccess() { return access; }
  void SetAccess(CommandAccessFlags flags) { access = flags; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdstop"}; }
  std::string GetDescription() { return "Stop a base duel game (resets score)."; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Hosting; };

  private:
  CommandAccessFlags access;
};

class HoldBDCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    if (game.GetZone() == Zone::Devastation) {
      // if (bb.ValueOr<bool>("HoldBD", false)) {
      if (bb.GetBDState() == BDState::Paused) {
        game.SendPrivateMessage(sender, "I am already holding a baseduel game.");
        return;
      }
      if (bb.GetBDState() == BDState::Stopped || bb.GetBDState() == BDState::Ended) {
        game.SendPrivateMessage(sender, "I am not running a baseduel game.");
        return;
      }

      //bb.Set<bool>("HoldBD", true);
      bb.SetBDState(BDState::Paused);
      game.SendChatMessage("Holding game. (Command sent by: " + sender + ")");
    } else {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
    }
  }

  HoldBDCommand() : access(CommandAccess_Private) {}
  CommandAccessFlags GetAccess() { return access; }
  void SetAccess(CommandAccessFlags flags) { access = flags; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdhold"}; }
  std::string GetDescription() { return "Hold a base duel game (score is saved)"; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Hosting; };

  private:
  CommandAccessFlags access;
};

class ResumeBDCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    if (game.GetZone() == Zone::Devastation) {
      
        //if (!bb.ValueOr<bool>("HoldBD", false)) {
      if (bb.GetBDState() == BDState::Stopped || bb.GetBDState() == BDState::Ended) {
        game.SendPrivateMessage(sender, "I am not running a baseduel game.");
        return;
      }

        if (bb.GetBDState() != BDState::Paused) {
        game.SendPrivateMessage(sender, "I am not holding a baseduel game. (it's currently running)");
        return;
        }

     // bb.Set<bool>("HoldBD", false);
      bb.SetBDState(BDState::Running);
      game.SendChatMessage("Resuming game. (Command sent by: " + sender + ")");
    } else {
      game.SendPrivateMessage(sender, "There is no support for baseduel in this zone.");
    }
  }

  ResumeBDCommand() : access(CommandAccess_Private) {}
  CommandAccessFlags GetAccess() { return access; }
  void SetAccess(CommandAccessFlags flags) { access = flags; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"bdresume"}; }
  std::string GetDescription() { return "Resume a base duel game"; }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Hosting; };

  private:
  CommandAccessFlags access;
};

class SwarmCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetSwarm()) {
        game.SendPrivateMessage(sender, "swarm currently ON.");
      } else {
        game.SendPrivateMessage(sender, "swarm currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning swarm ON.");
        bb.SetSwarm(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning swarm OFF.");
        bb.SetSwarm(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!swarm on\" or \"!swarm off\" or \"!swarm\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarm"}; }
  std::string GetDescription() {
    return "Enable swarm behavior. Bots will respawn quickly with low health when basing";
  }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Behavior; }
};

class LagAttachCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.ValueOr<bool>("allowlagattaching", true)) {
        game.SendPrivateMessage(sender, "lag attaching acurrently ON.");
      } else {
        game.SendPrivateMessage(sender, "lag attaching currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning lag attaching ON.");
        bb.Set<bool>("allowlagattaching", true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning lag attaching OFF.");
        bb.Set<bool>("allowlagattaching", false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!lagattach on\" or \"!lagattach off\" or \"!lagattach\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"lagattach"}; }
  std::string GetDescription() {
    return "Enable lag attach behavior.";
  }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Behavior; }
};

}  // namespace marvin