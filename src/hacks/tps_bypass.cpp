#include "../includes.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>

class $modify(GJBaseGameLayer) {

    void update(float dt) {
        auto& g = Global::get();

        if (!g.tpsEnabled) return GJBaseGameLayer::update(dt);
        if (Global::getTPS() == 240.f) return GJBaseGameLayer::update(dt);
        if (!PlayLayer::get()) return GJBaseGameLayer::update(dt);

        float newDt = 1.f / Global::getTPS();

        if (g.frameStepper) return GJBaseGameLayer::update(newDt);

        float realDt = dt + g.leftOver;
        if (realDt < 0.f) realDt = 0.f;

        if (newDt < dt) realDt = std::min(realDt, dt);

        auto startTime = std::chrono::high_resolution_clock::now();
        int mult = static_cast<int>(realDt / newDt);

        mult = std::min(mult, 20);

        for (int i = 0; i < mult; ++i) {
            GJBaseGameLayer::update(newDt);
            if (std::chrono::high_resolution_clock::now() - startTime > std::chrono::duration<double, std::milli>(14.f)) {
                mult = i + 1;
                break;
            }
        }

        g.leftOver = realDt - newDt * mult;
        g.leftOver = std::clamp(g.leftOver, -newDt, newDt);
    }

    float getModifiedDelta(float dt) {
        if (!Global::get().tpsEnabled) return GJBaseGameLayer::getModifiedDelta(dt);
        if (Global::getTPS() == 240.f) return GJBaseGameLayer::getModifiedDelta(dt);
        if (!PlayLayer::get()) return GJBaseGameLayer::getModifiedDelta(dt);

        float newDt = 1.f / Global::getTPS();

        if (0 < m_resumeTimer) {
            m_resumeTimer--;
            dt = 0.0;
        }

        float fVar2 = 1.0f;
        if (m_gameState.m_timeWarp <= 1.0f)
            fVar2 = m_gameState.m_timeWarp;

        double dVar1 = dt + m_extraDelta;
        float fVar3 = std::round(dVar1 / (fVar2 * newDt));
        double dVar4 = fVar3 * fVar2 * newDt;
        m_extraDelta = dVar1 - dVar4;

        return static_cast<float>(dVar4);
    }

};
