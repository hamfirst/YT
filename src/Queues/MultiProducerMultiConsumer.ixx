module;

//import_std

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

export module YT:MultiProducerMultiConsumer;

namespace YT
{
    export template <typename T, std::size_t Capacity>
    class MultiProducerMultiConsumer
    {
        static_assert(Capacity > 0, "MultiProducerMultiConsumer requires Capacity > 0");

    public:
        MultiProducerMultiConsumer() noexcept
        {
            for (std::size_t i = 0; i < Capacity; ++i)
            {
                m_Slots[i].m_Sequence.store(i, std::memory_order_relaxed);
            }
        }

        MultiProducerMultiConsumer(const MultiProducerMultiConsumer &) = delete;
        MultiProducerMultiConsumer(MultiProducerMultiConsumer &&) = delete;

        MultiProducerMultiConsumer & operator = (const MultiProducerMultiConsumer &) = delete;
        MultiProducerMultiConsumer & operator = (MultiProducerMultiConsumer &&) = delete;

        ~MultiProducerMultiConsumer() noexcept
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
            std::size_t position = m_EnqueuePos.load(std::memory_order_relaxed);

            while (true)
            {
                Slot & slot = m_Slots[position % Capacity];
                const std::size_t sequence = slot.m_Sequence.load(std::memory_order_acquire);

                if (sequence != position)
                {
                    return false;
                }

                if (m_EnqueuePos.compare_exchange_weak(position, position + 1, std::memory_order_acq_rel))
                {
                    new (SlotPtr(slot)) T(std::forward<Args>(args)...);
                    // Sequence numbers advance monotonically and act as version tags per slot,
                    // so reused slots are distinguished and ABA-style confusion is avoided.
                    slot.m_Sequence.store(position + 1, std::memory_order_release);
                    return true;
                }
            }
        }

        [[nodiscard]] bool TryDequeue(T & out) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                        std::is_nothrow_move_constructible_v<T>)
        {
            std::size_t position = m_DequeuePos.load(std::memory_order_relaxed);

            while (true)
            {
                Slot & slot = m_Slots[position % Capacity];
                const std::size_t sequence = slot.m_Sequence.load(std::memory_order_acquire);

                if (sequence != position + 1)
                {
                    return false;
                }

                if (m_DequeuePos.compare_exchange_weak(position, position + 1, std::memory_order_acq_rel))
                {
                    T * value_ptr = SlotPtr(slot);
                    out = std::move(*value_ptr);
                    value_ptr->~T();
                    slot.m_Sequence.store(position + Capacity, std::memory_order_release);
                    return true;
                }
            }
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
            const std::size_t enqueue_pos = m_EnqueuePos.load(std::memory_order_acquire);
            const std::size_t dequeue_pos = m_DequeuePos.load(std::memory_order_acquire);
            return enqueue_pos - dequeue_pos;
        }

        static consteval std::size_t MaxSize() noexcept
        {
            return Capacity;
        }

        void Clear() noexcept
        {
            std::size_t dequeue_pos = m_DequeuePos.load(std::memory_order_relaxed);
            const std::size_t enqueue_pos = m_EnqueuePos.load(std::memory_order_relaxed);

            while (dequeue_pos != enqueue_pos)
            {
                Slot & slot = m_Slots[dequeue_pos % Capacity];
                SlotPtr(slot)->~T();
                slot.m_Sequence.store(dequeue_pos + Capacity, std::memory_order_relaxed);
                ++dequeue_pos;
            }

            m_DequeuePos.store(enqueue_pos, std::memory_order_relaxed);
        }

    private:
        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

        struct Slot
        {
            std::atomic<std::size_t> m_Sequence = 0;
            Storage m_Storage;
        };

        [[nodiscard]] static T * SlotPtr(Slot & slot) noexcept
        {
            return std::launder(reinterpret_cast<T*>(&slot.m_Storage));
        }

    private:
        alignas(64) std::atomic<std::size_t> m_EnqueuePos = 0;
        alignas(64) std::atomic<std::size_t> m_DequeuePos = 0;
        std::array<Slot, Capacity> m_Slots = {};
    };
}
