#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "Util/BGM.hpp"
#include "Util/SFX.hpp"

/*
 * AudioManager：音效與音樂管理器
 *
 * 由 GameManager 持有（成員物件）。
 * 提供 PlayBGM / StopBGM / PlaySFX 等介面，
 * 內部用 map 快取已載入的 SFX 物件避免重複建構。
 *
 * 檔案不存在時：LOG_WARN + 靜音，絕不 crash。
 *
 * BGM 檔名約定（放在 Resources/Asset/audio/bgm/）：
 *   overworld.mp3、underground.mp3、starman.mp3、
 *   overworld_hurry.mp3、underground_hurry.mp3、
 *   level_clear.mp3、death.mp3、game_over.mp3
 *
 * SFX 檔名約定（放在 Resources/Asset/audio/sfx/）：
 *   jump_small.mp3、jump_super.mp3、stomp.mp3、coin.mp3、
 *   powerup.mp3、powerdown.mp3、powerup_appears.mp3、brick_break.mp3、bump.mp3、
 *   kick.mp3、fireball.mp3、pipe.mp3、1up.mp3、flagpole.mp3、
 *   countdown_tick.mp3
 */
class AudioManager {
public:
    AudioManager();

    // ─── BGM 控制 ─────────────────────────────────────────────────
    // name：不含副檔名與路徑，例如 "overworld"
    // loop：-1 無限循環（預設），0 = 播一次
    void PlayBGM(const std::string& name, int loop = -1);
    void StopBGM();
    // 暫停 / 恢復（暫停時不切換 BGM）
    void PauseBGM();
    void ResumeBGM();

    // ─── 狀態式 BGM 控制 ─────────────────────────────────────────
    // Area BGM 是關卡底層音樂；event BGM 會依優先序暫時覆蓋它。
    void SetAreaBGM(const std::string& name);
    void SetHurryUp(bool enabled);
    void PlayEventBGM(const std::string& name, int loop = -1);
    void EndEventBGM(const std::string& name);
    void ResetBGMState();

    // ─── SFX 控制 ─────────────────────────────────────────────────
    // name：不含副檔名與路徑，例如 "jump_small"
    // loop：0 播一次（預設），-1 無限
    // durationMs：-1 播完整音檔；正數會在指定毫秒後停止該次播放
    void PlaySFX(const std::string& name, int loop = 0, int durationMs = -1);

    // ─── 查詢 ──────────────────────────────────────────────────────
    // 目前正在播放哪個 BGM（name，空字串代表沒在播）
    const std::string& CurrentBGM() const { return m_CurrentBGMName; }

private:
    // 根據 name 組出 BGM 的完整路徑
    static std::string MakeBGMPath(const std::string& name);
    // 根據 name 組出 SFX 的完整路徑
    static std::string MakeSFXPath(const std::string& name);
    static int EventPriority(const std::string& name);
    static std::string HurryBGMName(const std::string& areaName);
    void RefreshAreaBGM();

    // 目前正在播放的 BGM 物件（nullptr = 沒有或音檔不存在）
    std::unique_ptr<Util::BGM> m_CurrentBGM;
    std::string                m_CurrentBGMName;   // 當前 BGM name，用來偵測重複呼叫
    std::string                m_AreaBGMName;      // 關卡底層 BGM
    std::string                m_ActiveEventBGMName;
    int                        m_ActiveEventPriority = 0;
    bool                       m_HurryUp = false;

    // SFX 快取：避免每次呼叫 PlaySFX 都重新建構 Util::SFX
    std::unordered_map<std::string, std::shared_ptr<Util::SFX>> m_SfxCache;
};

#endif // AUDIO_MANAGER_HPP
