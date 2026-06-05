#ifndef FIRE_FLOWER_ITEM_HPP
#define FIRE_FLOWER_ITEM_HPP

#include "Item.hpp"
#include "Util/Animation.hpp"
#include <memory>
#include <string>

class FireFlowerItem : public Item {
public:
    FireFlowerItem(glm::vec2 pos, const std::string& theme = "ground");

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "FireFlower"; }

private:
    std::shared_ptr<Util::Animation> m_Animation;
};

#endif
