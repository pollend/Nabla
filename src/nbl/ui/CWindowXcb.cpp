#ifdef _NBL_PLATFORM_LINUX_

#include "nbl/core/decl/smart_refctd_ptr.h"
#include "nbl/core/string/StringLiteral.h"

#include "nbl/system/DefaultFuncPtrLoader.h"

#include "nbl/ui/IWindowXcb.h"
#include "nbl/ui/CWindowXcb.h"
#include "nbl/ui/CWindowManagerXcb.h"

#include <cstdint>
#include <string>
#include <array>
#include <string_view>
#include <variant>

namespace nbl::ui {

static xcb_atom_t getAtomToken(const Xcb& functionTable, xcb_connection_t* connection,const std::string_view view) {
    xcb_intern_atom_cookie_t cookie = functionTable.pxcb_intern_atom(connection, 0, view.size(), view.data());
    auto* reply = functionTable.pxcb_intern_atom_reply(connection, cookie, static_cast<xcb_generic_error_t**>(nullptr));
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

static bool checkXcbCookie(const Xcb& functionTable, xcb_connection_t* connection, xcb_void_cookie_t cookie) {
    if (xcb_generic_error_t* error = functionTable.pxcb_request_check(connection, cookie))
    {
        printf("XCB error: %d", error->error_code);
        return false;
    }
    return true;
}

CWindowXcb::CDispatchThread::CDispatchThread(CWindowXcb& window):
    m_window(window) {

        auto& xcb = m_window.m_windowManager->getXcbFunctionTable();

        // m_connection = xcb.pxcb_connect(nullptr, nullptr);
}

void CWindowXcb::CDispatchThread::work(lock_t& lock){
    if(m_quit) {
        return;
    }
    auto& xcb = m_window.m_windowManager->getXcbFunctionTable();
    
    
    auto MW_DELETE_WINDOW = m_window.m_xcbConnection->resolveXCBAtom(m_window.m_WM_DELETE_WINDOW);
    auto NET_WM_PING = m_window.m_xcbConnection->resolveXCBAtom(m_window.m_NET_WM_PING);
    
  
    if(auto event = xcb.pxcb_wait_for_event(m_window.m_xcbConnection->getConnection())) {
        auto* eventCallback = m_window.getEventCallback();
        switch (event->response_type & ~0x80)
        {
            case 0: {
                xcb_generic_error_t* error = reinterpret_cast<xcb_generic_error_t*>(event);
                printf("XCB error: %d", error->error_code);
                break;
            }
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t* cne = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if(m_window.m_width != cne->width || 
                    m_window.m_height != cne->height) {
                    eventCallback->onWindowResized(&m_window, cne->width, cne->height);
                }
                if(m_window.m_x != cne->x || 
                    m_window.m_y != cne->y) {
                    eventCallback->onWindowMoved(&m_window, cne->x, cne->y);
                }
                break;
            }
            case XCB_DESTROY_WINDOW: {
                xcb_destroy_window_request_t* dwr = reinterpret_cast<xcb_destroy_window_request_t*>(event);
                if(dwr->window == m_window.m_xcbWindow) {
                    m_quit = true;
                    eventCallback->onWindowClosed(&m_window);
                }
                break;
            }
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* cme = reinterpret_cast<xcb_client_message_event_t*>(event);
                if(cme->data.data32[0] == MW_DELETE_WINDOW) {
                    xcb.pxcb_unmap_window(m_window.getXcbConnection(), m_window.m_xcbWindow);
                    xcb.pxcb_destroy_window(m_window.getXcbConnection(), m_window.m_xcbWindow);
                    xcb.pxcb_flush(m_window.getXcbConnection());
                    m_window.m_xcbWindow = 0;
                    m_quit = true; // we need to quit the dispatch thread
                    eventCallback->onWindowClosed(&m_window);
                } else if(cme->data.data32[0] == NET_WM_PING && cme->window != m_window.m_xcbParentWindow) {
                    xcb_client_message_event_t ev = *cme;
                    ev.response_type = XCB_CLIENT_MESSAGE;
                    ev.window = m_window.m_xcbWindow;
                    ev.type = NET_WM_PING;
                    xcb.pxcb_send_event(m_window.getXcbConnection(), 0, m_window.m_xcbWindow, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char*>(&ev));
                    xcb.pxcb_flush(m_window.getXcbConnection());
                }
                break;
            }
        }
        free(event);
    }

}


void CWindowXcb::CDispatchThread::init()
{

}

void CWindowXcb::CDispatchThread::exit()
{
    auto& xcb = m_window.m_windowManager->getXcbFunctionTable();
    // xcb.pxcb_disconnect(m_connection);
    // m_connection = nullptr;
}

CWindowManagerXcb::CWindowManagerXcb() {
}

core::smart_refctd_ptr<IWindow> CWindowManagerXcb::createWindow(IWindow::SCreationParams&& creationParams)
{
    std::string title = std::string(creationParams.windowCaption);
    auto window = core::make_smart_refctd_ptr<CWindowXcb>(core::smart_refctd_ptr<CWindowManagerXcb>(this), std::move(creationParams));
    window->setCaption(title);
    return window;
}

bool CWindowManagerXcb::setWindowSize_impl(IWindow* window, uint32_t width, uint32_t height) {
    auto wnd = static_cast<IWindowXcb*>(window);
    wnd->setWindowSize_impl(width, height);
    return true;
}

bool CWindowManagerXcb::setWindowPosition_impl(IWindow* window, int32_t x, int32_t y) {
    auto wnd = static_cast<IWindowXcb*>(window);
    wnd->setWindowPosition_impl(x, y);
    return true;
}

bool CWindowManagerXcb::setWindowRotation_impl(IWindow* window, bool landscape) {
    auto wnd = static_cast<IWindowXcb*>(window);
    wnd->setWindowRotation_impl(landscape);
    return true;
}

bool CWindowManagerXcb::setWindowVisible_impl(IWindow* window, bool visible) {
    auto wnd = static_cast<IWindowXcb*>(window);
    wnd->setWindowVisible_impl(visible);
    return true;
}

bool CWindowManagerXcb::setWindowMaximized_impl(IWindow* window, bool maximized) {
    auto wnd = static_cast<IWindowXcb*>(window);
    wnd->setWindowMaximized_impl(maximized);
    return true;
}


CWindowXcb::CWindowXcb(core::smart_refctd_ptr<CWindowManagerXcb>&& winManager, SCreationParams&& params):
    IWindowXcb(std::move(params)),
    m_windowManager(winManager),
    m_xcbConnection(core::make_smart_refctd_ptr<XcbConnection>(core::smart_refctd_ptr<CWindowManagerXcb>(m_windowManager))),
    m_dispatcher(*this) {
    
    auto& xcb = m_windowManager->getXcbFunctionTable();
    auto& xcbIccm = m_windowManager->getXcbIcccmFunctionTable();

    m_xcbWindow = xcb.pxcb_generate_id(m_xcbConnection->getConnection());

    const xcb_setup_t *xcbSetup = xcb.pxcb_get_setup(m_xcbConnection->getConnection());
    xcb_screen_t *screen = xcb.pxcb_setup_roots_iterator(xcbSetup).data;
    m_xcbParentWindow = screen->root;

    uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t valueList[] = {
        screen->black_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE
    };

    xcb_void_cookie_t xcbCheckResult = xcb.pxcb_create_window(
        m_xcbConnection->getConnection(), XCB_COPY_FROM_PARENT, m_xcbWindow, screen->root,
        static_cast<int16_t>(m_x),
        static_cast<int16_t>(m_y),
        static_cast<int16_t>(m_width),
        static_cast<int16_t>(m_height), 4,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, eventMask,
        valueList);

    setWindowSize_impl(m_width, m_height);
    
    auto WM_DELETE_WINDOW = m_xcbConnection->resolveXCBAtom(m_WM_DELETE_WINDOW);
    auto NET_WM_PING = m_xcbConnection->resolveXCBAtom(m_NET_WM_PING);
    auto WM_PROTOCOLS = m_xcbConnection->resolveXCBAtom(m_WM_PROTOCOLS);
    
    const std::array atoms {WM_DELETE_WINDOW, NET_WM_PING};
    xcb.pxcb_change_property(
            m_xcbConnection->getConnection(), 
            XCB_PROP_MODE_REPLACE, 
            m_xcbWindow, 
            WM_PROTOCOLS, XCB_ATOM_ATOM, 32, atoms.size(), atoms.data());


    auto motifHints = fetchMotifMWHints(getFlags().value);
    m_xcbConnection->setMotifWmHints(m_xcbWindow, motifHints);
    
    if(isAlwaysOnTop()) {
        XcbConnection::XCBAtomToken<core::StringLiteral("NET_WM_STATE_ABOVE")> NET_WM_STATE_ABOVE;
        m_xcbConnection->setNetMWState(
            m_xcbParentWindow,
            m_xcbWindow, false, m_xcbConnection->resolveXCBAtom(NET_WM_STATE_ABOVE));
    }

    xcb.pxcb_map_window(m_xcbConnection->getConnection(), m_xcbWindow);
    xcb.pxcb_flush(m_xcbConnection->getConnection());
    m_dispatcher.start();
}

CWindowXcb::~CWindowXcb()
{
}

bool CWindowXcb::setWindowSize_impl(uint32_t width, uint32_t height) {
    auto& xcb = m_windowManager->getXcbFunctionTable();
    auto& xcbIccm = m_windowManager->getXcbIcccmFunctionTable();

    xcb_size_hints_t hints = {0};

    xcbIccm.pxcb_icccm_size_hints_set_size(&hints, true, width, height);
    if(!isResizable()) {
        xcbIccm.pxcb_icccm_size_hints_set_min_size(&hints, width, height);
        xcbIccm.pxcb_icccm_size_hints_set_max_size(&hints, width, height);
    }
    xcbIccm.pxcb_icccm_set_wm_normal_hints(m_xcbConnection->getConnection(), m_xcbWindow, &hints);
    return  true;
}

bool CWindowXcb::setWindowPosition_impl(int32_t x, int32_t y) {
    auto& xcb = m_windowManager->getXcbFunctionTable();

    const int32_t values[] = { x, y };
    auto cookie = xcb.pxcb_configure_window_checked(m_xcbConnection->getConnection(), m_xcbWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    bool check = checkXcbCookie(xcb, m_xcbConnection->getConnection(), cookie);
    xcb.pxcb_flush(m_xcbConnection->getConnection());
    assert(check);
    return true;
}

void CWindowXcb::setCaption(const std::string_view& caption) {
    auto& xcb = m_windowManager->getXcbFunctionTable();

    xcb.pxcb_change_property(m_xcbConnection->getConnection(), XCB_PROP_MODE_REPLACE, m_xcbWindow, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, static_cast<uint32_t>(caption.size()), reinterpret_cast<const void* const>(caption.data()));
    xcb.pxcb_flush(m_xcbConnection->getConnection());
}

bool CWindowXcb::setWindowRotation_impl(bool landscape) {
    return true;
}

bool CWindowXcb::setWindowVisible_impl( bool visible) {
    auto& xcb = m_windowManager->getXcbFunctionTable();

    if(visible) {
        xcb.pxcb_map_window(m_xcbConnection->getConnection(), m_xcbWindow);
        xcb.pxcb_flush(m_xcbConnection->getConnection());
    } else {
        xcb.pxcb_unmap_window(m_xcbConnection->getConnection(), m_xcbWindow);
        xcb.pxcb_flush(m_xcbConnection->getConnection());
    }
    return true;
}

bool CWindowXcb::setWindowMaximized_impl(bool maximized) {
    auto& xcb = m_windowManager->getXcbFunctionTable();
    
    m_xcbConnection->setNetMWState(
        m_xcbParentWindow,
            m_xcbWindow, maximized && !isBorderless(), m_xcbConnection->resolveXCBAtom(m_NET_WM_STATE_FULLSCREEN));

    m_xcbConnection->setNetMWState(
        m_xcbParentWindow,
            m_xcbWindow, maximized && isBorderless(), 
            m_xcbConnection->resolveXCBAtom(m_NET_WM_STATE_MAXIMIZED_VERT),
            m_xcbConnection->resolveXCBAtom(m_NET_WM_STATE_MAXIMIZED_HORZ));

    xcb.pxcb_flush(m_xcbConnection->getConnection());
    return true;
}

}

#endif