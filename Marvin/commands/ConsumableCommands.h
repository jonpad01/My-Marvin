#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class RepelCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseRepel, false) == true) {
      if (bb.GetUseRepel()) {
      game.SendPrivateMessage(sender, "Already using repels.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels on.");
    }

    //bb.Set<bool>(BB::UseRepel, true);
    bb.SetUseRepel(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repel"}; }
  std::string GetDescription() { return "Sets repels on"; }
  int GetSecurityLevel() { return 0; }
};

class RepelOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseRepel, false) == false) {
      if (!bb.GetUseRepel()) {
      game.SendPrivateMessage(sender, "Repels were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels off.");
    }

    //bb.Set<bool>(BB::UseRepel, false);
    bb.SetUseRepel(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repeloff"}; }
  std::string GetDescription() { return "Sets repels off"; }
  int GetSecurityLevel() { return 0; }
};

class BurstCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseBurst, false) == true) {
    if (bb.GetUseBurst()) {
      game.SendPrivateMessage(sender, "Already using burst.");
    } else {
      game.SendPrivateMessage(sender, "Using burst.");
    }

    //bb.Set<bool>(BB::UseBurst, true);
    bb.SetUseBurst(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"burst"}; }
  std::string GetDescription() { return "Sets burst on"; }
  int GetSecurityLevel() { return 0; }
};

class BurstOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseBurst, false) == false) {
    if (!bb.GetUseBurst()) {
      game.SendPrivateMessage(sender, "Burst were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning bursts off.");
    }

    //bb.Set<bool>(BB::UseBurst, false);
    bb.SetUseBurst(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"burstoff"}; }
  std::string GetDescription() { return "Sets burst off"; }
  int GetSecurityLevel() { return 0; }
};

class DecoyCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseDecoy, false) == true) {
    if (bb.GetUseDecoy()) {
      game.SendPrivateMessage(sender, "Already using decoys.");
    } else {
      game.SendPrivateMessage(sender, "Using decoys.");
    }

    //bb.Set<bool>(BB::UseDecoy, true);
    bb.SetUseDecoy(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"decoy"}; }
  std::string GetDescription() { return "Sets decoys on"; }
  int GetSecurityLevel() { return 0; }
};

class DecoyOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseDecoy, false) == false) {
    if (!bb.GetUseDecoy()) {
      game.SendPrivateMessage(sender, "Decoys were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning decoys off.");
    }

    //bb.Set<bool>(BB::UseDecoy, false);
    bb.SetUseDecoy(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"decoyoff"}; }
  std::string GetDescription() { return "Sets decoys off"; }
  int GetSecurityLevel() { return 0; }
};

class RocketCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseRocket, false) == true) {
    if (bb.GetUseRocket()) {
      game.SendPrivateMessage(sender, "Already using rockets.");
    } else {
      game.SendPrivateMessage(sender, "Using rockets.");
    }

    //bb.Set<bool>(BB::UseRocket, true);
    bb.SetUseRocket(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rocket"}; }
  std::string GetDescription() { return "Sets rockets on"; }
  int GetSecurityLevel() { return 0; }
};

class RocketOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseRocket, false) == false) {
    if (!bb.GetUseRocket()) {
      game.SendPrivateMessage(sender, "Rockets were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning rockets off.");
    }

    //bb.Set<bool>(BB::UseRocket, false);
    bb.SetUseRocket(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rocketoff"}; }
  std::string GetDescription() { return "Sets rockets off"; }
  int GetSecurityLevel() { return 0; }
};

class BrickCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseBrick, false) == true) {
    if (bb.GetUseBrick()) {
      game.SendPrivateMessage(sender, "Already using bricks.");
    } else {
      game.SendPrivateMessage(sender, "Using bricks.");
    }

    //bb.Set<bool>(BB::UseBrick, true);
    bb.SetUseBrick(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"brick"}; }
  std::string GetDescription() { return "Sets bricks on"; }
  int GetSecurityLevel() { return 0; }
};

class BrickOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseBrick, false) == false) {
    if (!bb.GetUseBrick()) {
      game.SendPrivateMessage(sender, "Bricks were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning bricks off.");
    }

    //bb.Set<bool>(BB::UseBrick, false);
    bb.SetUseBrick(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"brickoff"}; }
  std::string GetDescription() { return "Sets bricks off"; }
  int GetSecurityLevel() { return 0; }
};

class PortalCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UsePortal, false) == true) {
    if (bb.GetUsePortal()) {
      game.SendPrivateMessage(sender, "Already using portals.");
    } else {
      game.SendPrivateMessage(sender, "Using portals.");
    }

    //bb.Set<bool>(BB::UsePortal, true);
    bb.SetUsePortal(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"portal"}; }
  std::string GetDescription() { return "Sets portals on"; }
  int GetSecurityLevel() { return 0; }
};

class PortalOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UsePortal, false) == false) {
    if (!bb.GetUsePortal()) {
      game.SendPrivateMessage(sender, "Portals were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning portals off.");
    }

    //bb.Set<bool>(BB::UsePortal, false);
    bb.SetUsePortal(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"portaloff"}; }
  std::string GetDescription() { return "Sets portals off"; }
  int GetSecurityLevel() { return 0; }
};

class ThorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseThor, false) == true) {
    if (bb.GetUseThor()) {
      game.SendPrivateMessage(sender, "Already using thors.");
    } else {
      game.SendPrivateMessage(sender, "Using thors.");
    }

    //bb.Set<bool>(BB::UseThor, true);
    bb.SetUseThor(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"thor"}; }
  std::string GetDescription() { return "Sets thors on"; }
  int GetSecurityLevel() { return 0; }
};

class ThorOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>(BB::UseThor, false) == false) {
    if (!bb.GetUseThor()) {
      game.SendPrivateMessage(sender, "Thors were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning thors off.");
    }

    //bb.Set<bool>(BB::UseThor, false);
    bb.SetUseThor(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"thoroff"}; }
  std::string GetDescription() { return "Sets thors off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
