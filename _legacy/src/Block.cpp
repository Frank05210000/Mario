#include "Block.hpp"

Block::Block(Type type, Vec2 position, Vec2 size)
    : m_Type(type), m_Position(position), m_Size(size) {}

bool Block::IsSolid() const {
    return m_Type != Type::FLAG;
}
