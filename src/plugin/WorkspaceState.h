#pragma once

#include <string>

namespace nppthemes::plugin {

struct WorkspaceSnapshot {
    bool menuHidden{false};
    bool toolbarHidden{false};
    bool tabbarHidden{false};
    bool statusbarHidden{false};
};

struct WorkspaceSession {
    bool active{false};
    std::string mode;
    WorkspaceSnapshot previous;
};

} // namespace nppthemes::plugin
