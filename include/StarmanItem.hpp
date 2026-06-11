#ifndef STARMAN_ITEM_HPP
#define STARMAN_ITEM_HPP

#include <memory>
#include "ThemeAssets.hpp"

#include "Item.hpp"

namespace Util {
    class Animation;
}

class StarmanItem : public Item {
public:
    StarmanItem(glm::vec2 pos, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "Star"; }

private:
    std::shared_ptr<Util::Animation> m_Animation;
};

#endif
