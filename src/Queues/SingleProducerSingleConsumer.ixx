module;

//import_std

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

export module YT:SingleProducerSingleConsumer;

namespace YT
{
    export template <typename T, std::size_t Capacity>
    class SingleProducerSingleConsumer
    {
        static_assert(Capacity > 0, "SingleProducerSingleConsumer requires Capacity > 0");

    public:
        SingleProducerSingleConsumer() noexcept = default;
        SingleProducerSingleConsumer(const SingleProducerSingleConsumer &) = delete;
        SingleProducerSingleConsumer(SingleProducerSingleConsumer &&) = delete;

        SingleProducerSingleConsumer & operator = (const SingleProducerSingleConsumer &) = delete;
        SingleProducerSingleConsumer & operator = (SingleProducerSingleConsumer &&) = delete;

        ~SingleProducerSingleConsumer() noexcept
        {
            Clear();
        }

        [[nodiscard]] bool TryEnqueue(const T & value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            return Emplace(value);
        }

        [[nodiscard]] bool TryEnqueue(T && value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            return Emplace(std::move(value));
        }

        template <typename... Args>
        [[nodiscard]] bool Emplace(Args &&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            const std::size_t tail = m_Tail.load(std::memory_order_relaxed);
            const std::size_t head = m_Head.load(std::memory_order_acquire);

            if ((tail - head) == Capacity)
            {
                return false;
            }

            new (SlotPtr(tail)) T(std::forward<Args>(args)...);
            m_Tail.store(tail + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool TryDequeue(T & out) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                        std::is_nothrow_move_constructible_v<T>)
        {
            const std::size_t head = m_Head.load(std::memory_order_relaxed);
            const std::size_t tail = m_Tail.load(std::memory_order_acquire);

            if (head == tail)
            {
                return false;
            }

            T * value_ptr = SlotPtr(head);
            out = std::move(*value_ptr);
            value_ptr->~T();
            m_Head.store(head + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool Empty() const noexcept
        {
            return Size() == 0;
        }

        [[nodiscard]] bool Full() const noexcept
        {
            return Size() == Capacity;
        }

        [[nodiscard]] std::size_t Size() const noexcept
        {
            const std::size_t head = m_Head.load(std::memory_order_acquire);
            const std::size_t tail = m_Tail.load(std::memory_order_acquire);
            return tail - head;
        }

        static consteval std::size_t MaxSize() noexcept
        {
            return Capacity;
        }

        void Clear() noexcept
        {
            std::size_t head = m_Head.load(std::memory_order_relaxed);
            const std::size_t tail = m_Tail.load(std::memory_order_relaxed);

            while (head != tail)
            {
                SlotPtr(head)->~T();
                ++head;
            }

            m_Head.store(tail, std::memory_order_relaxed);
        }

    private:
        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

        [[nodiscard]] T * SlotPtr(std::size_t index) noexcept
        {
            return std::launder(reinterpret_cast<T*>(&m_Buffer[index % Capacity]));
        }

        [[nodiscard]] const T * SlotPtr(std::size_t index) const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(&m_Buffer[index % Capacity]));
        }

    private:
        alignas(64) std::atomic<std::size_t> m_Head = 0;
        alignas(64) std::atomic<std::size_t> m_Tail = 0;
        std::array<Storage, Capacity> m_Buffer = {};
    };
}
