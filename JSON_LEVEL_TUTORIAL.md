# JSON Level Tutorial

This project loads playable level data from JSON files in `data/`. The current
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
  "backgroundImage": "level_image/1-1/ground.png",
  "theme": "ground",
  "levelWidth": 640,
  "levelHeight": 240,
  "playerSpawn": {
    "x": 32.0,
    "y": 176.0
  },
  "objects": []
}
```

- `backgroundImage`: image path relative to `Asset/`.
- `theme`: block sprite theme. Supported values currently used by the project are `ground` and `underground`.
- `levelWidth`: total level width in world pixels.
- `levelHeight`: total level height in world pixels.
- `playerSpawn`: Mario's starting top-left position.
- `objects`: list of all gameplay objects in the level.

## Theme

`theme` controls which block sprites are used.

```json
"theme": "ground"
```

`Brick`, `QuestionBlock`, `Wall`, and `Pipe` will use assets such as:

```text
Asset/block/ground/brick/brick.png
Asset/block/ground/question_block/question_block-1.png
Asset/block/ground/pipe/pipe_top/pipe_top.png
```

For underground levels:

```json
"theme": "underground"
```

The same object types use:

```text
Asset/block/underground/brick/brick.png
Asset/block/underground/question_block/question_block-1.png
Asset/block/underground/pipe/pipe_top/pipe_top.png
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
brick.

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
None
```

`PowerUp` spawns a mushroom when Mario is small, and a fire flower when Mario is
already powered up.

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
```

### Pipe

A solid pipe. It can also be enterable and transition to another JSON level.

```json
{
  "type": "Pipe",
  "x": 352.0,
  "y": 176.0,
  "width": 32.0,
  "height": 32.0,
  "opening": "up",
  "enterable": true,
  "targetLevel": "1-1_underground",
  "targetSpawn": {
    "x": 64.0,
    "y": 160.0
  }
}
```

Pipe fields:

- `opening`: `up`, `down`, `left`, or `right`.
- `enterable`: whether the player can use the pipe.
- `targetLevel`: target JSON file name without `.json`.
- `exitToLevel`: alternate target field used by some exit pipes.
- `targetSpawn`: optional player spawn point after transition.

### Flag

The end-of-level flag pole. The JSON position is the top of the flag pole.

```json
{
  "type": "Flag",
  "x": 576.0,
  "y": 32.0
}
```

## Complete Example

See `data/tutorial.json` for a small runnable example that includes every major
object type supported by the current loader.

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
