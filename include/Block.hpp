#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <memory>
#include <string>

#include "Util/GameObject.hpp"

class Camera; // 前置宣告
class Player;

struct BlockHitResult {
    bool isDestroyed = false;
    std::string spawnItem = "None";
};


class Block : public Util::GameObject {
public:
    enum class Type {
        Ground,
        Brick,
        Question,
        Pipe,
        Flag,
        Wall,
        MovingPlatform,
        TreePlatform,
        Hidden,
        MultiCoin,
        UsedOnHitBrick,
    };

    Block(glm::vec2 position, glm::vec2 size);
    virtual ~Block() = default;

    virtual void Update(float deltaTime);
    virtual void Draw(const Camera& camera);

    virtual Type GetType() const = 0;
    
    glm::vec2 GetPosition() const { return m_Position; }
    glm::vec2 GetSize() const { return m_Size; }

    // 與物理與互動有關的方法
    virtual bool IsSolid() const = 0;

    /* 單向平台（semisolid）：只有「從上方落下」才站得住；
     * 從下方跳穿、側面接觸一律穿過（不觸發 OnHit、不側推）。
     */
    virtual bool IsOneWay() const { return false; }

    virtual BlockHitResult OnHit(Player*) { return BlockHitResult(); }

    /* 從下方頂到時，是否會「彈動」並波及正上方的敵人與道具
     * （仿原版：只有磚塊、問號磚這類會反應的磚才會頂死上方敵人、彈飛道具）。
     * 純地形（地面、水管、樹台、牆…）回傳 false：頂到只是擋住，不殺敵也不彈道具。
     */
    virtual bool BumpsContentsAbove() const { return false; }

    // 道具夾帶相關
    void SetItemType(const std::string& itemType) { m_ItemType = itemType; }
    std::string GetItemType() const { return m_ItemType; }

protected:
    void SetSprite(const std::string& assetPath, float zIndex = 1.0f);

    glm::vec2 m_Position = {0.0f, 0.0f};  // 在世界座標的 x,y //0.0f是預設值，避免亂數
    glm::vec2 m_Size     = {0.0f, 0.0f};  // 寬度與高度 (預設為 16x16)
    std::string m_ItemType = "None";      // 磚塊夾帶的道具類型
    bool m_IsUsed = false;                // 是否已經被敲出內容物
};

#endif
