# Foxhollow Noclip

A standalone native mod for Star Fox Adventures running through Foxhollow. It lets the player walk through walls and other collision during normal on-foot gameplay, while still stopping at the edge of the map and keeping the player from falling forever.

It is based on the already-tested Noclip from the Foxhollow `feature/cheat-menu-noclip` branch, controlled directly by the 0 and Numpad 0 keys.

## Controls

| Key | Action |
| --- | --- |
| 0 / Numpad 0 | Toggle Noclip |

- 0 is the regular 0 key on the number row above the letters. Numpad 0, the 0 on the numeric keypad, works the same way.
- Each press toggles once. Holding a key does not repeat; release it and press it again to toggle again.
- Both keys are one control: pressing both at once, or one while the other is held, toggles only once.
- A key that is already held when the game window regains focus has to be released and pressed again.
- The keys only work during gameplay while the Foxhollow window has keyboard focus. A key pressed while the window is in the background is ignored, not saved for later.
- On Windows, Numpad 0 requires Num Lock to be on. With Num Lock off, or while Shift is held, it is not treated as a Noclip key. Insert is not a Noclip control.
- Noclip starts off and turns off when you leave the current save (returning to the title screen, the save select or a soft reset).
- There is no on-screen display. Changes are written to the Foxhollow log, for example `[Noclip] enabled`.

## Behavior

- Movement stays the game's own: Noclip only stops walls and objects from pushing the player back sideways. Floors and ceilings still work as usual.
- Walking into a wall no longer starts a climb, ledge grab or wall transition while Noclip is on.
- Map-boundary protection: horizontal movement stops when the step would leave the map.
- Fall protection: walking through geometry into a spot with no floor below holds the player at their current height instead of letting them fall forever. Once there is floor below again, normal falling and landing resume.
- Noclip works while Fox or Krystal is controllable on foot, including in combat stances. It does nothing in the Arwing, on the CloudRunner, during cutscenes, while climbing, while carrying an object, while controls are locked, or when the player is dead.
- In water, walls are still passed through, but fall protection does not apply. It also does not apply while standing on a moving platform.
- Void recovery: scenery outside the play area can look like it has ground below that never catches you. If a fall that began with Noclip on drops well below where it started, with no floor anywhere below, Noclip puts you back where you last stood on solid ground and writes `[Noclip] void recovery: returned to last safe position` to the log. It never returns you to a different map or layer, and it stays out of the way while Fly Mode is moving you up, down or hovering.
- Leaving a fixed-angle camera area through a wall instead of its exit: once that area unloads, the camera returns to the normal camera instead of staying on the old fixed view.
- Noclip is not invincibility. Damage, hazards and instant-death areas behave as usual.

## Compatibility

Works on its own, and together with:

- [Foxhollow Player Cheats](https://github.com/saulob/Foxhollow-Player-Cheats): with Fast Movement on, the player passes through walls at the doubled speed.
- [Foxhollow Fly Mode](https://github.com/saulob/Foxhollow-Fly-Mode): Fly Up, Fly Down and hovering keep working while Noclip is on, and Return to Safe Position works as usual. Fall protection steps aside while Fly Mode controls the height (rising, descending or hovering).

All three mods can be installed and used at the same time, in any load order. Noclip only uses 0 and Numpad 0, so it shares no key with Fly Mode (Home, End, Page Up and Page Down) or Player Cheats (1-5 and Numpad 1-5).

## Installation

**Recommended:** install through the Foxhollow Launcher once the mod is published there.

**Manual:** place the extracted mod folder in the Foxhollow Launcher's `mods` folder, so it looks like this:

```
mods/
  com.saulob.noclip/
    mod.json
    lib/
      windows-amd64/
        mod.dll
      linux-amd64/
        mod.so
      linux-arm64/
        mod.so
      macos-x86_64/
        mod.so
      macos-arm64/
        mod.so
```

Foxhollow only loads the library in the folder that matches your system and ignores the others, so you only need the folder for your platform.

Restart the game after installing.

## Platform support

| Platform | Folder | Status |
| --- | --- | --- |
| Windows x64 | `windows-amd64` | Tested in game |
| Linux x86_64 | `linux-amd64` | Build validation by GitHub Actions pending, in-game testing pending |
| Linux ARM64 | `linux-arm64` | Build validation by GitHub Actions pending, in-game testing pending |
| macOS Apple Silicon | `macos-arm64` | Build validation by GitHub Actions pending, in-game testing pending |
| macOS Intel | `macos-x86_64` | Build validation by GitHub Actions pending, in-game testing pending |

Official Foxhollow builds are currently published for Windows x64, Linux x86_64 and macOS Apple Silicon. The Linux ARM64 and macOS Intel libraries are for Foxhollow builds you compile yourself. Windows on ARM is not supported.

On Linux and macOS, 0 and Numpad 0 are read from the keyboard state of Foxhollow's own SDL3 runtime, so the mod needs no extra libraries (no separate SDL install) and does not use X11, Wayland or macOS keyboard APIs directly. There the keys are recognized by their position on the keyboard, so Numpad 0 works with Num Lock on or off. If the Foxhollow log shows `[Noclip] disabled: ...`, the mod could not find the game functions or keyboard input it needs and left the game unchanged.

## Repository

https://github.com/saulob/Foxhollow-Noclip

## License

[MIT](LICENSE)
