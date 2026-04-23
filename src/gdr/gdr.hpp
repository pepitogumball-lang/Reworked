#pragma once

#include <iostream>
#include <vector>
#include <optional>

#include "json.hpp"

#include <Geode/utils/VersionInfo.hpp>

geode::prelude::VersionInfo getVersion(std::vector<std::string> nums);

cocos2d::CCPoint dataFromString(std::string dataString);

std::vector<std::string> splitByChar(std::string str, char splitChar);

const std::string reworkedVersion = "v2.5.0";

// ---------------------------------------------------------------------------
// New JSON macro format (reworked-macro v1)
// ---------------------------------------------------------------------------
// Stores only explicit input events (down/up). Frames with no action are
// implicit - no need to store them. This avoids all issues with speedhack
// frame normalization producing corrupt frame numbers.
//
// File extension: .json  (e.g. "mylevel.json")
//
// Format:
// {
//   "format":    "reworked-macro",
//   "version":   1,
//   "author":    "...",
//   "description": "...",
//   "framerate": 240.0,
//   "duration":  12.533,
//   "inputs": [
//     { "frame": 0,  "button": 1, "player2": false, "down": true  },
//     { "frame": 12, "button": 1, "player2": false, "down": false },
//     { "frame": 48, "button": 1, "player2": false, "down": true  }
//   ]
// }
//
// Rules:
//   - "frame" is the absolute frame number from the start of the level (0-based).
//   - "down" true = button pressed, false = button released.
//   - "button" follows GD convention: 1 = jump/left, 2 = right (platformer).
//   - "player2" true = second player in dual mode.
//   - Frames between events are "do nothing" - not stored.
//   - Events MUST be sorted by frame in ascending order.
// ---------------------------------------------------------------------------

namespace rw_macro {

    using namespace nlohmann;

    struct MacroEvent {
        int    frame          = 0;
        int    button         = 1;
        bool   player2        = false;
        bool   down           = true;
        uint8_t subFrameIndex = 0;  // order within same frame (for fast taps on touch devices)

        MacroEvent() = default;
        MacroEvent(int f, int btn, bool p2, bool dn, uint8_t sfx = 0)
            : frame(f), button(btn), player2(p2), down(dn), subFrameIndex(sfx) {}

        bool operator<(const MacroEvent& o) const {
            if (frame != o.frame) return frame < o.frame;
            return subFrameIndex < o.subFrameIndex;
        }
    };

    struct MacroFile {
        std::string format      = "reworked-macro";
        int         version     = 1;
        std::string author;
        std::string description;
        float       framerate   = 240.f;
        float       duration    = 0.f;
        std::vector<MacroEvent> inputs;

        // Save to JSON bytes
        std::vector<uint8_t> exportData() const {
            json j;
            j["format"]      = format;
            j["version"]     = version;
            j["author"]      = author;
            j["description"] = description;
            j["framerate"]   = framerate;
            j["duration"]    = duration;
            j["inputs"]      = json::array();

            for (const auto& e : inputs) {
                json ev = {
                    {"frame",   e.frame},
                    {"button",  e.button},
                    {"player2", e.player2},
                    {"down",    e.down}
                };
                // Only write subFrameIndex when non-zero to keep small file size
                if (e.subFrameIndex > 0)
                    ev["sfx"] = e.subFrameIndex;
                j["inputs"].push_back(ev);
            }

            std::string s = j.dump(2); // pretty-print with 2-space indent
            return std::vector<uint8_t>(s.begin(), s.end());
        }

        // Load from JSON bytes. Returns empty MacroFile on failure.
        static MacroFile importData(const std::vector<uint8_t>& data) {
            MacroFile mf;
            json j = json::parse(data, nullptr, false);
            if (j.is_discarded()) return mf;

            // Must be our format
            if (!j.contains("format") || j["format"] != "reworked-macro") return mf;

            if (j.contains("version"))     mf.version     = j["version"].get<int>();
            if (j.contains("author"))      mf.author      = j["author"].get<std::string>();
            if (j.contains("description")) mf.description = j["description"].get<std::string>();
            if (j.contains("framerate"))   mf.framerate   = j["framerate"].get<float>();
            if (j.contains("duration"))    mf.duration    = j["duration"].get<float>();

            if (j.contains("inputs") && j["inputs"].is_array()) {
                for (const auto& item : j["inputs"]) {
                    if (!item.contains("frame")) continue;
                    MacroEvent e;
                    e.frame          = item["frame"].get<int>();
                    e.button         = item.value("button",  1);
                    e.player2        = item.value("player2", false);
                    e.down           = item.value("down",    true);
                    e.subFrameIndex  = item.value("sfx",     (uint8_t)0);
                    mf.inputs.push_back(e);
                }
            }

            // Ensure sorted
            std::sort(mf.inputs.begin(), mf.inputs.end());
            return mf;
        }

        bool empty() const { return inputs.empty(); }

        // Check if this looks like a valid recording (has at least one down event)
        bool isValid() const {
            for (const auto& e : inputs)
                if (e.down) return true;
            return false;
        }
    };

} // namespace rw_macro

// ---------------------------------------------------------------------------
// Legacy GDR format (kept for backwards compatibility)
// ---------------------------------------------------------------------------

namespace gdr {

	using namespace nlohmann;

	struct Bot {
		std::string name;
		std::string version;

		inline Bot(std::string const& name, std::string const& version)
			: name(name), version(version) {}
	};

	struct Level {
		uint32_t id;
		std::string name;

		Level() = default;

		inline Level(std::string const& name, uint32_t id = 0)
			: name(name), id(id) {}
	};

	struct FrameData {
		cocos2d::CCPoint pos = { 0.f, 0.f };
		float rotation = 0.f;
		bool rotate = true;
	};

	struct FrameFix {
		int frame;
		FrameData p1;
		FrameData p2;
	};

	class Input {
	protected:
		Input() = default;
		template <typename, typename>
		friend class Replay;
	public:
		uint32_t frame;
		int button;
		bool player2;
		bool down;
		// Sub-frame ordering index: when multiple inputs share the same frame
		// (e.g. a fast tap on Android where press+release land in the same physics frame),
		// this index preserves their original order so playback executes them correctly.
		// 0 = first input on this frame, 1 = second, etc.
		// Not serialized in legacy .gdr format (ignored on load = 0).
		uint8_t subFrameIndex = 0;

		inline virtual void parseExtension(json::object_t obj) {}
		inline virtual json::object_t saveExtension() const {
			return {};
		}

		inline Input(int frame, int button, bool player2, bool down, uint8_t subFrameIndex = 0)
			: frame(frame), button(button), player2(player2), down(down), subFrameIndex(subFrameIndex) {}


		inline static Input hold(int frame, int button, bool player2 = false) {
			return Input(frame, button, player2, true);
		}

		inline static Input release(int frame, int button, bool player2 = false) {
			return Input(frame, button, player2, false);
		}

		// Sort by frame first, then by subFrameIndex to preserve intra-frame order.
		inline bool operator<(Input const& other) const {
			if (frame != other.frame) return frame < other.frame;
			return subFrameIndex < other.subFrameIndex;
		}
	};

	template <typename S = void, typename T = Input>
	class Replay {
		Replay() = default;
	public:
		using InputType = T;
		using Self = std::conditional_t<std::is_same_v<S, void>, Replay<S, T>, S>;

		std::string author;
		std::string description;

		float duration;
		float gameVersion;
		float version = 1.0;

		float framerate = 240.f;

		int seed = 0;
		int coins = 0;

		bool ldm = false;

		Bot botInfo;
		Level levelInfo;

		std::vector<InputType> inputs;
		std::vector<FrameFix> frameFixes;

		uint32_t frameForTime(double time)
		{
			return static_cast<uint32_t>(time * (double)framerate);
		}

		virtual void parseExtension(json::object_t obj) {}
		virtual json::object_t saveExtension() const {
			return {};
		}

		Replay(std::string const& botName, std::string const& botVersion)
			: botInfo(botName, botVersion) {}

		static Self importData(std::vector<uint8_t> const& data, bool importInputs = true) {
			Self replay;
			json replayJson;

			replayJson = json::from_msgpack(data, true, false);
			if (replayJson.is_discarded()) {
				replayJson = json::parse(data, nullptr, false);
				if (replayJson.is_discarded()) return replay;
			}

			if (!replayJson["gameVersion"].is_null()) replay.gameVersion = replayJson["gameVersion"];
			if (!replayJson["description"].is_null()) replay.description = replayJson["description"];
			if (!replayJson["version"].is_null()) replay.version = replayJson["version"];
			if (!replayJson["duration"].is_null()) replay.duration = replayJson["duration"];
			if (!replayJson["author"].is_null()) replay.author = replayJson["author"];
			if (!replayJson["seed"].is_null()) replay.seed = replayJson["seed"];
			if (!replayJson["coins"].is_null()) replay.coins = replayJson["coins"];
			if (!replayJson["ldm"].is_null()) replay.ldm = replayJson["ldm"];

			if (!replayJson["bot"]["name"].is_null()) replay.botInfo.name = replayJson["bot"]["name"];
			if (!replayJson["bot"]["version"].is_null()) replay.botInfo.version = replayJson["bot"]["version"];
			if (!replayJson["level"]["id"].is_null()) replay.levelInfo.id = replayJson["level"]["id"];
			if (!replayJson["level"]["name"].is_null()) replay.levelInfo.name = replayJson["level"]["name"];

			std::string ver = replay.botInfo.version;
			
			if (replayJson.contains("framerate"))
				replay.framerate = replayJson["framerate"];

			bool rotation = ver.find("beta.") == std::string::npos && ver.find("alpha.") == std::string::npos;
			if (replay.botInfo.name == "xdBot" && ver == "v2.0.0") rotation = true;

			int offset = replay.botInfo.name == "xdBot" ? 1 : 0;

			if (offset == 1) {
				if (ver.front() == 'v') ver = ver.substr(1);

				std::vector<std::string> splitVer = splitByChar(ver, '.');

				if (splitVer.size() <= 3) {
					std::vector<std::string> realVer = {"2", "3", "6"};

					geode::prelude::VersionInfo macroVer = getVersion(splitVer);
					geode::prelude::VersionInfo checkVer = getVersion(realVer);

					if (macroVer >= checkVer) offset = false;
				}
			}

			replay.parseExtension(replayJson.get<json::object_t>());

			if (!importInputs)
				return replay;

			for (json const& inputJson : replayJson["inputs"]) {
				InputType input;

				if (!inputJson.contains("frame")) continue;
				if (inputJson["frame"].is_null()) continue;

				input.frame   = inputJson["frame"].get<int>() + offset;
				input.button  = inputJson["btn"];
				input.player2 = inputJson["2p"];
				input.down    = inputJson["down"];
				// Load subFrameIndex if present (new format); default 0 for legacy files.
				if (inputJson.contains("sfx") && !inputJson["sfx"].is_null())
					input.subFrameIndex = inputJson["sfx"].get<uint8_t>();
				input.parseExtension(inputJson.get<json::object_t>());

				replay.inputs.push_back(input);
			}

			if (!replayJson.contains("frameFixes")) return replay;

			for (json const& frameFixJson : replayJson["frameFixes"]) {
				FrameFix frameFix;

				if (!frameFixJson.contains("frame")) continue;
				if (frameFixJson["frame"].is_null()) continue;

				frameFix.frame = frameFixJson["frame"].get<int>() + offset;

				if (frameFixJson.contains("player1")) {

					frameFix.p1.pos = dataFromString(frameFixJson["player1"]);
					frameFix.p1.rotate = false;
					
					frameFix.p2.pos = dataFromString(frameFixJson["player2"]);
					frameFix.p2.rotate = false;

				}
				else if (frameFixJson.contains("player1X")) {

					frameFix.p1.pos = ccp(frameFixJson["player1X"], frameFixJson["player1Y"]);
					frameFix.p1.rotate = false;
					
					frameFix.p2.pos = ccp(frameFixJson["player2X"], frameFixJson["player2Y"]);
					frameFix.p2.rotate = false;

				} else if (frameFixJson.contains("p1")) {
					if (replay.botInfo.name != "xdBot" && replay.botInfo.name != "Reworked") rotation = false;

					if (frameFixJson["p1"].contains("x"))
						frameFix.p1.pos.x = frameFixJson["p1"]["x"];

					if (frameFixJson["p1"].contains("y"))
						frameFix.p1.pos.y = frameFixJson["p1"]["y"];

					if (frameFixJson["p1"].contains("r") && rotation)
						frameFix.p1.rotation = frameFixJson["p1"]["r"];

					if (frameFixJson.contains("p2")) {
						if (frameFixJson["p2"].contains("x"))
							frameFix.p2.pos.x = frameFixJson["p2"]["x"];

						if (frameFixJson["p2"].contains("y"))
							frameFix.p2.pos.y = frameFixJson["p2"]["y"];

						if (frameFixJson["p2"].contains("r") && rotation)
							frameFix.p2.rotation = frameFixJson["p2"]["r"];
					}
				} else continue;

				replay.frameFixes.push_back(frameFix);
			}

			return replay;
		}

		std::vector<uint8_t> exportData(bool exportJson = false) {
			json replayJson = saveExtension();
			replayJson["gameVersion"] = gameVersion;
			replayJson["description"] = description;
			replayJson["version"] = version;
			replayJson["duration"] = duration;
			replayJson["bot"]["name"] = botInfo.name;
			replayJson["bot"]["version"] = botInfo.version;
			replayJson["level"]["id"] = levelInfo.id;
			replayJson["level"]["name"] = levelInfo.name;
			replayJson["author"] = author;
			replayJson["seed"] = seed;
			replayJson["coins"] = coins;
			replayJson["ldm"] = ldm;
			replayJson["framerate"] = framerate;

			for (InputType const& input : inputs) {
				json inputJson = input.saveExtension();
				inputJson["frame"] = input.frame;
				inputJson["btn"]   = input.button;
				inputJson["2p"]    = input.player2;
				inputJson["down"]  = input.down;
				// Only write subFrameIndex when non-zero to keep legacy file size small.
				if (input.subFrameIndex > 0)
					inputJson["sfx"] = input.subFrameIndex;

				replayJson["inputs"].push_back(inputJson);
			}

			for (FrameFix const& frameFix : frameFixes) {
				json frameFixJson;

				json p1Json;
				json p2Json;

				if (frameFix.p1.pos.x != 0.f) p1Json["x"] = frameFix.p1.pos.x;
				if (frameFix.p1.pos.y != 0.f) p1Json["y"] = frameFix.p1.pos.y;
				if (frameFix.p1.rotation != 0.f) p1Json["r"] = frameFix.p1.rotation;

				if (frameFix.p2.pos.x != 0.f) p2Json["x"] = frameFix.p2.pos.x;
				if (frameFix.p2.pos.y != 0.f) p2Json["y"] = frameFix.p2.pos.y;
				if (frameFix.p2.rotation != 0.f) p2Json["r"] = frameFix.p2.rotation;

				if (p1Json.empty() && p2Json.empty()) continue;

				frameFixJson["frame"] = frameFix.frame;
				frameFixJson["p1"] = p1Json;

				if (!p2Json.empty())
					frameFixJson["p2"] = p2Json;

				replayJson["frameFixes"].push_back(frameFixJson);
			}

			if (exportJson) {
				std::string replayString = replayJson.dump();
				return std::vector<uint8_t>(replayString.begin(), replayString.end());
			}
			else {
				return json::to_msgpack(replayJson);
			}
		}
	};

}
