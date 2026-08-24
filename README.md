# Johnny Castaway Reborn — Windows 10/11 (64-bit)

An open-source engine that reimplements the classic **"Johnny Castaway"** screensaver
(Sierra / Dynamix, 1992), fixed to run natively on modern 64-bit Windows — as a real
Windows screensaver (`.scr`), in a window, or embedded as a live wallpaper via
[Wallpaper Engine](https://store.steampowered.com/app/431960/Wallpaper_Engine/).

This is a maintained fork of [sizious/johnny-castaway-reborn](https://github.com/sizious/johnny-castaway-reborn),
itself based on the original [jc_reborn](https://github.com/jno6809/jc_reborn) project by Jeremie Guillaume.
See [CHANGES.md](CHANGES.md) for what changed in this fork.

**➡️ [Get the Windows installer, tutorial, and full instructions on the project page](https://yoluha.github.io/johnny-castaway-reborn/)**

---

## ⚠️ This does NOT include the original game

`RESOURCE.MAP` and `RESOURCE.001` (and the sound files) are the property of
Sierra On-Line / Dynamix and are **not** included in this repository or in the
installer. You need to obtain your own copy of the original 1992 floppy disk
image — see the project page above for a known source and step-by-step
instructions. The installer includes a script that extracts and decompresses
everything automatically once you point it at the file you downloaded.

## What's in this fork

- Native Windows screensaver support (`/s`, `/p`, `/c` command-line convention)
- A graphical settings dialog (PT/EN/ES), no registry editor needed
- Multi-monitor support: pick a monitor, clone across all of them, or stretch
  across all of them as one continuous image (including portrait/vertical monitors)
- Several scale modes: auto, fill, fit, cover
- Old-monitor image filters: CRT scanlines, green/amber monochrome, strong CRT
  with vignette, faded/sepia
- Per-effect volume control
- Configurable mouse-dismiss behaviour (exit on click / on movement, independently)
- A live wallpaper tutorial using Wallpaper Engine's "Application" wallpaper type
- A Windows installer (NSIS) with a components page, license/disclaimer page,
  and a bundled PDF tutorial

## Building from source

Requires a C99 compiler and [SDL2](https://www.libsdl.org/).

```
# Windows (MinGW-w64 / MSYS2)
mingw32-make -f Makefile.MinGW

# Linux
make -f Makefile.linux
```

## License

GPL-3.0 — see [LICENSE](LICENSE). This is free software: you're welcome to
read it, modify it, and redistribute it under the same license.

## Disclaimer

This is a non-commercial fan project, made purely out of love and nostalgia
for the "Johnny Castaway" / "Screen Antics" franchise. No rights are claimed
over the original game, its graphics, sounds, characters, or trademark —
those belong to Sierra On-Line / Dynamix and their legal successors,
including Activision Blizzard. This project is not affiliated with,
sponsored by, endorsed by, or associated in any way with Sierra, Dynamix,
Activision Blizzard, or any current rights holder. See
[installer/DISCLAIMER.txt](installer/DISCLAIMER.txt) for the full trilingual
legal notice.

## Credits

- Original engine: Jeremie Guillaume ([jc_reborn](https://github.com/jno6809/jc_reborn))
- This fork's upstream: [sizious/johnny-castaway-reborn](https://github.com/sizious/johnny-castaway-reborn)
- Format research: Hans Milling ([JCOS](https://github.com/nivs1978/Johnny-Castaway-Open-Source)),
  Alexandre Fontoura ([castaway](https://github.com/xesf/castaway)),
  [Sierra Chest](http://sierrachest.com/index.php?a=games&id=255&title=johnny-castaway)
- Deark (resource extraction/decompression): Jason Summers ([deark](https://github.com/jsummers/deark))
- SDL2: zlib license

## Questions or issues?

Lucas Yoshiki Amaral — yoluha91@gmail.com
