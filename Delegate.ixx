module;

#include <functional>
#include <format>
#include <plf_hive.h>

export module YT:Delegate;

import :Types;

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
    private:
        DelegateHandle(void * element_ptr, uint64_t element_id, uint64_t delegate_id)
            : m_ElementPtr(element_ptr)
            , m_ElementId(element_id)
            , m_DelegateId(delegate_id)
        {

        }

    public:
        DelegateHandle() noexcept = default;
        DelegateHandle(const DelegateHandle &) noexcept = default;
        DelegateHandle(DelegateHandle &&) noexcept = default;

        DelegateHandle &operator=(const DelegateHandle &) noexcept = default;
        DelegateHandle &operator=(DelegateHandle &&) noexcept = default;

        ~DelegateHandle() noexcept = default;

        operator bool() const noexcept
        {
            return m_DelegateId != 0;
        }

    private:

        template <typename Signature>
        friend class Delegate;

        void * m_ElementPtr = nullptr;
        uint64_t m_ElementId = 0;
        uint64_t m_DelegateId = 0;
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

        template <typename ValidityCheck, typename Callback>
        DelegateHandle BindWithValidityCheck(ValidityCheck && validity_check, Callback && callback) noexcept
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                auto itr = m_Callbacks.insert(
                    CallbackData
                    {
                        .m_ElementId = DelegateId::GetNewId(),
                        .m_ValidityChecker = validity_check,
                        .m_Callback = std::forward<Callback>(callback),
                    });

                return DelegateHandle(&(*itr), itr->m_ElementId, m_Id);
            }
            else
            {
                m_Callbacks.emplace(
                    CallbackData
                    {
                        .m_ElementId = DelegateId::GetNewId(),
                        .m_ValidityChecker = validity_check,
                        .m_Callback = std::forward<Callback>(callback),
                    });

                return DelegateHandle(&m_Callbacks.value(), m_Callbacks->m_ElementId, m_Id);
            }

        }

        template <typename Callback>
        DelegateHandle BindNoValidityCheck(Callback && callback)
        {
            return BindWithValidityCheck([]{ return true; }, std::forward<Callback>(callback));
        }

        template <typename Handle, typename Callback>
        DelegateHandle BindWithHandle(Handle && handle, Callback && callback) noexcept
        {
            return BindWithValidityCheck([handle]{ return handle; }, std::forward<Callback>(callback));
        }

        void Unbind(DelegateHandle handle) noexcept
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                auto itr = m_Callbacks.get_iterator(static_cast<CallbackData *>(handle.m_ElementPtr));
                if (itr != m_Callbacks.end())
                {
                    if (handle.m_DelegateId == m_Id && handle.m_ElementId == itr->m_ElementId)
                    {
                        m_Callbacks.erase(itr);
                    }
                }
            }
            else
            {
                if (!m_Callbacks.has_value())
                {
                    return;
                }

                CallbackData & data = m_Callbacks.value();
                if (handle.m_DelegateId == m_Id && handle.m_ElementId == data.m_ElementId)
                {
                    m_Callbacks.reset();
                }
            }
        }

        void UnbindAll() noexcept
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                m_Callbacks.clear();
            }
            else
            {
                m_Callbacks.reset();
            }
        }

        template <typename ReturnTypeOpt = ReturnType> ReturnTypeOpt ExecuteWithDefault(
            ReturnTypeOpt && DefaultReturn, Args &&... args)
            requires (!std::is_same_v<ReturnType, void> && std::is_convertible_v<ReturnTypeOpt, ReturnType>)
        {
            Trim();

            if (!m_Callbacks.has_value())
            {
                return DefaultReturn;
            }

            return m_Callbacks->m_Callback(std::forward<Args>(args)...);
        }

        ReturnType Execute(Args &&... args)
        {
            Trim();

            if constexpr (std::is_same_v<ReturnType, void>)
            {
                for (CallbackData & callback_data : m_Callbacks)
                {
                    if (callback_data.m_ValidityChecker())
                    {
                        callback_data.m_Callback(std::forward<Args>(args)...);
                    }
                }

                return;
            }
            else
            {
                if (!m_Callbacks.has_value())
                {
                    if constexpr (std::is_default_constructible_v<ReturnType>)
                    {
                        return {};
                    }
                    else
                    {
                        FatalPrint("Callback list is empty and cannot create default return value");
                        throw Exception("Delegate callback list is empty");
                    }
                }

                return m_Callbacks->m_Callback(std::forward<Args>(args)...);
            }
        }

        ReturnType operator()(Args && ... args)
        {
            return Execute(std::forward<Args>(args)...);
        }

    private:

        void Trim() noexcept
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                for (auto itr = m_Callbacks.begin(); itr != m_Callbacks.end();)
                {
                    if (!itr->m_ValidityChecker())
                    {
                        m_Callbacks.erase(itr++);
                    }
                    else
                    {
                        ++itr;
                    }
                }
            }
            else
            {
                if (m_Callbacks.has_value())
                {
                    if (!m_Callbacks->m_ValidityChecker())
                    {
                        m_Callbacks.reset();
                    }
                }
            }
        }
    private:

        struct CallbackData
        {
            uint64_t m_ElementId = 0;
            std::function<bool()> m_ValidityChecker;
            std::function<ReturnType(Args...)> m_Callback;
        };

        uint64_t m_Id = DelegateId::GetNewId();

        std::conditional_t<std::is_same_v<ReturnType, void>,
            plf::hive<CallbackData>, std::optional<CallbackData>> m_Callbacks;
    };
}
