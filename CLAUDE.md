# Bloob: instructions for assistants

Bloob is a private film and series app for one owner's rooted LG OLED55G26LA
(webOS 9.2.2). **Read [`HANDOVER.md`](HANDOVER.md) before changing anything**:
it has the layout of the code, how to build and ship, the design and the open
work. The long version history that used to be here is in Git
(`git log`, and this file before 13 November 2026).

## Rules (the owner's; all in force)

1. **Never delete anything on the TV.** No files, packages, accounts or caches,
   not even to tidy up. Installing or updating Bloob is fine. Root commands
   must never risk bricking the TV.
2. **No secrets in Git, logs, screenshots or the package:** the AllDebrid key,
   the TMDB and MDBList keys (`keys.json` on the TV), the root password, and
   resolved stream URLs.
3. **Elements of A** is the name for a studied reference app. Never copy its
   code, artwork, icons, fonts, text or branding, and never name it otherwise.
4. **No ranking penalties the owner didn't ask for** (bitrate, packs, size).
5. **Don't bring back what the owner removed:** calibration (4.1), the Swiss
   design, long divider lines, folded "+N more" rows and edition tabs (4.2).
6. Say honestly what was tested where: on the desktop, on the TV, or deduced.
7. The service runs on the TV's old Node: no `?.` or `??` in `tv-app/service`.

## The owner's taste, learned the hard way

- **Glass, minimal, full-bleed artwork, quick menus.** The owner tried a
  Swiss/Helvetica redesign in 4.1 and rejected it in 4.2.
- Show a design as real screenshots before building a lot of it, and rebuild
  layouts rather than restyle them.
- Keep things simple: when in doubt, fewer rows, fewer labels, fewer options.
- The versions list must be easy to scan: one flat list per tab, best first,
  every release visible, with chips saying what each one is (IMAX, Extended,
  Remux, Season pack).

## Working rules

- Develop on the branch you are given; commit with clear messages; never
  commit extracted reference-app files, tooling downloads or account data.
- Raise the version in both `native/CMakeLists.txt` and
  `native/package/appinfo.json`; build with `native/scripts/build.sh`.
