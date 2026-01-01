# Main Levels Editor

Allows you to control what levels should be in level select layer. 

developing v8 rn/ 

im too lazy for it so here is some ai generated info :3

## Quick Start

### Basic Usage

Open Main Levels Editor menu from bottom-right button in Level Select or pause menu.

### Export Level

Play any level, pause it, click "Export as .level file" button. Choose where to save.

The .level file contains:
- Level data and objects
- Custom songs (mp3/ogg)
- Custom sound effects (mp3/ogg)

### Replace Official Level

Export your level, rename to `1.level` for Stereo Madness, `2.level` for Back On Track, etc. Put in mod config folder. Reload levels cache from mod menu or restart game.

### Add Custom Level

Export level, note its ID. Open mod settings, edit "Levels Listing" setting. Add your ID to the list like `1337,1:22,-2,-1`. Reload cache.

Or use "Insert to Level List" button in mod menu while playing the level.

### Edit Imported Level

Click edit button on level page (only for imported levels). Opens editor with all tools enabled.

Level Settings shows:
- Meta data editor (all level properties)
- Difficulty sprite selector
- Secret Coins replacement tool

Changes save to .level file automatically.

## Level Listing Format

Setting format: `1:22,-2,-1`

- Single ID: `1337`
- Range: `1:22` (levels 1 through 22)
- Reverse: `22:1` (levels 22 down to 1)
- Combined: `1337,1:22,-2,-1`

Special IDs:
- `-1` = Coming Soon page
- `-2` = The Tower

Can override with `LEVELS_LISTING.txt` file in config folder.

## Custom Audio

Place files in config folder:
- `audio/0.mp3` through `audio/21.mp3` for official tracks
- `songs/{id}.mp3` for custom songs
- `sfx/{id}.mp3` for sound effects

Edit track info through "Edit tracks" button in mod menu. Use arrows to switch between IDs. Edit artist info through "Edit artists" button.

Audio listing works same as level listing, edit in settings or use `AUDIO_LISTING.txt` file.

## Secret Coins

Enable "Type and ID hacks for Secret Coins" setting.

In editor (requires Unlisted Objects in Editor mod):
- Secret Coins appear in Create Tab
- Place like normal objects
- Save as Secret Coins

In gameplay:
- Secret Coins work like User Coins
- Collect and save progress normally

Use coin replacement tool in level settings to convert User Coins to Secret Coins.

## Difficulty Sprites

Place `diffIcon_10_btn_001.png` through `diffIcon_99_btn_001.png` in resources folder.

When editing imported level, difficulty selector opens automatically. Use arrows or scroll wheel to change. Saves immediately.

Works with More Difficulties and Demons in Between mods.

## Create Packs

### Texture Pack

Click "In resource pack (TP)" in mod menu. Creates pack with all imported levels and current settings. Opens Texture Loader automatically (desktop only). Pack saved to `geode.texture-loader/packs/` folder.

### Standalone Mod

Click "In modified .geode package". Creates new mod file with all levels included. Saved to mods folder. Disable original mod before using.

## Settings Override

Place `settings.json` in config folder to force settings. Useful for packs. Original settings backed up automatically. Delete file to restore.

## Important Settings

**Remove control UI**: Hide all mod buttons

**Verify level integrity**: Disable to allow modified level strings

**Replace difficulty sprite**: Enable custom difficulty icons

**Type and ID hacks for Secret Coins**: Enable Secret Coins in editor

**The Data Driven Achievements**: External achievement config via `achievements.json`

## File Locations

Config folder contains:
- `levels/` - Level files
- `audio.json` - Track metadata
- `artists.json` - Artist info
- `settings.json` - Settings override (optional)
- `achievements.json` - Custom achievements (optional)

Level files checked in order:
1. `levels/{id}.level`
2. `levels/{id}.json`
3. `{id}.level`
4. `{id}.json`

## Tips

Click "Reload levels cache" after adding new files.

Use .level format to include audio. Use .json for metadata only.

Edit button only works on imported levels. Export first if missing.

Delete button removes level from listing and deletes file.

Page position remembered after playing level.

Export from pause menu includes all audio files automatically.

Meta data editor shows errors inline. Fix JSON syntax if red text appears.

Click file paths in export dialog to open folders (desktop only).

## Mobile Notes

ZIP operations may fail on mobile. Warning shown for affected features.

File browser limited on mobile platforms.

Some buttons disabled if features unavailable.

## Achievement System

Enable "The Data Driven Achievements" setting. Creates `achievements.json` on first run with all vanilla achievements.

Edit JSON to customize. Use `achievements-sort.txt` to reorder. Click reload button in achievements menu to apply changes.

## Compatibility

Works with:
- Texture Loader (pack creation)
- Unlisted Objects in Editor (Secret Coins)
- More Difficulties (custom sprites)
- Demons in Between (demon tier sprites)