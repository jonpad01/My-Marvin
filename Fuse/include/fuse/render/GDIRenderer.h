#pragma once

#include <fuse/render/Renderer.h>

namespace fuse {
namespace render {

struct GDIRenderer : public Renderer {
  FUSE_EXPORT void OnNewFrame() override;
  FUSE_EXPORT void Render() override;

  FUSE_EXPORT void PushText(std::string_view text, const Vector2f& position, TextColor color,
                            RenderTextFlags flags = 0) override;
  FUSE_EXPORT void PushWorldLine(const Vector2f& world_from, const Vector2f& world_to, Color color) override;
  FUSE_EXPORT void PushScreenLine(const Vector2f& screen_from, const Vector2f& screen_to, Color color) override;

  FUSE_EXPORT Vector2f GetSurfaceSize() const override;

  FUSE_EXPORT void Inject();
  FUSE_EXPORT bool IsInjected() const { return injected; }

 private:
  std::vector<RenderableText> renderable_texts;
  std::vector<RenderableLine> renderable_lines;
  bool injected = false;
};

}  // namespace render
}  // namespace fuse
