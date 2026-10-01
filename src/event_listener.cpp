#include "../include/coreflow.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {
    struct DisplayNode {
        std::string path;
        enum Type { DRM, FB } type;
    };
}

bool EventListener::isScreenOn() {
    static const std::vector<DisplayNode> kNodes = {
        {"/sys/class/drm/card0-DSI-1/status", DisplayNode::DRM},
        {"/sys/class/drm/card0-DSI-2/status", DisplayNode::DRM},
        {"/sys/class/drm/card1-DSI-1/status", DisplayNode::DRM},
        {"/sys/class/graphics/fb0/blank", DisplayNode::FB}
    };

    for (const auto& node : kNodes) {
        std::ifstream file(node.path);
        if (!file.is_open()) {
            continue;
        }

        std::string status;
        if (!(file >> status)) {
            continue;
        }
        file.close();

        if (node.type == DisplayNode::DRM) {
            if (status == "connected") {
                return true;
            }
            if (status == "disconnected") {
                return false;
            }
        } else {
            if (status == "0") {
                return true;
            }
            if (status == "1" || status == "4") {
                return false;
            }
        }
    }

    FILE* pipe = popen("dumpsys power 2>/dev/null | grep -q 'Display Power: state=ON'", "r");
    if (!pipe) {
        return true;
    }
    int res = pclose(pipe);
    return (res == 0);
}