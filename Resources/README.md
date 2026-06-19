# Resources

Runtime data is centralized here.

- `data/`: playable level JSON loaded by `GameManager`.
- `Asset/`: images, fonts, sounds, and other assets loaded through `MakeAssetPath()`.

Level JSON `backgroundImage` values are relative to `Resources/Asset/`, for example:

```json
"backgroundImage": "level_image/1-1/1-1_ground_1.png"
```
