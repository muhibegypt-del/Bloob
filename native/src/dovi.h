#pragma once
// Bloob's Dolby Vision helper (dovi/, Rust around dovi_tool's dolby_vision
// crate, MIT; built by scripts/build-dovi.sh). Linked when build-dovi/ has it
// (BLOOB_HAVE_DOVI); demux.cpp is the only caller.
#include <cstddef>
#include <cstdint>
extern "C" {
// One RPU NAL unit (no start code, 0x7C 0x01 header included) rewritten as
// profile 8.1. Returns the new unit's length and sets *out (free it with
// bloob_dovi_free), or 0 when the unit is not a usable RPU.
size_t bloob_dovi_to81(const uint8_t *nal, size_t len, uint8_t **out);
int bloob_dovi_profile(const uint8_t *nal, size_t len);
void bloob_dovi_free(uint8_t *p, size_t len);
}
