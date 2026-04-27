module;

//import_std
#include <utility>
#include <memory>

module YT:WindowImpl;

import :Types;
import :WindowTypes;
import :WindowManager;
import :Widget;
import :Delegate;
import :Window;
import :FileMapper;

namespace YT
{
    WindowHandle::WindowHandle(WindowHandleData data) noexcept
        : m_Data(data)
    {

    }

    WindowHandle::operator bool() const noexcept
    {
        return m_Data && g_WindowManager->IsValidWindow(m_Data);
    }

    WindowRef::WindowRef(const WindowInitInfo & init_info) noexcept
    {
        m_WindowHandle = g_WindowManager->CreateWindow(init_info);
    }

    WindowRef::WindowRef(WindowRef &&rhs) noexcept
    {
        m_WindowHandle = rhs.m_WindowHandle;
        rhs.m_WindowHandle = {};
    }

    WindowRef & WindowRef::operator=(WindowRef && rhs) noexcept
    {
        if (m_WindowHandle && g_WindowManager)
        {
            g_WindowManager->CloseWindow(m_WindowHandle);
        }

        m_WindowHandle = rhs.m_WindowHandle;
        rhs.m_WindowHandle = {};
        return *this;
    }

    WindowRef::~WindowRef()
    {
        if (m_WindowHandle && g_WindowManager)
        {
            g_WindowManager->CloseWindow(m_WindowHandle);
        }
    }

    WindowRef::operator bool() const noexcept
    {
        return g_WindowManager->IsValidWindow(m_WindowHandle);
    }

    bool WindowRef::SetContent(const WidgetRef<WidgetBase> & widget_ref) noexcept
    {
        return g_WindowManager->SetWindowContent(m_WindowHandle, widget_ref);
    }

    bool WindowRef::SetContent(WidgetRef<WidgetBase> && widget_ref) noexcept
    {
        return g_WindowManager->SetWindowContent(m_WindowHandle, std::move(widget_ref));
    }

    OptionalPtr<Delegate<bool()>> WindowRef::OnCloseCallback() noexcept
    {
        return g_WindowManager->GetOnCloseCallback(m_WindowHandle);
    }

    void WindowRef::CloseWindow() noexcept
    {
        g_WindowManager->CloseWindow(m_WindowHandle);
    }


}
