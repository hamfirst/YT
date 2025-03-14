module;

#include <functional>
#include <plf_hive.h>

import YT.Types;

export module YT.Delegate;

namespace YT
{
    export template <typename Signature>
    class Delegate;

    export template <typename ReturnType, typename... Args>
    class Delegate<ReturnType(Args...)>;

    class DelegateId
    {
    public:
        static uint64_t GetNewId() noexcept
        {
            static uint64_t id = 0;
            return ++id;
        }
    };

    export class DelegateHandle
    {
    public:
        DelegateHandle() noexcept = default;
        DelegateHandle(const DelegateHandle &) noexcept = default;
        DelegateHandle(DelegateHandle &&) noexcept = default;

        DelegateHandle &operator=(const DelegateHandle &) noexcept = default;
        DelegateHandle &operator=(DelegateHandle &&) noexcept = default;

        ~DelegateHandle() noexcept = default;

        operator bool() const noexcept
        {
            return m_Id != 0;
        }

    private:

        template <typename Signature>
        friend class Delegate;

        size_t m_Index = 0;
        uint64_t m_Id = 0;
    };

    export template <typename ReturnType, typename... Args>
    class Delegate<ReturnType(Args...)> final
    {
    public:
        Delegate() noexcept = default;
        Delegate(const Delegate &) = delete;
        Delegate(Delegate &&) noexcept = default;

        Delegate & operator=(const Delegate &) = delete;
        Delegate & operator=(Delegate &&) noexcept = default;

        ~Delegate() noexcept = default;

    private:

        struct CallbackData
        {
            std::function<bool()> m_ValidityChecker;
            std::function<ReturnType(Args...)> m_Callback;
        };

        uint64_t m_Id = DelegateId::GetNewId();
        plf::hive<CallbackData> m_Callbacks;
    };
}
