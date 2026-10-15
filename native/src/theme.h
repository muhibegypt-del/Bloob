#pragma once
// Bloob visual system 2.2 (Glass): the single source of design tokens. Values
// match native/DESIGN.md; change them there and here together. Values marked
// [EoA Xn] are copied from the Elements of A studies; TOKENS.md row Xn gives the
// source. Unmarked values have no recorded reference value.
#include "gfx.h"

constexpr int W = 1920;
constexpr int H = 1080;

// Motion (ui.h Anim) [EoA M1-M11]: timed curves and springs.
enum class Ease { Linear, In, Out, InOut };
struct Motion {
  float ms = 300;                  // timed: duration
  Ease ease = Ease::Out;
  bool spring = false;             // spring: damping, stiffness, mass
  float damping = 30, stiffness = 150, mass = 1;
};
constexpr Motion timed(float ms, Ease e) { return Motion{ms, e, false, 30, 150, 1}; }
constexpr Motion springy(float damping, float stiffness) { return Motion{0, Ease::Out, true, damping, stiffness, 1}; }

namespace theme {

// ---- ground and status colours. Text colours come from label() by role (ui.h).
const Color black = rgba(0x000000);                // ground behind full-bleed artwork and video
const Color plain = rgba(0x2c2c2e);                // ground of pages without artwork [EoA C5]
const Color candidate = rgba(0xffffff, 26);        // 10%: chosen but not focused [EoA P5]
const Color hairline = rgba(0xffffff, 20);         // 8% [EoA C3]
const Color hairline_high = rgba(0xffffff, 82);    // 32%, Increase contrast [EoA C4]
const Color menu_surface = rgba(0xffffff, 26);     // 10% [EoA F5]
const Color menu_gradient = rgba(0x434343);        // [EoA F6]
const Color settings_platter = rgba(0xffffff, 26); // 10%: settings rows at rest [EoA C7]
const Color good = rgba(0x32d74b);                 // SystemGreen [EoA C10]
const Color caution = rgba(0xffa500);              // Orange, the only orange recorded [EoA C12]
const Color caution_ink = rgba(0x8a5a00);          // caution text on a white (focused) fill
const Color problem = rgba(0xff453a);              // SystemRed [EoA C11]

// ---- layout: the tvOS safe area and grid
constexpr int safe_x = 80, safe_y = 60;            // [EoA S1, S2]
constexpr int card_w = 320, card_h = 180, card_gap = 40;  // 16:9 cards, tvOS 5-column grid
constexpr int card_radius = 20;                    // grid preset D, radius M [EoA K1]
constexpr int card_grow = 50;                      // focused cards grow 50 wider, height in proportion [EoA K2]
constexpr int episode_w = 410, episode_h = 231;    // 4-column grid item (inferred episode card) [EoA K3]
constexpr int glass_radius = 32, badge_radius = 6;
constexpr int button_grow = 8;                     // focused buttons grow 8 wider; no scale [EoA B4]

// ---- buttons [EoA B1-B9]: Regular 66 high, padding 30, icon gap 12
constexpr int button_h = 66, button_small_h = 50, button_large_h = 84;
constexpr int button_pad = 30, button_icon_gap = 12;
constexpr int title_button_gap = 24;               // between hero and title-page buttons [EoA B7]
constexpr int stack_gap = 14;                      // settings and modal button stacks [EoA B8]
constexpr int title_button_min_w = 220, title_button_max_w = 420;  // [EoA B9]
const Color button_focus = rgba(0xffffff, 230);    // White90 [EoA B5]
const Color button_dark = rgba(0x000000, 128);     // Dark variant at rest: Black50 + 16% outline [EoA B10]
const Color button_dark_candidate = rgba(0xffffff, 51);  // White20
const Color button_default = rgba(0xffffff, 26);   // Default variant at rest: White10, candidate White60 [EoA B11]
const Color button_default_candidate = rgba(0xffffff, 153);
const Color button_player = rgba(0x1e1e1e, 230);   // DarkPlayer at rest: RGB(30,30,30) 90%, candidate Gray49 80% [EoA B12]
const Color button_player_candidate = rgba(0x7c7c7c, 204);
// Shared rounded shadow at elevation 1: opacity 0.4, blur 25, offset y 20 [EoA E1]
constexpr int shadow_blur = 25, shadow_dy = 20;
constexpr Uint8 shadow_alpha = 102;
// Cards [EoA K4]: idle dy 4, blur 12, Black18; focused dy 40, blur 45, Black30
constexpr int card_shadow_blur = 12, card_shadow_dy = 4, card_focus_shadow_blur = 45, card_focus_shadow_dy = 40;
constexpr Uint8 card_shadow_alpha = 46, card_focus_shadow_alpha = 77;
constexpr int card_text_pad = 14;                  // lockup padding, grid D [EoA K5]
constexpr int progress_pad_x = 18, progress_pad_y = 12;  // [EoA K6]
const Color progress_back = rgba(0xffffff, 77);    // LabelTertiary under LabelPrimary [EoA K7]

// ---- season pills [EoA P1-P6]
constexpr int pill_h = 66, pill_radius = 33, pill_pad = 30;
constexpr int pill_focus_grow = 8;                 // horizontal expansion when focused

// ---- spacing [EoA F1-F4]
constexpr int bar_pad_top = 30, bar_pad_bottom = 26;  // filter and tab bars
constexpr int control_gap = 16;                    // between buttons in a row
constexpr int menu_gap = 24;                       // between menu entries

// ---- title-on-art cards [EoA C6]: light artwork gets the stronger blend
constexpr float card_scrim_light = 0.55f, card_scrim_dark = 0.20f;
constexpr double card_scrim_light_luma = 0.5;      // where "light" starts (no reference value)

// ---- glass materials (GLASS-PLAN.md). Frost is baked from the blurred backdrop;
// over video the glass is smoked because the video plane cannot be read.
const Color frost_hairline = rgba(0xffffff, 41);   // 16%: outline on art, logos and buttons [EoA C13]
const Color smoke = rgba(0x000000, 115);           // 45%: glass over video
const Color smoke_hairline = rgba(0xffffff, 31);   // 12%
const Color solid = rgba(0x2c2c2e, 240);           // 94%: Lite, Reduce transparency, frost still baking
constexpr float frost_tint = 0.14f;                // white mixed into the blurred art
constexpr float frost_light = 0.10f;               // extra white along the top edge
constexpr float frost_saturation = 1.3f;           // glass makes colour a little richer
// Scrim strength over artwork follows its brightness under the text (0..1 luma):
// light art gets the strong wash, dark art the light one.
inline float scrim_for(float luma) { return luma < 0 ? 0.75f : 0.45f + 0.55f * (luma < 0.15f ? 0.0f : luma > 0.6f ? 1.0f : (luma - 0.15f) / 0.45f); }

// ---- motion [EoA M1-M9]: timed curves and springs (ui.h Anim)

constexpr Motion focus_in_rate = timed(85, Ease::In);          // shared focus arrival [EoA M1]
constexpr Motion focus_leave_rate = timed(230, Ease::Out);     // shared focus departure [EoA M2]
constexpr Motion button_in_rate = timed(75, Ease::In);         // buttons [EoA B6]
constexpr Motion button_leave_rate = timed(150, Ease::Out);
constexpr Motion pill_in_rate = timed(100, Ease::In);          // pills, list and menu rows [EoA P6]
constexpr Motion pill_out_rate = timed(200, Ease::Out);
constexpr Motion press_rate = timed(80, Ease::Out);            // press and release [EoA M3]
constexpr Motion fade_rate = timed(300, Ease::Linear);         // page crossfade [EoA M10]
constexpr Motion overlay_rate = timed(300, Ease::Linear);      // player controls show / hide [EoA V5]
constexpr Motion menu_in_rate = timed(380, Ease::Out);         // floating menu show [EoA V12]
constexpr Motion menu_out_rate = timed(210, Ease::Out);        // and hide
constexpr Motion scroll_rate = springy(30, 150);               // shelf and list scrolling [EoA M9]
constexpr Motion page_rate = springy(30, 150);                 // product canvas (title page fold) [EoA M9]
constexpr Motion backdrop_rate = timed(300, Ease::Out);        // hero artwork crossfade [EoA H10]
constexpr Motion image_fade_rate = timed(240, Ease::Out);      // artwork fade-in [EoA M11]
constexpr long long dwell_ms = 300;                // focus rest before related content loads [EoA M5]
constexpr long long spinner_delay_ms = 400;

// ---- player timers
constexpr long long hud_timeout_ms = 5000;           // [EoA V1]
constexpr long long hud_paused_timeout_ms = 8000;  // paused, or focus on a control [EoA V2]
constexpr long long hud_reduced_timeout_ms = 2000; // timeline-only, after the seek lands
constexpr long long seek_commit_ms = 400;          // Session::kSeekDebounceMs [EoA V7]
constexpr int hud_reader_factor = 2;               // screen reader doubles every delay [EoA V4]
constexpr long long product_art_timeout_ms = 7000; // artwork loading fallback [EoA L3]

}  // namespace theme

// Type scale: Inter at the Elements of A sizes and leading [EoA T1-T9], which match
// Apple's public tvOS scale; Display (hero titles) has no reference value.
enum class Text { Display, Title1, Title2, Title3, Title3Std, Headline, Callout, CalloutBold, Body, BodyBold, BodySemi, Caption1, Caption1Semi, Caption2, Caption2Bold, Caption3, Button, Pill };

// Glass text colours by role [EoA C1, C2]; Increase contrast raises the lower ones.
enum class Label { Primary, Secondary, Tertiary, Disabled };
