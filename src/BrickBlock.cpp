#include "BrickBlock.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

#include "GameConstants.hpp"

BrickBlock::BrickBlock(glm::vec2 position, const std::string& theme)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Theme(theme) {
    SetSprite("block/" + m_Theme + "/brick/brick.png");
}

BlockHitResult BrickBlock::OnHit(Player* /*player*/) {
    BlockHitResult result;

    if (m_ItemType != "None" && !m_IsUsed) {
        // 如果磚塊裡有藏金幣或是道具，行為就相當於問號方塊
        m_IsUsed = true;
        SetSprite("block/" + m_Theme + "/used_block/used_block.png");
        LOG_INFO("BrickBlock Hit! Spawning hidden item: {}", m_ItemType);
        result.spawnItem = m_ItemType;
        return result;
    }

    if (m_IsUsed) return result; // 已經空了

    // 一般磚塊頂碎邏輯：
    // TODO: 之後你可以加上 player->IsSuper() 來分別處理：
    // 大瑪利歐回傳 isDestroyed = true，小瑪利歐只觸發彈跳。
    // 目前先假設一律頂碎！
    
    LOG_INFO("BrickBlock Hit! Destroyed!");
    result.isDestroyed = true;
    return result;
}
