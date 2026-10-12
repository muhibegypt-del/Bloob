# Bloob

An LG webOS TV playback prototype and a research notebook for an independent player runtime called Buna.

- [TV app source and build instructions](tv-app/README.md)
- [Complete project handoff for the next assistant](CLAUDE.md)
- [Observed Elements of A system map](research/Elements-of-A-system-map.md)
- [Buna player study](research/Buna-player-study.md)
- [Paid-episode runtime observation](research/live-episode-study.md)
- [Installable prototype package](tv-app/artifacts/com.muhib.tvprototype_0.1.0_all.ipk)

The prototype is a separate app with a SolidJS interface, a local webOS service, source discovery through a hosted indexer, AllDebrid PIN pairing and link resolution, and playback through the TV's video element. The research notes distinguish code observations from live TV measurements and remaining unknowns.

Extracted Elements of A executables, assets, bundled JavaScript, decompiled implementation text, Ghidra databases, downloaded tools, and account data remain local and are excluded from this public repository. The bundled 30-second Big Buck Bunny test clip is attributed in `tv-app/service/www/media/CREDITS.txt`.
