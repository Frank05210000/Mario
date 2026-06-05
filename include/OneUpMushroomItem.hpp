#ifndef ONE_UP_MUSHROOM_ITEM_HPP
#define ONE_UP_MUSHROOM_ITEM_HPP

#include "Item.hpp"
#include <string>

class OneUpMushroomItem : public Item {
public:
    OneUpMushroomItem(glm::vec2 pos, const std::string& theme = "ground");

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "OneUp"; }
};

#endif
