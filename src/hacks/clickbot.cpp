#include "../includes.hpp"
#include "clickbot.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

$execute {
    auto& g = Global::get();

    if (!g.mod->setSavedValue("clickbot_defaults_v6", true)) {
        g.mod->setSavedValue("clickbot_holding_only", true);
        g.mod->setSavedValue("clickbot_playing_only", false);
        g.mod->setSavedValue("clickbot_soundpack", (int64_t)SoundPack::Default);
        g.mod->setSavedValue("clickbot_volume",    (int64_t)75);
        g.mod->setSavedValue("clickbot_pitch",     1.f);
    }

    // Initialize per-button defaults if missing or invalid
    for (const auto& name : buttonNames) {
        auto existing = g.mod->getSavedValue<matjson::Value>(name);
        auto parsed = matjson::Serialize<ClickSetting>::fromJson(existing);
        if (!parsed) {
            ClickSetting def;
            def.path     = g.mod->getResourcesDir() / fmt::format("default_{}.mp3", name);
            def.volume   = 75;
            def.pitch    = 1.f;
            def.disabled = false;
            g.mod->setSavedValue(name, matjson::Serialize<ClickSetting>::toJson(def));
        }
    }

    g.clickbotEnabled      = g.mod->getSavedValue<bool>("clickbot_enabled");
    g.clickbotOnlyPlaying  = g.mod->getSavedValue<bool>("clickbot_playing_only");
    g.clickbotOnlyHolding  = g.mod->getSavedValue<bool>("clickbot_holding_only");

    Clickbot::updateSounds();
};

int Clickbot::nameToIndex(const std::string& id) {
    for (int i = 0; i < (int)buttonNames.size(); ++i)
        if (buttonNames[i] == id) return i;
    return -1;
}

SoundPack Clickbot::getCurrentPack() {
    auto& g = Global::get();
    auto packId = g.mod->getSavedValue<int64_t>("clickbot_soundpack");
    switch ((int)packId) {
        case (int)SoundPack::Gaming:     return SoundPack::Gaming;
        case (int)SoundPack::Office:     return SoundPack::Office;
        case (int)SoundPack::Mechanical: return SoundPack::Mechanical;
        case (int)SoundPack::Laptop:     return SoundPack::Laptop;
        case (int)SoundPack::Wood:       return SoundPack::Wood;
        case (int)SoundPack::Custom:     return SoundPack::Custom;
        default:                         return SoundPack::Default;
    }
}

std::filesystem::path Clickbot::getPackDir(SoundPack pack) {
    auto& g = Global::get();
    std::filesystem::path resourcesDir = g.mod->getResourcesDir();
    auto it = soundPackDirNames.find((int)pack);
    if (it != soundPackDirNames.end())
        return resourcesDir / "sounds" / it->second;
    return resourcesDir / "sounds" / "default";
}

void Clickbot::initSystem() {
    auto& c = get();
    if (!c.system) {
        FMODAudioEngine* fmodEngine = FMODAudioEngine::sharedEngine();
        if (fmodEngine) c.system = fmodEngine->m_system;
    }
    if (!c.system) return;

    if (!c.clickGroup) {
        FMOD::ChannelGroup* masterGroup = nullptr;
        c.system->getMasterChannelGroup(&masterGroup);
        c.system->createChannelGroup("Reworked_Clickbot", &c.clickGroup);
        if (c.clickGroup && masterGroup)
            masterGroup->addGroup(c.clickGroup);
    }
}

void Clickbot::loadSoundsForPack(SoundPack pack) {
    auto& c = get();
    if (!c.system) return;

    std::filesystem::path packDir = getPackDir(pack);

    for (int i = 0; i < (int)buttonNames.size(); ++i) {
        const std::string& name = buttonNames[i];
        std::filesystem::path soundPath = packDir / fmt::format("{}.mp3", name);

        if (pack == SoundPack::Default) {
            soundPath = Global::get().mod->getResourcesDir() / fmt::format("default_{}.mp3", name);
        }

        if (!std::filesystem::exists(soundPath)) {
            std::filesystem::path fallback = Global::get().mod->getResourcesDir() / fmt::format("default_{}.mp3", name);
            if (!std::filesystem::exists(fallback)) continue;
            soundPath = fallback;
        }

        if (c.sounds[i]) {
            c.sounds[i]->release();
            c.sounds[i] = nullptr;
        }

        FMOD_MODE mode = FMOD_CREATESAMPLE | FMOD_LOOP_OFF | FMOD_2D;
        FMOD_RESULT result = c.system->createSound(soundPath.string().c_str(), mode, nullptr, &c.sounds[i]);
        if (result != FMOD_OK) {
            log::warn("[Clickbot] Failed to load sound '{}': error {}", soundPath.string(), (int)result);
            c.sounds[i] = nullptr;
        }
    }
}

void Clickbot::updateSounds() {
    initSystem();
    if (!get().system) return;
    loadSoundsForPack(getCurrentPack());
}

void Clickbot::playSound(const std::string& id, bool player2, bool isDual) {
    auto& c = get();
    if (!c.system) { initSystem(); return; }

    int idx = nameToIndex(id);
    if (idx < 0 || !c.sounds[idx]) return;

    auto& g = Global::get();
    int masterVol = (int)g.mod->getSavedValue<int64_t>("clickbot_volume");
    if (masterVol <= 0) return;

    float vol = std::clamp(masterVol / 100.f, 0.f, 1.f);

    if (isDual) vol *= 0.85f;

    FMOD::Channel* ch = nullptr;
    FMOD_RESULT result = c.system->playSound(c.sounds[idx], c.clickGroup, false, &ch);
    if (result != FMOD_OK || !ch) return;

    ch->setVolume(vol);

    float pitch = std::clamp(g.mod->getSavedValue<float>("clickbot_pitch"), 0.5f, 2.f);
    if (isDual) pitch *= 0.95f;
    ch->setPitch(pitch);

    c.channels[idx] = ch;
}

class $modify(GJBaseGameLayer) {

    void handleButton(bool hold, int button, bool player2) {
        GJBaseGameLayer::handleButton(hold, button, player2);
        auto& g = Global::get();

        if (!g.clickbotEnabled) return;
        if (button < 1 || button > 3) return;
        if (!hold && g.clickbotOnlyHolding) return;

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;
        if (g.clickbotOnlyPlaying && g.state != state::playing) return;

        std::string btn = button == 1 ? "click" : (button == 2 ? "left" : "right");
        std::string soundId = (hold ? "hold_" : "release_") + btn;

        bool isDual = pl->m_levelSettings->m_twoPlayerMode;
        Clickbot::playSound(soundId, player2, isDual);
    }

};
