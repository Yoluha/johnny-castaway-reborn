# Changes in this fork

Based on [sizious/johnny-castaway-reborn](https://github.com/sizious/johnny-castaway-reborn).
Summary of what changed here:

## Windows integration
- Fixed working-directory handling so resource files load correctly regardless
  of how the executable is launched (screensaver, finish-page "run now",
  Wallpaper Engine, direct double-click)
- Config file moved to `%APPDATA%\JohnnyCastawayReborn\` instead of relying on
  a non-standard `HOME` environment variable
- Declared the process DPI-aware, fixing window positioning/sizing on
  multi-monitor setups with mixed display scaling

## Settings dialog (`scrconfig.c`)
- New native Win32 dialog (PT/EN/ES, switches language live) covering every
  setting below — no registry editing needed
- Monitor and audio device lists resize to fit every detected device, no
  scrollbars
- Defensive validation on every field, so a corrupted or hand-edited config
  file can't crash the dialog or the engine

## Display
- New scale mode: `cover` — fills the screen edge-to-edge without letterbox
  bars and without distorting the image (crops overflow instead)
- Multi-monitor "extend" mode fixed to actually stay on top/focused instead of
  silently rendering behind other windows
- Five image filters instead of one: CRT scanlines, green monochrome, amber
  monochrome, strong CRT with vignette, faded/sepia — the CRT effect is also
  now applied before scaling so it stays visible at any zoom level

## Input / screensaver behaviour
- Mouse click and mouse movement now dismiss the screensaver (previously only
  keyboard input did) — each independently configurable in settings
- Preview-box embedding no longer exits prematurely on a transient false
  negative when checking whether the parent window still exists

## Audio
- Volume control (0–100), with proper format-aware mixing instead of a raw
  memory copy

## Installer (NSIS)
- Windows installer with license/disclaimer page, component selection
  (engine / screensaver / fetch original game files), and a bundled PDF
  tutorial shown automatically at the end of setup
- Trilingual (PT/EN/ES) legal disclaimer, readme, and tutorial
- A live-wallpaper tutorial using Wallpaper Engine's "Application" wallpaper
  type, since Windows' internal WorkerW mechanism this project used to rely
  on for a native live wallpaper is undocumented, unsupported, and does not
  behave consistently across Windows 11 builds — Wallpaper Engine is the
  reliable, officially-supported path instead
