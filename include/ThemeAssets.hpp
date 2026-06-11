#ifndef THEME_ASSETS_HPP
#define THEME_ASSETS_HPP

#include "AssetPath.hpp"
#include <string>

enum class Theme { Ground, Underground };

/*
 * ThemeAssets：主題圖片解析器
 *
 * 封裝「目前關卡主題」，提供統一的 Sprite() 方法解析圖片路徑。
 * 呼叫端使用 {theme} 佔位符，不需要知道實際的資料夾名稱。
 *
 * 範例：
 *   assets.Sprite("enemy/Goomba/{theme}/normal/walk/walk-1.png")
 *   → MakeAssetPath("enemy/Goomba/underground/normal/walk/walk-1.png")
 */
class ThemeAssets {
public:
    ThemeAssets() = default;
    explicit ThemeAssets(Theme t)
        : m_Seg(t == Theme::Underground ? "underground" : "ground") {}

    // 將路徑樣板中的 {theme} 替換為主題段落，並回傳完整資產路徑。
    std::string Sprite(const std::string& tmpl) const {
        static const std::string kPlaceholder = "{theme}";
        const auto pos = tmpl.find(kPlaceholder);
        if (pos == std::string::npos)
            return MakeAssetPath(tmpl);
        return MakeAssetPath(
            tmpl.substr(0, pos) + m_Seg + tmpl.substr(pos + kPlaceholder.size()));
    }

    const std::string& Segment() const { return m_Seg; }

private:
    std::string m_Seg = "ground";
};

#endif
