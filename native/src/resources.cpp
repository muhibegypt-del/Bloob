#include "resources.h"

const char *gpu_class_name(GpuClass c) {
  switch (c) {
    case GpuClass::Surfaces: return "surfaces";
    case GpuClass::Images: return "images";
    case GpuClass::Shapes: return "shapes";
    case GpuClass::Glyphs: return "glyphs";
    case GpuClass::Icons: return "icons";
    case GpuClass::Layers: return "layers";
    case GpuClass::Count: break;
  }
  return "?";
}

GpuBudget &gpu_budget() {
  static GpuBudget budget;
  return budget;
}

static long long g_frame = 1;
long long current_frame() { return g_frame; }
void advance_frame() { g_frame++; }

CpuDomains &cpu_domains() {
  static CpuDomains domains;
  return domains;
}

const char *memory_policy_name(MemoryPolicy p) {
  switch (p) {
    case MemoryPolicy::Normal: return "normal";
    case MemoryPolicy::Playback: return "playback";
    case MemoryPolicy::Background: return "background";
  }
  return "?";
}

// Starting values, to tune from the inspector on the TV. The reference's own
// default tracked graphics budget is 50 MiB (study M3); Bloob's covers every
// texture it creates, including the two full-screen surfaces (about 16 MB).
size_t budget_for(MemoryPolicy p) {
  switch (p) {
    case MemoryPolicy::Normal: return 96u * 1024 * 1024;
    case MemoryPolicy::Playback: return 56u * 1024 * 1024;
    case MemoryPolicy::Background: return 24u * 1024 * 1024;
  }
  return 96u * 1024 * 1024;
}
