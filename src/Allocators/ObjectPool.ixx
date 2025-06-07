module;

#include <concepts>
#include <cassert>

export module YT:ObjectPool;

import :Types;
import :FixedBlockAllocator;

namespace YT
{
    export template <typename T>
    class ObjectPool;

    export template <typename T>
    class PooledObject
    {
        PooledObject(T * ptr, ObjectPool<T> * return_pool)
        {
            m_T = ptr;
            m_ReturnPool = return_pool;
        }

        friend class ObjectPool<T>;
    public:

        PooledObject() = default;

        PooledObject(const PooledObject &) = delete;
        PooledObject(PooledObject && rhs) noexcept
        {
            m_T = rhs.m_T;
            m_ReturnPool = rhs.m_ReturnPool;
            rhs.m_T = nullptr;
            rhs.m_ReturnPool = nullptr;
        }

        PooledObject& operator = (const PooledObject &) = delete;
        PooledObject& operator = (PooledObject && rhs) noexcept
        {
            if (this != &rhs)
            {
                if (m_T)
                {
                    m_ReturnPool->ReturnToPool(*m_T);
                }

                m_T = rhs.m_T;
                m_ReturnPool = rhs.m_ReturnPool;
                rhs.m_T = nullptr;
                rhs.m_ReturnPool = nullptr;
            }
            return *this;
        }

        ~PooledObject() noexcept
        {
            if (m_T)
            {
                m_ReturnPool->ReturnToPool(*m_T);
            }
        }

        T & operator * () noexcept
        {
            assert(m_T != nullptr);
            return *m_T;
        }

        const T & operator * () const noexcept
        {
            assert(m_T != nullptr);
            return *m_T;
        }

        T * operator -> () noexcept
        {
            assert(m_T != nullptr);
            return m_T;
        }

        const T * operator -> () const noexcept
        {
            assert(m_T != nullptr);
            return m_T;
        }

        explicit operator bool() const noexcept
        {
            return m_T != nullptr;
        }

    private:
        T * m_T = nullptr;
        ObjectPool<T> * m_ReturnPool = nullptr;
    };

    template <typename T>
    class ObjectPool
    {
    public:
        ObjectPool() noexcept = default;
        ObjectPool(const ObjectPool &) = delete;
        ObjectPool(ObjectPool &&) = delete;

        ObjectPool& operator = (const ObjectPool &) = delete;
        ObjectPool& operator = (ObjectPool &&) = delete;

        ~ObjectPool() noexcept
        {
            while (m_FreeList != nullptr)
            {
                auto* next = m_FreeList->m_Next;
                m_FreeList->m_Data.~T();
                m_Allocator.Free(m_FreeList);
                m_FreeList = next;
            }
        }

        template <typename Initializer, typename Reinitializer>
        PooledObject<T> GetFreeObject(Initializer && initializer, Reinitializer && reinitializer) noexcept
            requires std::constructible_from<T, std::invoke_result_t<Initializer>> &&
                std::invocable<Reinitializer, T &>
        {
            if(m_FreeList == nullptr)
            {
                ObjectPoolData * new_object = new (m_Allocator.Allocate()) ObjectPoolData
                {
                    initializer()
                };

                return PooledObject<T>(&new_object->m_Data, this);
            }

            ObjectPoolData * free_object = m_FreeList;
            m_FreeList = m_FreeList->m_Next;

            free_object->m_Next = nullptr;
            reinitializer(free_object->m_Data);
            return PooledObject<T>(&free_object->m_Data, this);
        }

        template <typename Initializer>
        PooledObject<T> GetFreeObject(Initializer && initializer) noexcept
            requires std::constructible_from<T, std::invoke_result_t<Initializer>>
        {
            return GetFreeObject<Initializer>(std::forward<Initializer>(initializer), [](T &){});
        }
    private:

        friend class PooledObject<T>;

        void ReturnToPool(T & object) noexcept
        {
            ObjectPoolData * free_object_pool_data = reinterpret_cast<ObjectPoolData*>(&object);

            assert(m_Allocator.Owns(free_object_pool_data));
            assert(free_object_pool_data->m_Next == nullptr);

            free_object_pool_data->m_Next = m_FreeList;
            m_FreeList = free_object_pool_data;
        }

    private:
        struct ObjectPoolData
        {
            T m_Data;
            ObjectPoolData * m_Next = nullptr;
        };

        FixedBlockAllocator<ObjectPoolData> m_Allocator;
        ObjectPoolData * m_FreeList = nullptr;

    };
}