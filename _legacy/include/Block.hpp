#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "Entity.hpp"

class Block {
public:
    enum class Type {
        GROUND,
        BRICK,
        QUESTION,
        PIPE,
        FLAG,
    };

    Block(Type type, Vec2 position, Vec2 size);

    Type GetType() const { return m_Type; }
    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetSize() const { return m_Size; }
    bool IsSolid() const;

private:
    Type m_Type;
    Vec2 m_Position {};
    Vec2 m_Size {};
};

#endif
