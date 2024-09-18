#pragma once

#include <fuse/Platform.h>
#include <fuse/Types.h>

namespace fuse {
namespace render {

struct FUSE_EXPORT Color {
  u32 value = 0;

  static inline Color FromRGB(u8 r, u8 g, u8 b) {
    Color color;

    color.value = RGB(r, g, b);

    return color;
  }

  static inline Color FromHSV(float h, float s, float v) {
    if (s <= 0.0) {
      return FromRGB((u8)(v * 255), (u8)(v * 255), (u8)(v * 255));
    }

    float hh = h;

    if (hh >= 360.0) hh = 0.0f;
    hh /= 60.0f;

    long i = (long)hh;
    float ff = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * ff));
    float t = v * (1.0f - (s * (1.0f - ff)));

    float r = 0;
    float g = 0;
    float b = 0;

    switch (i) {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;

      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      case 5:
      default:
        r = v;
        g = p;
        b = q;
        break;
    }

    return FromRGB((u8)(r * 255), (u8)(g * 255), (u8)(b * 255));
  }
};

}  // namespace render
}  // namespace fuse
