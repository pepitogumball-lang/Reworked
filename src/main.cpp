#include "includes.hpp"

#include "ui/record_layer.hpp"
#include "practice_fixes/practice_fixes.hpp"
#include "hacks/layout_mode.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

$execute {

    geode::listenForSettingChanges<std::string>("macro_accuracy", [](std::string value) {
        auto& g = Global::get();
        g.frameFixes = false;
        g.inputFixes = false;
        if (value == "Frame Fixes") g.frameFixes = true;
        if (value == "Input Fixes") g.inputFixes = true;
    }, Mod::get());

    geode::listenForSettingChanges<int64_t>("frame_fixes_limit", [](int64_t value) {
        Global::get().frameFixesLimit = value;
    }, Mod::get());

    geode::listenForSettingChanges<bool>("lock_delta", [](bool value) {
        Global::get().lockDelta = value;
    }, Mod::get());

    geode::listenForSettingChanges<bool>("auto_stop_playing", [](bool value) {
        Global::get().stopPlaying = value;
    }, Mod::get());

};

class $modify(PlayLayer) {

    struct Fields {
        int delayedLevelRestart = -1;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& g = Global::get();

        if (m_fields->delayedLevelRestart != -1 && Global::getCurrentFrame() >= m_fields->delayedLevelRestart) {
            m_fields->delayedLevelRestart = -1;
            resetLevelFromStart();
        }
    }

    void onQuit() {
        auto& g = Global::get();

        if (Mod::get()->getSettingValue<bool>("disable_speedhack") && g.speedhackEnabled)
            Global::toggleSpeedhack();

        g.blockOrbRewards = false;
        g.leftOver = 0.f;

        PlayLayer::onQuit();
    }

    void pauseGame(bool b1) {
        Global::updateKeybinds();

        if (!Global::get().renderer.tryPause()) return;

        auto& g = Global::get();

        if (!m_player1 || !m_player2) return PlayLayer::pauseGame(b1);
        if (g.state != state::recording) return PlayLayer::pauseGame(b1);

        g.ignoreRecordAction = true;
        int frame = Global::getCurrentFrame() + 1;

        auto releaseIfHeld = [&](PlayerObject* p, int btn, bool p2) {
            if (p->m_holdingButtons[btn]) {
                handleButton(false, btn, p2);
                g.macro.inputs.push_back(input(frame, btn, p2, false));
            }
        };

        releaseIfHeld(m_player1, 1, false);
        if (m_levelSettings->m_platformerMode) {
            releaseIfHeld(m_player1, 2, false);
            releaseIfHeld(m_player1, 3, false);
        }

        if (m_levelSettings->m_twoPlayerMode) {
            releaseIfHeld(m_player2, 1, true);
            if (m_levelSettings->m_platformerMode) {
                releaseIfHeld(m_player2, 2, true);
                releaseIfHeld(m_player2, 3, true);
            }
        }

        g.ignoreRecordAction = false;
        PlayLayer::pauseGame(b1);
    }

    bool init(GJGameLevel* level, bool b1, bool b2) {
        auto& g = Global::get();
        g.firstAttempt = true;
        g.blockOrbRewards = false;

        if (!PlayLayer::init(level, b1, b2)) return false;

        Global::updateKeybinds();

        auto now = std::chrono::system_clock::now();
        g.currentSession = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        g.lastAutoSaveFrame = 0;

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto& g = Global::get();
        g.leftOver = 0.f;
        int frame = Global::getCurrentFrame();

        if (!m_isPracticeMode)
            g.renderer.levelStartFrame = frame;

        if (g.restart && m_levelSettings->m_platformerMode && g.state != state::none)
            m_fields->delayedLevelRestart = frame + 2;

        Global::updateSeed(true);

        g.safeMode = false;

        if (g.layoutMode)
            g.safeMode = true;

        bool autoSafe = g.mod->getSavedValue<bool>("macro_auto_safe_mode");
        g.blockOrbRewards = (g.state != state::none) && autoSafe;

        g.currentAction = 0;
        g.currentFrameFix = 0;
        g.restart = false;

        if (g.state == state::playing)
            Macro::buildFrameMap();

        if (g.state == state::recording)
            Macro::updateInfo(this);

        if ((!m_isPracticeMode || frame <= 1 || g.checkpoints.empty()) && g.state == state::recording) {
            g.macro.inputs.clear();
            g.macro.frameFixes.clear();
            g.checkpoints.clear();

            g.macro.framerate = 240.f;
            g.tpsEnabled = false;
            g.tps = 240.f;
            g.mod->setSavedValue("macro_tps", 240.f);
            g.mod->setSavedValue("macro_tps_enabled", false);
            if (g.layer) static_cast<RecordLayer*>(g.layer)->updateTPS();

            float initSpeed = g.speedhack <= 0.01f ? 1.f : g.speedhack;
            g.shOffset = 0.f;
            g.shRawFrameAtChange = frame;
            g.shPrevSpeed = initSpeed;

            PlayerData p1Data = PlayerPracticeFixes::saveData(m_player1);
            PlayerData p2Data = PlayerPracticeFixes::saveData(m_player2);

            InputPracticeFixes::applyFixes(this, p1Data, p2Data, frame);
            Macro::resetVariables();

            m_player1->m_holdingRight = false;
            m_player1->m_holdingLeft  = false;
            if (m_player2) {
                m_player2->m_holdingRight = false;
                m_player2->m_holdingLeft  = false;
            }

            m_player1->m_holdingButtons[2] = false;
            m_player1->m_holdingButtons[3] = false;
            if (m_player2) {
                m_player2->m_holdingButtons[2] = false;
                m_player2->m_holdingButtons[3] = false;
            }
        }

        if (!m_levelSettings->m_platformerMode || (!g.mod->getSavedValue<bool>("macro_always_practice_fixes") && g.state != state::recording)) return;

        g.ignoreRecordAction = true;
        for (int i = 0; i < 4; i++) {
            bool player2 = !(sidesButtons[i] > 2);
            if (g.heldButtons[sidesButtons[i]])
                handleButton(true, indexButton[sidesButtons[i]], player2);
        }
        g.ignoreRecordAction = false;
    }

};

class $modify(BGLHook, GJBaseGameLayer) {

    struct Fields {
        bool macroInput = false;
        bool p1J = false, p1L = false, p1R = false;
        bool p2J = false, p2L = false, p2R = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto& g = Global::get();
        PlayLayer* pl = PlayLayer::get();

        if (!pl)
            return GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        Global::updateSeed();

        bool rendering = g.renderer.recording || g.renderer.recordingAudio;

        if (g.state != state::none || rendering) {

            bool autoSafe = g.mod->getSavedValue<bool>("macro_auto_safe_mode");
            g.blockOrbRewards = autoSafe;

            if (!g.firstAttempt) {
                g.renderer.dontRender = false;
                g.renderer.dontRecordAudio = false;
            }

            int frame = Global::getCurrentFrame();
            if (frame > 2 && g.firstAttempt && g.macro.xdBotMacro) {
                g.firstAttempt = false;

                if ((m_levelSettings->m_platformerMode || rendering) && !m_levelEndAnimationStarted)
                    return pl->resetLevelFromStart();
                else if (!m_levelEndAnimationStarted)
                    return pl->resetLevel();
            }

            if (g.previousFrame == frame && frame != 0 && g.macro.xdBotMacro)
                return GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (g.state == state::none)
            return;

        int frame = Global::getCurrentFrame();
        g.previousFrame = frame;

        // Reset polling states on level restart
        if (g.restart) {
            m_fields->p1J = m_fields->p1L = m_fields->p1R = false;
            m_fields->p2J = m_fields->p2L = m_fields->p2R = false;
        }

        // --- ANDROID ROBUST RECORDING (POLLING) ---
        if (g.state == state::recording && !g.ignoreRecordAction) {
            auto recordInput = [&](PlayerObject* p, bool p2, int btn, bool& lastState) {
                if (!p) return;
                bool currentState = p->m_holdingButtons[btn];
                if (currentState != lastState) {
                    g.macro.recordAction(frame, btn, p2, currentState);
                    lastState = currentState;
                    if (g.inputFixes) g.macro.recordFrameFix(frame, m_player1, m_player2);
                }
            };

            recordInput(m_player1, false, 1, m_fields->p1J);
            if (m_levelSettings->m_platformerMode) {
                recordInput(m_player1, false, 2, m_fields->p1L);
                recordInput(m_player1, false, 3, m_fields->p1R);
            }
            if (m_levelSettings->m_twoPlayerMode && m_player2) {
                recordInput(m_player2, true, 1, m_fields->p2J);
                if (m_levelSettings->m_platformerMode) {
                    recordInput(m_player2, true, 2, m_fields->p2L);
                    recordInput(m_player2, true, 3, m_fields->p2R);
                }
            }
        }

        if (g.macro.xdBotMacro && g.restart && !m_levelEndAnimationStarted) {
            g.restart = false;
            g.macro.xdBotPlaying = false;
            g.macro.xdBotMacro = false;
            g.macro.xdBotPos = 0;
            g.macro.xdBotFrame = 0;

            if (g.macro.p1Inputs.empty()) {
                g.macro.xdBotMacro = false;
            } else {
                g.macro.xdBotMacro = true;
                g.macro.xdBotPlaying = true;
            }
        }

        if (g.state != state::playing)
            return;

        if (g.macro.xdBotMacro && g.macro.xdBotPlaying) {
            while (g.macro.xdBotPos < g.macro.p1Inputs.size() && g.macro.p1Inputs[g.macro.xdBotPos].frame <= frame) {
                auto& inp = g.macro.p1Inputs[g.macro.xdBotPos];
                if (inp.frame == frame) {
                    m_fields->macroInput = true;
                    handleButton(inp.down, inp.button, inp.player2);
                    m_fields->macroInput = false;
                }
                g.macro.xdBotPos++;
            }
        } else {
            for (auto& inp : g.macro.inputs) {
                if (inp.frame == frame) {
                    m_fields->macroInput = true;
                    handleButton(inp.down, inp.button, inp.player2);
                    m_fields->macroInput = false;
                }
            }
        }
    }

    void handleButton(bool hold, int button, bool player2) {
        auto& g = Global::get();
        int frame = Global::getCurrentFrame();

        if (g.p2mirror && m_gameState.m_isDualMode && !g.autoclicker) {
            GJBaseGameLayer::handleButton(
                g.mod->getSavedValue<bool>("p2_input_mirror_inverted") ? !hold : hold,
                button, !player2
            );
        }

        if (g.state == state::none)
            return GJBaseGameLayer::handleButton(hold, button, player2);

        if (g.state == state::playing) {
            if (g.mod->getSavedValue<bool>("macro_ignore_inputs") && !m_fields->macroInput)
                return;
            else
                return GJBaseGameLayer::handleButton(hold, button, player2);
        }
        
        if (g.ignoreFrame != -1 && hold) {
            if (frame <= g.ignoreFrame) return;
            else g.ignoreFrame = -1;
        }

        if (frame >= 10 && hold)
            Global::hasIncompatibleMods();

        int playerIdx = m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0;
        bool isDelayedInput   = g.delayedFrameInput[playerIdx] != -1;
        bool isDelayedRelease = g.delayedFrameReleaseMain[playerIdx] != -1;

        if ((isDelayedInput || g.ignoreJumpButton == frame || isDelayedRelease) && button == 1) {
            if (g.ignoreJumpButton >= frame)
                g.delayedFrameInput[playerIdx] = g.ignoreJumpButton + 1;
            return;
        }

        GJBaseGameLayer::handleButton(hold, button, player2);
    }
};

class $modify(PauseLayer) {

    void onPracticeMode(CCObject* sender) {
        PauseLayer::onPracticeMode(sender);
        if (Global::get().state != state::none) PlayLayer::get()->resetLevel();
    }

    void onNormalMode(CCObject* sender) {
        PauseLayer::onNormalMode(sender);
        auto& g = Global::get();
        g.checkpoints.clear();
        if (g.restart) {
            if (PlayLayer* pl = PlayLayer::get())
                pl->resetLevel();
        }
    }

    void onQuit(CCObject* sender) {
        PauseLayer::onQuit(sender);

        Global::get().blockOrbRewards = false;
        Macro::resetState();

        Loader::get()->queueInMainThread([] {
            auto& g = Global::get();
            if (g.renderer.recording) g.renderer.stop();
            if (g.renderer.recordingAudio) g.renderer.stopAudio();
        });
    }

    void goEdit() {
        PauseLayer::goEdit();

        Global::get().blockOrbRewards = false;
        Macro::resetState();

        Loader::get()->queueInMainThread([] {
            auto& g = Global::get();
            if (g.renderer.recording) g.renderer.stop();
            if (g.renderer.recordingAudio) g.renderer.stopAudio();
        });
    }

};


