#pragma once

#include "../includes.hpp"

struct RenderPreset {
    const char* name;
    int width;
    int height;
    int fps;
    int bitrate;
    const char* codec;
};

static const std::vector<RenderPreset> renderPresets = {
    { "480p  30fps",  854,  480, 30,  4, "libx264" },
    { "720p  60fps", 1280,  720, 60,  8, "libx264" },
    { "1080p 60fps", 1920, 1080, 60, 12, "libx264" },
    { "1440p 60fps", 2560, 1440, 60, 20, "libx264" },
    { "4K    60fps", 3840, 2160, 60, 40, "libx264" },
};

class RenderPresetsLayer : public geode::Popup {

    bool init(float w, float h, const char* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        if (!Popup::init(w, h, bg, bgRect)) return false;
        setTitle("Render Presets");

        float mw = m_mainLayer->getContentSize().width;
        float mh = m_mainLayer->getContentSize().height;

        auto menu = CCMenu::create();
        menu->setPosition(ccp(0, 0));

        float startY = mh - 52;
        float rowH   = 26;

        for (size_t i = 0; i < renderPresets.size(); i++) {
            const auto& preset = renderPresets[i];

            auto spr = CCSprite::createWithSpriteFrameName("GJ_longBtn_001.png");
            if (!spr) spr = CCSprite::create();
            spr->setScaleX(1.5f);
            spr->setScaleY(0.7f);

            auto lbl = CCLabelBMFont::create(preset.name, "bigFont.fnt");
            lbl->setScale(0.38f);
            lbl->setAnchorPoint({0.5f, 0.5f});
            lbl->setPosition(spr->getContentSize() / 2);
            spr->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RenderPresetsLayer::onPreset));
            btn->setPosition(mw / 2, startY - rowH * i);
            btn->setTag(static_cast<int>(i));
            menu->addChild(btn);
        }

        m_mainLayer->addChild(menu);
        return true;
    }

    void onPreset(CCObject* obj) {
        int idx = static_cast<CCNode*>(obj)->getTag();
        if (idx < 0 || idx >= static_cast<int>(renderPresets.size())) return;

        const auto& p = renderPresets[idx];
        auto mod = Mod::get();

        mod->setSavedValue("render_width2",  std::to_string(p.width));
        mod->setSavedValue("render_height",  std::to_string(p.height));
        mod->setSavedValue("render_fps",     std::to_string(p.fps));
        mod->setSavedValue("render_bitrate", std::to_string(p.bitrate));
        mod->setSavedValue("render_codec",   std::string(p.codec));

        FLAlertLayer::create("Preset Applied",
            fmt::format("Applied preset: <cy>{}</c>", p.name).c_str(),
            "OK")->show();

        keyBackClicked();
    }

public:

    STATIC_CREATE(RenderPresetsLayer, 300, 195)
};
