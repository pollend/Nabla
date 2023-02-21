#ifndef C_WINDOW_MANAGER_XCB
#define C_WINDOW_MANAGER_XCB

#ifdef _NBL_PLATFORM_LINUX_
#include "nbl/core/decl/Types.h"

#include "nbl/system/DefaultFuncPtrLoader.h"

#include "nbl/ui/IWindow.h"
#include "nbl/ui/IWindowXcb.h"
#include "nbl/ui/IWindowManager.h"
#include "nbl/ui/CWindowXcb.h"

#include <xcb/xcb.h>

#include <functional>
#include <memory>
#include <string>

namespace nbl::ui
{
	NBL_SYSTEM_DECLARE_DYNAMIC_FUNCTION_CALLER_CLASS(Xcb, system::DefaultFuncPtrLoader,
		xcb_destroy_window,
		xcb_generate_id,
		xcb_create_window,
		xcb_connect,
		xcb_disconnect,
		xcb_create_window_checked,
		xcb_map_window_checked,
		xcb_get_setup,
		xcb_setup_roots_iterator,
		xcb_flush,
		xcb_intern_atom,
		xcb_intern_atom_reply,
		xcb_unmap_window,
		xcb_get_property,
		xcb_get_property_reply,
		xcb_change_property,
		xcb_configure_window_checked,
		xcb_get_property_value,
		xcb_get_property_value_length,
		xcb_wait_for_event,
		xcb_send_event,
		xcb_request_check
	);

class CWindowManagerXcb : public IWindowManager
{
public:

	virtual bool setWindowSize_impl(IWindow* window, uint32_t width, uint32_t height) override {
		auto wnd = static_cast<IWindowXcb*>(window);
		wnd->setWindowSize_impl(width, height);
	}
	virtual bool setWindowPosition_impl(IWindow* window, int32_t x, int32_t y) override {
		auto wnd = static_cast<IWindowXcb*>(window);
		wnd->setWindowPosition_impl(x, y);
	}
	virtual bool setWindowRotation_impl(IWindow* window, bool landscape) override {
		auto wnd = static_cast<IWindowXcb*>(window);
		wnd->setWindowRotation_impl(landscape);
	}
	virtual bool setWindowVisible_impl(IWindow* window, bool visible) override {
		auto wnd = static_cast<IWindowXcb*>(window);
		wnd->setWindowVisible_impl(visible);
	}
	virtual bool setWindowMaximized_impl(IWindow* window, bool maximized) override {
		auto wnd = static_cast<IWindowXcb*>(window);
		wnd->setWindowMaximized_impl(maximized);
	}

	inline SDisplayInfo getPrimaryDisplayInfo() const override final {
		
		return SDisplayInfo();
	}

    CWindowManagerXcb();
    ~CWindowManagerXcb() override = default;

	virtual core::smart_refctd_ptr<IWindow> createWindow(IWindow::SCreationParams&& creationParams) override;

	const Xcb& getXcbFunctionTable() const { return m_xcb; }

	virtual void destroyWindow(IWindow* wnd) override final {}

private:
	Xcb m_xcb = Xcb("xcb"); // function tables
};


}
#endif
#endif