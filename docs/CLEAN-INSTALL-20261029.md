# F-Zero first-install qualification, October 29

The owner added clean-install tests to the overnight projection work: Art, Woden, DRIVE and iRacing first, then
F-Zero's recomp installation/configuration experience. Lulu owns this initial F-Zero preparation in isolated branch
`lulu/fzero-clean-install-20261029`, based on a03c722. The canonical checkout has unrelated dirty work; it is preserved.

The smallest public-facing candidate is the existing stock-only, ROM-free package path (`stage_unified.py` then
`package_unified.py`), with Setup.cmd and the launcher. Do not substitute the private embedded-Deluxe integration
or its provisional Shared DLL. No ROM, Deluxe data, owner profile or game executable was copied or published here.
Existing STD-013/032/035 adoption gaps remain open; no new standards or force changes.

Initial check found `python tests/test_unified_package.py` failed before running any tests: dynamic loading of
stage_unified omitted its tools directory, so setup_bundle could not import. The test now adds its repository-relative
tools directory and imports the two modules normally. All 35 existing synthetic package tests pass (29.115 seconds),
covering package boundaries with synthetic PE fixtures. Evidence: `artifacts/clean-install-20261029/package-tests.log`.
This does not prove an actual downloadable package or installation works.

Next bounded qualification:

1. Identify/build one exact stock-only candidate with a complete runtime dependency closure and manifest; do not
   assume a source checkout, compiler, Python, LaunchBox or Stream Deck exists on the player's machine.
2. Extract into a fresh path under the project's ignored artifacts or E:/Source/temp/fzero. Use Setup.cmd to open
   the launcher; select a user-supplied supported USA ROM through the ordinary flow. Test missing/wrong ROM errors.
3. Verify chosen ROM and launcher settings survive restart, Skip launcher can be recovered with Setup.cmd, and
   controls/screens can be configured without hand-copying DLLs. Force feedback remains explicitly opt-in.
4. Verify first gameplay separately, then update/remove/rollback retaining ROM references, configs, wheel profiles
   and saves. Reuse existing ROM-persistence and launcher-options recipes after reviewing their desktop/input paths.
5. Use the first failures to specify Wheelkit's download/destination/asset-selection flow. No generic installer
   framework and no public release switch before the real package path is qualified.

No build, installation, ROM import, desktop action or game launch occurred in this initial check. Runtime requires
the shared slot and proven state isolation; a synthetic package test is not player acceptance.

## Run plan (mida for guna 476889; lulu owns the qualification)

Candidate: build the current main (includes c587f1b: glass-center eye, `TripleCurveRadiusMm`) in a clean build dir,
then `python tools/stage_unified.py --build <build> --output <stage>` (stock-only: no `--private-*` flags) and
`python tools/package_unified.py --payload <stage> --output <dir>`, then `--verify <zip>`. Record zip SHA-256, VERSION
and source commit. Offline gates first: `tests/test_unified_package.py`, the geometry and video tests.

Rig block (one guna slot, ~25 min, game closed beforehand, force Off throughout):
1. Extract the zip to a fresh `E:/Source/temp/fzero/clean-<date>` (not under E:/Games). Hash the tree.
2. Setup.cmd opens the launcher. With no ROM: the launcher asks for one (capture the launcher window by hwnd).
3. Pick a wrong file (any non-ROM): refused with a readable error, nothing written but settings.
4. Pick the owner's supported USA ROM by path (read in place; never copied into the repo or artifacts):
   `rom.cfg` written, hash check passes, game starts to the title. Close.
5. Restart via Setup.cmd: ROM remembered, launcher settings kept; set Skip launcher, restart, then recover the
   launcher through Setup.cmd.
6. Controls/screens through the launcher only (no DLL copying); qualify flat/default output first. A separate
   diagnostic with `TripleCurveRadiusMm=1500` may check curved sides, but the center is still planar at c587f1b;
   do not count that as complete corrected triples. Record the actual output/window arrangement and captures.
7. Removal/rollback: retain configs, ROM references, profiles, saves and evidence before removing any disposable
   extracted payload. Verify each resolved removal path stays inside the isolated test root. Confirm the owner's
   installed F-Zero and receipt remain byte-identical. Check writes outside the test root; directory listings alone
   do not establish unchanged file contents. A normal portable-folder removal does not prove update/rollback;
   those remain separate checks against an exact prior package.
Input only to the launcher/game window when it is the verified foreground (or through the launcher's own
automation if it has one); no Steam involvement. Pass/fail per step goes in this file with capture paths.
