#ifndef GAME_HPP
#define GAME_HPP

#include "Camera.hpp"
#include "Map.hpp"
#include "Player.hpp"
#include "Util/Renderer.hpp"

class Game {
public:
    void Start();
    void Update();

private:
    void BuildScene();
    void UpdateCamera();

    Camera m_Camera;
    Map m_Map;
    Player m_Player;
    Util::Renderer m_Renderer;
};

#endif
