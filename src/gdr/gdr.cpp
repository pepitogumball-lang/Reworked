#include <geode/Geode.hpp>

#include "gdr.hpp"

cocos2d::CCPoint dataFromString(std::string dataString) {
    std::stringstream ss(dataString);
    std::string item;

    float xPos = 0.f;
    float yPos = 0.f;

    for (int i = 0; i < 3; i++) {
        if (!std::getline(ss, item, ',')) break;
        try {
            if (!item.empty()) {
                if (i == 1) xPos = std::stof(item);
                else if (i == 2) yPos = std::stof(item);
            }
        } catch (...) {
            geode::log::warn("dataFromString: failed to parse field {}: '{}'", i, item);
        }
    }

    return { xPos, yPos };
};

std::vector<std::string> splitByChar(std::string str, char splitChar) {
    std::vector<std::string> strs;
    strs.reserve(std::count(str.begin(), str.end(), splitChar) + 1);

    size_t start = 0;
    size_t end = str.find(splitChar);
    while (end != std::string::npos) {
        strs.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(splitChar, start);
    }
    strs.emplace_back(str.substr(start));

    return strs;
}

geode::prelude::VersionInfo getVersion(std::vector<std::string> nums) {
    if (nums.size() < 3) return geode::prelude::VersionInfo(0, 0, 0);

    int major = geode::utils::numFromString<int>(nums[0]).unwrapOr(0);
    int minor = geode::utils::numFromString<int>(nums[1]).unwrapOr(0);
    int patch = geode::utils::numFromString<int>(nums[2]).unwrapOr(0);

    if (major < 0) major = 0;
    if (minor < 0) minor = 0;
    if (patch < 0) patch = 0;

    return geode::prelude::VersionInfo(
        static_cast<size_t>(major),
        static_cast<size_t>(minor),
        static_cast<size_t>(patch)
    );
}
