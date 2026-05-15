#ifndef FIRE_FLOWER_ITEM_HPP
#define FIRE_FLOWER_ITEM_HPP

#include "Item.hpp"
#include "Util/Animation.hpp"
#include <memory>

class FireFlowerItem : public Item {
public:
    FireFlowerItem(glm::vec2 pos);

    void Update(float deltaTime) override;
    void OnCollect(Player* player) override;
    std::string GetType() const override { return "PowerUp"; }

private:
    std::shared_ptr<Util::Animation> m_Animation;
};

#endif
