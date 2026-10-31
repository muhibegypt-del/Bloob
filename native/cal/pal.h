#pragma once
// Pal's SDR grey flow (research/Pal-in-Bloob.md §9–11, native/PAL-PLAYBOOK.md): a grey
// report at 0–100% in 5% steps, Pal's adjustments to the 1D table in the TV, a safety
// gate before anything is written, and a reference correction Pal may start from.
// Pure functions: bloob-cal does the measuring and the writing.
#include <array>
#include <string>
#include <vector>

#include "colour.h"
#include "lut.h"
#include "procedure.h"

namespace cal {

// The report's levels, in percent of the signal: 0, 5, … 100.
std::vector<int> pal_levels();
inline int pal_code(int pct) { return code_limited(pct / 100.0); }

// Which entry of LG's 1024-entry 1D table a video level lands on. Not documented: Bloob's
// procedure has assumed the normalised video signal (64→0, 940→1023); the 10-bit code itself
// (64→64, 940→940) is the other reading. pal-start measures which one this TV uses.
enum class TableIndex { Signal, Code };
double pal_table_x(int pct, TableIndex m);

// The goal Pal works to (§9): ΔE2000 relative to the target white.
struct PalGoal {
  double avg = 0.5, max = 1.0;  // over the scored levels
  double white_pct = 2.0;       // white luminance within this of the target
  int first_scored = 10;        // 0% is OLED black; 5% is below what the Spyder5 reads reliably
};

struct GreyLevel {
  int pct = 0, code = 64;
  bool measured = false;
  Vec3 XYZ, target;
  double de = 0;
  bool scored = false;
  int reads = 0;
  bool suspect = false;  // readings disagreed even after a third
  std::string note;
};

struct GreySummary {
  double avg = 0, max = 0;
  int max_at = -1;
  double white_Y = 0, white_err_pct = 0;
  bool goal_met = false;
};

// Absolute XYZ the target asks for at a grey level.
Vec3 pal_target(const SdrTarget &t, int pct);
// Fills each level's target, ΔE and scored flag, and returns the summary.
GreySummary pal_summarise(std::vector<GreyLevel> &levels, const SdrTarget &t, const PalGoal &goal);

// Is this grey reading plausible? Near neutral once it is bright enough to tell, and not
// darker than the level below (allowing for the meter's wobble).
bool pal_grey_sane(const Vec3 &XYZ, const Vec3 *below, double wobble_pct, std::string *why);

// Pal's answer: for each channel, a multiplier at each report level (1 = leave the table as it
// is there). Between levels the multiplier is interpolated smoothly (PCHIP), so the table's own
// fine shape is kept: new(v) = table(v) × m(v).
struct PalAdjust {
  std::vector<int> pct;                     // ascending, from pal_levels()
  std::array<std::vector<double>, 3> mult;  // R, G, B
  std::string note;
};
// Checks the shape and the range of an adjustment (multipliers within [0.5, 1.5]).
bool pal_adjust_valid(const PalAdjust &a, std::string *why);
Lut1D pal_apply(const Lut1D &current, const PalAdjust &a, TableIndex m = TableIndex::Signal);
// The table's value at each report level, per channel.
std::array<std::vector<double>, 3> pal_table_points(const Lut1D &t, TableIndex m = TableIndex::Signal);

// The safety gate (§11, rubric 2): finite values in [0, 1], every channel rising, no entry
// moved more than `max_step` from the table in the TV, and no spike (second difference).
struct GateResult {
  bool ok = false;
  std::string why;
  double max_step = 0, max_bend = 0;
};
GateResult pal_gate(const Lut1D &next, const Lut1D &current, double max_step = 0.15, bool check_bend = true);

// The reference correction: each grey's measured colour is split into R, G and B drive with
// the measured primaries (scaled so the measured white is 1, 1, 1); the drive each channel
// should have for the target is found on that channel's measured curve (table value → drive)
// and becomes a multiplier. `damp` < 1 takes part of the step (later passes). Levels too dark
// to read take a softened copy of the nearest readable level's multiplier.
PalAdjust pal_propose(const std::vector<GreyLevel> &levels, const Lut1D &current, const Vec3 primaries[3], const SdrTarget &t,
                      double damp, TableIndex m, std::vector<std::string> *notes);

// How much light the corrected white needs: the target white in drive terms (the measured
// primaries scaled so the measured white is 1, 1, 1); its largest channel is the factor the
// light must rise by, since no channel can go above full. pal-start sets light with it before
// any correction ("white first"), so a correction never has to be followed by more light.
double pal_white_need(const Vec3 &white, const Vec3 prim[3], const SdrTarget &t);

// A table equal to `current` except that entries from x0 to x1 hold the value at x0: a flat
// step, so a patch landing there reads darker (pal-start's index check). Still rising.
Lut1D pal_plateau(const Lut1D &current, double x0, double x1);

}  // namespace cal
