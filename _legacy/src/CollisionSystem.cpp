#include "CollisionSystem.h"

bool CollisionSystem::Intersects(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w &&
           a.x + a.w > b.x &&
           a.y < b.y + b.h &&
           a.y + a.h > b.y;
}

std::vector<const LevelObject*> CollisionSystem::GetSolidObjects(const LevelData& level) {
    std::vector<const LevelObject*> objects;
    objects.reserve(level.objects.size());

    for (const auto& object : level.objects) {
        if (object.solid) {
            objects.push_back(&object);
        }
    }

    return objects;
}

bool CollisionSystem::CheckEntityCollision(const Rect& entityBox, const LevelData& level) {
    for (const auto* object : GetSolidObjects(level)) {
        if (Intersects(entityBox, object->collider)) {
            return true;
        }
    }
    return false;
}
