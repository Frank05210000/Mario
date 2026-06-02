#ifndef LEVEL_COIN_ITEM_HPP
#define LEVEL_COIN_ITEM_HPP

#include <memory>

#include "Item.hpp"

namespace Util {
    class Animation;
}

class LevelCoinItem : public Item {
public:
    LevelCoinItem(glm::vec2 pos);

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "LevelCoin"; }

private:
    std::shared_ptr<Util::Animation> m_CoinAnim;
};

#endif
