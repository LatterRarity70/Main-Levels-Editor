# Main Levels Editor

Allows you to control what levels should be in level select layer. 

developing v8 rn/ 

### Temporary guide

#### <cr>at first i recommend you to read each setting info.</c>

Exporting and sharing levels: 
- "Export as .level file" button added to level pause layer. 
- By using this button you can create .level file of level you currently played and paused. 

The .level file is a ZIP archive of:
- _data.json
- Songs and SFXs files (mp3/ogg)

To replace level:
- export your level in .level file
- rename it to `{123273888}.level` (like 1.level)
- put it to mod config folder (or any other search paths of game)
- reload game

To add as new one:
- export level
- copy its id (f.e. 1337)
- add id to level listing setting (123,273,888,1-22,-2,-1)
- reload game
