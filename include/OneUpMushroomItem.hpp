#ifndef ONE_UP_MUSHROOM_ITEM_HPP
#define ONE_UP_MUSHROOM_ITEM_HPP

#include "Item.hpp"
#include "ThemeAssets.hpp"

class OneUpMushroomItem : public Item {
public:
    OneUpMushroomItem(glm::vec2 pos, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "OneUp"; }
};

#endif
