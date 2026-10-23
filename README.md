# MGHook

Hook for Metal Gear & Metal Gear 2: Solid Snake (Master Collection) that translates the strings built into the game's executable and replaces the font it draws its UTF-8 text with.

Table of Contents
-----------------
- [Game Info](#game-info)
- [Legal notes](#legal-notes)
- [Features](#features)
- [Build Requirements](#build-requirements)
- [Compiling](#compiling)
- [Installing](#installing)
- [Configuration](#configuration)
- [Usage](#usage)
- [Credits](#credits)

Game Info
---------
![METAL GEAR & METAL GEAR 2: Solid Snake](https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/2131680/library_600x900.jpg "METAL GEAR & METAL GEAR 2: Solid Snake")

|         Type | Value                                                                   |
|-------------:|:------------------------------------------------------------------------|
| Developer(s) | Konami                                                                  |
| Publisher(s) | Konami                                                                  |
|  Director(s) | Hideo Kojima (original games)                                           |
|  Producer(s) | Noriaki Okamura (Master Collection)                                     |
|       Engine | Proprietary                                                             |
|  Platform(s) | Windows, PlayStation 4, PlayStation 5, Xbox Series X/S, Nintendo Switch |
|     Genre(s) | Action-adventure, stealth                                               |
|      Mode(s) | Single-player                                                           |

Legal notes
-----------

- The project contains no original assets from the game.
- To use this project you need to have an original copy of the game (bought from [Steam](https://store.steampowered.com/app/2131680/METAL_GEAR__METAL_GEAR_2_Solid_Snake/)), the project doesn't make piracy easier and doesn't break any of the DRM included in-game.
- The project is not affiliated with or endorsed by Konami.

Features
--------

- Translation of the strings `METAL GEAR.exe` carries (save and load messages, menu labels, difficulty names), including translations longer than the original
- Replacement of the font the engine draws UTF-8 text with, of any size, so that letters the game lacks (such as Polish ones) can be added
- Character maps per string region, for the regions drawn with fonts that only have the game's own characters
- Leaves the game's files unchanged: the executable is patched in memory when the game starts
- Optional log to a console and to a file

Build Requirements
------------------

- Visual Studio 2026

### Libraries (installed by vcpkg):
- zlib
- GoogleTest (unit tests)

Compiling
---------

See the [shared build workflows](https://github.com/GrzybDev/actions/tree/v1/.github/workflows) `cpp-checks.yml` and `cpp-dll.yml`, which CI and the release build run, for more detailed steps

1. [Integrate vcpkg with Visual Studio](https://learn.microsoft.com/vcpkg/commands/integrate)
2. Build the solution `MGHook.slnx` in Visual Studio (x64 Release for the hook)
3. Run `x64\Release\MGHook.Tests.exe` to run the unit tests; set `MGHOOK_GAME_EXE` to the path of `METAL GEAR.exe` to also check what the hook knows about the game against your copy

The hook is `x64\Release\MFReadWrite.dll`.

Installing
----------

1. Copy `MFReadWrite.dll`, either from the latest build in [GitHub Releases](https://github.com/GrzybDev/MGHook/releases) or from `x64\Release` if you compiled it yourself, to the game folder (next to `METAL GEAR.exe`); the game loads it instead of the system's `MFReadWrite.dll`, to which the hook passes the game's calls
2. Put the files described in [Configuration](#configuration) next to it

To uninstall, delete `MFReadWrite.dll`.

Configuration
-------------

All files are optional and are read from the folder of `MFReadWrite.dll`. Text files are UTF-8, with or without a byte order mark.

### MGHook.ini

| Key           | Description                                                                                                   | Allowed values | Default |
|:-------------:|:--------------------------------------------------------------------------------------------------------------|:--------------:|:-------:|
| `Log.Console` | Show the hook's log in a console window                                                                       | 0 or 1         | 0       |
| `Log.File`    | Save the hook's log to `MGHook.log`                                                                           | 0 or 1         | 0       |
| `Font.Export` | When there is no replacement font, save the game's own font to `EngineSupport\Fonts\MGS_Font_nht.raw` to edit | 0 or 1         | 0       |

```ini
[Log]
File=1
```

### translations.txt

Each line maps a string of the game, byte for byte, to its translation: `original=translation`. Every string of the [regions](#string-regions) that reads exactly like the original is translated. Lines starting with `#` or `;` are comments, and a later line with the same original replaces an earlier one.

Nothing is trimmed, since many strings of the game start or end with spaces: everything before the first `=` not written as `\=` is the original and everything after it the translation. Lines with an empty original or translation are skipped and logged. The escapes `\n` (new line), `\r`, `\t`, `\\`, `\=` (an `=` in the original) and `\xHH` (any byte) work on both sides.

```ini
# Save and load messages
No data present on storage device.=Brak danych na nośniku.
Save Successful=Zapisano
DATA LOAD=WCZYTAJ
APPROVED BY =ZATWIERDZIŁ 
```

A translation that fits in the original's place is written there; a longer one is kept elsewhere and every pointer to the original is turned to it (when nothing points at the original, it stays as it is and the log says so). The log lists each translated string and each original it did not find.

### String regions

The hook knows four regions of `METAL GEAR.exe` 2.0.2.0; with any other build it leaves the strings alone and says so in the log.

| Region        | Holds                                                         | Encoding                                                                      |
|:--------------|:--------------------------------------------------------------|:------------------------------------------------------------------------------|
| `difficulty`  | Difficulty names (`VERY EASY` … `EXTREMO EUROPEO`)            | 8-bit text: lowercase letters and some symbols stand for accented capitals    |
| `save_labels` | Save labels (`CLEAR DATA`, `ORIGINAL`, …)                     | The same 8-bit text                                                           |
| `menus`       | Menu labels and screen titles (`DATA LOAD`, `OK`, `CANCEL`, …) | The same 8-bit text                                                           |
| `messages`    | Save, load and storage messages in six languages              | UTF-8, drawn with the font below                                              |

### charmap.txt and charmap_\<region\>.txt

Character maps replace characters a font lacks with codes it has, after the translation is read: `character=replacement`, where the character is a single UTF-8 character and the replacement one or more bytes (escapes as in `translations.txt`). `charmap.txt` applies to every region and `charmap_<region>.txt` (such as `charmap_menus.txt`) to one region, overriding `charmap.txt` for the characters it maps. Each character is replaced once, so a replacement is never mapped again.

```ini
# charmap_menus.txt: draw Polish letters with codes whose glyphs were redrawn in the menu font
Ą=\x5B
Ł=\x5D
ż=z
```

### Font

`EngineSupport\Fonts\MGS_Font_nht.raw` replaces the TNOF font built into `METAL GEAR.exe`, which the engine draws UTF-8 text with (the `messages` region, among others). It may be larger than the original. Set `Font.Export=1` and start the game once to get the original, then edit it with [MGTools](https://github.com/GrzybDev/MGTools) (`mgtools tnof export` and `mgtools tnof import`).

Usage
-----

1. Install the hook as described in [Installing](#installing)
2. Add the translations, character maps and font you need (see [Configuration](#configuration))
3. Start the game from Steam as usual

When the game starts, before any of its code runs, the hook:
1. Points the game's call that unpacks its built-in font at the replacement and tells the game how large it is
2. Writes each translation over its original, or elsewhere with every pointer to the original turned to it
3. Leaves everything else to the game; its Media Foundation calls go on to the system's `MFReadWrite.dll`

Credits
-------

- [GrzybDev](https://grzyb.dev)

Special thanks to:
- Konami (for making the game)
- Hideo Kojima (for creating the Metal Gear series)
