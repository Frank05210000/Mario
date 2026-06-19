# JSON Level Tutorial

This project loads playable level data from JSON files in `Resources/data/`. The current
runtime format is a compact object list: the background is one image, while
collision, enemies, blocks, pipes, and the flag are described as JSON objects.

## Coordinate System

- The world origin is the top-left corner: `(0, 0)`.
- `x` increases to the right.
- `y` increases downward.
- `TILE_SIZE` is `16`, so most positions are multiples of `16`.
- `GAME_SCALE` is `3`, so one 16x16 world tile is drawn as 48x48 screen pixels.

Example:

```json
{
  "x": 128.0,
  "y": 144.0
}
```

This means the object's top-left corner is 128 pixels from the left edge and
144 pixels from the top edge in world coordinates.

## Top-Level Fields

```json
{
  "backgroundImage": "level_image/1-1/1-1_ground_1.png",
  "theme": "ground",
  "levelWidth": 640,
  "levelHeight": 240,
  "playerSpawn": {
    "x": 32.0,
    "y": 176.0
  },
  "checkpoints": [
    {
      "x": 2000.0,
      "y": 176.0
    }
  ],
  "introCutscene": {
    "type": "autoEnterPipe",
    "pipeTargetLevel": "1-2_underground_1",
    "walkSpeed": 60.0,
    "pipeEntryDuration": 1.0
  },
  "objects": []
}
```

- `backgroundImage`: image path relative to `Resources/Asset/`.
- `theme`: block sprite theme. Supported values currently used by the project are `ground` and `underground`.
- `levelWidth`: total level width in world pixels.
- `levelHeight`: total level height in world pixels.
- `playerSpawn`: Mario's starting top-left position.
- `checkpoints`: optional array of midpoint positions. When Mario's `x` position
  passes a checkpoint's `x`, that checkpoint is stored as the respawn location.
  On death the game uses the latest reached checkpoint instead of `playerSpawn`.
  Checkpoints are cleared when changing levels or starting a new game. The `y`
  value should match the normal ground spawn height so Mario appears on solid
  ground after respawning. If `checkpoints` is omitted the level behaves as
  before (always respawn at `playerSpawn`).
- `introCutscene`: optional opening sequence. The only supported `type` is
  `autoEnterPipe`, which locks player input, walks Mario right to the
  `EnterablePipe` whose destination matches `pipeTargetLevel`, plays the pipe
  entry animation, and changes to that target level. Omit this field for normal
  playable starts.
- `objects`: list of all gameplay objects in the level.

## Theme

`theme` controls which block sprites are used.

```json
"theme": "ground"
```

`Brick`, `QuestionBlock`, and `Wall` use theme assets. Pipe objects are
invisible because pipe art is expected to be part of the background image.
Example block assets:

```text
Resources/Asset/block/ground/brick/brick.png
Resources/Asset/block/ground/question_block/question_block-1.png
Resources/Asset/block/ground/pipe/pipe_top/pipe_top.png
```

For underground levels:

```json
"theme": "underground"
```

The same object types use:

```text
Resources/Asset/block/underground/brick/brick.png
Resources/Asset/block/underground/question_block/question_block-1.png
Resources/Asset/block/underground/pipe/pipe_top/pipe_top.png
```

## Object Types

### Ground

Invisible solid collision. Use this for floor platforms that match the
background image.

```json
{
  "type": "Ground",
  "x": 0.0,
  "y": 208.0,
  "width": 640.0,
  "height": 32.0
}
```

### Wall

A visible solid 16x16 block. Useful for stairs, barriers, and terrain details.

```json
{
  "type": "Wall",
  "x": 480.0,
  "y": 192.0
}
```

### Brick

A visible 16x16 brick block. If `itemType` is omitted, it behaves as a normal
breakable brick. Small Mario only bounces it; powered-up Mario breaks it.

```json
{
  "type": "Brick",
  "x": 192.0,
  "y": 144.0
}
```

Optional hidden item:

```json
{
  "type": "Brick",
  "x": 208.0,
  "y": 144.0,
  "itemType": "Coin"
}
```

### UsedOnHitBrickBlock

A visible 16x16 brick block that does not disappear when hit. On the first hit
from below, it bounces and turns into a used block. If `itemType` is set, it also
spawns that item.

```json
{
  "type": "UsedOnHitBrickBlock",
  "x": 224.0,
  "y": 144.0,
  "itemType": "None"
}
```

### QuestionBlock

A 16x16 question block. If `itemType` is omitted, it defaults to `Coin`.

```json
{
  "type": "QuestionBlock",
  "x": 128.0,
  "y": 144.0,
  "itemType": "Coin"
}
```

Common item types used by the current game code:

```text
Coin
PowerUp
Mushroom
FireFlower
OneUp
Star
None
```

`PowerUp` spawns a mushroom when Mario is small, and a fire flower when Mario is
already powered up.

### HiddenBlock

An invisible solid 16x16 block. It is revealed when Mario hits it from below,
then spawns its `itemType`.

```json
{
  "type": "HiddenBlock",
  "x": 1024.0,
  "y": 144.0,
  "itemType": "OneUp"
}
```

Useful item types include `Coin`, `PowerUp`, `OneUp`, and `Star`.

### HiddenQuestionBlock

An invisible 16x16 question-style block. The player cannot see it before hit,
but hitting it from below reveals a used block and spawns its `itemType`.

```json
{
  "type": "HiddenQuestionBlock",
  "x": 1024.0,
  "y": 128.0,
  "itemType": "PowerUp"
}
```

### MultiCoinBlock

A solid 16x16 block that can spawn several coins before becoming a used block.
If `coinCount` is omitted, it defaults to 10.

```json
{
  "type": "MultiCoinBlock",
  "x": 512.0,
  "y": 144.0,
  "coinCount": 10
}
```

### Coin / CollectibleCoin

A placed level coin. It starts active in the level and is collected by player
contact, unlike `CoinItem`, which is the short animation spawned from blocks.

```json
{
  "type": "Coin",
  "x": 640.0,
  "y": 112.0
}
```

### EnemySpawn

An enemy spawn point. The enemy is queued first, then actually created when the
camera gets close enough.

```json
{
  "type": "EnemySpawn",
  "x": 256.0,
  "y": 192.0,
  "enemyType": "Goomba"
}
```

Supported enemy types:

```text
Goomba
Koopa
KoopaParatroopa
PiranhaPlant
```

`PiranhaPlant` is useful for pipe areas. It has its own movement and does not
use normal block collision.

Koopa fields:

- `variant`: `green` or `red`. Red Koopas turn around at platform edges.
- `flightMode`: for `KoopaParatroopa`, `hop` or `verticalPatrol`.

Runtime behavior follows SMB1 rules used by this project:

- Green Koopas walk off platform edges; red Koopas turn around while walking.
- `hop` Paratroopas repeatedly jump after landing.
- `verticalPatrol` Paratroopas stay at their spawn X and move vertically.
- The first stomp removes a Paratroopa's wings. A later stomp puts it in its
  shell.
- A sliding shell stops when stomped. A stationary shell can be kicked from
  the side and warns visually shortly before reviving.

```json
{
  "type": "EnemySpawn",
  "x": 768.0,
  "y": 160.0,
  "enemyType": "KoopaParatroopa",
  "variant": "red",
  "flightMode": "hop"
}
```

### EnterablePipe

An invisible solid pipe trigger. The pipe art is expected to be part of the
background image; this object only provides collision and level transition
logic.

```json
{
  "type": "EnterablePipe",
  "x": 352.0,
  "y": 176.0,
  "segments": 2,
  "opening": "up",
  "targetLevel": "1-1_underground"
}
```

EnterablePipe fields:

- `opening`: `up`, `down`, `left`, or `right`.
- `segments`: pipe length in 16 px tiles. Vertical pipes are always 2 tiles wide; horizontal pipes are always 2 tiles tall.
- `targetLevel`: target JSON file name without `.json`.
- `exitToLevel`: alternate target field used by some exit pipes.
- `targetSpawn`: legacy optional player spawn override. The editor no longer emits this field; each scene should define exactly one top-level `playerSpawn`, and pipes should use the target scene's spawn.

### PipeCollision

An invisible solid collision box used for background pipe art that is not
enterable. Adjacent 16x16 cells can be merged by the editor into a larger box.

```json
{
  "type": "PipeCollision",
  "x": 704.0,
  "y": 176.0,
  "width": 32.0,
  "height": 48.0
}
```

### Editor-only: PiranhaPipePreset

`tilemap_editor.py` may save `PiranhaPipePreset` in project JSON as a design
shortcut for an upward pipe with a piranha plant. It is not a runtime game JSON
type. Object JSON export expands it into one `PipeCollision` object and one
`EnemySpawn` with `enemyType: "PiranhaPlant"`.

### Flag

The end-of-level flag pole. The JSON position is the bottom of the flag pole;
the game computes the pole height from `FLAG_POLE_TILES`.

```json
{
  "type": "Flag",
  "x": 576.0,
  "y": 192.0,
  "clearWalkTiles": 6.0,
  "castleFlagBaseTiles": 4.0
}
```

- `clearWalkTiles` is optional and defaults to `6.0`. It controls the tile
  distance from the flag pole X to the castle door center. Mario slides down at
  `poleX + 0.5 tile`, so the actual walking distance is this value minus half a
  tile.
- `castleFlagBaseTiles` is optional and defaults to `4.0`. It controls the
  castle flag base height above the flag-pole ground. Use larger values for
  tall castle backgrounds such as 1-3.

### MovingPlatform

A visible solid platform that moves and carries the player when stood on.

```json
{
  "type": "MovingPlatform",
  "x": 528.0,
  "y": 152.0,
  "segments": 3,
  "moveAxis": "horizontal",
  "moveMode": "oscillate",
  "startDirection": "right",
  "moveTiles": 4,
  "moveSpeed": 35.0
}
```

MovingPlatform fields:

- `segments`: platform length in 16 px tiles.
- `moveTiles`: movement distance in 16 px tiles.
- `moveAxis`: `horizontal` or `vertical`.
- `moveMode`: `oscillate` or `verticalWrap`.
- `startDirection`: `right`/`left` for horizontal movement, `down`/`up` for vertical movement.
- `moveSpeed`: speed in pixels per second.

### TreePlatform

A 1-3 style tree-top platform. `segments` controls how many 16px pieces are
used across the top.

```json
{
  "type": "TreePlatform",
  "x": 432.0,
  "y": 128.0,
  "segments": 4
}
```

## Complete Example

See the playable files in `Resources/data/` for complete examples of the supported
object types and level transitions.

## Loading Flow

1. `GameManager` calls `LevelLoader::Load(path)`.
2. `LevelLoader` reads the JSON into `LevelData`.
3. `GameManager::LoadLevel()` creates the background image.
4. Each object in `objects` is converted into a C++ game object.
5. `EnemySpawn` objects are stored in a spawn queue until the camera approaches.
6. `playerSpawn` sets Mario's starting position.

The current system is best understood as:

```text
one full background image + JSON gameplay/collision objects
```

It is not a traditional tile-id map renderer.
