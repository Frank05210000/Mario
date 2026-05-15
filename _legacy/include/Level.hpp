#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <string>
#include <vector>

#include "Block.hpp"
#include "Enemy.hpp"

class Level {
public:
    static constexpr float kTileSize = 16.0F;

    void LoadDemoLevel();
    bool LoadFromFile(const std::string& path);
    void Update(float deltaTime);

    const std::vector<Block>& GetBlocks() const { return m_Blocks; }
    const std::vector<Enemy>& GetEnemies() const { return m_Enemies; }
    Vec2 GetPlayerSpawn() const { return m_PlayerSpawn; }

private:
    void Clear();
    void AddTile(char symbol, int column, int row);

    std::vector<Block> m_Blocks;
    std::vector<Enemy> m_Enemies;
    Vec2 m_PlayerSpawn {64.0F, 240.0F};
};

#endif
