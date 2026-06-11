#ifndef COIN_ITEM_HPP
#define COIN_ITEM_HPP

#include "Item.hpp"
#include <memory>
#include "ThemeAssets.hpp"

namespace Util {
    class Animation;
}

class CoinItem : public Item {
public:
    CoinItem(glm::vec2 pos, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "Coin"; }

private:
    std::shared_ptr<Util::Animation> m_CoinAnim;
};

#endif
