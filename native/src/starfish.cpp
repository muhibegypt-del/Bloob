// The TV's media pipeline for elementary streams (3.0 engine): LG's
// StarfishMediaAPIs in libplayerAPIs.so.1, the library Kodi's webOS build
// feeds. It is opened at run time rather than linked, so Bloob still starts
// (and plays through uMediaServer) on a TV without it.
//
// The library is C++. Bloob carries its own C++ runtime, so only these cross
// the boundary: a block of memory we allocate for the object, plain C strings
// in, and one std::string out (Feed). Both runtimes use the same std::string
// layout (the C++11 ABI, as the symbol says) and glibc's malloc underneath, so
// freeing that string here is safe. Member functions are called through
// member-function pointers so the compiler applies the ABI's calling rules.
#include "es_player.h"

#include "log.h"

#if defined(BLOOB_STARFISH)

#include <dlfcn.h>

#include <cstring>
#include <new>

#include <starfish-media-pipeline/StarfishMediaAPIs.h>

namespace {

using Callback = void (*)(int, long long, const char *, void *);

struct Api {
  void *lib = nullptr;
  void (*construct)(void *, const char *) = nullptr;
  void (*destruct)(void *) = nullptr;
  bool (StarfishMediaAPIs::*load)(const char *, Callback, void *) = nullptr;
  std::string (StarfishMediaAPIs::*feed)(const char *) = nullptr;
  bool (StarfishMediaAPIs::*play)() = nullptr;
  bool (StarfishMediaAPIs::*pause)() = nullptr;
  bool (StarfishMediaAPIs::*unload)() = nullptr;
  bool (StarfishMediaAPIs::*push_eos)() = nullptr;
  bool (StarfishMediaAPIs::*set_hdr)(const char *) = nullptr;
  bool (StarfishMediaAPIs::*foreground)() = nullptr;
  bool (StarfishMediaAPIs::*background)() = nullptr;
};

template <typename M>
bool member(void *lib, const char *symbol, M &out) {
  void *fn = dlsym(lib, symbol);
  if (!fn) {
    logf("engine", "libplayerAPIs has no %s", symbol);
    return false;
  }
  // A non-virtual member pointer is {function, this-adjustment 0} on ARM and elsewhere.
  struct {
    void *fn;
    long adj;
  } raw{fn, 0};
  static_assert(sizeof(M) == sizeof(raw), "member pointer layout");
  std::memcpy(&out, &raw, sizeof out);
  return true;
}

const Api *api() {
  static Api a;
  static bool tried = false;
  if (tried) return a.lib ? &a : nullptr;
  tried = true;
  void *lib = dlopen("libplayerAPIs.so.1", RTLD_NOW | RTLD_LOCAL);
  if (!lib) {
    logf("engine", "no libplayerAPIs on this TV (%s)", dlerror());
    return nullptr;
  }
  a.construct = reinterpret_cast<void (*)(void *, const char *)>(dlsym(lib, "_ZN17StarfishMediaAPIsC1EPKc"));
  a.destruct = reinterpret_cast<void (*)(void *)>(dlsym(lib, "_ZN17StarfishMediaAPIsD1Ev"));
  bool ok = a.construct && a.destruct &&
            member(lib, "_ZN17StarfishMediaAPIs4LoadEPKcPFvixS1_PvES2_", a.load) &&
            member(lib, "_ZN17StarfishMediaAPIs4FeedB5cxx11EPKc", a.feed) &&
            member(lib, "_ZN17StarfishMediaAPIs4PlayEv", a.play) &&
            member(lib, "_ZN17StarfishMediaAPIs5PauseEv", a.pause) &&
            member(lib, "_ZN17StarfishMediaAPIs6UnloadEv", a.unload) &&
            member(lib, "_ZN17StarfishMediaAPIs7pushEOSEv", a.push_eos) &&
            member(lib, "_ZN17StarfishMediaAPIs10setHdrInfoEPKc", a.set_hdr) &&
            member(lib, "_ZN17StarfishMediaAPIs16notifyForegroundEv", a.foreground) &&
            member(lib, "_ZN17StarfishMediaAPIs16notifyBackgroundEv", a.background);
  if (!ok) {
    dlclose(lib);
    return nullptr;
  }
  a.lib = lib;
  logf("engine", "TV media pipeline ready (libplayerAPIs)");
  return &a;
}

class StarfishSink : public EsSink {
 public:
  explicit StarfishSink(const Api *api) : api_(api) {}
  ~StarfishSink() override { unload(); }

  bool load(const std::string &payload, Event on_event) override {
    unload();
    on_event_ = std::move(on_event);
    // A fresh object per pipeline: Kodi keeps one per playback, but Bloob
    // reloads on every seek and a new object never carries old state.
    void *mem = ::operator new(sizeof(StarfishMediaAPIs), std::nothrow);
    if (!mem) return false;
    std::memset(mem, 0, sizeof(StarfishMediaAPIs));
    api_->construct(mem, nullptr);
    obj_ = static_cast<StarfishMediaAPIs *>(mem);
    (obj_->*api_->foreground)();
    if (!(obj_->*api_->load)(payload.c_str(), &StarfishSink::callback, this)) {
      logf("engine", "Load refused: %.600s", payload.c_str());
      destroy();
      return false;
    }
    return true;
  }
  void set_hdr(const std::string &payload) override {
    if (obj_ && !(obj_->*api_->set_hdr)(payload.c_str())) logf("engine", "setHdrInfo refused");
  }
  std::string feed(const uint8_t *data, size_t size, long long pts_ns, bool video) override {
    if (!obj_) return "Error: no pipeline";
    char json[160];
    std::snprintf(json, sizeof json, "{\"bufferAddr\":\"%#lx\",\"bufferSize\":%zu,\"pts\":%lld,\"esData\":%d}",
                  static_cast<unsigned long>(reinterpret_cast<uintptr_t>(data)), size, pts_ns, video ? 1 : 2);
    return (obj_->*api_->feed)(json);
  }
  bool play() override { return obj_ && (obj_->*api_->play)(); }
  bool pause() override { return obj_ && (obj_->*api_->pause)(); }
  bool push_eos() override { return obj_ && (obj_->*api_->push_eos)(); }
  void unload() override {
    if (!obj_) return;
    (obj_->*api_->unload)();
    destroy();
  }
  void notify_foreground() override {
    if (obj_) (obj_->*api_->foreground)();
  }
  void notify_background() override {
    if (obj_) (obj_->*api_->background)();
  }
  const char *name() const override { return "starfish"; }

 private:
  static void callback(int type, long long num, const char *str, void *data) {
    auto *self = static_cast<StarfishSink *>(data);
    if (self->on_event_) self->on_event_(type, num, str ? str : "");
  }
  void destroy() {
    if (!obj_) return;
    api_->destruct(obj_);
    ::operator delete(static_cast<void *>(obj_));
    obj_ = nullptr;
  }

  const Api *api_;
  StarfishMediaAPIs *obj_ = nullptr;
  Event on_event_;
};

}  // namespace

std::unique_ptr<EsSink> make_starfish_sink() {
  const Api *a = api();
  return a ? std::unique_ptr<EsSink>(new StarfishSink(a)) : nullptr;
}

#else

std::unique_ptr<EsSink> make_starfish_sink() { return nullptr; }

#endif
