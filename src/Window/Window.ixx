module;

#include <atomic>

export module YT:Window;

import :Types;
import :Widget;
import :WindowTypes;
import :Delegate;

namespace YT
{
    export class WindowHandle
    {
    public:
        explicit WindowHandle(WindowHandleData data) noexcept;

        operator bool() const noexcept;

    private:
        WindowHandleData m_Data;
    };

    export class WindowRef final
    {
    public:
        explicit WindowRef(const WindowInitInfo & init_info) noexcept;

        WindowRef() noexcept = default;
        WindowRef(const WindowRef &) noexcept = delete;
        WindowRef(WindowRef &&) noexcept = default;
        WindowRef &operator=(const WindowRef &) noexcept = delete;
        WindowRef &operator=(WindowRef &&) noexcept = default;

        ~WindowRef();

        operator bool() const noexcept;

        bool SetContent(const WidgetRef<WidgetBase> & widget_ref) noexcept;
        bool SetContent(WidgetRef<WidgetBase> && widget_ref) noexcept;

        OptionalPtr<Delegate<bool()>> OnCloseCallback() noexcept;

        void CloseWindow() noexcept;

    private:

        WindowHandleData m_WindowHandle;
    };
}

