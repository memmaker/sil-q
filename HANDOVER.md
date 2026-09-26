# Sil-Q 1.5.0: handover

RVIP import (`~/Games/rvip-tools/RVIP.md`, case A) and web port (`~/Games/rogue2wasm.md`).

## Source and changes

- Base: **Sil-Q 1.5.0**, https://github.com/sil-quirk/sil-q tag `v1.5.0`,
  commit `09a53ab8b167660742add9ed7842b4c2301737fd` (GitHub tarball
  `v1.5.0.tar.gz`, sha256 `d54e352ac5554b69f4b78916700d59f2395cf5d2c941a8ed9b4ee6646bc04319`).
  The folder here was an unzipped copy of exactly that tree (checked with `diff -r`).
- Original source: https://github.com/sil-quirk/sil-q/tree/09a53ab8b167660742add9ed7842b4c2301737fd
- Our changes: https://github.com/memmaker/sil-q/compare/15e6566...main
  (repo `memmaker/sil-q`; first commit `15e6566` is the untouched upstream tree).
- Remotes: `upstream` = sil-quirk/sil-q, `memmaker` = our repo.

## Playing

- Desktop: `~/Desktop/Games/Roguelikes/Sil-Q.app` runs `play.sh` (X11/XQuartz).
  The original Cocoa app `Sil.app` is untouched; its alias is now
  `Sil-Q (Cocoa)` (it keeps its own saves in `~/Documents/Sil/Sil-Q`).
- Web: https://ruzzoli.de/roguelikes/sil-q/ (build `web/build.sh`, deploy `web/deploy.sh`).
- Docs: `~/Desktop/Games/Roguelikes/Docs/sil-q.html` (entry in `build-docs.py`, saving text in `guides.py`).

## What was done

- **Frontend: X11** (`src/main-x11.c`, already had MicroChasm tiles).
  Chosen over the Cocoa app because RVIP needs the env-var window layout
  and the X11 test tools (`xsend`/`xwd`); the Cocoa app can't be tested
  without global input. `src/Makefile.std` uses `/opt/X11`.
  `play.sh`: 15×30 font + big tiles (30×30 map cells), `-s` = nearest-neighbour,
  windows: 1 Inventory, 2 Messages, 3 Monster list, 4 Recall, 5 Combat rolls
  (`lib/pref/user-x11.prf`, processed after the savefile in
  `process_some_user_pref_files()`; term names in `variable.c` renamed to match).
- **ASan**: one run (birth, menus, explore, fights, death screen) found an
  upstream out-of-bounds read in `get_move_wander()` (`melee2.c`: random
  direction from `ddd[]` used as an index into `ddy_ddd[]`). Fixed. ASan
  binary and objects removed.
- **Auto-explore `P`, stairs `<`/`>`** (`cmd2.c` `explore_step()`,
  `do_cmd_stairs()`), done before this session; verified: stops on monsters in
  view, disturbances, locked doors (skipped on the next press).
- **Enter menu** (`src/cmd-rvip.c` `do_cmd_command_menu()`, called from
  `process_command()` for `\r`/`\n`): six groups (following the `?` help
  screen), 60 commands, content-sized boxes (`rvip_box()`, aligned to the
  two-cell big tiles so restoring the screen leaves no half tiles).
- **Inventory 3c** (`cmd-rvip.c` `do_cmd_inven_screen()`): cursor, letter =
  main action, Shift = drop, Ctrl = examine, Enter/5 = item menu, numpad keys,
  reopen after an action unless a monster is in view (`inven_reopen`, checked
  before `request_command()` in `dungeon.c`). Actions run the normal
  commands with the item preselected (`item_preselect` in `get_item()`).
  Every item prompt shows the list with a cursor (8/2, 4/6 switch
  inventory/equipment/floor, 5/Enter choose; tags 0/1/3/7/9 still work).
  Keypad Enter + - * / . macros in `pref-x11.prf`. No mouse (Sil's z-term
  has none).
- **Tiles**: MicroChasm 16×16 (the only set Sil-Q ships; no Gervais set in
  1.5.0), nearest-neighbour everywhere. Unknown grids show the tileset's dark
  "unknown" pattern.
- **Sound**: Sil-Q's own events (message types → `sound()`), plus new
  sound-only events raised in the code: `SOUND_MON_HIT` (monster hits you,
  `melee1.c`), `SOUND_WIELD` (`cmd3.c`), `SOUND_SING` (`change_song()`),
  `SOUND_BREATH`, `SOUND_LOCKPICK`, `SOUND_DISARM`. `lib/xtra/sound/sound.cfg`:
  Sil-Q's samples first, empty/new events filled from the Dubtrain pack (only
  used `.wav`s copied). X11 build has no sound output; the web build plays them.
- **Fix**: `-u<name>` never built the savefile path (always started a new
  character); `main.c` now calls `process_player_name(TRUE)`.

## Web port

- `src/main-web.c` (from Quickband's): no mouse, no `EVT_RESIZE` in Sil's
  z-term, so a main-window resize at the prompt queues `^R` (redraw);
  `TERM_XTRA_SOUND` → page. Options `auto_more`/`center_player` default on
  (`option_norm[]` is no longer `const`).
- `web/sil.js` (from `quickband.js`): TILE 16, `tiles.png` =
  `lib/xtra/graf/16x16_microchasm.png` (already has alpha). Saves in
  IndexedDB under `/sil-q/lib/{save,user,apex}`. Savefiles are named after
  the character (`0.<name>`): the page passes `-u<name>` of the most recently
  saved one (pushed into `Module.arguments` after IDBFS loaded). No saves →
  Sil-Q's title menu (tutorial / new / open by name). After death or Ctrl-X
  the game returns to that title menu; its "Quit" shows the Play-again overlay.
- Music: `web/music/new_town.ogg` loops while a game is on (Sil-Q has no
  town and no music). Sound and music buttons start off.
- Help page: `web/make-help.py` (Docs page + web saving text + "About this version").

## Tested

- X11 (with `xsend`/`xwd` on the test windows only): birth, Enter menu and
  its groups, inventory/equipment screens, item menu → throw with the item
  preselected, reopen after the action, wear prompt with cursor, explore,
  save/load (`c` Open saved character), ASan run to a death.
- Web, locally and live (built-in browser pane): birth, tiles, all
  subwindows filled (inventory, messages, monster list with icons), Enter
  menu, inventory + item menu, explore, Ctrl-S → `0.Webtest` in IndexedDB,
  `_web_request_save()`, reload continues the character, every options
  screen, gutter drag, zoom, Help panel, sound events (`eat`, `wield`) fetched
  and played, sound toggle persisted, 1000×650 resize, Ctrl-X → scores →
  title menu → Quit → Play-again overlay; no console errors. Test saves in
  the browser were deleted afterwards; test files in `lib/` too.
- Not tested: Combat rolls / Recall window contents in a real fight on the
  web, the tutorial on the web, Import save.

## Notes for next time

- `git log` shows a commit `wip: port + web (WASM) build state` that was
  made and pushed (creating `memmaker/sil-q`) by another process while this
  import was running; its content is this import's working tree at that time.
- Prompt line (RVIP step 5 / W4, 2026-09-26): the live message row is shown in a
  box over the map by `RvipWM.prompt` (rvip-wm.js). A key hides it only while
  the game waits for a command, so a question stays up until answered.
  Here: `js_next_event(inkey_flag && character_generated)` in `src/main-web.c`;
  the page tracks term 0 row 0 (`row0` in `text`/`wipe`/`clear`) and sends it on
  `fresh(0)`.
