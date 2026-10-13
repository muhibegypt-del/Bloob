# Bloob

An LG webOS TV playback prototype and a research notebook for an independent player runtime called Buna.

- [TV app source and build instructions](tv-app/README.md)
- [Native runtime reconstruction target](docs/Buna-native-runtime-target.md)
- [Verified 32-bit ARM build and load probe](native-host/README.md)
- [QML experiment and its failed TV acceptance](native-app/README.md)
- [Complete project handoff for the next assistant](CLAUDE.md)
- [Observed Elements of A system map](research/Elements-of-A-system-map.md)
- [Buna player study](research/Buna-player-study.md)
- [Paid-episode runtime observation](research/live-episode-study.md)
- [Installable package (0.2.0)](tv-app/artifacts/com.muhib.tvprototype_0.2.0_all.ipk)

The working app has a SolidJS interface, a local webOS service, source discovery through a hosted indexer with a recommendation-first chooser, AllDebrid PIN pairing with verified, multi-location storage, and playback through **Buna**, its own session runtime over the TV's video element. A separate QML experiment was installed for research, but the owner found its UI and video unusable in normal use. It is not the native architecture target. The research notes distinguish code observations from live TV measurements and remaining unknowns.

Extracted Elements of A executables, assets, bundled JavaScript, decompiled implementation text, Ghidra databases, downloaded tools, and account data remain local and are excluded from this public repository. The bundled 30-second Big Buck Bunny test clip is attributed in `tv-app/service/www/media/CREDITS.txt`.
