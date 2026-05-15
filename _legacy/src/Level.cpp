#include "Level.hpp"

#include <fstream>
#include <string>

void Level::LoadDemoLevel() {
    Clear();

    for (int column = 0; column < 32; ++column) {
        m_Blocks.emplace_back(Block::Type::GROUND, Vec2 {column * 16.0F, 320.0F}, Vec2 {16.0F, 16.0F});
    }

    m_Blocks.emplace_back(Block::Type::QUESTION, Vec2 {160.0F, 224.0F}, Vec2 {16.0F, 16.0F});
    m_Blocks.emplace_back(Block::Type::BRICK, Vec2 {176.0F, 224.0F}, Vec2 {16.0F, 16.0F});
    m_Blocks.emplace_back(Block::Type::PIPE, Vec2 {320.0F, 272.0F}, Vec2 {32.0F, 48.0F});
    m_Blocks.emplace_back(Block::Type::FLAG, Vec2 {480.0F, 160.0F}, Vec2 {16.0F, 160.0F});

    m_Enemies.emplace_back(Enemy::Type::GOOMBA, Vec2 {240.0F, 304.0F});
    m_Enemies.emplace_back(Enemy::Type::KOOPA, Vec2 {400.0F, 304.0F});
}

bool Level::LoadFromFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    Clear();
    std::string line;
    int row = 0;
    while (std::getline(input, line)) {
        for (int column = 0; column < static_cast<int>(line.size()); ++column) {
            AddTile(line[static_cast<std::size_t>(column)], column, row);
        }
        ++row;
    }
    return true;
}

void Level::Update(float deltaTime) {
    for (auto& enemy : m_Enemies) {
        if (enemy.IsActive()) {
            enemy.Update(deltaTime);
        }
    }
}

void Level::Clear() {
    m_Blocks.clear();
    m_Enemies.clear();
    m_PlayerSpawn = {64.0F, 240.0F};
}

void Level::AddTile(char symbol, int column, int row) {
    const auto x = static_cast<float>(column) * kTileSize;
    const auto y = static_cast<float>(row) * kTileSize;

    switch (symbol) {
        case 'G':
            m_Blocks.emplace_back(Block::Type::GROUND, Vec2 {x, y}, Vec2 {kTileSize, kTileSize});
            break;
        case 'B':
            m_Blocks.emplace_back(Block::Type::BRICK, Vec2 {x, y}, Vec2 {kTileSize, kTileSize});
            break;
        case 'Q':
            m_Blocks.emplace_back(Block::Type::QUESTION, Vec2 {x, y}, Vec2 {kTileSize, kTileSize});
            break;
        case 'P':
            m_Blocks.emplace_back(Block::Type::PIPE, Vec2 {x, y - (kTileSize * 2.0F)}, Vec2 {kTileSize * 2.0F, kTileSize * 3.0F});
            break;
        case 'F':
            m_Blocks.emplace_back(Block::Type::FLAG, Vec2 {x, y - (kTileSize * 9.0F)}, Vec2 {kTileSize, kTileSize * 10.0F});
            break;
        case 'E':
            m_Enemies.emplace_back(Enemy::Type::GOOMBA, Vec2 {x, y});
            break;
        case 'K':
            m_Enemies.emplace_back(Enemy::Type::KOOPA, Vec2 {x, y});
            break;
        case 'S':
            m_PlayerSpawn = {x, y};
            break;
        default:
            break;
    }
}
