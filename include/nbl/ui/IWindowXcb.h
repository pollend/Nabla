#ifndef __NBL_I_WINDOW_XCB_H_INCLUDED__
#define __NBL_I_WINDOW_XCB_H_INCLUDED__

#include "nbl/ui/IWindow.h"

#ifdef _NBL_PLATFORM_LINUX_

#include <xcb/xcb.h>
namespace nbl::ui
{

class NBL_API2 IWindowXcb : public IWindow
{
    protected:
        virtual ~IWindowXcb() = default;
        inline IWindowXcb(SCreationParams&& params) : IWindow(std::move(params)) {}

    public:
        using IWindow::IWindow;

        const void* getNativeHandle() const { return nullptr; }
        virtual xcb_window_t getXcbWindow() const = 0;
        virtual xcb_connection_t* getXcbConnection() const = 0;

        virtual bool setWindowSize_impl(uint32_t width, uint32_t height) = 0;
        virtual bool setWindowPosition_impl(int32_t x, int32_t y) = 0;
        virtual bool setWindowRotation_impl(bool landscape) = 0;
        virtual bool setWindowVisible_impl(bool visible) = 0;
        virtual bool setWindowMaximized_impl(bool maximized) = 0;
};

}

#endif

#endif