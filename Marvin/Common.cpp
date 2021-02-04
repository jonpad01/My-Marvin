#include <chrono>

#include "Common.h"
#include "RayCaster.h"
#include "Debug.h"

namespace marvin {


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
        if (disc >= 0.0f) {

            float t1 = (-b - std::sqrt(disc)) / (2.0f * a);
            float t2 = (-b + std::sqrt(disc)) / (2.0f * a);

            if (t1 >= 0.0f && t2 >= 0.0f) { 
                t = std::min(t1, t2);
            }
            else {
                t = std::max(t1, t2);
            }
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


    bool Common::ShootWall(Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction, float proj_speed, int alive_time, uint8_t bounces) {

        direction = Normalize((direction * proj_speed) + game_->GetPlayer().velocity);

        Vector2f position = game_->GetPosition();
        Vector2f velocity = game_->GetPlayer().velocity;

        Vector2f center = GetWindowCenter();
        Vector2f center_adjust = GetWindowCenter();
        float wall_debug_y = 0.0f;

        // RenderText("Bot::CalculateWallShot", center - Vector2f(wall_debug_y, 300), RGB(100, 100, 100), RenderText_Centered);

        float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());

        float travel = (proj_speed + speed_adjust) * (alive_time / 100.0f);
        float traveled_dist = 0.0f;


        for (int i = 0; i < bounces + 1; ++i) {

            if (i != 0) {

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

                Vector2f to_target = target_pos - position;
                CastResult to_target_line = RayCast(game_->GetMap(), position, Normalize(to_target), to_target.Length());

                Vector2f tosPosition = solution - position;
                CastResult tosPosition_line = RayCast(game_->GetMap(), position, Normalize(tosPosition), tosPosition.Length());



                if (!tosPosition_line.hit) {


                    float test_radius = target_radius * 3.0f;
                    Vector2f box_min = solution - Vector2f(test_radius, test_radius);
                    Vector2f box_extent(test_radius * 1.2f, test_radius * 1.2f);
                    float dist;
                    Vector2f norm;

                    //the calculate shot function can return correct shots but at long or short distances depending on player velocity
                    if (RayBoxIntersect(position, direction, box_min, box_extent, &dist, &norm)) {
                        if (dist < travel && hit == true) {
                            RenderLine(center_adjust, center_adjust + (direction * tosPosition_line.distance * 16), RGB(100, 100, 100));
                            RenderLine(center_adjust, center_adjust + (Normalize(tosPosition) * tosPosition_line.distance * 16), RGB(100, 0, 0));

                            RenderText("hit", center - Vector2f(wall_debug_y, 200), RGB(100, 100, 100), RenderText_Centered);
                            return true;
                        }
                    }
                }
                
            }

            CastResult wall_line = RayCast(game_->GetMap(), position, direction, travel);

            if (wall_line.hit) {
                  RenderLine(center_adjust, center_adjust + (direction * wall_line.distance * 16), RGB(100, 100, 100));

                center_adjust += (direction * wall_line.distance * 16);

                direction = direction - (wall_line.normal * (2.0f * direction.Dot(wall_line.normal)));
                velocity = velocity - (wall_line.normal * (2.0f * velocity.Dot(wall_line.normal)));

                position = wall_line.position;

                travel -= wall_line.distance;
                traveled_dist += wall_line.distance;
            }
            else {
                RenderLine(center_adjust, center_adjust + (direction * travel * 16), RGB(100, 100, 100));
                return false;
            }
        }
        return false;
    }

    void Common::RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to) {
        Vector2f center = GetWindowCenter();

        Vector2f diff = to - from;
        from = (from - screenCenterWorldPosition) * 16.0f;
        to = from + (diff * 16.0f);

        RenderLine(center + from, center + to, RGB(200, 0, 0));
    }

    void Common::RenderPath(GameProxy& game, behavior::Blackboard& blackboard) {
        std::vector<Vector2f> path = blackboard.ValueOr("path", std::vector<Vector2f>());
        const Player& player = game.GetPlayer();

        if (path.empty()) return;

        for (std::size_t i = 0; i < path.size() - 1; ++i) {
            Vector2f current = path[i];
            Vector2f next = path[i + 1];

            if (i == 0) {
                RenderWorldLine(player.position, player.position, current);
            }

            RenderWorldLine(player.position, current, next);
        }
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

            Vector2f to_target = game_->GetPosition() - path[i];
            CastResult result = RayCast(game_->GetMap(), game_->GetPosition(), Normalize(to_target), to_target.Length());

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






}
