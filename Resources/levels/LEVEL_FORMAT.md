# Level JSON Format

This project uses a JSON-based level format so map data can be loaded without
hardcoding background paths, spawn points, or collision values in C++.

## Design Goals

- Keep the coordinate system explicit: top-left origin, `x` right, `y` down.
- Use pre-scaled background assets so runtime code does not need map scaling.
- Support a minimal playable level now.
- Leave stable extension points for enemies, objects, and richer collision.

## Required Top-Level Fields

```json
{
  "formatVersion": 1,
  "levelId": "1-1",
  "coordinateSystem": {
    "origin": "top-left",
    "xAxis": "right",
    "yAxis": "down"
  },
  "background": {},
  "player": {},
  "collision": {},
  "enemies": [],
  "objects": []
}
```

## Background

```json
"background": {
  "image": "image/stage/1-1.png",
  "assetSize": {
    "width": 6752,
    "height": 960
  },
  "viewport": {
    "x": 0,
    "y": 0,
    "width": 6752,
    "height": 480
  },
  "worldSize": {
    "width": 6752,
    "height": 480
  }
}
```

Field meaning:

- `image`: asset path relative to `Mario/Asset/`
- `assetSize`: actual file size of the already scaled background image
- `viewport`: crop rectangle taken directly from the already scaled image
- `worldSize`: final world-space size used by gameplay and camera

## Player

```json
"player": {
  "spawn": {
    "x": 240,
    "y": 353.92
  },
  "size": {
    "width": 32,
    "height": 64
  },
  "groundY": 417.92
}
```

Field meaning:

- `spawn`: player top-left world position
- `size`: player sprite size in world coordinates
- `groundY`: default floor line used by the current simple jump/landing logic

## Collision

Current minimal format supports a floor line and future solid rectangles.

```json
"collision": {
  "ground": {
    "type": "line",
    "y": 417.92
  },
  "rects": [],
  "segments": []
}
```

Field meaning:

- `ground`: single horizontal ground line for the current prototype
- `rects`: future AABB solid blocks/platforms/pipes
- `segments`: future line-based or slope-style collision data

Suggested rectangle shape:

```json
{
  "id": "pipe-01",
  "type": "solid",
  "x": 512,
  "y": 320,
  "width": 64,
  "height": 96
}
```

## Enemies

```json
"enemies": [
  {
    "id": "goomba-01",
    "type": "goomba",
    "spawn": { "x": 900, "y": 386 }
  }
]
```

## Objects

```json
"objects": [
  {
    "id": "question-block-01",
    "type": "question-block",
    "position": { "x": 640, "y": 256 },
    "size": { "width": 32, "height": 32 }
  }
]
```

## Stability Rules

- Do not repurpose existing field names with different meanings.
- Add new optional fields instead of changing the meaning of old ones.
- Bump `formatVersion` only when a loader can no longer safely read older data.
