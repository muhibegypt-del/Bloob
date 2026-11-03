# The interface on webOS: resolution, scaling and sharp text (3.1.4, 25 October 2026)

The owner reported serious aliasing and pixelated text on the TV.

## What the platform allows (read)

- **LG's "Supported App Resolution" page (webostv.developer.lge.com/develop/specifications/app-resolution):**
  - UHD models: graphics display resolution 1920×1080; video playback 3840×2160.
  - FHD models: 1280×720 graphics; 1920×1080 video.
  - The default graphic resolution is 1920×1080; `appinfo.json` `resolution` can lower it to 1280×720.
- **What follows:** no app can draw a 4K interface on the owner's G2. The TV upscales the 1080p graphics plane 2× to the panel, and LG's own menus go through the same path.
- **Video is separate (owner's reading of `videooutput/getStatus`, 25 October 2026):** 3840×2024 on the 3840×2160 panel, 23.976 fps, Dolby Vision.

## Causes found in Bloob's renderer (read, then fixed)

1. **Glyph placement:**
   - The atlas is 2× oversampled horizontally (stb_truetype) so glyphs can sit at sub-pixel positions.
   - `Font::draw` rounded each glyph's left and right edges to whole pixels separately. Each glyph was stretched or squeezed by up to a pixel, and the oversampling was resampled at the wrong ratio: uneven stems and a blotchy, pixelated look.
   - **Fix:** `SDL_RenderCopyF` at the exact float position and width, with the baseline on a whole pixel. The function is looked up at run time; the integer path is kept for an SDL older than 2.0.10.
2. **Rounded corners:**
   - Disc textures were 4×4 supersampled, giving only 17 coverage levels, visible as steps on large radii after the 2× upscale.
   - **Fix:** coverage from the pixel centre's distance to the edge, in 256 levels.
3. **Button focus growth:**
   - An odd number of pixels moved the button a pixel left on alternate frames.
   - **Fix:** growth is now always even.

## Not changed, and next if text is still soft

- **The TV's upscaler for the graphics plane** is outside Bloob's control.
- **No hinting:** stb_truetype doesn't hint glyphs. The NDK sysroot has FreeType (`libfreetype.so.6`, on the TV with webOS). Rendering with FreeType's light hinting (vertical only, keeping sub-pixel positions) is the standard way to get crisper stems at 23–30 px.
- **Desktop snapshots can't show the TV result:** they use SDL's software renderer, which scales textures without filtering.
