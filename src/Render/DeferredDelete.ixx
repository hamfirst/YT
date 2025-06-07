module;

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

module YT:DeferredDelete;

import :Types;

namespace YT
{
    /**
     * @brief A type-erased container for deferred deletion of objects.
     * 
     * This class provides a fixed-size buffer to store objects of any type
     * and ensures their proper destruction when the container is destroyed.
     * The buffer size is optimized to avoid false sharing.
     */
    class DeferredDelete
    {
    public:
        DeferredDelete() = default;

        static constexpr size_t MaxDataSize = std::hardware_constructive_interference_size - sizeof(void *) * 2;

        /**
         * @brief Constructs a DeferredDelete with the given object.
         * @tparam T The type of object to store
         * @param t The object to store
         */
        template <typename T>
        explicit DeferredDelete(T && t) noexcept requires (
            !std::is_same_v<T, DeferredDelete> && 
            std::is_move_constructible_v<T> &&
            sizeof(T) <= MaxDataSize &&
            alignof(T) <= alignof(std::max_align_t)
        )
        {
            new (m_Data) T(std::forward<T>(t));
            m_Mover = [](void * data, void * dest)
            {
                new (dest) T(std::move(*static_cast<T*>(data)));
            };

            m_Deleter = [](void * data)
            {
                static_cast<T*>(data)->~T();
            };
        }

        DeferredDelete(const DeferredDelete &) = delete;
        
        /**
         * @brief Move constructor.
         * @param rhs The DeferredDelete to move from
         */
        DeferredDelete(DeferredDelete && rhs) noexcept
        {
            if (rhs.m_Deleter)
            {
                m_Deleter = rhs.m_Deleter;
                m_Mover = rhs.m_Mover;
                rhs.m_Mover(rhs.m_Data, m_Data);
                rhs.m_Deleter(rhs.m_Data);
                rhs.m_Deleter = nullptr;
                rhs.m_Mover = nullptr;
            }
        }

        DeferredDelete & operator=(const DeferredDelete &) = delete;
        
        /**
         * @brief Move assignment operator.
         * @param rhs The DeferredDelete to move from
         * @return Reference to this DeferredDelete
         */
        DeferredDelete & operator=(DeferredDelete && rhs) noexcept
        {
            if (this != &rhs)
            {
                if (m_Deleter)
                {
                    m_Deleter(m_Data);
                }

                if (rhs.m_Mover)
                {
                    m_Deleter = rhs.m_Deleter;
                    m_Mover = rhs.m_Mover;
                    rhs.m_Mover(rhs.m_Data, m_Data);
                    rhs.m_Deleter(rhs.m_Data);
                    rhs.m_Deleter = nullptr;
                    rhs.m_Mover = nullptr;
                }
                else
                {
                    m_Deleter = nullptr;
                    m_Mover = nullptr;
                }
            }
            return *this;
        }

        /**
         * @brief Checks if the DeferredDelete contains an object.
         * @return true if an object is stored, false otherwise
         */
        operator bool() const noexcept
        {
            return m_Deleter != nullptr;
        }

        /**
         * @brief Destructor that ensures proper cleanup of stored object.
         */
        ~DeferredDelete()
        {
            if (m_Deleter)
            {
                m_Deleter(m_Data);
            }
        }

    private:
        /// Buffer sized to avoid false sharing
        std::byte m_Data[MaxDataSize] = {};
        /// Function pointer for type-erased deletion
        void (*m_Deleter)(void*) = nullptr;
        /// Function pointer for type-erased moving
        void (*m_Mover)(void *, void *) = nullptr;
    };
}
