#pragma once

#include <fuse/Math.h>
#include <fuse/render/Color.h>

namespace fuse {
namespace render {

struct RenderableLine {
  Vector2f from;
  Vector2f to;
  Color color;
};

}  // namespace render
}  // namespace fuse
