#pragma once

#include <fuse/render/Color.h>
#include <fuse/render/Primitive.h>
#include <fuse/render/Text.h>

#include <vector>

namespace fuse {
namespace render {

struct Renderer {
  virtual void OnNewFrame() = 0;
  virtual void Render() = 0;

  virtual void PushText(std::string_view text, const Vector2f& position, TextColor color,
                        RenderTextFlags flags = 0) = 0;
  virtual void PushWorldLine(const Vector2f& world_from, const Vector2f& world_to, Color color) = 0;
  virtual void PushScreenLine(const Vector2f& screen_from, const Vector2f& screen_to, Color color) = 0;

  virtual Vector2f GetSurfaceSize() const = 0;
};

}  // namespace render
}  // namespace fuse
