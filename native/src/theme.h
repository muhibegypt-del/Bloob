#pragma once
// Bloob visual system 1.1: the single source of design tokens. Values match
// native/DESIGN.md; change them there and here together.
#include "gfx.h"

constexpr int W = 1920;
constexpr int H = 1080;

namespace theme {

// ---- colour. Text alphas are tuned for a dark ground at viewing distance.
const Color ground = rgba(0x0b0c0e);
const Color raised = rgba(0x17181c);       // panels, keys, rows
const Color raised2 = rgba(0x202227);      // missing-art base, pressed areas
const Color primary = rgba(0xf5f5f7);      // text: titles, focused labels
const Color secondary = rgba(0xf5f5f7, 186);  // text: meta, descriptions
const Color tertiary = rgba(0xf5f5f7, 128);   // text: labels, hints; 19 px and up only
const Color hairline = rgba(0xffffff, 26);
const Color glass = rgba(0x1c1e23, 244);   // panels and sheets over blurred artwork
const Color glass_light = rgba(0xffffff, 38);  // unfocused buttons and pills
const Color focus = rgba(0xffffff);        // focused fills and frames
const Color ink = rgba(0x0b0c0e);          // text on focus
const Color ink2 = rgba(0x0b0c0e, 170);
const Color accent = rgba(0xf5b942);       // Bloob amber: progress, "best match"
const Color good = rgba(0x5cd58c);
const Color caution = rgba(0xf5b942);
const Color caution_ink = rgba(0x8a5a00);
const Color problem = rgba(0xff7a6b);

// ---- spacing (8 px grid) and layout
constexpr int s1 = 8, s2 = 16, s3 = 24, s4 = 32, s5 = 48, s6 = 64, s7 = 96;
constexpr int margin = 96;   // title-safe left/right edge (5%)
constexpr int top = 54;      // title-safe top

// ---- shape
constexpr int card_radius = 14;
constexpr int panel_radius = 28;
constexpr int button_h = 72;

// ---- cards
constexpr int card_w = 384, card_h = 216, card_gap = 32;   // 16:9 shelf card
constexpr float card_lift = 1.08f;                          // focused scale
constexpr int episode_w = 400, episode_h = 225;

// ---- motion: rates for eased values (settle ≈ 4.6 / rate seconds)
constexpr float focus_rate = 33.0f;     // ≈140 ms: focus answers the key at once
constexpr float focus_out_rate = 16.0f; // ≈290 ms: focus leaves more gently than it arrives
constexpr float scroll_rate = 18.0f;    // ≈260 ms: shelves and lists
constexpr float page_rate = 14.0f;      // ≈320 ms: page-level moves
constexpr float fade_rate = 23.0f;      // ≈200 ms: overlays, HUD
constexpr float backdrop_rate = 13.0f;  // ≈350 ms crossfade once decoded
constexpr long long dwell_ms = 250;     // focus rest before the backdrop follows
constexpr long long spinner_delay_ms = 400;
constexpr long long hud_timeout_ms = 5000;
constexpr long long hud_controls_timeout_ms = 8000;  // focus resting on the controls row
constexpr long long hud_reduced_timeout_ms = 2000;   // timeline-only, after the seek lands

// ---- Glass (2.0, native/GLASS-PLAN.md). Screens move to these phase by phase;
// the 1.x tokens above go in G8.
constexpr int safe_x = 80, safe_y = 60;            // tvOS safe area
const Color black = rgba(0x000000);                // ground behind full-bleed artwork
const Color plain = rgba(0x0b0b0d);                // ground of pages without artwork
const Color frost_hairline = rgba(0xffffff, 38);   // 15%: edge of a frosted panel
const Color smoke = rgba(0x000000, 115);           // 45%: glass over video (cannot be blurred)
const Color smoke_hairline = rgba(0xffffff, 31);   // 12%
const Color solid = rgba(0x2c2c2e, 240);           // 94%: Lite, Reduce transparency, frost still baking
const Color candidate = rgba(0xffffff, 51);        // 20%: chosen but not focused
constexpr float frost_tint = 0.14f;                // white mixed into the blurred art
constexpr float frost_light = 0.10f;               // extra white along the top edge
constexpr float frost_saturation = 1.3f;           // glass makes colour a little richer
constexpr int glass_radius = 32, badge_radius = 6;
constexpr float control_lift = 1.06f;              // focused buttons and pills
constexpr int g_card_w = 320, g_card_h = 180, g_card_gap = 40;  // tvOS 5-column grid
constexpr float focus_in_rate = 46.0f;             // ≈100 ms
constexpr float focus_leave_rate = 23.0f;          // ≈200 ms
constexpr float overlay_rate = 15.0f;              // ≈300 ms: player controls, sheets
constexpr long long hud_paused_timeout_ms = 8000;  // paused, or focus on a control
constexpr long long seek_commit_ms = 400;
// Scrim strength over artwork follows its brightness under the text (0..1 luma):
// light art gets the strong wash, dark art the light one.
inline float scrim_for(float luma) { return luma < 0 ? 0.75f : 0.45f + 0.55f * (luma < 0.15f ? 0.0f : luma > 0.6f ? 1.0f : (luma - 0.15f) / 0.45f); }

}  // namespace theme

// Type scale. Inter at fixed sizes; nothing below 19 px.
enum class Text { Display, Title1, Title2, Headline, Body, Callout, Caption, Footnote, Eyebrow, Button,
                  // Glass: Apple's public tvOS scale (size/leading), floor 23. Renamed in G8.
                  TVDisplay, TVTitle1, TVTitle2, TVTitle3, TVHeadline, TVCallout, TVBody, TVCaption1, TVCaption2, TVButton };

// Glass text colours by role; High contrast raises the lower ones.
enum class Label { Primary, Secondary, Tertiary, Disabled };
