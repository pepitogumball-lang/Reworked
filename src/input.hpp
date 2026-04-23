#pragma once
#include "gdr/gdr.hpp"

struct input : gdr::Input {
    input() = default;
    input(int frame, int button, bool player2, bool down, uint8_t subFrameIndex = 0)
        : Input(frame, button, player2, down, subFrameIndex) {}

    bool operator==(const input& other) const {
        return frame == other.frame &&
               player2 == other.player2 &&
               button == other.button &&
               down == other.down &&
               subFrameIndex == other.subFrameIndex;
    }
};
