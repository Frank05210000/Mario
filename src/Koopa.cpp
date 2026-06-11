#include "Koopa.hpp"

#include <filesystem>
#include <vector>

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"
#include "Util/Logger.hpp"

Koopa::Koopa(float startX, float startY, Variant variant, const std::string& theme) : m_Variant(variant) {
    m_Position = {startX, startY - TILE_SIZE};
    m_Size     = {TILE_SIZE, TILE_SIZE * 2.0f};  // 世界尺寸：16x32
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    LoadSprites(theme);
    UpdateDrawable();
}

void Koopa::LoadSprites(const std::string& theme) {
    // ─── 根據 Variant 決定用哪組路徑 ──────────────────────────────────
    // Red Koopa：嘗試載入 ground/red/ 路徑（若不存在則 fallback 綠色）
    std::string walkDir  = "enemy/Koopa/" + theme + "/normal";
    std::string walkDirR = "enemy/Koopa/" + theme + "/reverse";

    if (m_Variant == Variant::Red) {
        const std::string redWalkPath =
            MakeAssetPath("enemy/Koopa/" + theme + "/red/walk/walk-1.png");
        if (std::filesystem::exists(redWalkPath)) {
            walkDir  = "enemy/Koopa/" + theme + "/red";
            walkDirR = "enemy/Koopa/" + theme + "/red_reverse";
            LOG_INFO("Koopa: loaded Red variant sprites from '{}'", redWalkPath);
        } else {
            LOG_WARN("Koopa: Red variant sprites not found ({}), using Green fallback.",
                     redWalkPath);
        }
    }

    m_WalkLeftAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(walkDir + "/walk/walk-1.png"),
            MakeAssetPath(walkDir + "/walk/walk-2.png"),
        },
        true,
        160,
        true);
    m_WalkRightAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(walkDirR + "/walk/walk-1.png"),
            MakeAssetPath(walkDirR + "/walk/walk-2.png"),
        },
        true,
        160,
        true);
    m_ShellImage = std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Koopa/" + theme + "/normal/shell/shell.png"));

    // 翻轉殼圖片（死亡動畫用）；ground 目錄有 shell_flip.png，underground 沒有則 fallback
    const std::string flipPath = MakeAssetPath("enemy/Koopa/" + theme + "/normal/shell/shell_flip.png");
    if (std::filesystem::exists(flipPath)) {
        m_ShellFlipImage = std::make_shared<Util::Image>(flipPath);
    } else {
        LOG_WARN("Koopa: shell_flip.png not found for theme '{}', using shell as fallback.", theme);
        m_ShellFlipImage = m_ShellImage; // fallback
    }
}

void Koopa::Update(float deltaTime) {
    // ─── 翻轉死亡飛出：只套用重力 + 位移，飛出畫面後停止 ────────────
    if (m_IsDying) {
        SetDrawable(m_ShellFlipImage);
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        m_Position.y += m_Velocity.y * deltaTime;
        // 已死亡（IsAlive == false），Enemy::Draw 會隱藏它；
        // 這裡只需繼續物理讓它飛出畫面即可（GameManager 虛空判定會移除）
        return;
    }

    if (m_InShell && !m_IsSliding) {
        // 縮殼停止狀態：只套用重力，不走路
        SetDrawable(m_ShellImage);
        ApplyGravity(deltaTime);

        // 喚醒計時器
        m_WakeUpTimer += deltaTime;
        if (m_WakeUpTimer >= WAKE_UP_DELAY) {
            // 還原為正常走路狀態
            m_InShell     = false;
            m_WakeUpTimer = 0.0f;
            // 恢復原尺寸（若曾壓縮過）
            if (m_Size.y < TILE_SIZE * 2.0f) {
                m_Position.y -= (TILE_SIZE * 2.0f - m_Size.y);
                m_Size.y = TILE_SIZE * 2.0f;
            }
            m_Velocity.x = -m_WalkSpeed;   // 重新往左走
            UpdateDrawable();
            LOG_INFO("Koopa shell woke up and started walking.");
        }
        return;
    }

    if (m_IsSliding) {
        // 殼滑行狀態：套用重力 + 水平快速移動（由 Enemy 的碰撞系統控制掉頭）
        SetDrawable(m_ShellImage);
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        return;
    }

    // 正常狀態：水平移動 + 重力（委派 Enemy::Update）
    UpdateDrawable();
    Enemy::Update(deltaTime);
}

void Koopa::Stomp() {
    // 龜：縮進殼裡，停止走路
    m_InShell     = true;
    m_IsSliding   = false;
    m_WakeUpTimer = 0.0f;
    m_Velocity.x  = 0.0f;

    // 換成殼的圖片
    SetDrawable(m_ShellImage);

    // 龜殼狀態身高變為 1 格高，保持腳底位置不變
    if (m_Size.y > TILE_SIZE) {
        m_Position.y += (m_Size.y - TILE_SIZE);
        m_Size.y = TILE_SIZE;
    }
}

void Koopa::Kick(bool kickLeft) {
    // 縮殼被踢出：朝踢出方向橫向高速滑行
    // 原版 NES 殼速度 ≈ 10 tiles/s × 16 × 3 = 480，取略保守 380
    m_IsSliding   = true;
    m_WakeUpTimer = 0.0f;   // 重置喚醒計時（滑行中不應喚醒）
    const float SHELL_SPEED = 380.0f;
    m_Velocity.x = kickLeft ? -SHELL_SPEED : SHELL_SPEED;
}

void Koopa::Die(bool flipLeft) {
    // 翻轉死亡：標記 Alive=false（停止碰撞），但仍用 IsDying 繼續物理更新
    m_IsDying    = true;
    m_InShell    = false;
    m_IsSliding  = false;
    SetAlive(false); // 不再參與碰撞

    // 給一個向上的初速 + 水平小速度（模擬 NES 翻轉飛出）
    m_Velocity.y = -280.0f;
    m_Velocity.x = flipLeft ? -60.0f : 60.0f;

    SetDrawable(m_ShellFlipImage);
    SetVisible(true); // 確保仍可見
}

void Koopa::Draw(const Camera& camera) {
    if (m_IsDying) {
        // 翻轉死亡：即使 IsAlive == false 也保持可見
        SetVisible(true);
        glm::vec2 centerPos = {
            m_Position.x + m_Size.x * 0.5f,
            m_Position.y + m_Size.y * 0.5f
        };
        m_Transform.translation = camera.WorldToScreen(centerPos);
        return;
    }
    Enemy::Draw(camera);
}

void Koopa::UpdateDrawable() {
    if (m_InShell || m_IsSliding) {
        SetDrawable(m_ShellImage);
        return;
    }

    if (m_Velocity.x > 0.0f) {
        SetDrawable(m_WalkRightAnim);
    } else {
        SetDrawable(m_WalkLeftAnim);
    }
}
