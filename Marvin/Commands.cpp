

#include "Commands.h"

#include "Common.h"

namespace marvin {
namespace mods {
std::vector<std::string> modlist{"tm_master", "baked cake", "x-demo", "lyra.", "profile", "monkey", "neostar"};
}

std::string PrefixCheck(std::string message) {
  if (message.empty()) {
    return message;
  }

  if (message[0] == '.' || message[0] == '!') {
    message.erase(message.begin());
  }

  return message;
}

bool HasAccess(std::string player) {
  player = Lowercase(player);

  for (std::size_t i = 0; i < mods::modlist.size(); i++) {
    if (player == mods::modlist[i]) {
      return true;
    }
  }
  return false;
}

void SendHelpMenu(GameProxy& game, std::string player) {
  game.SendChatMessage(":" + player + ":!commands {.c} -- see command list (pm)");
  game.SendChatMessage(":" + player + ":!modlist {.ml} -- see who can lock marv (pm)");
}

void SendModList(GameProxy& game, std::string player) {
  std::string message = ":" + player + ":";

  for (std::size_t i = 0; i < mods::modlist.size(); i++) {
    message += ", " + mods::modlist[i];
  }
  game.SendChatMessage(message);
}

void SendCommandsMenu(GameProxy& game, std::string player) {
  game.SendChatMessage(
      ":" + player +
      ":Format: enable command/disable command {shorthand command} -- description (accepted chat type) [restrictions]");
  game.SendChatMessage(":" + player + ":");
  game.SendChatMessage(":" + player +
                       ":!lockmarv/!unlockmarv {.lm/.um} -- only mods can send bot commands (any) [mod only]");
  game.SendChatMessage(":" + player +
                       ":!lockfreq/!unlockfreq {.lf/.uf} -- disable/enable the bots freq manager (any) [devastation]");
  game.SendChatMessage(":" + player + ":");
  game.SendChatMessage(":" + player + ":!anchor {.a} -- enable anchor behavior when basing (pm)");
  game.SendChatMessage(":" + player + ":!rush {.r} -- enable rush behavior when basing (pm)");
  game.SendChatMessage(":" + player + ":");
  game.SendChatMessage(":" + player + ":!multi/multioff {.m/.mo} -- toggle multifire on/off (pm)");
  game.SendChatMessage(
      ":" + player +
      ":!swarm/!swarmoff {.s/.so} -- bots will respawn quickly with low health when basing (any) [mod only]");
  game.SendChatMessage(":" + player +
                       ":!repel/!repeloff {.rep/.ro} -- use repels in basing behavior when rushing (any)");
  game.SendChatMessage(":" + player + ":");
  game.SendChatMessage(":" + player + ":!setship # {.ss} -- setship 9 to spec marv (any)");
  game.SendChatMessage(":" + player + ":!setfreq # {.sf} -- usefull for pulling bots back onto dueling freqs (any)");
}

}  // namespace marvin
