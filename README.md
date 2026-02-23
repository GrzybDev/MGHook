# MGHook

Hook for Metal Gear & Metal Gear 2: Solid Snake (Master Collection) that enables runtime string translation with character substitution support.

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
|       Engine | Proprietary                                                             |
|  Platform(s) | Windows, PlayStation 4, PlayStation 5, Xbox Series X/S, Nintendo Switch |
|     Genre(s) | Action-adventure, stealth                                               |
|      Mode(s) | Single-player                                                           |

Legal notes
-----------

- The project doesn't contain ***any*** original assets from the game!
- To use this project you need to have an original copy of the game (bought from [Steam](https://store.steampowered.com/app/2131680/METAL_GEAR__METAL_GEAR_2_Solid_Snake/), the project doesn't make piracy easier and doesn't break any of the DRM included in-game.

Features
--------

- Runtime UI/string translation via external translation file
- Custom character substitution map for languages with non-standard characters
- DLL proxy hook — no permanent modifications to game files
- Debug console and file logging support

Build Requirements
------------------

- Visual Studio 2022

Compiling
---------

1. Open `MGHook.slnx` in Visual Studio
2. Build project in Visual Studio (x64 Debug or Release)

The output DLL will be named `MFReadWrite.dll`.

Installing
----------

1. Copy `MFReadWrite.dll` from either the latest build in [GitHub Releases](https://github.com/GrzybDev/MGHook/releases) or from the build output folder if you compiled it yourself
2. Place `MFReadWrite.dll` in the game directory (next to the game executable) — the hook will proxy calls to the real system `MFReadWrite.dll` automatically
3. Place `translations.txt` and (optionally) `charmap.txt` in the same directory as `MFReadWrite.dll`

Configuration
-------------

MGHook uses two text files for configuration, both placed alongside the hook DLL:

### translations.txt

A simple key-value file that defines string replacements. Each line maps an original in-game string to its translation, separated by `=`. Lines starting with `;` or `#` are treated as comments. UTF-8 encoding is supported (with optional BOM).

Example:

```ini
# UI translations
CONTINUE=KONTYNUUJ
NEW GAME=NOWA GRA
SAVE=ZAPISZ
LOAD=WCZYTAJ
OPTIONS=OPCJE
```

### charmap.txt

An optional character substitution map for remapping individual characters. This is useful for languages whose characters are not present in the game's built-in font (UTF-8 encoding). Each line maps one UTF-8 source character to one UTF-8 replacement character, separated by `=`. Lines starting with `;` or `#` are treated as comments.

Example:

```ini
# Polish character substitutions
ą=a
ć=c
ę=e
ł=l
ń=n
ó=o
ś=s
ź=z
ż=z
```

### Debug logging

To enable debug logging, create one or both of the following empty files next to the game executable:

| File          | Description                                     |
|:-------------:|:-----------------------------------------------:|
| `.debug`      | Opens a console window showing hook log output  |
| `.debug_log`  | Saves hook log to `mghook.log` next to the DLL  |

Usage
-----

1. Install the hook as described in [Installing](#installing)
2. Create a `translations.txt` file with your translations (see [Configuration](#configuration))
3. Optionally create a `charmap.txt` file if your language requires character remapping
4. Launch the game — MGHook will automatically load translations and patch strings in memory at startup

On startup the hook will:
1. Load the real system `MFReadWrite.dll` and proxy all Media Foundation calls
2. Load `charmap.txt` (if present) for character substitution
3. Load `translations.txt` and scan game memory for matching strings
4. Patch matched strings in-place and redirect pointer tables to translated versions

If a translation is longer than the original string's memory slot, it will be stored in a separately allocated buffer and all pointer references will be redirected.

Credits
-------

- [GrzybDev](https://grzyb.dev)

Special thanks to:
- Konami (for making the game)
- Hideo Kojima (for creating the Metal Gear series)
