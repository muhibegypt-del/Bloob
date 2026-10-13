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
constexpr float scroll_rate = 18.0f;    // ≈260 ms: shelves and lists
constexpr float page_rate = 14.0f;      // ≈320 ms: page-level moves
constexpr float fade_rate = 23.0f;      // ≈200 ms: overlays, HUD
constexpr float backdrop_rate = 13.0f;  // ≈350 ms crossfade once decoded
constexpr long long dwell_ms = 250;     // focus rest before the backdrop follows
constexpr long long spinner_delay_ms = 400;
constexpr long long hud_timeout_ms = 5000;

}  // namespace theme

// Type scale. Inter at fixed sizes; nothing below 19 px.
enum class Text { Display, Title1, Title2, Headline, Body, Callout, Caption, Footnote, Eyebrow, Button };
