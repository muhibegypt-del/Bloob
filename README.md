# Bloob

An LG webOS TV playback prototype and a research notebook for an independent player runtime called Buna.

- [TV app source and build instructions](tv-app/README.md)
- [Native Buna preview source and TV install notes](native-app/README.md)
- [Complete project handoff for the next assistant](CLAUDE.md)
- [Observed Elements of A system map](research/Elements-of-A-system-map.md)
- [Buna player study](research/Buna-player-study.md)
- [Paid-episode runtime observation](research/live-episode-study.md)
- [Installable package (0.2.0)](tv-app/artifacts/com.muhib.tvprototype_0.2.0_all.ipk)
- [Installable native preview (0.2.2)](tv-app/artifacts/com.muhib.bloob.native_0.2.2_all.ipk)

The working app has a SolidJS interface, a local webOS service, source discovery through a hosted indexer with a recommendation-first chooser, AllDebrid PIN pairing with verified, multi-location storage, and playback through **Buna**, its own session runtime over the TV's video element. A separate native Qt Quick preview now renders its own home, details, and source screens and plays a test clip through Qt Multimedia on the TV. It currently uses the working app's local service; resolved title playback in the native path remains to be verified. The research notes distinguish code observations from live TV measurements and remaining unknowns.

Extracted Elements of A executables, assets, bundled JavaScript, decompiled implementation text, Ghidra databases, downloaded tools, and account data remain local and are excluded from this public repository. The bundled 30-second Big Buck Bunny test clip is attributed in `tv-app/service/www/media/CREDITS.txt`.
