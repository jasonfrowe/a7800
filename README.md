# **A7800 - the Atari 7800 Emulator** #

What is A7800?
==============
A7800 is a fork of the MAME Atari 7800 driver, with several enhancements added.

* Support for emulation of Proline Joysticks, VCS Joysticks, Lightguns, Paddles, Driving Controllers, Keypads, Trak-Balls, Amiga Mice, and ST Mice.
* Maria DMA timing has been improved further, with the addition of accurate DMA hole penalties.
* A bug in the existing RIOT emulation has been fixed.
* POKEY sound emulation improvements.
* SALLY (CPU) and MARIA (Graphics chip) performance adjustments.
* Selectable and improved palettes with enhanced screen options.
* Audio indication of no ROM loaded silenced.
* BIOS file(s) no longer required and made optional.
* Streamlined UI including menu options to have an Atari 7800 system focus.
* Implementation of XM control registers updated.
* Graphical register updates made mid-scanline are now displayed mid-scanline.
* Bankset bankswitching support added.
* POKEY@800 added for non-banked, supergame, and bankset formats.
* Machine targets a7800dev and a7800pdev added, which display DMA usage per-scanline.

MAME compatibility and syntax has been maintained, to allow for the reuse of MAME configuration files and front-ends.

This Fork (llvm-mos-sdk-7800)
==============================
This fork is vendored as an owned submodule of [llvm-mos-sdk-7800](https://github.com/jasonfrowe/llvm-mos-sdk-7800), where it serves as that project's test/verification emulator for its Atari 7800 LLVM-MOS platform port. It's no longer a hands-off external dependency -- it gets modified as needed to make that project's own test suite actually work, most importantly by adding the headless Lua-scripting support (register/memory tracing, scripted input injection, screenshot capture) that development there depends on.

Changes made for that purpose:

* **Fixed a real POKEY@450 crash.** The `A78_TYPE0_POK450`-family cart-slot install (`src/mame/drivers/a7800.cpp`) claimed the full `0x0400-0x04FF` address range for POKEY, when only `0x0400-0x047F` should be, per the 7800 Software Guide's own memory map -- `0x0480-0x04FF` is real 6532 RIOT RAM, already backed by storage in the base memory map, and explicitly documented as "Don't Use" for external device mapping. This silently replaced that RAM with a stub returning a constant `0xFF` for every out-of-range read; the BIOS's own boot sequence walks its PC into that hijacked range and executes the `0xFF` stream as garbage 6502 instructions, corrupting zero page until it crashes -- independent of anything the actual cart ROM contains. Narrowed the claimed range to `0x0400-0x047F` to fix it.
* **Fixed the build on a modern (2026) Apple Silicon macOS toolchain.** `3rdparty/zlib/zutil.h`'s `TARGET_OS_MAC` guard now fires on every modern Apple platform (not just classic pre-X Mac OS as originally assumed), breaking `<stdio.h>` parsing throughout the build; scoped it back out with `__APPLE__`. The plain `macosx`/`make` target is hardcoded to 32-bit (i386), which this machine can no longer link at all -- use `macosx_x64_clang` instead. Build with:
  ```
  NO_USE_MIDI=1 NO_USE_BGFX=1 USE_BUNDLED_LIB_SDL2=1 make macosx_x64_clang -j$(sysctl -n hw.ncpu)
  ```
  (disables portmidi and the Carbon-era native macOS debugger UI, both of which use APIs Apple has since removed, and builds SDL2 from the already-vendored `3rdparty/SDL2` source instead of linking Homebrew's arm64-only build against this nominally-x86_64 target).
* **Made headless debugger scripting (`-debugscript`) actually work.** Under the `-debugger none` OSD backend, `src/osd/modules/debugger/none.cpp` previously called `.go()` immediately on every stop without ever processing a pending `-debugscript`'s commands (`debugger_cpu::process_source_file()` only reads script lines while execution is stopped, a window this stub never left open) -- so `-debugscript` was a silent no-op under this backend. It now processes the script first. Separately, the debugger console's own output (`src/emu/debug/debugcon.cpp`) only ever went to an internal text buffer no UI reads under `-debugger none`; mirrored it to stdout so breakpoint/watchpoint output is actually visible for headless use. Together, these are what made real watchpoint-based diagnosis of the POKEY crash above possible in the first place.
* **Added an opt-in cartridge-bus cycle logger** (`src/mame/drivers/a7800.cpp`), for diagnosing cart-memory-mapping issues without a full debugger session. Set `A7800_CARTBUS_LOG=<path>` to write a CSV of every CPU cycle touching `$4000` and above (cycle count, read/write, address, data, and the internal code path that serviced it); `A7800_CARTBUS_LIMIT=<n>` caps the row count for long runs.

None of the above are upstream MAME/7800-devtools changes -- they're specific to this fork's role as llvm-mos-sdk-7800's test emulator, and the intent is to keep them scoped that way (fix real bugs, add opt-in diagnostics) rather than diverge from upstream behavior for normal play.
