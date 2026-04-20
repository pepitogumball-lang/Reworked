#pragma once

#include "../includes.hpp"
#include "../macro.hpp"

class SaveMacroLayer : public geode::Popup, public TextInputDelegate {

    TextInput* authorInput = nullptr;
    TextInput* descInput   = nullptr;
    TextInput* pathInput   = nullptr;

    bool init(float w, float h, const char* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        if (!Popup::init(w, h, bg, bgRect)) return false;
        setTitle("Save Macro");

        float mw = m_mainLayer->getContentSize().width;
        float mh = m_mainLayer->getContentSize().height;

        auto addLabel = [&](const char* text, float y) {
            auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setScale(0.32f);
            lbl->setOpacity(200);
            lbl->setPosition(mw / 2 - 130, y);
            m_mainLayer->addChild(lbl);
        };

        auto& g = Global::get();
        std::filesystem::path macrosFolder = g.mod->getSettingValue<std::filesystem::path>("macros_folder");

        std::string levelBase = "macro";
        if (PlayLayer* pl = PlayLayer::get()) {
            if (pl->m_level && !pl->m_level->m_levelName.empty()) {
                levelBase = pl->m_level->m_levelName;
                static const std::string invalid = "/\\:*?\"<>|";
                for (char& c : levelBase)
                    if (invalid.find(c) != std::string::npos) c = '_';
            }
        }
        std::string defaultPath = (macrosFolder / levelBase).string();

        addLabel("Author:", mh - 55);
        authorInput = TextInput::create(180, "Author");
        authorInput->setPosition(mw / 2 + 30, mh - 55);
        authorInput->setString(GJAccountManager::sharedState() ? GJAccountManager::sharedState()->m_username : "");
        authorInput->setDelegate(this);
        m_mainLayer->addChild(authorInput);

        addLabel("Description:", mh - 85);
        descInput = TextInput::create(180, "Description");
        descInput->setPosition(mw / 2 + 30, mh - 85);
        descInput->setDelegate(this);
        m_mainLayer->addChild(descInput);

        addLabel("Path:", mh - 115);
        pathInput = TextInput::create(180, "Path");
        pathInput->setPosition(mw / 2 + 30, mh - 115);
        pathInput->setString(defaultPath);
        pathInput->setDelegate(this);
        m_mainLayer->addChild(pathInput);

        auto menu = CCMenu::create();
        menu->setPosition(ccp(0, 0));

        auto saveLabel = CCLabelBMFont::create("Save", "goldFont.fnt");
        saveLabel->setScale(0.7f);
        auto saveBtn = CCMenuItemSpriteExtra::create(saveLabel, this, menu_selector(SaveMacroLayer::onSave));
        saveBtn->setPosition(mw / 2, mh - 148);
        menu->addChild(saveBtn);

        m_mainLayer->addChild(menu);
        return true;
    }

    void onSave(CCObject*) {
        std::string author = authorInput ? authorInput->getString() : "";
        std::string desc   = descInput   ? descInput->getString()   : "";
        std::string path   = pathInput   ? pathInput->getString()   : "";

        if (path.empty()) {
            FLAlertLayer::create("Error", "Please enter a file path.", "OK")->show();
            return;
        }

        int result = Macro::save(author, desc, path);
        if (result == 0) {
            FLAlertLayer::create("Saved", "Macro saved successfully!", "OK")->show();
            keyBackClicked();
        } else {
            FLAlertLayer::create("Error", ("Failed to save macro. Code: " + std::to_string(result)).c_str(), "OK")->show();
        }
    }

    void textChanged(CCTextInputNode*) override {}

public:

    STATIC_CREATE(SaveMacroLayer, 320, 190)

    static void open() {
        SaveMacroLayer::create()->show();
    }
};
