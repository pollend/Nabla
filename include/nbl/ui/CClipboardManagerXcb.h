#ifndef _NBL_UI_C_CLIPBOARD_MANAGER_XCB_INCLUDED_
#define _NBL_UI_C_CLIPBOARD_MANAGER_XCB_INCLUDED_

#ifdef _NBL_PLATFORM_LINUX_

#include "nbl/ui/IClipboardManager.h"
#include "nbl/ui/XcbConnection.h"

namespace nbl::ui
{

class NBL_API2 CClipboardManagerXcb final : public IClipboardManager
{
		using base_t = IClipboardManager;
	public:
		inline CClipboardManagerXcb(core::smart_refctd_ptr<XcbConnection>&& connect): 
			m_xcbConnection(std::move(connect)) {}

		virtual std::string getClipboardText() override;
		virtual bool setClipboardText(const std::string_view& data) override;
	private:
		core::smart_refctd_ptr<XcbConnection> m_xcbConnection;

		XcbConnection::XCBAtomToken<core::StringLiteral("CLIPBOARD")> m_CLIPBOARD;
		XcbConnection::XCBAtomToken<core::StringLiteral("UTF8_STRING")> m_UTF8_STRING;
};

}

#endif

#endif