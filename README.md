# Foxhollow Noclip

A standalone native mod for Star Fox Adventures running through Foxhollow. It lets the player walk through walls and other collision during normal on-foot gameplay, while still stopping at the edge of the map and keeping the player from falling forever.

It is based on the already-tested Noclip from the Foxhollow `feature/cheat-menu-noclip` branch, controlled directly by F12.

## Controls

| Key | Action |
| --- | --- |
| F12 | Toggle Noclip |

- F12 acts once per press. Holding it does not repeat.
- A key that is already held when the game window regains focus has to be released and pressed again.
- F12 only works during gameplay while the game window is focused.
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

All three mods can be installed and used at the same time, in any load order.

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
```

Restart the game after installing.

## Platform support

- Windows x64

Other Foxhollow platforms are not supported by this mod yet.

## Repository

https://github.com/saulob/Foxhollow-Noclip

## License

[MIT](LICENSE)
