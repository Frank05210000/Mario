#include "Game.hpp"

void Game::Start() {
    m_Map.load();
    UpdateCamera();
    m_Map.render(m_Camera.GetX());
    m_Player.render(m_Camera.GetX());
    BuildScene();
}

void Game::Update() {
    m_Player.update();
    UpdateCamera();
    m_Player.ClampToCameraBounds(m_Camera.GetX());
    m_Map.render(m_Camera.GetX());
    m_Player.render(m_Camera.GetX());
    m_Renderer.Update();
}

void Game::BuildScene() {
    m_Renderer = Util::Renderer();
    if (const auto backgroundObject = m_Map.GetBackgroundObject();
        backgroundObject != nullptr) {
        m_Renderer.AddChild(backgroundObject);
    }
    if (const auto playerObject = m_Player.GetGameObject();
        playerObject != nullptr) {
        m_Renderer.AddChild(playerObject);
    }
}

void Game::UpdateCamera() {
    m_Camera.Update(
        m_Player.GetPosition().x,
        Map::kWorldSize.x);
}
