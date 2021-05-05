#include "Pathfinder.h"

#include <algorithm>
#include <cmath>

#include "../RayCaster.h"
#include "../Bot.h"
#include "../Debug.h"

namespace marvin {



    Vector2f LastLOSNode(const Map& map, std::size_t index, bool count_down, std::vector<Vector2f> path, float radius) {

        Vector2f position;

        if (path.empty()) { return position; }

        if (count_down) {
            //position = path.back();

            for (std::size_t i = index; i > 0; i--) {

                Vector2f current = path[i];

                if (!RadiusRayCastHit(map, path[index], current, radius)) {
                    position = current;
                }
                else { break; }
            }
        }
        else {
          //  position = path.front();

            for (std::size_t i = index; i < path.size(); i++) {

                Vector2f current = path[i];

                if (!RadiusRayCastHit(map, path[index], current, radius)) {
                    position = current;
                }
                else { break; }
            }

        }
        return position;
    }



    float PathLength(std::vector<Vector2f> path, Vector2f pos1, Vector2f pos2) {
      
        float path_distance = 0.0f;
        
        if (path.empty()) { return path_distance; }

        std::size_t index1 = FindPathIndex(path, pos1);
        std::size_t index2 = FindPathIndex(path, pos2);

        if (index1 == index2) {
            return pos1.Distance(pos2);
        }
        std::size_t start = std::min(index1, index2);
        std::size_t end = std::max(index1, index2);

        for (std::size_t i = start; i < end; i++) {
            path_distance += path[i].Distance(path[i + 1]);
        }

        return path_distance;
    }

    std::size_t FindPathIndex(std::vector<Vector2f> path, Vector2f position) {

        std::size_t path_index = 0;
        float closest_distance = std::numeric_limits<float>::max();

        if (path.empty()) { return path_index; }

        for (std::size_t i = 0; i < path.size(); i++) {

            float distance = position.DistanceSq(path[i]);

            if (closest_distance > distance) {
                path_index = i;
                closest_distance = distance;
            }
        }

        return path_index;
    }

    namespace path {

        

        Vector2f ClosestWall(const Map& map, Vector2f pos, int search) {
            float closest_dist = std::numeric_limits<float>::max();
            Vector2f closest;

            Vector2f base(std::floor(pos.x), std::floor(pos.y));
            for (int y = -search; y <= search; ++y) {
                for (int x = -search; x <= search; ++x) {
                    Vector2f current = base + Vector2f((float)x, (float)y);

                    if (!map.IsSolid((unsigned short)current.x, (unsigned short)current.y)) {
                        continue;
                    }

                    float dist = BoxPointDistance(current, Vector2f(1, 1), pos);

                    if (dist < closest_dist) {
                        closest_dist = dist;
                        closest = current;
                    }
                }
            }

            return closest;
        }

        bool IsPassablePath(const Map& map, Vector2f from, Vector2f to, float radius) {
            const Vector2f direction = Normalize(to - from);
            const Vector2f side = Perpendicular(direction) * radius;
            const float distance = from.Distance(to);

            CastResult cast_center = RayCast(map, from, direction, distance);
            CastResult cast_side1 = RayCast(map, from + side, direction, distance);
            CastResult cast_side2 = RayCast(map, from - side, direction, distance);

            return !cast_center.hit && !cast_side1.hit && !cast_side2.hit;
        }

        NodePoint ToNodePoint(const Vector2f v, float radius, const Map& map) {
            NodePoint np;

            np.x = (uint16_t)v.x;
            np.y = (uint16_t)v.y;

            Vector2f check_pos(np.x, np.y);

            //searches outward from the center, prevents the end node from breaking the pathfinder
            if (!map.CanOccupy(check_pos, radius)) {
                for (int i = 1; i < radius + 2; i++) {
                    for (int x = -i; x < i + 1; x++) {
                        for (int y = -i; y < i + 1; y++) {
                            Vector2f check_pos(np.x + (float)x, np.y + (float)y);
                            if (map.CanOccupy(check_pos, radius)) {
                                np.y += y;
                                np.x += x;
                                return np;
                            }
                        }
                    }
                }
            }

            return np;
        }

        float Euclidean(const Node* from, const Node* to) {
            float dx = static_cast<float>(from->point.x - to->point.x);
            float dy = static_cast<float>(from->point.y - to->point.y);

            return std::sqrt(dx * dx + dy * dy);
        }

        Pathfinder::Pathfinder(std::unique_ptr<NodeProcessor> processor) : processor_(std::move(processor)) {}

        std::vector<Vector2f> Pathfinder::FindPath(const Map& map, std::vector<Vector2f> mines, const Vector2f& from, const Vector2f& to, float radius) {
           
            std::vector<Vector2f> path;

            processor_->ResetNodes();

            Node* start = processor_->GetNode(ToNodePoint(from, radius, map));
            Node* goal = processor_->GetNode(ToNodePoint(to, radius, map));
            
            if (start == nullptr || goal == nullptr) {
                return path;
            }

            //clear vector then add start node
            openset_.Clear();
            openset_.Push(start);

            //at the start there is only one node here, the start node
            while (!openset_.Empty()) {
                //grab front item then delete it
                Node* node = openset_.Pop();

                //this is the only way to break the pathfinder
                if (node == goal) {
                    break;
                }

                node->closed = true;

                //returns neighbor nodes that are not solid
                NodeConnections connections = processor_->FindEdges(mines, node, start, goal, radius);

                for (std::size_t i = 0; i < connections.count; ++i) {
                    Node* edge = connections.neighbors[i];
                    //float cost = node->g + edge->weight;
                    float dx = (float)(node->point.x - edge->point.x);
                    float dy = (float)(node->point.y - edge->point.y);
                    float d = std::sqrt(dx * dx + dy * dy);
                    float cost = node->g + edge->weight * d;

                    Node* parent = node->parent;
                    if (parent) {
                        NodePoint parent_diff(parent->point.x - node->point.x, parent->point.y - node->point.y);
                        NodePoint current_diff(node->point.x - edge->point.x, node->point.y - edge->point.y);

                        edge->rotations = node->rotations;

                        if (parent_diff.x != current_diff.x || parent_diff.y != current_diff.y) {
                            ++edge->rotations;
                        }
                    }

                    if (edge->closed && cost < edge->g) {
                        edge->closed = false;
                    }

                    if (edge->openset && cost < edge->g) {
                        edge->g = cost;
                        edge->f = edge->g + edge->h;
                        edge->parent = node;
                        //sorts from highest to lowest
                        openset_.Update();
                    }
                    //all valid neighbors get in on first pass
                    else if (!edge->openset && !edge->closed) {
                        //weight
                        edge->g = cost;
                        //distance?
                        edge->h = Euclidean(goal, edge);
                        //combined
                        edge->f = edge->g + edge->h;
                        //it parent
                        edge->parent = node;
                        openset_.Push(edge);
                        edge->openset = true;
                    }
                }
            } 
            if (goal->parent) { path.push_back(Vector2f(start->point.x + 0.5f, start->point.y + 0.5f)); }

            // Construct path backwards from goal node
            std::vector<NodePoint> points;
            Node* current = goal;
            while (current != nullptr && current != start) {
                points.push_back(current->point);
                current = current->parent;
            }

            // Reverse and store as vector
            for (std::size_t i = 0; i < points.size(); ++i) {
                std::size_t index = points.size() - i - 1;
                Vector2f pos(points[index].x + 0.5f, points[index].y + 0.5f);

                path.push_back(pos);
            }

            return path;
        }

        std::vector<Vector2f> Pathfinder::SmoothPath(const std::vector<Vector2f>& path, const Map& map, float ship_radius) {
            std::vector<Vector2f> result;

            // How far away it should try to push the path from walls
            float push_distance = ship_radius * 1.5f;

            if (ship_radius < 1.0f) {
                push_distance = 2.0f;
            }
            //RenderText("Push: " + std::to_string(push_distance), GetWindowCenter() - Vector2f(0, 100), RGB(100, 100, 100), RenderText_Centered);
            result.resize(path.size());

            if (!path.empty()) {
                result[0] = path[0];
            }

            for (std::size_t i = 1; i < path.size(); ++i) {
                Vector2f current = path[i];
                Vector2f closest = ClosestWall(map, current, (int)std::ceil(push_distance + 1));
                Vector2f new_pos = current;

                if (closest != Vector2f(0, 0)) {
                    // Attempt to push the path outward from the wall
                    // TODO: iterative box penetration push

                    Vector2f center = closest + Vector2f(0.5, 0.5);
                    Vector2f direction = Normalize(center - current);
                    CastResult cast_result = RayCast(map, current, direction, push_distance);

                    if (cast_result.hit) {
                        Vector2f hit = cast_result.position;
                        float dist = hit.Distance(current);
                        float force = push_distance - dist;

                        new_pos = current + Normalize(current - hit) * force;
                    }
                }

                if (current != new_pos) {
                    // Make sure the new node is in line of sight
                    if (!IsPassablePath(map, current, new_pos, ship_radius)) {
                        new_pos = current;
                    }
                }

                result[i] = new_pos;
            }

#if 1 // Don't cull the path if this is enabled
            return result;
#endif

            if (result.size() <= 2) return result;

            std::vector<Vector2f> minimum;
            minimum.reserve(result.size());
            Vector2f prev = result[0];
            for (std::size_t i = 1; i < result.size(); ++i) {
                Vector2f curr = result[i];
                Vector2f direction = Normalize(curr - prev);
                Vector2f side = Perpendicular(direction) * ship_radius;
                float dist = prev.Distance(curr);
                CastResult cast_center = RayCast(map, prev, direction, dist);
                CastResult cast_side1 = RayCast(map, prev + side, direction, dist);
                CastResult cast_side2 = RayCast(map, prev - side, direction, dist);
                if (cast_center.hit || cast_side1.hit || cast_side2.hit) {
                    if (minimum.size() > result.size()) {
                        minimum = result;
                        break;
                    }
                    if (!minimum.empty() && result[i - 1] != minimum.back()) {
                        minimum.push_back(result[i - 1]);
                        prev = minimum.back();
                        i--;
                    }
                    else {
                        minimum.push_back(result[i]);
                        prev = minimum.back();
                    }
                }
            }
            minimum.push_back(result.back());
            result = minimum;
            return result;
        }
        
        float GetWallDistance(const Map& map, u16 x, u16 y, u16 radius) {
            float closest_sq = std::numeric_limits<float>::max();

            for (i16 offset_y = -radius; offset_y < radius; ++offset_y) {
                for (i16 offset_x = -radius; offset_x < radius; ++offset_x) {
                    u16 check_x = x + offset_x;
                    u16 check_y = y + offset_y;

                    if (map.IsSolid(check_x, check_y)) {
                        float dist_sq = (float)(offset_x * offset_x + offset_y * offset_y);

                        if (dist_sq < closest_sq) {
                            closest_sq = dist_sq;
                        }
                    }
                }
            }

            return std::sqrt(closest_sq);
        }



        std::vector<Vector2f> Pathfinder::CreatePath(std::vector<Vector2f> path, Vector2f from, Vector2f to, float radius) {
            bool build = true;

            std::vector<Vector2f> new_path = path;

            if (!new_path.empty()) {
                // Check if the current destination is the same as the requested one.
                if (new_path.back().DistanceSq(to) < 3 * 3) {
                    Vector2f pos = processor_->GetGame().GetPosition();
                    Vector2f next = new_path.front();
                    Vector2f direction = Normalize(next - pos);
                    Vector2f side = Perpendicular(direction);
                    float radius = processor_->GetGame().GetShipSettings().GetRadius();

                    float distance = next.Distance(pos);

                    // Rebuild the path if the bot isn't in line of sight of its next node.
                    CastResult center = RayCast(processor_->GetGame().GetMap(), pos, direction, distance);
                    CastResult side1 = RayCast(processor_->GetGame().GetMap(), pos + side * radius, direction, distance);
                    CastResult side2 = RayCast(processor_->GetGame().GetMap(), pos - side * radius, direction, distance);

                    if (!center.hit && !side1.hit && !side2.hit) {
                        build = false;
                    }
                }
            }

            if (build) {
                std::vector<Vector2f> mines;
                new_path.clear();
                //#if 0
                for (Weapon* weapon : processor_->GetGame().GetWeapons()) {
                    const Player* weapon_player = processor_->GetGame().GetPlayerById(weapon->GetPlayerId());
                    if (weapon_player == nullptr) continue;
                    if (weapon_player->frequency == processor_->GetGame().GetPlayer().frequency) continue;
                    if (weapon->GetType() & 0x8000) mines.push_back(weapon->GetPosition());
                }
                //#endif
                new_path = FindPath(processor_->GetGame().GetMap(), mines, from, to, radius);
                new_path = SmoothPath(new_path, processor_->GetGame().GetMap(), radius);
            }

            return new_path;
        }



        void Pathfinder::CreateMapWeights(const Map& map) {
            for (u16 y = 0; y < 1024; ++y) {
                for (u16 x = 0; x < 1024; ++x) {
                    if (map.IsSolid(x, y)) continue;

                    Node* node = this->processor_->GetNode(NodePoint(x, y));

                    u16 close_distance = 8;

                    float distance = GetWallDistance(map, x, y, close_distance);

                    if (distance == 0) distance = 1;

                    if (distance < close_distance) {
                        node->weight = 12.0f / distance;
                        node->previous_weight = 12.0f / distance;
                    }
                }
            }
        }

    }  // namespace path
}  // namespace marvin
