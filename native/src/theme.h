#pragma once
// Bloob visual system 2.0 (Glass): the single source of design tokens. Values
// match native/DESIGN.md; change them there and here together.
#include "gfx.h"

constexpr int W = 1920;
constexpr int H = 1080;

namespace theme {

// ---- ground and status colours. Text colours come from label() by role (ui.h).
const Color black = rgba(0x000000);                // ground behind full-bleed artwork and video
const Color plain = rgba(0x0b0b0d);                // ground of pages without artwork
const Color candidate = rgba(0xffffff, 51);        // 20%: chosen but not focused
const Color good = rgba(0x5cd58c);
const Color caution = rgba(0xf5b942);
const Color caution_ink = rgba(0x8a5a00);          // caution text on a white (focused) fill
const Color problem = rgba(0xff7a6b);

// ---- layout: the tvOS safe area and grid
constexpr int safe_x = 80, safe_y = 60;
constexpr int card_w = 320, card_h = 180, card_gap = 40;  // 16:9 cards, tvOS 5-column grid
constexpr int card_radius = 14;
constexpr float card_lift = 1.08f;                 // focused card scale
constexpr int episode_w = 400, episode_h = 225;
constexpr int glass_radius = 32, badge_radius = 6;
constexpr float control_lift = 1.06f;              // focused buttons and pills

// ---- glass materials (GLASS-PLAN.md). Frost is baked from the blurred backdrop;
// over video the glass is smoked because the video plane cannot be read.
const Color frost_hairline = rgba(0xffffff, 38);   // 15%: edge of a frosted panel
const Color smoke = rgba(0x000000, 115);           // 45%: glass over video
const Color smoke_hairline = rgba(0xffffff, 31);   // 12%
const Color solid = rgba(0x2c2c2e, 240);           // 94%: Lite, Reduce transparency, frost still baking
constexpr float frost_tint = 0.14f;                // white mixed into the blurred art
constexpr float frost_light = 0.10f;               // extra white along the top edge
constexpr float frost_saturation = 1.3f;           // glass makes colour a little richer
// Scrim strength over artwork follows its brightness under the text (0..1 luma):
// light art gets the strong wash, dark art the light one.
inline float scrim_for(float luma) { return luma < 0 ? 0.75f : 0.45f + 0.55f * (luma < 0.15f ? 0.0f : luma > 0.6f ? 1.0f : (luma - 0.15f) / 0.45f); }

// ---- motion: rates for eased values (settle ≈ 4.6 / rate seconds)
constexpr float focus_in_rate = 46.0f;             // ≈100 ms: focus answers the key at once
constexpr float focus_leave_rate = 23.0f;          // ≈200 ms: and leaves more gently
constexpr float fade_rate = 23.0f;                 // ≈200 ms: page transitions
constexpr float overlay_rate = 15.0f;              // ≈300 ms: player controls, sheets
constexpr float scroll_rate = 18.0f;               // ≈260 ms: shelves and lists
constexpr float page_rate = 14.0f;                 // ≈320 ms: page-level moves
constexpr float backdrop_rate = 13.0f;             // ≈350 ms crossfade once decoded
constexpr long long dwell_ms = 250;                // focus rest before the backdrop follows
constexpr long long spinner_delay_ms = 400;

// ---- player timers
constexpr long long hud_timeout_ms = 5000;
constexpr long long hud_paused_timeout_ms = 8000;  // paused, or focus on a control
constexpr long long hud_reduced_timeout_ms = 2000; // timeline-only, after the seek lands
constexpr long long seek_commit_ms = 400;          // Session::kSeekDebounceMs

}  // namespace theme

// Type scale: Inter at Apple's public tvOS sizes (size/leading); Display is for hero titles only. Floor 23.
enum class Text { Display, Title1, Title2, Title3, Headline, Callout, Body, Caption1, Caption2, Button };

// Glass text colours by role; High contrast raises the lower ones.
enum class Label { Primary, Secondary, Tertiary, Disabled };
