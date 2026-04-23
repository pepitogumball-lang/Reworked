#include "includes.hpp"
#include "ui/record_layer.hpp"
#include "ui/game_ui.hpp"

#include <Geode/modify/PlayLayer.hpp>

void Macro::recordAction(int frame, int button, bool player2, bool hold) {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    auto& g = Global::get();

    if (g.macro.inputs.empty())
        Macro::updateInfo(pl);

    if (g.tpsEnabled) g.macro.framerate = g.tps;

    if (Macro::flipControls())
      player2 = !player2;

    float speed = g.speedhack;
    if (speed <= 0.01f) speed = 1.f;

    // FIX: Si shRawFrameAtChange quedo mayor que el frame actual (estado corrupto),
    // lo reseteamos para evitar frames negativos o absurdamente grandes.
    if (g.shRawFrameAtChange > frame) {
        g.shRawFrameAtChange = frame;
        g.shOffset = static_cast<float>(frame);
    }

    int normalizedFrame = static_cast<int>(std::round(
        g.shOffset + static_cast<float>(frame - g.shRawFrameAtChange) / speed
    ));
    if (normalizedFrame < 0) normalizedFrame = 0;

    // Sub-frame ordering: if multiple inputs land on the same frame
    // (common on Android/tablet with fast taps where press+release arrive
    // within the same physics frame), assign an incrementing subFrameIndex
    // so playback can reproduce them in their original order instead of
    // collapsing them into a single simultaneous event.
    uint8_t sfx = 0;
    if (!g.macro.inputs.empty()) {
        const auto& last = g.macro.inputs.back();
        if (static_cast<int>(last.frame) == normalizedFrame) {
            sfx = static_cast<uint8_t>(
                std::min(static_cast<int>(last.subFrameIndex) + 1, 255)
            );
        }
    }

    g.macro.inputs.push_back(input(normalizedFrame, button, player2, hold, sfx));
}

void Macro::recordFrameFix(int frame, PlayerObject* p1, PlayerObject* p2) {
    float p1Rotation = p1->getRotation();
    float p2Rotation = p2 ? p2->getRotation() : 0.f;

    while (p1Rotation < 0 || p1Rotation > 360)
      p1Rotation += p1Rotation < 0 ? 360.f : -360.f;
    
    while (p2Rotation < 0 || p2Rotation > 360)
      p2Rotation += p2Rotation < 0 ? 360.f : -360.f;

    cocos2d::CCPoint p2Pos = p2 ? p2->getPosition() : ccp(0.f, 0.f);

    Global::get().macro.frameFixes.push_back({
      frame,
      { p1->getPosition(), p1Rotation },
      { p2Pos, p2Rotation }
    });
}

bool Macro::flipControls() {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return GameManager::get()->getGameVariable("0010");

    return pl->m_levelSettings->m_platformerMode ? false : GameManager::get()->getGameVariable("0010");
}

static std::string sanitizeName(std::string name) {
    static const std::string invalid = "/\\:*?\"<>|";
    for (char& c : name)
        if (invalid.find(c) != std::string::npos) c = '_';
    return name;
}

void Macro::autoSave(GJGameLevel* level, int number) {
    if (!level) level = PlayLayer::get() != nullptr ? PlayLayer::get()->m_level : nullptr;
    if (!level) return;

    std::string levelname = sanitizeName(level->m_levelName);
    std::filesystem::path autoSavesPath = Mod::get()->getSettingValue<std::filesystem::path>("autosaves_folder");

    std::error_code ec;
    std::filesystem::create_directories(autoSavesPath, ec);
    if (ec) {
        log::warn("autoSave: could not create folder {}: {}", autoSavesPath.string(), ec.message());
        return;
    }

    std::filesystem::path path = autoSavesPath / fmt::format("autosave_{}_{}", levelname, number);

    std::string username = GJAccountManager::sharedState() != nullptr ? GJAccountManager::sharedState()->m_username : "";
    int result = Macro::save(username, fmt::format("AutoSave {} in level {}", number, levelname), path.string());

    if (result != 0)
        log::debug("Failed to autosave macro. ID: {}. Path: {}", result, path);
}

void Macro::tryAutosave(GJGameLevel* level, CheckpointObject* cp) {
    auto& g = Global::get();

    if (g.state != state::recording) return;
    if (!g.autosaveEnabled) return;
    if (!g.checkpoints.contains(cp)) return;
    if (g.checkpoints[cp].frame < g.lastAutoSaveFrame) return;

    std::filesystem::path autoSavesPath = g.mod->getSettingValue<std::filesystem::path>("autosaves_folder");

    {
        std::error_code ec;
        std::filesystem::create_directories(autoSavesPath, ec);
        if (ec) return log::debug("tryAutosave: cannot create autosaves folder: {}", ec.message());
    }

    std::string levelname = sanitizeName(level->m_levelName);
    std::filesystem::path path = autoSavesPath / fmt::format("autosave_{}_{}", levelname, g.currentSession);
    std::error_code ec;
    std::filesystem::remove(path.string() + ".gdr", ec);
    if (ec) log::warn("Failed to remove previous autosave");

    autoSave(level, g.currentSession);

}

void Macro::updateInfo(PlayLayer* pl) {
    if (!pl) return;

    auto& g = Global::get();

    GJGameLevel* level = pl->m_level;
    if (level->m_lowDetailModeToggled != g.macro.ldm)
        g.macro.ldm = level->m_lowDetailModeToggled;

    int id = level->m_levelID.value();
    if (id != g.macro.levelInfo.id)
        g.macro.levelInfo.id = id;

    std::string name = level->m_levelName;
    if (name != g.macro.levelInfo.name)
        g.macro.levelInfo.name = name;

    std::string author = (GJAccountManager::sharedState() != nullptr)
        ? GJAccountManager::sharedState()->m_username : "";
    if (g.macro.author != author)
        g.macro.author = author;

    if (g.macro.author == "")
        g.macro.author = "N/A";

    g.macro.botInfo.name = "Reworked";
    g.macro.botInfo.version = reworkedVersion;
    g.macro.xdBotMacro = true;
}

void Macro::updateTPS() {
    auto& g = Global::get();

    g.tpsEnabled = false;
    g.tps = 240.f;

    g.mod->setSavedValue("macro_tps", g.tps);
    g.mod->setSavedValue("macro_tps_enabled", g.tpsEnabled);

    if (g.layer) static_cast<RecordLayer*>(g.layer)->updateTPS();
}

void Macro::normalizeImportedFrames(Macro& macro) {
    float fr = macro.framerate;

    if (fr > 0.f && std::abs(fr - 240.f) > 0.5f) {
        float scale = 240.f / fr;

        for (auto& inp : macro.inputs)
            inp.frame = static_cast<int>(std::round(inp.frame * scale));

        for (auto& fix : macro.frameFixes)
            fix.frame = static_cast<int>(std::round(fix.frame * scale));

        macro.framerate = 240.f;

        log::info("normalizeImportedFrames: rescaled {} inputs from {}fps to 240fps (scale={:.4f})",
                  macro.inputs.size(), fr, scale);
    }
}

bool Macro::isBroken(const Macro& macro) {
    const auto& inputs = macro.inputs;
    if (inputs.size() < 4) return false;

    int zeroGaps = 0;
    for (size_t i = 1; i < inputs.size(); i++) {
        if (inputs[i].frame <= inputs[i - 1].frame)
            zeroGaps++;
    }

    return zeroGaps > static_cast<int>(inputs.size()) / 4;
}

void Macro::buildFrameMap() {
    auto& g = Global::get();
    g.frameMap.clear();
    g.frameMap.reserve(g.macro.inputs.size());

    // FIX: Filtrar frames absurdamente grandes (mas de 10 minutos a 240fps = 144000 frames).
    // Estos frames corruptos causaban que el juego hiciera speedrun hasta ese frame al reproducir.
    constexpr int MAX_REASONABLE_FRAME = 240 * 60 * 10; // 144000

    for (const auto& inp : g.macro.inputs) {
        if (static_cast<int>(inp.frame) > MAX_REASONABLE_FRAME) {
            log::warn("buildFrameMap: skipping corrupt frame {} (exceeds max {})", inp.frame, MAX_REASONABLE_FRAME);
            continue;
        }
        g.frameMap[static_cast<int>(inp.frame)].push_back(inp);
    }

    // Sort each frame's inputs by subFrameIndex so multi-input frames (e.g. fast taps
    // on Android where press+release arrive in the same physics frame) replay in the
    // correct original order.
    for (auto& [frame, inputs] : g.frameMap) {
        if (inputs.size() > 1) {
            std::stable_sort(inputs.begin(), inputs.end(),
                [](const input& a, const input& b) {
                    return a.subFrameIndex < b.subFrameIndex;
                });
        }
    }
}

void Macro::fixInputs() {
    auto& inputs = Global::get().macro.inputs;

    if (inputs.empty()) return;

    if (!inputs[0].down) {
        inputs.insert(inputs.begin(), input(0, inputs[0].button, inputs[0].player2, true));
    }
}

int Macro::saveJson(std::string author, std::string desc, std::string path) {
    auto& g = Global::get();

    if (g.macro.inputs.empty()) return 31;

    // Build rw_macro::MacroFile from current inputs
    rw_macro::MacroFile mf;
    mf.author      = author;
    mf.description = desc;
    mf.framerate   = g.macro.framerate > 0.f ? g.macro.framerate : 240.f;
    mf.duration    = (mf.framerate > 0.f && !g.macro.inputs.empty())
                     ? g.macro.inputs.back().frame / mf.framerate
                     : 0.f;

    for (const auto& inp : g.macro.inputs) {
        mf.inputs.emplace_back(
            static_cast<int>(inp.frame),
            inp.button,
            inp.player2,
            inp.down,
            inp.subFrameIndex  // preserve intra-frame order for Android fast taps
        );
    }

    // Validate: must have at least one down event
    if (!mf.isValid()) {
        log::warn("Macro::saveJson: no valid down events - recording appears corrupt");
        return 31;
    }

    std::string extension = ".json";
    int iterations = 0;
    while (std::filesystem::exists(path + extension)) {
        iterations++;
        if (iterations > 1) {
            int length = 3 + (int)std::to_string(iterations - 1).length();
            path.erase(path.length() - length, length);
        }
        path += fmt::format(" ({})", iterations);
    }
    path += extension;

    log::debug("Macro::saveJson: saving to {}", path);

    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        if (ec) log::warn("saveJson: create_directories failed: {}", ec.message());
    }

    std::ofstream f;
#ifdef GEODE_IS_WINDOWS
    std::wstring widePath = Utils::widen(path);
    if (widePath != L"Widen Error")
        f.open(widePath, std::ios::binary);
    if (!f)
#endif
        f.open(path, std::ios::binary);

    if (!f) return 20;

    auto data = mf.exportData();
    f.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!f) { f.close(); return 21; }
    f.close();
    return 0;
}

bool Macro::loadJson(std::filesystem::path path) {
    std::ifstream f;
#ifdef GEODE_IS_WINDOWS
    {
        std::wstring wp = Utils::widen(path.string());
        if (wp != L"Widen Error") f.open(wp, std::ios::binary);
    }
    if (!f.is_open())
#endif
        f.open(path, std::ios::binary);

    if (!f.is_open()) return false;

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    f.close();

    rw_macro::MacroFile mf = rw_macro::MacroFile::importData(data);
    if (mf.empty()) return false;

    auto& g = Global::get();
    g.macro = Macro();
    g.macro.framerate   = mf.framerate;
    g.macro.duration    = mf.duration;
    g.macro.author      = mf.author;
    g.macro.description = mf.description;
    g.macro.botInfo.name    = "Reworked";
    g.macro.botInfo.version = reworkedVersion;

    for (const auto& e : mf.inputs) {
        g.macro.inputs.emplace_back(e.frame, e.button, e.player2, e.down, e.subFrameIndex);
    }

    log::info("Macro::loadJson: loaded {} events from {}", g.macro.inputs.size(), path.string());
    return true;
}

int Macro::save(std::string author, std::string desc, std::string path, bool json) {
    auto& g = Global::get();

    if (g.macro.inputs.empty()) return 31;

    std::string extension = json ? ".gdr.json" : ".gdr";

    int iterations = 0;

    while (std::filesystem::exists(path + extension)) {
        iterations++;

        if (iterations > 1) {
            int length = 3 + std::to_string(iterations - 1).length();
            path.erase(path.length() - length, length);
        }

        path += fmt::format(" ({})", std::to_string(iterations));
    }

    path += extension;

    log::debug("Saving macro to path: {}", path);

    g.macro.author = author;
    g.macro.description = desc;
    g.macro.duration = (g.macro.framerate > 0.f)
        ? g.macro.inputs.back().frame / g.macro.framerate
        : 0.f;

    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        if (ec) log::warn("create_directories failed: {}", ec.message());
    }

    std::ofstream f;
    #ifdef GEODE_IS_WINDOWS
    std::wstring widePath = Utils::widen(path);
    if (widePath != L"Widen Error")
        f.open(widePath, std::ios::binary);
    if (!f)
    #endif
        f.open(path, std::ios::binary);

    if (!f)
        return 20;

    std::vector<gdr::FrameFix> frameFixes = g.macro.frameFixes;

    auto data = g.macro.exportData(json);

    f.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!f) {
        f.close();
        return 21;
    }

    f.close();

    return 0;
}

bool Macro::loadXDFile(std::filesystem::path path) {

    Macro newMacro = Macro::XDtoGDR(path);
    if (newMacro.description == "fail")
        return false;

    Global::get().macro = newMacro;
    return true;
}

Macro Macro::XDtoGDR(std::filesystem::path path) {

    Macro newMacro;
    newMacro.author = "N/A";
    newMacro.description = "N/A";
    newMacro.gameVersion = GEODE_GD_VERSION;

    std::ifstream file;
    #ifdef GEODE_IS_WINDOWS
    {
        std::wstring wp = Utils::widen(path.string());
        if (wp != L"Widen Error") file.open(wp);
    }
    if (!file.is_open())
    #endif
        file.open(path);
    std::string line;

    if (!file.is_open()) {
        newMacro.description = "fail";
        return newMacro;
    }

    bool firstIt = true;
    bool andr = false;

    float fpsMultiplier = 1.f;

    while (std::getline(file, line)) {
        std::string item;
        std::stringstream ss(line);
        std::vector<std::string> action;

        while (std::getline(ss, item, '|'))
            action.push_back(item);

        if (action.empty()) continue;

        if (action.size() < 4) {
            if (action[0] == "android")
                fpsMultiplier = 4.f;
            else {
                try {
                    int fps = std::stoi(action[0]);
                    if (fps > 0) fpsMultiplier = 240.f / fps;
                } catch (...) {}
            }

            continue;
        }

        int frame = static_cast<int>(round(std::stoi(action[0]) * fpsMultiplier));
        int button = std::stoi(action[2]);
        bool hold = action[1] == "1";
        bool player2 = action[3] == "1";

        if (action.size() < 5) {
            newMacro.inputs.push_back(input(frame, button, player2, hold));
            continue;
        }

        bool posOnly = action[4] == "1";

        if (!posOnly)
            newMacro.inputs.push_back(input(frame, button, player2, hold));
        else {
            if (action.size() < 13) {
                log::warn("XDtoGDR: posOnly line has only {} fields, skipping", action.size());
                continue;
            }
            cocos2d::CCPoint p1Pos = ccp(std::stof(action[5]), std::stof(action[6]));
            cocos2d::CCPoint p2Pos = ccp(std::stof(action[11]), std::stof(action[12]));

            newMacro.frameFixes.push_back({ frame, {p1Pos, 0.f, false}, {p2Pos, 0.f, false} });
        }
    }

    file.close();

    return newMacro;

}

void Macro::removeInputsAfter(int frame) {
    auto& g = Global::get();
    // Eclipse model: truncate inputs after `frame`, keep everything before.
    g.macro.inputs.erase(
        std::remove_if(g.macro.inputs.begin(), g.macro.inputs.end(),
            [frame](const input& inp) { return static_cast<int>(inp.frame) > frame; }),
        g.macro.inputs.end()
    );
    g.macro.frameFixes.erase(
        std::remove_if(g.macro.frameFixes.begin(), g.macro.frameFixes.end(),
            [frame](const gdr::FrameFix& ff) { return ff.frame > frame; }),
        g.macro.frameFixes.end()
    );
}

void Macro::resetVariables() {
    auto& g = Global::get();

    g.frameMap.clear();

    g.ignoreFrame = -1;
    g.ignoreJumpButton = -1;

    g.delayedFrameReleaseMain[0] = -1;
    g.delayedFrameReleaseMain[1] = -1;

    g.delayedFrameInput[0] = -1;
    g.delayedFrameInput[1] = -1;

    g.addSideHoldingMembers[0] = false;
    g.addSideHoldingMembers[1] = false;
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++)
            g.delayedFrameRelease[x][y] = -1;
    }
}

void Macro::resetState(bool cp) {
    auto& g = Global::get();

    g.restart = false;
    g.state = state::none;

    if (!cp)
        g.checkpoints.clear();

    Interface::updateLabels();
    Interface::updateButtons();

    Macro::resetVariables();
}

void Macro::togglePlaying() {
    if (Global::hasIncompatibleMods()) return;

    auto& g = Global::get();
    
    if (g.layer) {
        static_cast<RecordLayer*>(g.layer)->playing->toggle(Global::get().state != state::playing);
        static_cast<RecordLayer*>(g.layer)->togglePlaying(nullptr);
    } else {
        RecordLayer* layer = RecordLayer::create();
        layer->togglePlaying(nullptr);
        layer->onClose(nullptr);
    }
}

void Macro::toggleRecording() {
    if (Global::hasIncompatibleMods()) return;
    
    auto& g = Global::get();
    
    if (g.layer) {
        static_cast<RecordLayer*>(g.layer)->recording->toggle(Global::get().state != state::recording);
        static_cast<RecordLayer*>(g.layer)->toggleRecording(nullptr);
    } else {
        RecordLayer* layer = RecordLayer::create();
        layer->toggleRecording(nullptr);
        layer->onClose(nullptr);
    }
}

bool Macro::shouldStep() {
    auto& g = Global::get();

    if (g.stepFrame) return true;
    if (Global::getCurrentFrame() == 0) return true;

    return false;
}
