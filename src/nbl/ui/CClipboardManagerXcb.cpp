
#ifdef _NBL_PLATFORM_LINUX_

#include "nbl/ui/CClipboardManagerXcb.h"

namespace nbl::ui
{
    std::string CClipboardManagerXcb::getClipboardText() {
        // xcb_clipboard
        return "";
    }

    bool CClipboardManagerXcb::setClipboardText(const std::string_view& data) {
        
        return false;
    }
}

#endif