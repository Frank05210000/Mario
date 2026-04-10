#include "Game.hpp"

void Game::Start() {
    m_Map.load();
    m_Player.SetSpawnPosition(m_Map.GetPlayerSpawn());
    UpdateCamera();
    m_Map.render(m_Camera.GetX());
    m_Player.render(m_Camera.GetX());
    BuildScene();
}

void Game::Update() {
    m_Player.update(m_Map);
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
    for (const auto& object : m_Map.GetObjectGameObjects()) {
        m_Renderer.AddChild(object);
    }
    if (const auto playerObject = m_Player.GetGameObject();
        playerObject != nullptr) {
        m_Renderer.AddChild(playerObject);
    }
}

void Game::UpdateCamera() {
    m_Camera.Update(
        m_Player.GetPosition().x,
        m_Map.GetWorldWidth());
}
