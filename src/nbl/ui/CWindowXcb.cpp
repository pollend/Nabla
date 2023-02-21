#include "nbl/core/decl/smart_refctd_ptr.h"
#include "nbl/ui/IWindowXcb.h"
#include <string>

#ifdef _NBL_PLATFORM_LINUX_


#include "nbl/system/DefaultFuncPtrLoader.h"

#include "nbl/ui/CWindowXcb.h"
#include "nbl/ui/CWindowManagerXcb.h"

#include <array>
#include <string_view>
#include <variant>

// xcb
#include <xcb/xcb.h>
#include <xcb/xproto.h>

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
        m_connection = xcb.pxcb_connect(nullptr, nullptr);
}

void CWindowXcb::CDispatchThread::work(lock_t& lock){
    if(m_quit) {
        return;
    }
    auto& xcb = m_window.m_windowManager->getXcbFunctionTable();

    if(auto event = xcb.pxcb_wait_for_event(m_connection)) {
        auto* eventCallback = m_window.getEventCallback();
        switch (event->response_type & ~0x80)
        {
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t* cne = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if(m_window.m_width != cne->width || m_window.m_height != cne->height) {
                    eventCallback->onWindowResized(&m_window, cne->width, cne->height);
                }
                eventCallback->onWindowMoved(&m_window, cne->x, cne->y);
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
                if(cme->data.data32[0] == m_window.m_WM_DELETE_WINDOW) {
                    xcb.pxcb_unmap_window(m_connection, m_window.m_xcbWindow);
                    xcb.pxcb_flush(m_connection);
                    m_window.m_xcbWindow = 0;
                    m_quit = true; // we need to quit the dispatch thread
                    eventCallback->onWindowClosed(&m_window);
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
    xcb.pxcb_disconnect(m_connection);
    m_connection = nullptr;
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

CWindowXcb::CWindowXcb(core::smart_refctd_ptr<CWindowManagerXcb>&& winManager, SCreationParams&& params):
    IWindowXcb(std::move(params)),
    m_windowManager(winManager),
    m_dispatcher(*this) {

    auto& xcb = m_windowManager->getXcbFunctionTable();
    m_xcbWindow = xcb.pxcb_generate_id(m_dispatcher.m_connection);
    
    // setup some shared state between the window and the dispatch thread
    m_NET_WM_STATE = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_WM_STATE");
    m_NET_WM_STATE_FULLSCREEN = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_WM_STATE_FULLSCREEN");
    m_NET_WM_STATE_MAXIMIZED_VERT = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_WM_STATE_MAXIMIZED_VERT");
    m_NET_WM_STATE_MAXIMIZED_HORZ = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_WM_STATE_MAXIMIZED_HORZ");
    m_NET_MOVERESIZE_WINDOW = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_MOVERESIZE_WINDOW");
    m_NET_REQUEST_FRAME_EXTENTS = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_REQUEST_FRAME_EXTENTS");
    m_NET_FRAME_EXTENTS = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_FRAME_EXTENTS");
    m_NET_WM_PID = getAtomToken(xcb, m_dispatcher.m_connection,"_NET_WM_PID");

    m_MW_PROTOCOL = getAtomToken(xcb, m_dispatcher.m_connection,"WM_PROTOCOLS");
    m_WM_DELETE_WINDOW = getAtomToken(xcb, m_dispatcher.m_connection,"WM_DELETE_WINDOW");
    // m_NABLA_INTERNAL_CLOSE_NOTIFICATION = getAtomToken(xcb, m_dispatcher.m_connection,"NABLA_INTERNAL_CLOSE_NOTIFICATION");
    // create window

    const xcb_setup_t *xcbSetup = xcb.pxcb_get_setup(m_dispatcher.m_connection);
    xcb_screen_t *screen = xcb.pxcb_setup_roots_iterator(xcbSetup).data;
    m_xcbParentWindow = screen->root;

    uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    const uint32_t interestedEvents = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_DESTROY_WINDOW;
    uint32_t valueList[] = { screen->black_pixel, interestedEvents };

    xcb_void_cookie_t create_cookie = xcb.pxcb_create_window_checked(
        m_dispatcher.m_connection, XCB_COPY_FROM_PARENT, m_xcbWindow, screen->root,
        static_cast<int16_t>(m_x),
        static_cast<int16_t>(m_y),
        static_cast<int16_t>(m_width),
        static_cast<int16_t>(m_height), 4,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, eventMask,
        valueList);
    bool check = checkXcbCookie(xcb, m_dispatcher.m_connection, create_cookie);
    assert(check);

    auto atoms = std::array<xcb_atom_t, 1>{ m_WM_DELETE_WINDOW};
    xcb.pxcb_change_property(m_dispatcher.m_connection, XCB_PROP_MODE_REPLACE, m_xcbWindow, m_MW_PROTOCOL, 4, 32, atoms.size(), reinterpret_cast<const void* const>(atoms.data()));

    xcb.pxcb_map_window_checked(m_dispatcher.m_connection, m_xcbWindow);
    xcb.pxcb_flush(m_dispatcher.m_connection);

    m_dispatcher.start();
}

bool CWindowXcb::setWindowSize_impl(uint32_t width, uint32_t height) {
    auto& xcb = m_windowManager->getXcbFunctionTable();

    const uint32_t values[] = { width, height};
    xcb_void_cookie_t cookie = xcb.pxcb_configure_window_checked(m_dispatcher.m_connection, m_xcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    bool check = checkXcbCookie(xcb, m_dispatcher.m_connection, cookie);
    assert(check);
    xcb.pxcb_flush(m_dispatcher.m_connection);
    return  true;
}

bool CWindowXcb::setWindowPosition_impl(int32_t x, int32_t y) {
    auto& xcb = m_windowManager->getXcbFunctionTable();
    const int32_t values[] = { x, y };
    auto cookie = xcb.pxcb_configure_window_checked(m_dispatcher.m_connection, m_xcbWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    bool check = checkXcbCookie(xcb, m_dispatcher.m_connection, cookie);
    assert(check);
    xcb.pxcb_flush(m_dispatcher.m_connection);
    return true;
}

void CWindowXcb::setCaption(const std::string_view& caption) {
    auto& xcb = m_windowManager->getXcbFunctionTable();
    auto cookie = xcb.pxcb_change_property(m_dispatcher.m_connection, XCB_PROP_MODE_REPLACE, m_xcbWindow, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, static_cast<uint32_t>(caption.size()), reinterpret_cast<const void* const>(caption.data()));
    bool check = checkXcbCookie(xcb, m_dispatcher.m_connection, cookie);
    assert(check);
    xcb.pxcb_flush(m_dispatcher.m_connection);
}

bool CWindowXcb::setWindowRotation_impl(bool landscape) {
    return true;
}
bool CWindowXcb::setWindowVisible_impl( bool visible) {
    return true;
}
bool CWindowXcb::setWindowMaximized_impl(bool maximized) {
    auto& xcb = m_windowManager->getXcbFunctionTable();
    {
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.type = m_NET_WM_STATE;
        event.window = m_xcbWindow;
        event.format = 32;
        event.sequence = 0;
        event.data.data32[0] = maximized ? 1l : 0l;
        event.data.data32[1] = m_NET_WM_STATE_FULLSCREEN;
        event.data.data32[2] = 0;
        event.data.data32[3] = 1;
        event.data.data32[4] = 0;
        xcb_void_cookie_t result = xcb.pxcb_send_event(
            m_dispatcher.m_connection, 1, m_xcbParentWindow, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
            (const char*)&event);
        checkXcbCookie(xcb, m_dispatcher.m_connection, result);
    }
    if(!maximized) {
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.type = m_NET_WM_STATE;
        event.window = m_xcbWindow;
        event.format = 32;
        event.sequence = 0;
        event.data.data32[0] = m_NET_WM_STATE_MAXIMIZED_VERT;
        event.data.data32[1] = m_NET_WM_STATE_MAXIMIZED_HORZ;
        event.data.data32[2] = 0;
        event.data.data32[3] = 0;
        event.data.data32[4] = 0;
        xcb_void_cookie_t result = xcb.pxcb_send_event(
            m_dispatcher.m_connection, 1, m_xcbParentWindow, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
            (const char*)&event);
        checkXcbCookie(xcb, m_dispatcher.m_connection, result);
    }
    return true;
}

CWindowXcb::~CWindowXcb()
{
}

}

#endif