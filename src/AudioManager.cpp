#include "AudioManager.hpp"

#include "AssetPath.hpp"
#include "Util/Logger.hpp"

#include <algorithm>
#include <filesystem>

namespace {
constexpr int kSfxChannelCount = 32;
constexpr unsigned int kCoinSfxCooldownMs = 35;
constexpr const char* kDebugStarBGMName = "debug_starman_hurry_loop";
}

// ─── 路徑組裝輔助 ────────────────────────────────────────────────────────────

std::string AudioManager::MakeBGMPath(const std::string& name) {
    return MakeAssetPath("audio/bgm/" + name + ".mp3");
}

std::string AudioManager::MakeSFXPath(const std::string& name) {
    return MakeAssetPath("audio/sfx/" + name + ".mp3");
}

int AudioManager::EventPriority(const std::string& name) {
    if (name == "death" || name == "game_over" || name == "level_clear") {
        return 3;
    }
    if (name == "starman" || name == kDebugStarBGMName) {
        return 2;
    }
    return 1;
}

std::string AudioManager::HurryBGMName(const std::string& areaName) {
    if (areaName == "overworld" || areaName == "underground") {
        return areaName + "_hurry";
    }
    return areaName;
}

// ─── 建構子 ──────────────────────────────────────────────────────────────────

AudioManager::AudioManager()
    : m_CurrentBGMName("") {
    Mix_AllocateChannels(kSfxChannelCount);
    // 無需預載任何資源；一切按需載入，缺檔優雅降級
    LOG_INFO("AudioManager initialized.");
}

// ─── BGM 控制 ────────────────────────────────────────────────────────────────

void AudioManager::PlayBGM(const std::string& name, int loop) {
    // 相同曲目已在播放中，不重複開始
    if (name == m_CurrentBGMName && m_CurrentBGM) {
        return;
    }

    const std::string path = MakeBGMPath(name);

    // 缺檔優雅降級：僅 warn，不 crash
    if (!std::filesystem::exists(path)) {
        LOG_WARN("AudioManager: BGM file not found: '{}'. Audio silenced.", path);
        m_CurrentBGM.reset();
        m_CurrentBGMName = name; // 仍然記錄名稱，避免反覆 warn
        return;
    }

    try {
        // 先停止舊 BGM 再建新的（Util::BGM 規格：同時只能播一首）
        m_CurrentBGM.reset();
        m_CurrentBGM     = std::make_unique<Util::BGM>(path);
        m_CurrentBGMName = name;
        m_CurrentBGM->SetVolume(VolumeMixLevel(m_VolumePercent)); // 套用目前主音量
        m_CurrentBGM->Play(loop);
        LOG_INFO("AudioManager: PlayBGM '{}' (loop={})", name, loop);
    } catch (const std::exception& e) {
        LOG_WARN("AudioManager: Failed to play BGM '{}': {}", name, e.what());
        m_CurrentBGM.reset();
    } catch (...) {
        LOG_WARN("AudioManager: Unknown error while playing BGM '{}'.", name);
        m_CurrentBGM.reset();
    }
}

void AudioManager::StopBGM() {
    if (m_CurrentBGM) {
        // FadeOut(0) 等同立即停止（不依賴 SDL_mixer 的全域停止，
        // 避免影響 AssetStore 內部狀態）
        m_CurrentBGM->FadeOut(0);
        m_CurrentBGM.reset();
        LOG_INFO("AudioManager: BGM stopped (was '{}')", m_CurrentBGMName);
    }
    m_CurrentBGMName = "";
}

void AudioManager::PauseBGM() {
    if (m_CurrentBGM) {
        m_CurrentBGM->Pause();
    }
}

void AudioManager::ResumeBGM() {
    if (m_CurrentBGM) {
        m_CurrentBGM->Resume();
    }
}

void AudioManager::SetAreaBGM(const std::string& name) {
    m_AreaBGMName = name;
    RefreshAreaBGM();
}

void AudioManager::SetHurryUp(bool enabled) {
    if (m_HurryUp == enabled) {
        return;
    }
    m_HurryUp = enabled;
    RefreshAreaBGM();
}

void AudioManager::PlayEventBGM(const std::string& name, int loop) {
    const int priority = EventPriority(name);
    if (!m_ActiveEventBGMName.empty() && priority < m_ActiveEventPriority) {
        LOG_INFO("AudioManager: ignored lower-priority event BGM '{}' while '{}' is active.",
                 name, m_ActiveEventBGMName);
        return;
    }

    m_ActiveEventBGMName = name;
    m_ActiveEventPriority = priority;
    PlayBGM(name, loop);
}

void AudioManager::EndEventBGM(const std::string& name) {
    if (m_ActiveEventBGMName != name) {
        return;
    }
    m_ActiveEventBGMName.clear();
    m_ActiveEventPriority = 0;
    RefreshAreaBGM();
}

void AudioManager::ResetBGMState() {
    m_AreaBGMName.clear();
    m_ActiveEventBGMName.clear();
    m_ActiveEventPriority = 0;
    m_HurryUp = false;
    StopBGM();
}

void AudioManager::RefreshAreaBGM() {
    if (!m_ActiveEventBGMName.empty()) {
        return;
    }
    if (m_AreaBGMName.empty()) {
        StopBGM();
        return;
    }

    PlayBGM(m_HurryUp ? HurryBGMName(m_AreaBGMName) : m_AreaBGMName);
}

// ─── SFX 控制 ────────────────────────────────────────────────────────────────

void AudioManager::PlaySFX(const std::string& name, int loop, int durationMs) {
    if (name == "coin") {
        const unsigned int now = SDL_GetTicks();
        const auto last = m_LastSfxPlayTicks.find(name);
        if (last != m_LastSfxPlayTicks.end() && now - last->second < kCoinSfxCooldownMs) {
            return;
        }
        m_LastSfxPlayTicks[name] = now;
    }

    // 先查快取
    auto it = m_SfxCache.find(name);
    if (it != m_SfxCache.end()) {
        // 快取命中：可能是已驗證存在的 SFX，或是已標記為「缺檔」的 nullptr
        if (it->second) {
            it->second->SetVolume(VolumeMixLevel(m_VolumePercent));
            it->second->Play(loop, durationMs);
        }
        // nullptr 表示音檔不存在，直接靜音（不重複 warn）
        return;
    }

    const std::string path = MakeSFXPath(name);

    // 缺檔優雅降級：插入 nullptr 到快取，下次直接靜音
    if (!std::filesystem::exists(path)) {
        LOG_WARN("AudioManager: SFX file not found: '{}'. Audio silenced.", path);
        m_SfxCache[name] = nullptr;
        return;
    }

    try {
        auto sfx = std::make_shared<Util::SFX>(path);
        m_SfxCache[name] = sfx;
        sfx->SetVolume(VolumeMixLevel(m_VolumePercent));
        sfx->Play(loop, durationMs);
        LOG_INFO("AudioManager: PlaySFX '{}' (loop={})", name, loop);
    } catch (const std::exception& e) {
        LOG_WARN("AudioManager: Failed to play SFX '{}': {}", name, e.what());
        m_SfxCache[name] = nullptr; // 避免下次再試
    } catch (...) {
        LOG_WARN("AudioManager: Unknown error while playing SFX '{}'.", name);
        m_SfxCache[name] = nullptr;
    }
}

// ─── 音量控制 ────────────────────────────────────────────────────────────────

void AudioManager::SetVolumePercent(int percent) {
    m_VolumePercent = std::clamp(percent, 0, 100);
    const int mix = VolumeMixLevel(m_VolumePercent);

    // BGM：Mix_VolumeMusic 為全域設定，套到目前曲目即生效並延續到後續曲目
    if (m_CurrentBGM) {
        m_CurrentBGM->SetVolume(mix);
    }
    // SFX：Mix_VolumeChunk 是逐音效設定，更新所有已快取的音效
    for (auto& [name, sfx] : m_SfxCache) {
        if (sfx) {
            sfx->SetVolume(mix);
        }
    }
    LOG_INFO("AudioManager: master volume set to {}%", m_VolumePercent);
}
