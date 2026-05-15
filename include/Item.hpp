#ifndef ITEM_HPP
#define ITEM_HPP

#include "Util/GameObject.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"

class Player;

enum class ItemState {
    Spawning, // 正在自方塊中升起
    Active,   // 自由活動中 (走路、受重力)
    Collected // 已被收集或該消滅
};

class Item : public Util::GameObject {
public:
    Item(glm::vec2 spawnPosition) {
        m_Position = spawnPosition;
        m_SpawnBaseY = spawnPosition.y; // 記下剛產生的原點 Y
    }

    virtual ~Item() = default;

    // ----- 必須實作的邏輯 -----
    virtual void Update(float deltaTime) = 0;
    
    // 渲染置中邏輯 (同 Block, Player)
    virtual void Draw(const Camera& camera);

    // 當馬力歐觸碰時發生
    virtual void OnCollect(Player* player) = 0;

    // ----- Getter/Setter -----
    ItemState GetState() const { return m_State; }
    virtual std::string GetType() const = 0;
    glm::vec2 GetPosition() const { return m_Position; }
    void SetPosition(glm::vec2 pos) { m_Position = pos; }
    glm::vec2 GetSize() const     { return m_Size; }
    glm::vec2 GetVelocity() const { return m_Velocity; }
    void SetVelocity(glm::vec2 vel) { m_Velocity = vel; }

protected:
    void ApplyGravity(float deltaTime);

    glm::vec2 m_Position = {0.0f, 0.0f};
    glm::vec2 m_Velocity = {0.0f, 0.0f};
    glm::vec2 m_Size     = {TILE_SIZE, TILE_SIZE};

    ItemState m_State = ItemState::Spawning;
    float m_SpawnBaseY = 0.0f;
    float m_Gravity = 900.0f;
};

#endif
