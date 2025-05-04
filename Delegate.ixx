module;

#include <functional>
#include <format>
#include <plf_hive.h>

export module YT:Delegate;

import :Types;

namespace YT
{
    /**
     * @brief Forward declaration of the Delegate template class.
     * @tparam Signature The function signature type (e.g., void(int, float))
     */
    export template <typename Signature>
    class Delegate;

    /**
     * @brief Forward declaration of the Delegate template specialization.
     * @tparam ReturnType The return type of the delegate function
     * @tparam Args The argument types of the delegate function
     */
    export template <typename ReturnType, typename... Args>
    class Delegate<ReturnType(Args...)>;

    /**
     * @brief A unique identifier generator for delegates.
     * 
     * This class provides thread-safe generation of unique IDs for delegate instances.
     */
    class DelegateId
    {
    public:
        /**
         * @brief Generates a new unique ID for a delegate.
         * @return A unique 64-bit identifier.
         */
        static uint64_t GetNewId() noexcept
        {
            static uint64_t id = 0;
            return ++id;
        }
    };

    /**
     * @brief A handle to a bound delegate callback.
     * 
     * Used to manage the lifetime of delegate bindings and check their validity.
     * Provides a way to unbind callbacks and verify their existence.
     */
    export class DelegateHandle
    {
    private:
        /**
         * @brief Private constructor used by Delegate to create handles.
         * @param element_ptr Pointer to the callback data
         * @param element_id Unique ID of the callback
         * @param delegate_id ID of the owning delegate
         */
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

        /**
         * @brief Checks if the handle is valid.
         * @return true if the handle is valid, false otherwise.
         */
        operator bool() const noexcept
        {
            return m_DelegateId != 0;
        }

    private:
        template <typename Signature>
        friend class Delegate;

        void * m_ElementPtr = nullptr;    ///< Pointer to the callback data
        uint64_t m_ElementId = 0;        ///< Unique ID of the callback
        uint64_t m_DelegateId = 0;       ///< ID of the owning delegate
    };

    /**
     * @brief A type-safe delegate system for callback management.
     * 
     * This class provides a flexible and type-safe way to manage callbacks with support for:
     * - Both void and non-void return types
     * - Optional validity checking
     * - Multiple callbacks for void delegates
     * - Single callback for non-void delegates
     * - Thread-safe ID generation
     * 
     * @tparam ReturnType The return type of the delegate function
     * @tparam Args The argument types of the delegate function
     */
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

        /**
         * @brief Binds a callback with a validity check.
         * @tparam ValidityCheck Type of the validity check function
         * @tparam Callback Type of the callback function
         * @param validity_check Function that returns true if the callback is valid
         * @param callback The callback function to bind
         * @return A handle to the bound callback
         */
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
                CallbackData & new_elem = m_Callbacks.emplace(
                    CallbackData
                    {
                        .m_ElementId = DelegateId::GetNewId(),
                        .m_ValidityChecker = validity_check,
                        .m_Callback = std::forward<Callback>(callback),
                    });

                return DelegateHandle(&new_elem, new_elem.m_ElementId, m_Id);
            }
        }

        /**
         * @brief Binds a callback without validity checking.
         * @tparam Callback Type of the callback function
         * @param callback The callback function to bind
         * @return A handle to the bound callback
         */
        template <typename Callback>
        DelegateHandle BindNoValidityCheck(Callback && callback) noexcept
        {
            return BindWithValidityCheck([]{ return true; }, std::forward<Callback>(callback));
        }

        /**
         * @brief Binds a callback with a handle-based validity check.
         * @tparam Handle Type of the handle
         * @tparam Callback Type of the callback function
         * @param handle The handle to check for validity
         * @param callback The callback function to bind
         * @return A handle to the bound callback
         */
        template <typename Handle, typename Callback>
        DelegateHandle BindWithHandle(Handle && handle, Callback && callback) noexcept
        {
            return BindWithValidityCheck([handle]{ return handle; }, std::forward<Callback>(callback));
        }

        /**
         * @brief Unbinds a callback using its handle.
         * @param handle The handle of the callback to unbind
         */
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

        /**
         * @brief Unbinds all callbacks.
         */
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

        /**
         * @brief Executes the delegate with a default return value if no callback is bound.
         * @tparam ReturnTypeOpt The type of the default return value
         * @param DefaultReturn The default return value
         * @param args The arguments to pass to the callback
         * @return The result of the callback or the default value
         */
        template <typename ReturnTypeOpt = ReturnType> ReturnType ExecuteWithDefault(
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

        /**
         * @brief Executes the delegate.
         * @param args The arguments to pass to the callback
         * @return The result of the callback
         * @throws Exception if no callback is bound and ReturnType is not default constructible
         */
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

        /**
         * @brief Executes the delegate using the function call operator.
         * @param args The arguments to pass to the callback
         * @return The result of the callback
         */
        ReturnType operator()(Args && ... args)
        {
            return Execute(std::forward<Args>(args)...);
        }

    private:
        /**
         * @brief Removes invalid callbacks from the delegate.
         * 
         * For void delegates, removes all callbacks that fail their validity check.
         * For non-void delegates, removes the callback if it fails its validity check.
         */
        void Trim() noexcept
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                for (auto itr = m_Callbacks.begin(); itr != m_Callbacks.end();)
                {
                    CallbackData & callback_data = *itr;
                    if (!callback_data.m_ValidityChecker())
                    {
                        itr = m_Callbacks.erase(itr);
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
        /**
         * @brief Data structure for storing callback information.
         */
        struct CallbackData
        {
            uint64_t m_ElementId = 0;                    ///< Unique ID of the callback
            std::function<bool()> m_ValidityChecker;     ///< Function to check callback validity
            std::function<ReturnType(Args...)> m_Callback; ///< The actual callback function
        };

        uint64_t m_Id = DelegateId::GetNewId();          ///< Unique ID of this delegate

        /// Storage for callbacks (hive for void delegates, optional for non-void)
        std::conditional_t<std::is_same_v<ReturnType, void>,
            plf::hive<CallbackData>, std::optional<CallbackData>> m_Callbacks;
    };
} 
