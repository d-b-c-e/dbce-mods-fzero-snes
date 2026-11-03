# Raw snapshot hat-array bound

DBC-328 follow-up to b75d883.

The retained Release consumer build reproduced GCC -Wstringop-overflow in
raw_snapshot: offset 4 into hats[4] at the SDL3 hat read. The preceding code
already clamps hat_count to 0..4. Source reconciliation therefore establishes
ambiguous indexing in the optimized diagnostic, not a demonstrated reachable
overflow. The loop rereads the struct member around external SDL calls.

The shared SDL2/SDL3 loop now explicitly requires the index to be below the
array capacity as well as hat_count. Existing count clamps, zero initialization,
SDL hat values, injection arguments, cardinal/diagonal mappings and neutral
lookups remain unchanged. Valid counts 0..4 read exactly the same indices.
No other reader or injection path was changed.

Validation: retained single-config Release consumer clean compile/link reproduced
the warning before the change; admission, changed production reader compilation
and consumer link passed afterward with no warnings. Existing test_gamepad.c
cardinal/diagonal/neutral checks compiled in the baseline build and linked in the
updated consumer. They were NOT executed. No new tests or runtime hooks.
SDL2 uses the same bound by source inspection; no SDL2 compilation is claimed.
Evidence: .local/hat-bounds-evidence/before-build.log and after-build.log.

Archie DBC-330's source verdict remains separate and unchanged. runtimeAllowed=false.
The next unfinished milestone is independent launcher preparation with its
reviewed child-environment boundary; no launcher implementation or runtime
admission is claimed here. Preserve DBC-65 recovery and Sandy's rig ownership.
No live config/save/install/game/input/desktop/public push or player meaning
change. All STD-001..040 are represented; STD-019 unchecked, STD-021/031/038
pending and STD-037 partial remain unchanged.
