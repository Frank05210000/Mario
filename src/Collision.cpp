#include "Collision.hpp"

Rect Collision::MakeEntityBounds(const Entity& entity, Vec2 size) {
    const auto position = entity.GetPosition();
    return Rect {position.x, position.y, size.x, size.y};
}

Rect Collision::MakeBlockBounds(const Block& block) {
    const auto position = block.GetPosition();
    const auto size = block.GetSize();
    return Rect {position.x, position.y, size.x, size.y};
}

bool Collision::Intersects(const Rect& a, const Rect& b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}
