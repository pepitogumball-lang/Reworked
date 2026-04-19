#pragma once

#include "../includes.hpp"

class NoclipSettingsLayer : public geode::Popup {

    bool init(float w, float h, const char* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        if (!Popup::init(w, h, bg, bgRect)) return false;

        setTitle("Noclip Settings");

        auto menu = CCMenu::create();
        menu->setPosition(ccp(0, 0));

        auto addToggle = [&](const char* label, const char* key, float y) {
            auto lbl = CCLabelBMFont::create(label, "bigFont.fnt");
            lbl->setAnchorPoint({0, 0.5f});
            lbl->setScale(0.35f);
            lbl->setPosition(m_mainLayer->getContentSize().width / 2 - 100, y);
            lbl->setOpacity(200);
            m_mainLayer->addChild(lbl);

            auto on  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
            auto off = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
            auto toggle = CCMenuItemToggler::create(off, on, this, menu_selector(NoclipSettingsLayer::onToggle));
            toggle->setPosition(m_mainLayer->getContentSize().width / 2 + 80, y);
            toggle->setScale(0.55f);
            toggle->toggle(Mod::get()->getSavedValue<bool>(key));
            toggle->setID(key);
            menu->addChild(toggle);
        };

        float h2 = m_mainLayer->getContentSize().height;
        addToggle("Player 1 Noclip", "macro_noclip_p1", h2 / 2 + 15);
        addToggle("Player 2 Noclip", "macro_noclip_p2", h2 / 2 - 15);

        m_mainLayer->addChild(menu);
        return true;
    }

    void onToggle(CCObject* obj) {
        auto toggle = static_cast<CCMenuItemToggler*>(obj);
        Mod::get()->setSavedValue<bool>(toggle->getID(), !toggle->isToggled());
    }

public:

    STATIC_CREATE(NoclipSettingsLayer, 260, 130)

    void open(CCObject*) {
        NoclipSettingsLayer::create()->show();
    }
};
