module;

#include <cstddef>
#include <cstdlib>
#include <cstring>

export module YT:OwnedBuffer;

import :Types;

namespace YT
{
    /**
     * Heap-owned copy of a byte range. Frees the allocation in the destructor.
     */
    export class OwnedBuffer final
    {
    public:
        OwnedBuffer() noexcept = default;

        /** Allocates and copies `span`; on allocation failure the buffer is left empty. */
        explicit OwnedBuffer(const Span<const std::byte> & span) noexcept
        {
            if (span.empty())
            {
                return;
            }

            m_Data = static_cast<std::byte *>(std::malloc(span.size()));
            if (m_Data == nullptr)
            {
                return;
            }

            std::memcpy(m_Data, span.data(), span.size());
            m_Size = span.size();
        }

        OwnedBuffer(OwnedBuffer && rhs) noexcept
            : m_Data(rhs.m_Data)
            , m_Size(rhs.m_Size)
        {
            rhs.m_Data = nullptr;
            rhs.m_Size = 0;
        }

        OwnedBuffer & operator=(OwnedBuffer && rhs) noexcept
        {
            if (this != &rhs)
            {
                std::free(m_Data);
                m_Data = rhs.m_Data;
                m_Size = rhs.m_Size;
                rhs.m_Data = nullptr;
                rhs.m_Size = 0;
            }

            return *this;
        }

        OwnedBuffer(const OwnedBuffer &) = delete;
        OwnedBuffer & operator=(const OwnedBuffer &) = delete;

        ~OwnedBuffer() noexcept
        {
            std::free(m_Data);
            m_Data = nullptr;
            m_Size = 0;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_Data == nullptr || m_Size == 0;
        }

        [[nodiscard]] Span<const std::byte> AsSpan() const noexcept
        {
            return { m_Data, m_Size };
        }

        explicit operator bool() const noexcept
        {
            return !empty();
        }

    private:
        std::byte * m_Data = nullptr;
        std::size_t m_Size = 0;
    };
}
