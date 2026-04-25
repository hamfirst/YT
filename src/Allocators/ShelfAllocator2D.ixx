
module;

#include <cstdint>
#include <algorithm>

export module YT:ShelfAllocator2D;

import glm;

import :Types;

namespace YT
{
    /**
     * @brief Simple 2D shelf allocator for packing axis-aligned rectangles into a fixed atlas.
     *
     * Allocations grow left-to-right along a horizontal shelf. When the current shelf cannot
     * fit the next rectangle's width, a new shelf starts at y = previous shelf top + previous
     * shelf height. There is no free list or deallocation; this is suitable for offline or
     * append-only atlas layouts (e.g. texture regions, UI tiles).
     */
    export class ShelfAllocator2D final
    {
    public:
        /**
         * @brief Constructs an allocator for a [0, width) x [0, height) pixel region.
         *
         * @param width  Atlas width in pixels. Allocations must satisfy origin.x + w <= width.
         * @param height Atlas height in pixels. Allocations must satisfy origin.y + h <= height.
         */
        ShelfAllocator2D(std::uint32_t width, std::uint32_t height) noexcept
            : m_Width(width), m_Height(height)
        {

        }

        /**
         * @brief Reserves a rectangle of the given size and returns its top-left corner.
         *
         * Coordinates are integer pixel origins cast to float for use with glm::vec2.
         * Placement is greedy: first fit on the current shelf, else advance to a new shelf.
         *
         * @param width  Requested rectangle width in pixels.
         * @param height Requested rectangle height in pixels.
         * @return Top-left (x, y) if the rectangle fits; empty optional if it does not.
         */
        Optional<glm::vec2> Allocate(std::uint32_t width, std::uint32_t height) noexcept
        {
            if (m_ShelfY + height > m_Height)
            {
                return {};
            }

            if (m_ShelfX + width > m_Width)
            {
                std::uint32_t new_x = 0;
                std::uint32_t new_y = m_ShelfY + m_ShelfHeight;

                if (new_x + width > m_Width)
                {
                    return {};
                }

                if (new_y + height > m_Height)
                {
                    return {};
                }

                m_ShelfX = new_x;
                m_ShelfY = new_y;

                m_ShelfHeight = 0;
            }

            m_ShelfHeight = std::max(m_ShelfHeight, height);

            std::uint32_t x = m_ShelfX;
            std::uint32_t y = m_ShelfY;

            m_ShelfX += width;

            return glm::vec2{ static_cast<float>(x), static_cast<float>(y) };
        }

        /** @brief Returns the atlas width passed to the constructor. */
        std::uint32_t GetWidth() const noexcept { return m_Width; }
        /** @brief Returns the atlas height passed to the constructor. */
        std::uint32_t GetHeight() const noexcept { return m_Height; }

    private:

        std::uint32_t m_Width = 0;   ///< Atlas width.
        std::uint32_t m_Height = 0;  ///< Atlas height.

        std::uint32_t m_ShelfX = 0;       ///< Next allocation x on the current shelf.
        std::uint32_t m_ShelfY = 0;       ///< Current shelf top y.
        std::uint32_t m_ShelfHeight = 0;  ///< Max height of rectangles placed on the current shelf.
    };
}
