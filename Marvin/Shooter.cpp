#include "Shooter.h"

#include "RayCaster.h"
#include "Debug.h"



namespace marvin {

    bool CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile, Vector2f* Solution) {
        bool hit = true;
        //directional vector poiting to target from shooter
        Vector2f totarget = pTarget - pShooter;
        Vector2f v = vTarget - vShooter;

        //dot product
        float a = v.Dot(v) - sProjectile * sProjectile;
        float b = 2 * v.Dot(totarget);
        float c = totarget.Dot(totarget);

        //quadratic formula
        float disc = (b * b) - 4.0f * a * c;
        float t = 0.0f;

        //if disc or t are below zero it means the target is moving away at a speed faster than the bullet
        //the shot is impossible, t will equal 0 and the solution will be the targets position, but the hit will return false
        //this is called the square root discriminant
        if (disc >= 0.0f && a != 0.0f && c != 0.0f) {

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

        //the solution is basically the directional vector from the shooter to this position
        Vector2f solution = pTarget + (v * t);
        //this is the intersected position
        Vector2f position = pTarget + (vTarget * t);
        //return a position with the direcrtional solution and the magnitude solution
        if (Solution) *Solution = pShooter + Normalize(solution - pShooter) * pShooter.Distance(position);
        //this solution is used for bouncing shots, determining hover distance, and bullet travel time to target
        return hit;
    }


    bool BounceShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction, bool* bomb_hit, Vector2f* wall_pos) {

        Vector2f position = game.GetPosition();
        float radius = game.GetShipSettings().GetRadius();


        Vector2f center = GetWindowCenter();
        float wall_debug_y = 0.0f;

        uint8_t bounces = 10;

        float proj_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
        float alive_time = (float)game.GetSettings().BulletAliveTime;

        std::vector<Vector2f> pos_adjust;

        //if ship has double barrel make 2 calculations with offset positions
        if ((game.GetShipSettings().DoubleBarrel & 1) != 0) {
            Vector2f side = Perpendicular(game.GetPlayer().GetHeading());
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
            position = game.GetPosition() + pos_adjust[i];

            //when the last shot is reached change to bomb settings, if bomb bounce is 0 this is skipped
            if (i == (pos_adjust.size() - 1)) {
                bounces = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombBounceCount;
                proj_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
                alive_time = (float)game.GetSettings().BombAliveTime;
            }


            float travel = (proj_speed + (game.GetPlayer().velocity * game.GetPlayer().GetHeading())) * (alive_time / 100.0f);
            Vector2f sDirection = Normalize((direction * proj_speed) + game.GetPlayer().velocity);
            Vector2f velocity = game.GetPlayer().velocity;

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
                CastResult t2shot_line = RayCast(game.GetMap(), target_pos, Normalize(t2shot), t2shot.Length());

                Vector2f tosPosition = solution - position;
                CastResult tosPosition_line = RayCast(game.GetMap(), position, Normalize(tosPosition), tosPosition.Length());

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
                           // RenderWorldLine(game_.GetPosition(), position, solution, RGB(100, 0, 0));

                           // RenderText("HIT", center - Vector2f(wall_debug_y, 40), RGB(100, 100, 100), RenderText_Centered);
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


                CastResult wall_line = RayCast(game.GetMap(), position, sDirection, travel);


                if (wall_line.hit) {

                    // RenderWorldLine(game_.GetPosition(), position, wall_line.position, RGB(0, 100, 100));

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
                    //  RenderWorldLine(game_.GetPosition(), position, position + sDirection * travel, RGB(0, 100, 100));
                     // RenderWorldLine(game_.GetPosition(), position, solution, RGB(100, 0, 0));
                    break;
                }
            }
        }
        return false;
    }


    bool CanShootGun(GameProxy& game, const Map& map, Vector2f player, Vector2f target) {

        float bullet_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;

        float bullet_travel = (bullet_speed + (game.GetPlayer().velocity * game.GetPlayer().GetHeading())) * ((float)game.GetSettings().BulletAliveTime / 100.0f);

        if (player.Distance(target) > bullet_travel) return false;
        if (map.GetTileId(player) == marvin::kSafeTileId) return false;

        return true;
    }

    bool CanShootBomb(GameProxy& game, const Map& map, Vector2f player, Vector2f target) {

        float bomb_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;

        float bomb_travel = (bomb_speed + (game.GetPlayer().velocity * game.GetPlayer().GetHeading())) * ((float)game.GetSettings().BombAliveTime / 100.0f);

        if (player.Distance(target) > bomb_travel) return false;
        if (map.GetTileId(player) == marvin::kSafeTileId) return false;

        return true;
    }






} //namespace marvin