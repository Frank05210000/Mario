#ifndef MUSHROOM_ITEM_HPP
#define MUSHROOM_ITEM_HPP

#include "Item.hpp"
#include <string>

class MushroomItem : public Item {
public:
    MushroomItem(glm::vec2 pos, const std::string& theme = "ground");

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "Mushroom"; }
};

#endif
