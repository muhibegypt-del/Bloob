#pragma once
// Headphones mode (3.2): Bloob's own binaural room responses. Every virtual
// speaker gets a left-ear and a right-ear impulse response, computed from
// published acoustic models rather than measured files (whose licences vary,
// and the repository is public):
//   - the head: Brown and Duda's spherical-head shadow filter and interaural
//     time difference ("A structural model for binaural sound synthesis",
//     IEEE Trans. Speech and Audio Processing, 1998), with their pinna echoes
//     (front/back cues) and a shoulder reflection;
//   - the room: image-source reflections up to third order, each arriving
//     through the head model from its own direction and darkened by the walls,
//     then a decaying diffuse tail, different at each ear. The room is what
//     puts the sound outside the head.
// The owner can also put a measured IR in the data folder (see headphones.h);
// that file never enters the repository.
#include <string>
#include <vector>

namespace binaural {

struct Room {
  const char *id;    // saved in settings
  const char *name;  // shown in Settings
  float width, depth, height;  // metres
  float distance;              // listener to speakers, metres
  float reflection;            // wall reflection coefficient, 0..1
  float tail_db;               // diffuse tail level against the direct sound
  float rt60;                  // seconds
};

// The styles offered in Settings, the default first.
const std::vector<Room> &rooms();
const Room &room_named(const std::string &id);

// A virtual speaker: FFmpeg's channel name ("FL", "LFE"…), its direction
// (degrees; azimuth positive to the left, 0 straight ahead) and a level in dB.
// LFE has no direction: its pair stays silent (the headphone filter mixes it itself).
struct Speaker {
  std::string name;
  float azimuth = 0, elevation = 0;
  float gain_db = 0;
};
// The speakers of "7.1", "5.1(side)" or "stereo", in that layout's channel
// order (the order the headphone filter's map lists them).
std::vector<Speaker> layout_speakers(const std::string &layout);

// 2N channels for N speakers (speaker k: left ear 2k, right ear 2k+1), each
// `length` samples. Scaled so a front speaker's direct sound peaks near 1.
struct Responses {
  int rate = 48000;
  int length = 0;
  std::vector<std::vector<float>> channels;
};
Responses compute(const Room &room, const std::vector<Speaker> &speakers, int rate = 48000);

}  // namespace binaural
