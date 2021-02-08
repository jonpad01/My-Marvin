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


    bool Common::CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile, Vector2f* Solution) {
        bool hit = true;
        //directional vector poiting to target from shooter
        Vector2f totarget = pTarget - pShooter;
        Vector2f v = vTarget - vShooter;

        float a = v.Dot(v) - sProjectile * sProjectile;
        float b = 2 * v.Dot(totarget);
        float c = totarget.Dot(totarget);

        Vector2f solution;
        Vector2f position;

        //quadratic formula
        float disc = (b * b) - 4.0f * a * c;
        float t = 0.0f;

        //if disc or t are below zero it means the target is moving away at a speed faster than the bullet
        //the shot is impossible, t will equal 0 and the solution will be the targets position, but the hit will return false
        //this is called ther square root discriminator
        if (disc >= 0.0f && a != 0.0f) {

            float t1 = (-b - std::sqrt(disc)) / (2.0f * a);
            float t2 = (-b + std::sqrt(disc)) / (2.0f * a);

            if (t1 >= 0.0f && t2 >= 0.0f) { 
                t = std::min(t1, t2);
            }
            else {
                t = std::max(t1, t2);
            }
            //this shouldnt happen, but im pretty sure it does sometimes
            if (t < 0.0f) {
                t = 0.0f;
                hit = false;
            }
        }
        else { hit = false; }
 
        //predicted position with correct angle (adjusts for shooters velocity)
        //the solution is basically the directional vector from the shooter to this position
        solution = pTarget + (v * t);
        //predicted intersect position
        position = pTarget + (vTarget * t);
        
        //use the solutions direction and predicted intersection to adjust length
        Vector2f toshot = solution - pShooter;
        if (Solution) *Solution = pShooter + Normalize(toshot) * pShooter.Distance(position);

        return hit;
    }


    bool Common::ShootWall(Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction, bool* bomb_hit, Vector2f* wall_pos) {

        Vector2f position = game_.GetPosition();
        float radius = game_.GetShipSettings().GetRadius();
        

        Vector2f center = GetWindowCenter();
        float wall_debug_y = 0.0f;
  
        uint8_t bounces = 10;

        float proj_speed = game_.GetSettings().ShipSettings[game_.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
        float alive_time = (float)game_.GetSettings().BulletAliveTime;

        std::vector<Vector2f> pos_adjust;

        //if ship has double barrel make 2 calculations with offset positions
        if ((game_.GetShipSettings().DoubleBarrel & 1) != 0) {
            Vector2f side = Perpendicular(game_.GetPlayer().GetHeading());
            //Vector2f side = game_->GetPlayer().GetHeading();

            pos_adjust.push_back(side * radius * 0.8f);
            pos_adjust.push_back(side * -radius * 0.8f);
        }
        //if not make one calculation from center
        else {
            pos_adjust.push_back(Vector2f(0, 0));
        }
        //push back one more position for bomb calculation
        pos_adjust.push_back(Vector2f(0, 0));

        if (bomb_hit) *bomb_hit = false;

        // RenderText("Bot::CalculateWallShot", center - Vector2f(wall_debug_y, 300), RGB(100, 100, 100), RenderText_Centered);

        for (std::size_t i = 0; i < pos_adjust.size(); i++) {

            float traveled_dist = 0.0f;
            position = game_.GetPosition() + pos_adjust[i];
            
            //when the last shot is reached change to bomb settings, if bomb bounce is 0 this is skipped
            if (i == (pos_adjust.size() - 1)) {
                bounces = game_.GetSettings().ShipSettings[game_.GetPlayer().ship].BombBounceCount;
                proj_speed = game_.GetSettings().ShipSettings[game_.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
                alive_time = (float)game_.GetSettings().BombAliveTime;
            }
   

            float travel = (proj_speed + game_.GetPlayer().velocity.Length()) * (alive_time / 100.0f);
            Vector2f sDirection = Normalize((direction * proj_speed) + game_.GetPlayer().velocity);
            Vector2f velocity = game_.GetPlayer().velocity;

            for (int j = 0; j < bounces + 1; ++j) {

                bool hit = true;

                //how long a bullet at normal speed would take to travel the total distance to the target
                float travel_time = (traveled_dist + target_pos.Distance(position)) / proj_speed;
                //calculate a new speed using time and the position the shot will be calulated at
                float traveled_speed = target_pos.Distance(position) / travel_time;

                Vector2f solution;

                //the shot solution, calculated at the wall, travel time is included by reducing the bullet speed
                if (!CalculateShot(position, target_pos, velocity, target_vel, traveled_speed, &solution)) {
                    hit = false;
                }

                Vector2f t2shot = solution - target_pos;
                CastResult t2shot_line = RayCast(game_.GetMap(), target_pos, Normalize(t2shot), t2shot.Length());

                Vector2f tosPosition = solution - position;
                CastResult tosPosition_line = RayCast(game_.GetMap(), position, Normalize(tosPosition), tosPosition.Length());

                if (!tosPosition_line.hit && !t2shot_line.hit) {

                    float test_radius = target_radius * 2.0f;
                    Vector2f box_min = solution - Vector2f(test_radius, test_radius);
                    Vector2f box_extent(test_radius * 1.2f, test_radius * 1.2f);
                    float dist;
                    Vector2f norm;

                    //the calculate shot function can return correct shots but at long or short distances depending on player velocity
                    if (RayBoxIntersect(position, sDirection, box_min, box_extent, &dist, &norm)) {
                        if (position.Distance(solution) <= travel && hit == true) {
                            //RenderWorldLine(game_->GetPosition(), position, position + tosPosition * position.Distance(solution));
                            RenderWorldLine(game_.GetPosition(), position, solution, RGB(100, 0, 0));

                            RenderText("HIT", center - Vector2f(wall_debug_y, 40), RGB(100, 100, 100), RenderText_Centered);
                            if (i == (pos_adjust.size() - 1)) {
                                if (bomb_hit) {
                                    *bomb_hit = true;
                                }
                            }
                            if (j == 0 && wall_pos) {
                                    *wall_pos = solution;
                            }
                            return true;
                        }
                    }
                }


                CastResult wall_line = RayCast(game_.GetMap(), position, sDirection, travel);
                

                if (wall_line.hit) {

                    RenderWorldLine(game_.GetPosition(), position, wall_line.position, RGB(0, 100, 100));

                    sDirection = sDirection - (wall_line.normal * (2.0f * sDirection.Dot(wall_line.normal)));
                    velocity = velocity - (wall_line.normal * (2.0f * velocity.Dot(wall_line.normal)));

                    position = wall_line.position;

                    travel -= wall_line.distance;
                    traveled_dist += wall_line.distance;

                    if (j == 0 && wall_pos) {
                        *wall_pos = position;
                    }
                }
                else {
                    RenderWorldLine(game_.GetPosition(), position, position + sDirection * travel, RGB(0, 100, 100));
                    RenderWorldLine(game_.GetPosition(), position, solution, RGB(100, 0, 0));
                    break;
                }
            }
        }
        return false;
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


    std::size_t Common::FindPathNode(std::vector<Vector2f> path, Vector2f position) {

        std::size_t node = 0;
        float closest_distance = std::numeric_limits<float>::max();

        for (std::size_t i = 0; i < path.size(); i++) {

            Vector2f to_target = game_.GetPosition() - path[i];
            CastResult result = RayCast(game_.GetMap(), game_.GetPosition(), Normalize(to_target), to_target.Length());

            if (result.hit) { continue; }

            float distance = position.Distance(path[i]);

            if (closest_distance > distance) {
                node = i;
                closest_distance = distance;
            }
        }
        return node;
    }


    float Common::PathLength(std::vector<Vector2f> path, std::size_t index1, std::size_t index2) {

        std::size_t start;
        std::size_t end;

        if (index1 < index2) {
            start = index1;
            end = index2;
        }
        else {
            start = index2;
            end = index1;
        }

        float distance = 0.0f;

        for (start; start < end; start++) {
            distance += path[start].Distance(path[start + 1]);
        }

        return distance;
    }

    bool Common::CanShootGun(const Map& map, const Player& bot_player, const Player& target) {

        float bullet_speed = game_.GetSettings().ShipSettings[game_.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
        float speed_adjust = (game_.GetPlayer().velocity * game_.GetPlayer().GetHeading());

        float bullet_travel = ((bullet_speed + speed_adjust) / 100.0f) * game_.GetSettings().BulletAliveTime;

        if (bot_player.position.Distance(target.position) > bullet_travel) return false;
        if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

        return true;
    }

    bool Common::CanShootBomb(const Map& map, const Player& bot_player, const Player& target) {

        float bomb_speed = game_.GetSettings().ShipSettings[game_.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
        float speed_adjust = (game_.GetPlayer().velocity * game_.GetPlayer().GetHeading());

        float bomb_travel = ((bomb_speed + speed_adjust) / 100.0f) * game_.GetSettings().BombAliveTime;

        if (bot_player.position.Distance(target.position) > bomb_travel) return false;
        if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

        return true;
    }



    std::vector<Vector2f> Common::CreatePath(path::Pathfinder& pathfinder, std::vector<Vector2f> path, Vector2f from, Vector2f to, float radius) {
        bool build = true;

        if (!path.empty()) {
            // Check if the current destination is the same as the requested one.
            if (path.back().DistanceSq(to) < 3 * 3) {
                Vector2f pos = game_.GetPosition();
                Vector2f next = path.front();
                Vector2f direction = Normalize(next - pos);
                Vector2f side = Perpendicular(direction);
                float radius = game_.GetShipSettings().GetRadius();

                float distance = next.Distance(pos);

                // Rebuild the path if the bot isn't in line of sight of its next node.
                CastResult center = RayCast(game_.GetMap(), pos, direction, distance);
                CastResult side1 = RayCast(game_.GetMap(), pos + side * radius, direction, distance);
                CastResult side2 = RayCast(game_.GetMap(), pos - side * radius, direction, distance);

                if (!center.hit && !side1.hit && !side2.hit) {
                    build = false;
                }
            }
        }

        if (build) {
            std::vector<Vector2f> mines;
            //#if 0
            for (Weapon* weapon : game_.GetWeapons()) {
                const Player* weapon_player = game_.GetPlayerById(weapon->GetPlayerId());
                if (weapon_player == nullptr) continue;
                if (weapon_player->frequency == game_.GetPlayer().frequency) continue;
                if (weapon->GetType() & 0x8000) mines.push_back(weapon->GetPosition());
            }
            //#endif
            path = pathfinder.FindPath(game_.GetMap(), mines, from, to, radius);
            path = pathfinder.SmoothPath(path, game_.GetMap(), radius);
        }

        return path;
    }



}
