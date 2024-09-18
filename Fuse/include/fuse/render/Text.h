#pragma once

#include <fuse/Math.h>
#include <fuse/Types.h>

#include <string>

namespace fuse {
namespace render {

enum class TextColor : u16 { White, Green, Blue, Red, Yellow, Fuchsia, DarkRed, Pink };

enum RenderTextFlag {
  RenderText_Centered = (1 << 0),
};

using RenderTextFlags = u16;

struct RenderableText {
  std::string text;
  Vector2f position;

  TextColor color;
  RenderTextFlags flags;
};

}  // namespace render
}  // namespace fuse
