#ifndef HIDDEN_BLOCK_HPP
#define HIDDEN_BLOCK_HPP

#include <string>

#include "Block.hpp"

class HiddenBlock : public Block {
public:
    HiddenBlock(glm::vec2 position, const std::string& theme);

    Type GetType() const override { return Type::Hidden; }

    // 隱形方塊只有在被揭露後才是實體（會阻擋移動）。
    // 揭露前，玩家可以穿過它走路/站立，但往上跳的碰撞
    // 仍會在 CheckBlockCollision Pass 1 特別處理，讓頂碰觸發 OnHit。
    bool IsSolid() const override { return m_IsUsed; }

    // 查詢方塊是否已被揭露（=已被從下方頂過一次）
    bool IsRevealed() const { return m_IsUsed; }

    BlockHitResult OnHit(Player* player) override;

private:
    std::string m_Theme = "ground";
};

#endif
