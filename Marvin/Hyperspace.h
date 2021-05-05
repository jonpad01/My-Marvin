#pragma once

#include <fstream>
#include <memory>

//#include "KeyController.h"
//#include "Common.h"
//#include "Steering.h"
#include "behavior/BehaviorEngine.h"
//#include "path/Pathfinder.h"
//#include "platform/ContinuumGameProxy.h"
//#include "Time.h"

namespace marvin {
    namespace hs {



        class HSFlaggerSort : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSSetRegionNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSDefensePositionNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSFreqMan : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSShipMan : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSWarpToCenter : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSAttachNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSToggleNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class HSPatrolBaseNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


    } // namesspace hs
}  // namespace marvin










