#ifndef __C_WINDOW_XCB_H_INCLUDED__
#define __C_WINDOW_XCB_H_INCLUDED__

#include <nbl/ui/IWindowXcb.h>
#include <xcb/xproto.h>

namespace nbl::ui
{

class CWindowManagerXcb;

class NBL_API2 CWindowXcb final : public IWindowXcb
{
	
public:

	CWindowXcb(core::smart_refctd_ptr<CWindowManagerXcb>&& winManager, SCreationParams&& params);
	~CWindowXcb();

	// Display* getDisplay() const override { return m_dpy; }
	xcb_window_t getXcbWindow() const override { return m_xcbWindow; }
	xcb_connection_t* getXcbConnection() const override {
		return m_dispatcher.m_connection;
	}

	virtual IClipboardManager* getClipboardManager() override {
		return nullptr;
	}
	virtual ICursorControl* getCursorControl() override {
		return nullptr;
	}
	virtual IWindowManager* getManager() override {
		return nullptr;
	}

	virtual bool setWindowSize_impl(uint32_t width, uint32_t height) override;
	virtual bool setWindowPosition_impl(int32_t x, int32_t y) override;
	virtual bool setWindowRotation_impl(bool landscape) override;
	virtual bool setWindowVisible_impl(bool visible) override;
	virtual bool setWindowMaximized_impl(bool maximized) override;

	virtual void setCaption(const std::string_view& caption) override;
	
	
private:
    CWindowXcb(core::smart_refctd_ptr<system::ISystem>&& sys, uint32_t _w, uint32_t _h, E_CREATE_FLAGS _flags);

	core::smart_refctd_ptr<CWindowManagerXcb> m_windowManager;
	xcb_atom_t m_WM_DELETE_WINDOW = 0;
	xcb_atom_t m_NET_WM_STATE = 0;
	xcb_atom_t m_NET_WM_STATE_FULLSCREEN = 0;
	xcb_atom_t m_NET_WM_STATE_MAXIMIZED_VERT = 0;
	xcb_atom_t m_NET_WM_STATE_MAXIMIZED_HORZ = 0;
	xcb_atom_t m_NET_MOVERESIZE_WINDOW = 0;
	xcb_atom_t m_NET_REQUEST_FRAME_EXTENTS = 0;
	xcb_atom_t m_NET_FRAME_EXTENTS = 0;
	xcb_atom_t m_NET_WM_PID = 0;
	xcb_atom_t m_MW_PROTOCOL = 0;
	xcb_window_t m_xcbWindow = 0;
	xcb_window_t m_xcbParentWindow = 0;

	static inline constexpr uint32_t CircularBufferSize = 256u;
    class CDispatchThread final : public system::IThreadHandler<CDispatchThread>
	{
	public:

		inline CDispatchThread(CWindowXcb& window);
		inline ~CDispatchThread() {
		}

		void init();
		void exit();
		void work(lock_t& lock);

		inline bool wakeupPredicate() const { return true; }
		inline bool continuePredicate() const { return true; }
	private:
		CWindowXcb& m_window;
		xcb_connection_t* m_connection = nullptr;
		friend class CWindowXcb;
	} m_dispatcher;
};

}

#endif
