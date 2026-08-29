# Firmware Audit — August 2026

> **Resolution status (2026-08-29):** every finding below has been addressed on
> this branch, in the commits following the audit report:
>
> - **A1** — resolved as a documentation error, per the maintainer: the
>   N-Filter strip is the vent light (as the original firmware had it), so the
>   vent animation is restored, the orphaned fire-end stop calls are gone, and
>   the firing-light description in README/AGENTS is corrected. GPIO 28 is
>   renamed to the vent relay it drives and held solid during vents (issue #10).
> - **A2 + C4** — conditional tap clearing and interrupt-masked flag clears.
> - **A3** — the speed multiplier is now the single cyclotron authority, applied
>   only on Afterlife packs outside party mode; `adj_monitor` and
>   `wait_for_sequence_end` no longer stomp configured animation speeds.
> - **A4** — `sound_resume()` frame fixed (`\x06`); unused driver primitives marked.
> - **A5/A6** — the simulator now compiles the firmware's own animation sources
>   and steps in 4 ms firmware ticks; all 48 previews regenerated (future strip at
>   its real 16 pixels), name parsing fixed, workflow triggers on what it builds.
> - **C1** — `AnimationController` rewritten: fixed ring-buffer queue,
>   interrupt-masked mutators, deferred destruction via `pack_animations_reap()`;
>   no allocation or free ever happens in ISR context.
> - **C2** — the Afterlife cooldown and powerdown fade run from the main loop on
>   timestamps; no sound I/O, state writes, or ramp writes from the ISR. The
>   cooldown also yields to re-fire and power-off now.
> - **C3** — `show_leds()` is called only from the ISR (plus one pre-timer
>   warm-up frame that also moves FastLED's DMA buffer allocation to main).
> - **C5** — `pack_ctx.state` and `cy_speed_ramp` are single-context (main) after
>   C2; `update_pack_colors()` masks interrupts around the color writes.
> - **D** — dead code removed (`FillAnimation`, `CylonFadeOutAnimation` +
>   `CY_PATTERN_RING_FADE_OUT`, `WaitAction`/`CallbackAction`, unused globals,
>   the unused `mode_change_major` parameter, stale comments/flags);
>   `PC_SPEED_DEFAULT` renamed `ADJ_SPEED_POT`; `PS_FIRE_COOLDOWN` is
>   interruptible. The song-track rotation (96…96+count) is documented in code.
> - **E** — README/SOFTWARE README/AGENTS.md corrected (module names,
>   PS_FEEDBACK, ISR-vs-main description, badge, ADJ0 wording, Cylon claim);
>   AGENTS.md gained a concurrency-rules section; firmware version bumped
>   to 1.2.0.
>
> The findings text below is kept as written for the audit record.

Full review of the pack light and sound firmware at commit `0aaba33` (main), covering
race conditions, unreachable code, logic conflicts, and whether the animation previews
and documentation match what the firmware actually does. The firmware was compiled
against pico-sdk 2.1.1 (clean build, no project warnings), and the LED simulator was
built and run to verify the rendering findings empirically.

**Execution model recap** (this is where most findings come from): a repeating timer
ISR (`pack_timer_isr`, `klystron.cpp:54`) polls switches, advances all three
`AnimationController`s, and pushes LEDs every ~4 ms + execution time. The main loop
runs `pack_state_process()` continuously. Both contexts freely call into the same
controllers, sound driver, LED driver, and flag bytes.

---

## A. Functional bugs (verified, deterministic)

### A1. The N‑Filter "firing light" feature does not exist in the code
README.md:37 claims: *"The 'Future' light … is now correctly tied to the pack's firing
state and is no longer activated by the vent sequence."* AGENTS.md's acceptance
matrix repeats it (*"N‑Filter light activates only during firing states"*).

The code does the opposite:

- No firing path ever starts a future-strip animation. The only
  `g_future_controller.play()` calls in the tree are party mode
  (`party_sequences.cpp:173`) and the vent sequence (`monitors.cpp:509-513`).
- `PS_FIRE` / `PS_SLIME_FIRE` call `g_future_controller.stop()` when firing *ends*
  (`pack_state.cpp:270,323`) — stopping an animation that was never started; both
  calls are no-ops.
- `full_vent()` actively animates the future strip (strobe, or shift-rotate on
  Afterlife) — the exact behavior the README says was removed.

So: firing never lights the N‑Filter, and venting does. Either the feature was lost in
the animation-framework refactor or the docs describe an unimplemented plan. The
acceptance checklist in AGENTS.md cannot currently pass as written.

### A2. TVG party-mode fire taps are randomly swallowed
`PS_OFF` executes `clear_fire_tap()` unconditionally at the end of every pass
(`pack_state.cpp:162`) — including while a song is playing, which is exactly when taps
are meaningful (party-animation cycling, README.md:38).

`song_monitor()` samples the tap flag at the top of the pass (`monitors.cpp:60`). Any
tap the ISR raises *after* that sample but *before* `PS_OFF`'s clear is discarded
unseen. That vulnerable stretch (the `PS_OFF` body incl. `ring_monitor()`'s two ADC
reads) is most of the loop pass, so in TVG packs a large fraction of party-cycle taps
simply do nothing. Users can work around it unknowingly by holding the button >135 ms
(that path advances via the debounced rising edge instead), and non‑TVG packs are
unaffected (they never set the tap flag) — which makes the symptom look like
"flaky button" rather than a logic bug.

Fix shape: make the clear conditional (`if (!song_is_playing()) clear_fire_tap();`),
or consume taps in exactly one place.

Note: `song_monitor` itself is careful *not* to blanket-clear taps
(`monitors.cpp:127-132` explains why. `PS_OFF` defeats that care.)

### A3. Two authorities fight over the cyclotron animation speed
`pack_state_process()` force-writes the current cyclotron animation's speed **every
pass, in every state** from the fixed-point multiplier
(`speed = 1000·2¹⁶ / cy_speed_multiplier`, `pack_state.cpp:111-117`), while
`adj_monitor()` writes a *different* formula (`850 ms · cy_speed_multiplier / 2¹⁶`,
`monitors.cpp:313`, note the multiplier applied in the **opposite direction**), and
blocking helpers in `pack.cpp` (`wait_for_sequence_end`, the Afterlife startup loops)
carry their own copies of the pump. Concrete reachable consequences:

- **Party "Cylon Scanner" is broken on the cyclotron ring.** Party mode configures
  40 ms/step on all three strips (160 ms for N=4, `party_sequences.cpp:126-131`), but
  party runs in `PS_OFF` where the top-of-process setter jams the cyclotron
  animation to ~1000 ms/step (25× slower than the powercell/future eyes). Worse,
  after an Afterlife power-down the multiplier is left at 0
  (`pack.cpp:327`), the setter's guard then writes speed **0**, and the cyclotron eye
  steps every ISR tick (~6 ms — a fast flicker). Either way the three strips visibly
  desync; the ANIMATIONS.md previews show them in sync. (Only the Cylon party pattern
  is affected — the other three ignore the speed parameter.)
- **Classic/TVG power-down fade finishes ~3× early.** `pack_powerdown_sequences`
  configures 2900/3100 ms fades (`pack_config.cpp:37-38`), but
  `wait_for_sequence_end` (`pack.cpp:255-269`) overrides the running `FadeAnimation`
  to 1000 ms every 20 ms, so the cyclotron is dark in ~1.7 s while the powercell
  drain and shutdown sound run ~3 s.
- **`adj_monitor`'s cyclotron branch is effectively dead** the rest of the time: its
  value only gets written on change and is immediately re-overwritten every pass by
  the top setter. Consequently neither ADJ0 nor the heat effect ever influences
  cyclotron speed in any pack type (`adj_to_ms_cycle` ignores both when
  `apply_cy_speed` is set, `monitors.cpp:264-267`) — README.md:140 implies ADJ0 is
  only ignored "in Afterlife configurations".
- During Afterlife fire/cooldown ramps the two formulas alternate within each pass;
  because of the ramp's 10 ms grain the intended (top-setter) value wins almost all
  the time, with occasional one-tick jumps to the wrong-direction value.

The intended design appears to be "cyclotron speed = 1000 ms / multiplier, period".
Removing the multiplier math from `adj_monitor`'s cyclotron path and making the
per-animation configured speed authoritative when a multiplier isn't in play (or
excluding party/feedback animations from the top setter) would reconcile this.

### A4. `sound_resume()` sends a malformed serial frame
`sound_module.cpp:121`: `uart_puts(uart0, "\x7E\xFF\x0G")` — `\x0G` is not a valid
escape; C parses it as `\x0` followed by `'G'`, so the frame starts
`7E FF 00 47 0D …` instead of `7E FF 06 0D …`. The DFPlayer would reject it.
Currently harmless because nothing calls `sound_resume()` (see C1), but it's a
landmine for whoever wires up pause/resume.

### A5. All "Video Game / Fade Mode" animation previews are static images
`sim/example_sketch.cpp:121-124` splits the animation name at the **first**
underscore: `video_game_cyclotron_sequence` → mode `"video"`, base
`"game_cyclotron_sequence"`, which matches no render branch and falls through to the
solid-color fallback (with the wrong 300-frame duration as well).

Verified two ways: running the freshly built simulator
(`video_game_cyclotron_sequence` → 300 frames, **1 unique**;
`movie_cyclotron_sequence` → 360 frames, 31 unique) and decoding the committed GIFs
(`video_game_powercell_…gif`: 75 frames, 1 unique; `video_game_cyclotron_…gif`:
2 unique). The whole "Video Game / Fade Mode" section of ANIMATIONS.md shows frozen
solid-color pictures.

---

## B. Animation previews and docs vs. actual firmware behavior

The renderer (`sim/example_sketch.cpp`) is a **from-scratch reimplementation**, not
the firmware's animation code — `render-animations.yml` even *triggers* on changes to
`SOFTWARE/animations.cpp`/`.h` (lines 12-13, 19-20) but never compiles those files,
so the previews re-render identically no matter what changes in the firmware.
Divergences confirmed by decoding the committed GIFs and reading `animations.cpp`:

| Preview (GIF/sim) | Firmware reality |
|---|---|
| Movie cyclotron: **4 dots at once** rotating (4–5 lit clusters measured mid-GIF) | `RotateAnimation` lights **one** of the four table positions at a time (`animations.cpp:344-366`) |
| Afterlife cyclotron: 4 dots, double speed | Single Cylon "eye" whose speed follows the spin-up multiplier |
| TVG cyclotron: two dots at ring-opposite positions | One position crossfading to the next from the same 4-entry table (`RotateFadeAnimation`) |
| Powercell: single scrolling dot | `ScrollAnimation` **fills** the column LED by LED, then clears (`animations.cpp:199-209`) |
| Party Rainbow Fade: spatial rainbow gradient scrolling | All LEDs the **same** color cycling through the spectrum (`fill_solid`, `animations.cpp:450-454`) |
| Party Cylon: eye with fading trail | Hard single pixel, strip blanked every frame — **no trail** (`draw_cylon_eye`, `animations.cpp:33-38`) |
| Future/N‑Filter previews: idle rotate with fade in/out, **18 LEDs** | No idle future animation exists at all; hardware/firmware strip is **16** LEDs (`NUM_LEDS_FUTURE`, HARDWARE/README) |

Related doc drift:

- README.md:91-92: *"The Cylon effect always draws the trailing 'eye'"* — false;
  there is no trail in `CylonAnimation`.
- ANIMATIONS.md's local-regeneration instructions use `fps=30`, the CI workflow uses
  `fps=15` — locally regenerated GIFs won't match committed ones.
- ANIMATIONS.md labels the future column "Future/Vent (18)".

If the previews are meant to be authoritative, the long-term fix is to link the sim
against the real `animations.cpp` (the sim scaffolding already exists); short-term,
at least fix the `video_game_` parsing and the 18→16 LED count.

---

## C. Race conditions and cross-context hazards

These are design-level issues; individually low-probability per event, but the
windows are hit constantly (every ISR tick vs. every state transition). They are the
kind of thing behind "it glitched once at a con and never again."

### C1. `AnimationController` is mutated from both contexts with no locking, and frees/allocates heap in the ISR
- ISR side: `update()` pops the action queue, `reset()`s `unique_ptr`s (i.e. calls
  `delete`), and runs queued actions (`animation_controller.cpp:25-38,63-69`).
- Main side: `play()`/`stop()`/`enqueue()` destroy and allocate animations/actions
  (`std::make_unique`, `std::queue` on `std::deque`) — e.g. every power-up,
  power-down, mode change, vent, party toggle.

If the ISR fires while the main loop is inside `play()`/`stop()` (queue node
half-linked, `unique_ptr` mid-swap, or newlib `malloc`/`free` mid-operation), the
result is a torn container or corrupted heap — newlib's allocator is not
IRQ-reentrant on the same core, and the Pico SDK malloc mutex does not protect
against IRQ preemption. Everything else in this section is aggravated by this one.

Also inside this framework: the Afterlife power-down chain enqueues
`CallbackAction([]{ g_cyclotron_controller.stop(); })` (`pack.cpp:306`). When it
runs, `stop()` → `currentAction.reset()` **destroys the CallbackAction while its own
`start()` is still on the stack**. It happens to survive because the lambda is
capture-less; any capture added later turns it into a use-after-free.

Fix shape: defer all structural mutation to one context — e.g. controllers own a
small command mailbox (set from main, applied at the top of `update()` in the ISR),
or bracket `play/stop/enqueue` and `update` with `save_and_disable_interrupts()`.
Pre-allocating animations statically instead of `make_unique` per transition would
remove heap traffic from the hot path entirely.

### C2. The fire-cooldown callback does sound I/O inside the ISR
The last step of the `PS_FIRE` → cooldown chain (`pack_state.cpp:307-317`) runs from
the timer ISR (its `start()` fires when the preceding `WaitAction` completes inside
`update()`), and it calls `sound_stop()` + `hum_monitor()` →
`sound_start_safely()` (`monitors.cpp:141-165`), which **busy-waits on the DFPlayer
BUSY pin** — typically ~50–200 ms for a stop-then-start round-trip, bounded only by
two 5 s timeouts. For that entire time LEDs freeze and inputs go unpolled.

Worse, its UART writes can interleave byte-for-byte with a sound command the main
loop was in the middle of sending (`sound_start` is six separate `uart_putc` calls),
corrupting both frames — an occasional wrong/missing sound right after an Afterlife
firing stop would be this. The same callback also plays the powercell animation and
flips `pack_ctx.state` from the ISR (see C5).

### C3. `show_leds()` / `FastLED.show()` is re-entered from two contexts
The ISR calls `show_leds()` every tick; the main loop also calls it from
`PS_OFF` (`pack_state.cpp:135`), the `pack.cpp` blocking loops (`196,223,265`),
`full_vent()` (`monitors.cpp:545`) and throughout `board_test.cpp`. The RP2040
FastLED driver (`clockless_arm_rp2040.h:229-302`) fills a **single per-controller
DMA buffer** and re-triggers the channel; a `show()` preempted after the busy-check
can have its buffer overwritten mid-fill and its active DMA transfer restarted by
the ISR's `show()` — corrupted frames on the wire. The 400 Hz frame cap's busy-wait
also runs inside the ISR (this is the "interval moves with LED load" effect the
comments already acknowledge). The main-loop `show_leds()` calls are redundant while
the ISR is running — dropping them (or gating the ISR's) removes the overlap;
`g_brightness_ramp.update()` from both contexts goes away with it.

### C4. Lost-event read-modify-write races on the shared flag bytes
The main loop's `clear_fire_tap()` / `clear_song_toggle()` / `clear_pack_pu_req()`
(`klystron_IO_support.cpp:341-351`) and `song &= 0x7F` (`monitors.cpp:121,142`) are
non-atomic RMWs on bytes the ISR also RMWs (`user_switch_flags` set/clear on every
tick, `check_fire_switch_isr`). If the ISR sets SONG_TOGGLE/PACK_PU_REQ/FIRE_TAP
between the main loop's load and store, the event is erased. The window is a few
instructions but the clears run millions of times per hour — expect a fraction of a
percent of song toggles / power-up requests to silently vanish ("the switch didn't
take"). Wrap the clears in `save_and_disable_interrupts()`/`restore_interrupts()`
(or use the SIO bit-set/clear aliases) and the class disappears.

### C5. Smaller cross-context frictions
- `pack_ctx.state` is written from the ISR (cooldown callback → `PS_IDLE`) and read
  by the main-loop `switch`, but is not `volatile`/atomic — works today because
  opaque calls force reloads; fragile under LTO (`pack_state.cpp:41-45,316`).
- `cy_speed_ramp.go()` runs from ISR callbacks while the main loop is inside
  `cy_speed_ramp.update()`; `_ramp<T>` is multi-field with no guards
  (`libs/RAMP/Ramp.h:80-91`) — a torn update yields one bogus multiplier sample.
- `volatile CRGB cyclotron_color` etc. are 3-byte structs updated from main while
  ISR animations read them — worst case one frame renders a mixed color.
- `rand()` is called from both contexts (newlib state RMW) — harmless for an LED
  toy, worth knowing.

### What was checked and is NOT racy
- The ADC mux (`adc_select_input`) is only ever touched from main-loop context
  (`ring_monitor`/`adj_monitor`/`board_test`); the ISR never reads the ADC.
- `temperature`/`monster_timer`/`response_timer` are aligned ≤32-bit volatiles with
  single-writer-per-transition protocols — benign.
- The "remainder LEDs off" invariant holds: `show_leds()` masks
  `[g_cyclotron_led_count, 40)` on every frame (`addressable_LED_support.cpp:62-88`),
  including when N is reduced mid-animation. (The commented-out
  `mask_cyclotron_leds()` in `pack_timer_isr` is redundant, not a missing call.)
- The recently reworked TVG fire tap/hold classifier (`check_fire_switch_isr`) is
  sound: widths measured between raw edges on the hardware clock, the
  window-expired latch makes "tap" and "fire" mutually exclusive and exhaustive,
  and the non-TVG path clears the gate every poll so a pack-type change mid-press
  can't wedge it. Code matches the README's 135 ms description, including
  `-DFIRE_TAP_WINDOW_MS`.

---

## D. Unreachable / dead code and logic leftovers

| Item | Location | Note |
|---|---|---|
| `sound_pause` / `sound_resume` / `sound_repeat` | `sound_module.cpp:105-141` | Never called; `sound_resume` also has bug A4. `sound_repeat` sends `index+1` unlike `sound_start` — suspicious if ever revived. |
| `FillAnimation` | `animations.cpp:212-232` | Never instantiated anywhere. |
| `CylonFadeOutAnimation` + `CY_PATTERN_RING_FADE_OUT` | `animations.cpp:161-190`, `pack.cpp:316-318` | The pattern id appears in no `pack_powerdown_sequences` entry, so the `case` is unreachable. |
| `cyclotron_after_set[3][3]`, `cyclotron_seq_num` | `cyclotron_sequences.cpp:19,21` | Defined, declared extern, used nowhere. |
| 5th column of `cyc_classic_pos` | `cyclotron_sequences.cpp:26-31` | Only indices 0–3 are ever read. |
| `mode_change_major()`'s first parameter | `monitors.cpp:324` | `cyclotron_pattern_base` unused; callers pass 7/5/5/5 for nothing (compiler confirms). |
| Empty Afterlife branch | `led_patterns.cpp:41-43` | `if (...) { /* no action */ }` |
| Double `pack_state_set_mode(PACK_MODE_PROTON_STREAM)` in `PS_OFF` | `pack_state.cpp:136-138` + `145-147` | Second call unconditionally repeats the first. |
| `board_test_done` static flag | `klystron.cpp:102,120-124` | The check runs exactly once per boot; the flag can never be observed true. |
| Stale comment: "Ensure any LED DMA transfers have completed…" | `monitors.cpp:143-144` | No such code follows. |
| `PC_SPEED_DEFAULT` | `monitors.h:22` | Misnomer — it's the ADC channel selector (0 = ADJ0), also passed for cyclotron lookups. |
| `party_mode_run()` doc comment | `party_sequences.cpp:37`, `party_sequences.h:60` | Says "called on every main loop iteration"; it is called from the timer ISR. |

Behavioral gap acknowledged in-source: `PS_FIRE_COOLDOWN` (`pack_state.cpp:259-265`)
ignores the fire button *and the power switch* for the ~5 s cooldown — you cannot
re-fire, and a power-off is acted on only after the queued actions promote the state
back to `PS_IDLE`. The comment says interruption "could" be added; today the pack
just feels unresponsive for those seconds.

---

## E. Documentation nits (top-level README et al.)

- README "Firmware architecture" says *"a repeating timer calls into the pack state
  machine"* — inverted: the timer ISR runs animations/inputs; the **main loop** runs
  the state machine (SOFTWARE/README.md has it right). All module references use
  `.c` names (`klystron.c`, `pack_state.c`…) though the tree is `.cpp`.
- The state table (README.md:53-64) omits `PS_FEEDBACK` (the ADJ1 rainbow state,
  `pack_state.h:58`).
- README build badge points at `ajquick/klystron`, not this repository — it will not
  reflect this repo's CI.
- `pack_song_count = 3` yields a four-track rotation (96, 97, 98, 99 →
  `sound_start_safely(96 + (song & 0x7f))`, `monitors.cpp:83-84`): the wrap plays
  track 96, the increments play 97–99. Worth confirming the SD card actually has
  four song tracks starting at 96, or the last toggle in each cycle plays nothing.
- CMake `project(... VERSION 1.1.3)` while the last version-bump commit message says
  1.1.2 — make sure that's an intentional bump, since the board test announces the
  CMake version.

---

## F. Suggested priorities

1. **A2 + C4** — small, safe fixes (conditional tap clear; IRQ-guarded flag clears)
   that directly remove user-visible input flakiness.
2. **A3** — decide on a single cyclotron speed authority; this fixes party Cylon,
   the power-down fade duration, and makes ADJ0/heat behavior intentional.
3. **A1** — decide whether the N‑Filter firing light should exist (implement) or not
   (fix README + AGENTS checklist); today the docs promise something the pack
   doesn't do.
4. **C1/C2/C3** — schedule the concurrency cleanup: move controller mutation into
   one context, get sound I/O out of the ISR (a "pending sound op" flag the main
   loop services would do), stop calling `show_leds()` from the main loop.
5. **A5/B** — fix the sim's `video_game_` parsing and 18→16 future count; longer
   term, render previews from the real `animations.cpp`.
6. Sweep section D's dead code when convenient.
