#pragma once


#if 0
struct Radar {
    Vector2f world_min;
    Vector2f world_max;
 
    inline Vector2f GetCenter() const {
      return (world_min + world_max) * 0.5f;
    }
 
    void Calculate(const Player& player) {
      Vector2f surface_size = Fuse::Get().GetRenderer().GetSurfaceSize();
 
      s16 dim = ((((u16)surface_size.x / 6) / 4) * 8) / 2;
      u32 full_dim = ((u32)surface_size.x * 8) / Fuse::Get().GetSettings().MapZoomFactor;
 
      u32 ivar8 = ((u32)surface_size.x / 6) + ((u32)surface_size.x >> 0x1F);
      s32 ivar5 = full_dim;
      u32 ivar6 = (u32)(player.position.y * 16) * ivar5;
      u32 ivar4 = ((ivar8 >> 2) - (ivar8 >> 0x1F)) * 8 * 4;
 
      ivar8 = (ivar4 + (ivar4 >> 0x1F & 7U)) >> 3;
      ivar4 = (u32)(player.position.x * 16) * ivar5;
 
      s32 texture_min_x = ((s32)(ivar4 + (ivar4 >> 0x1F & 0x3FFFU)) >> 0xE) - ivar8 / 2;
      s32 texture_min_y = ((s32)(ivar6 + (ivar6 >> 0x1F & 0x3FFFU)) >> 0xE) - ivar8 / 2;
 
      ivar5 = ivar5 - ivar8;
 
      if (texture_min_x < 0) {
        texture_min_x = 0;
      } else if (ivar5 < texture_min_x) {
        texture_min_x = ivar5;
      }
 
      if (texture_min_y < 0) {
        texture_min_y = 0;
      } else if (ivar5 < texture_min_y) {
        texture_min_y = ivar5;
      }
 
      s32 texture_max_x = texture_min_x + ivar8;
      s32 texture_max_y = texture_min_y + ivar8;
 
      Vector2f min_uv = Vector2f(texture_min_x / (float)full_dim, texture_min_y / (float)full_dim);
      Vector2f max_uv = Vector2f(texture_max_x / (float)full_dim, texture_max_y / (float)full_dim);
 
      world_min = Vector2f(min_uv.x * 1024.0f, min_uv.y * 1024.0f);
      world_max = Vector2f(max_uv.x * 1024.0f, max_uv.y * 1024.0f);
    }
  };
#endif