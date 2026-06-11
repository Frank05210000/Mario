#ifndef MUSHROOM_ITEM_HPP
#define MUSHROOM_ITEM_HPP

#include "Item.hpp"
#include "ThemeAssets.hpp"

class MushroomItem : public Item {
public:
    MushroomItem(glm::vec2 pos, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "Mushroom"; }
};

#endif
