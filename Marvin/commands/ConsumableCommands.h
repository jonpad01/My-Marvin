#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class RepelCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRepel", false) == true) {
      game.SendPrivateMessage(sender, "Already using repels.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels on.");
    }

    bb.Set<bool>("UseRepels", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repel", "rep"}; }
  std::string GetDescription() { return "Sets repels on"; }
  int GetSecurityLevel() { return 0; }
};

class RepelOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRepel", false) == false) {
      game.SendPrivateMessage(sender, "Repels were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels off.");
    }

    bb.Set<bool>("UseRepel", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repeloff", "ro"}; }
  std::string GetDescription() { return "Sets repels off"; }
  int GetSecurityLevel() { return 0; }
};

class BurstCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseBurst", false) == true) {
      game.SendPrivateMessage(sender, "Already using burst.");
    } else {
      game.SendPrivateMessage(sender, "Using burst.");
    }

    bb.Set<bool>("UseBurst", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"burst"}; }
  std::string GetDescription() { return "Sets burst on"; }
  int GetSecurityLevel() { return 0; }
};

class BurstOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseBurst", false) == false) {
      game.SendPrivateMessage(sender, "Burst were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning bursts off.");
    }

    bb.Set<bool>("UseBurst", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"burstoff"}; }
  std::string GetDescription() { return "Sets burst off"; }
  int GetSecurityLevel() { return 0; }
};

class DecoyCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseDecoy", false) == true) {
      game.SendPrivateMessage(sender, "Already using decoys.");
    } else {
      game.SendPrivateMessage(sender, "Using decoys.");
    }

    bb.Set<bool>("UseDecoy", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"decoy"}; }
  std::string GetDescription() { return "Sets decoys on"; }
  int GetSecurityLevel() { return 0; }
};

class DecoyOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseDecoy", false) == false) {
      game.SendPrivateMessage(sender, "Decoys were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning decoys off.");
    }

    bb.Set<bool>("UseDecoy", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"decoyoff"}; }
  std::string GetDescription() { return "Sets decoys off"; }
  int GetSecurityLevel() { return 0; }
};

class RocketCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRocket", false) == true) {
      game.SendPrivateMessage(sender, "Already using rockets.");
    } else {
      game.SendPrivateMessage(sender, "Using rockets.");
    }

    bb.Set<bool>("UseRocket", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rocket"}; }
  std::string GetDescription() { return "Sets rockets on"; }
  int GetSecurityLevel() { return 0; }
};

class RocketOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRocket", false) == false) {
      game.SendPrivateMessage(sender, "Rockets were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning rockets off.");
    }

    bb.Set<bool>("UseRocket", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rocketoff"}; }
  std::string GetDescription() { return "Sets rockets off"; }
  int GetSecurityLevel() { return 0; }
};

class BrickCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseBrick", false) == true) {
      game.SendPrivateMessage(sender, "Already using bricks.");
    } else {
      game.SendPrivateMessage(sender, "Using bricks.");
    }

    bb.Set<bool>("UseBrick", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"brick"}; }
  std::string GetDescription() { return "Sets bricks on"; }
  int GetSecurityLevel() { return 0; }
};

class BrickOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseBrick", false) == false) {
      game.SendPrivateMessage(sender, "Bricks were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning bricks off.");
    }

    bb.Set<bool>("UseBrick", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"brickoff"}; }
  std::string GetDescription() { return "Sets bricks off"; }
  int GetSecurityLevel() { return 0; }
};

class PortalCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UsePortal", false) == true) {
      game.SendPrivateMessage(sender, "Already using portals.");
    } else {
      game.SendPrivateMessage(sender, "Using portals.");
    }

    bb.Set<bool>("UsePortal", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"portal"}; }
  std::string GetDescription() { return "Sets portals on"; }
  int GetSecurityLevel() { return 0; }
};

class PortalOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UsePortal", false) == false) {
      game.SendPrivateMessage(sender, "Portals were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning portals off.");
    }

    bb.Set<bool>("UsePortal", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"portaloff"}; }
  std::string GetDescription() { return "Sets portals off"; }
  int GetSecurityLevel() { return 0; }
};

class ThorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseThor", false) == true) {
      game.SendPrivateMessage(sender, "Already using thors.");
    } else {
      game.SendPrivateMessage(sender, "Using thors.");
    }

    bb.Set<bool>("UseThor", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"thor"}; }
  std::string GetDescription() { return "Sets thors on"; }
  int GetSecurityLevel() { return 0; }
};

class ThorOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseThor", false) == false) {
      game.SendPrivateMessage(sender, "Thors were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning thors off.");
    }

    bb.Set<bool>("UseThor", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"thoroff"}; }
  std::string GetDescription() { return "Sets thors off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
