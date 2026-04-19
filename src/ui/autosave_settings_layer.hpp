#pragma once

#include "../includes.hpp"

class AutoSaveLayer : public geode::Popup, public TextInputDelegate {

    TextInput* intervalInput = nullptr;

    bool init(float w, float h, const char* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        if (!Popup::init(w, h, bg, bgRect)) return false;
        setTitle("Autosave Settings");

        float mw = m_mainLayer->getContentSize().width;
        float mh = m_mainLayer->getContentSize().height;

        auto menu = CCMenu::create();
        menu->setPosition(ccp(0, 0));

        auto addLabel = [&](const char* text, float x, float y, float scale = 0.32f) {
            auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setScale(scale);
            lbl->setOpacity(200);
            lbl->setPosition(x, y);
            m_mainLayer->addChild(lbl);
        };

        auto addToggle = [&](const char* label, const char* key, float y) {
            addLabel(label, mw / 2 - 110, y);
            auto on  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
            auto off = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
            auto toggle = CCMenuItemToggler::create(off, on, this, menu_selector(AutoSaveLayer::onToggle));
            toggle->setPosition(mw / 2 + 70, y);
            toggle->setScale(0.55f);
            toggle->toggle(Mod::get()->getSavedValue<bool>(key));
            toggle->setID(key);
            menu->addChild(toggle);
        };

        addToggle("Interval Autosave:", "autosave_interval_enabled", mh - 50);
        addToggle("Checkpoint Autosave:", "autosave_checkpoint_enabled", mh - 72);
        addToggle("Level End Autosave:", "autosave_levelend_enabled", mh - 94);

        addLabel("Interval (min):", mw / 2 - 110, mh - 116);
        intervalInput = TextInput::create(70, "10");
        intervalInput->setPosition(mw / 2 + 50, mh - 116);
        intervalInput->setFilter("0123456789");
        intervalInput->setString(Mod::get()->getSavedValue<std::string>("autosave_interval"));
        intervalInput->setDelegate(this);
        intervalInput->setID("autosave_interval");
        m_mainLayer->addChild(intervalInput);

        m_mainLayer->addChild(menu);
        return true;
    }

    void onToggle(CCObject* obj) {
        auto toggle = static_cast<CCMenuItemToggler*>(obj);
        bool val = !toggle->isToggled();
        Mod::get()->setSavedValue<bool>(toggle->getID(), val);
        auto& g = Global::get();
        if (std::string(toggle->getID()) == "autosave_interval_enabled")
            g.autosaveIntervalEnabled = val;
    }

    void textChanged(CCTextInputNode* input) override {
        std::string val = input->getString();
        Mod::get()->setSavedValue(std::string(input->getID()), val);
        auto result = geode::utils::numFromString<float>(val);
        if (result) Global::get().autosaveInterval = static_cast<int>(result.unwrap() * 60);
    }

public:

    STATIC_CREATE(AutoSaveLayer, 280, 165)

    void open(CCObject*) {
        AutoSaveLayer::create()->show();
    }
};
