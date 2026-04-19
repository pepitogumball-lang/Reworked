#pragma once

#include "../includes.hpp"
#include "../macro.hpp"

class MacroInfoLayer : public geode::Popup {

    bool init(float w, float h, const char* bg = "GJ_square01.png", cocos2d::CCRect bgRect = {}) {
        if (!Popup::init(w, h, bg, bgRect)) return false;
        setTitle("Macro Info");

        auto& g   = Global::get();
        auto& macro = g.macro;

        float mw = m_mainLayer->getContentSize().width;
        float mh = m_mainLayer->getContentSize().height;

        auto addRow = [&](const char* key, std::string val, float y) {
            auto keyLbl = CCLabelBMFont::create(key, "bigFont.fnt");
            keyLbl->setAnchorPoint({0.f, 0.5f});
            keyLbl->setScale(0.3f);
            keyLbl->setOpacity(160);
            keyLbl->setPosition(mw / 2 - 140, y);
            m_mainLayer->addChild(keyLbl);

            auto valLbl = CCLabelBMFont::create(val.c_str(), "bigFont.fnt");
            valLbl->setAnchorPoint({1.f, 0.5f});
            valLbl->setScale(0.3f);
            valLbl->setOpacity(230);
            valLbl->setPosition(mw / 2 + 140, y);
            m_mainLayer->addChild(valLbl);
        };

        int    actionCount = static_cast<int>(macro.inputs.size());
        std::string levelName  = macro.levelInfo.name.empty() ? "N/A" : macro.levelInfo.name;
        std::string author     = macro.author.empty()         ? "N/A" : macro.author;
        std::string desc       = macro.description.empty()    ? "N/A" : macro.description;
        std::string framerate  = fmt::format("{:.1f}", macro.framerate);
        std::string duration   = macro.inputs.empty() ? "N/A" :
            fmt::format("{:.2f}s", macro.inputs.back().frame / macro.framerate);
        std::string ffCount    = std::to_string(static_cast<int>(macro.frameFixes.size()));

        float startY = mh - 48;
        float rowH   = 20;

        addRow("Level:",       levelName,                    startY);
        addRow("Author:",      author,                       startY - rowH);
        addRow("Description:", desc,                         startY - rowH * 2);
        addRow("Actions:",     std::to_string(actionCount),  startY - rowH * 3);
        addRow("Frame Fixes:", ffCount,                      startY - rowH * 4);
        addRow("Framerate:",   framerate,                    startY - rowH * 5);
        addRow("Duration:",    duration,                     startY - rowH * 6);

        return true;
    }

public:

    STATIC_CREATE(MacroInfoLayer, 340, 220)
};
