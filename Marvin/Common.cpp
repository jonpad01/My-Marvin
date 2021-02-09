#include <chrono>

#include "Common.h"
#include "RayCaster.h"
#include "Debug.h"

namespace marvin {


    //the viewing area the bot could see if it were a player
    bool InRect(Vector2f pos, Vector2f min_rect, Vector2f max_rect) {
        return ((pos.x >= min_rect.x && pos.y >= min_rect.y) &&
            (pos.x <= max_rect.x && pos.y <= max_rect.y));
    }

    //probably checks if the target is dead
    bool IsValidPosition(Vector2f position) {
        return position.x >= 0 && position.x < 1024 && position.y >= 0 && position.y < 1024;
    }

    uint64_t Common::GetTime() {
        return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
    }


    std::size_t Common::FindOpenFreq(std::vector<uint16_t> list, std::size_t start_pos) {

        int open_freq = 0;

        for (std::size_t i = start_pos; i < list.size(); i++) {
            if (list[i] == 0) {
                open_freq = i;
                break;
            }
        }
        return open_freq;
    }
}
