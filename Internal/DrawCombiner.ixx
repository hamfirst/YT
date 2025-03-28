module;

#include <tuple>
#include <iterator>
#include <ranges>

export module YT:DrawCombiner;

import :Types;

namespace YT
{
    template <typename T>
    class DrawList final
    {
    public:

        template <typename DataType>
        void Emplace(DataType && data) noexcept
        {
            if (m_List.empty())
            {
                m_List.emplace_back(Vector<T> { std::forward<DataType>(data) });
            }
            else
            {
                m_List.back().emplace_back(std::forward<DataType>(data));
            }
        }

        void Append(DrawList && other) noexcept
        {
            if (m_List.empty())
            {
                m_List = std::move(other.m_List);
            }
            else
            {
                m_List.append_range(
                    std::make_move_iterator(other.m_List.begin()),
                    std::make_move_iterator(other.m_List.end()));
            }
        }

        void Clear() noexcept
        {
            m_List.clear();
        }

        [[nodiscard]] bool Empty() const noexcept
        {
            if (m_List.empty())
            {
                return true;
            }

            for (const Vector<T> & list : m_List)
            {
                if (!list.empty())
                {
                    return false;
                }
            }
            return true;
        }

        Vector<Span<const T>> Summarize() const noexcept
        {
            Vector<Span<const T>> spans;

            for (const Vector<T> & list : m_List)
            {
                spans.emplace_back(Span<const T>(list));
            }

            return spans;
        }

    private:
        Vector<Vector<T>> m_List;
    };

    template <typename ... Types>
    class DrawCombiner final
    {
    public:

        template <typename Type>
        void PushData(Type && type) noexcept
        {
            using T = std::decay_t<Type>;
            std::get<DrawList<T>>(m_Data).Emplace(std::forward<Type>(type));
        }

        [[nodiscard]] bool NeedsFlush() const noexcept
        {
            bool needs_flush = false;
            std::apply([&]<typename ListType>(ListType & list)
            {
                if (!list.Empty())
                {
                    needs_flush = true;
                }
            }, m_Data);

            return needs_flush;
        }

        template <typename Type>
        [[nodiscard]] bool NeedsFlushForNewData() const noexcept
        {
            bool needs_flush = false;
            std::apply([&]<typename ListType>(ListType & list)
            {
                if (!IsListSame<Type>(list) && !list.Empty())
                {
                    needs_flush = true;
                }
            }, m_Data);

            return needs_flush;
        }

        template <typename FlushFunc>
        void Flush(FlushFunc && flush_func) noexcept
        {
            std::apply([&]<typename ListType>(ListType & list)
            {
                if (!list.Empty())
                {
                    flush_func(list);
                    list.Clear();
                }
            }, m_Data);
        }

        void Combine(DrawCombiner && other) noexcept
        {
            std::apply([&]<typename ListType>(ListType & list)
            {
                auto & other_list = other.m_Data.template get<ListType>();
                list.Append(std::move(other_list));
            });
        }

    private:

        template <typename Type, typename VecType>
        static constexpr bool IsListSame(const DrawList<VecType>& vec)
        {
            return std::is_same_v<Type, VecType>;
        }

        std::tuple<DrawList<Types> ...> m_Data;
    };
}
