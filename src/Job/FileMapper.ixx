
module;

#include <cstddef>
#include <cstdint>

export module YT:FileMapper;

import :Types;

namespace YT
{
    export class MappedFile
    {
    public:
        MappedFile() = default;

        explicit MappedFile(const StringView & file_name) noexcept;

        MappedFile(const MappedFile&) = delete;
        MappedFile(MappedFile && rhs) noexcept;

        MappedFile & operator=(const MappedFile&) = delete;
        MappedFile & operator=(MappedFile && rhs) noexcept;

        ~MappedFile();

        [[nodiscard]] Span<const std::byte> GetData() const noexcept;

    private:

        int m_FD = -1;

        std::size_t m_Size = 0;
        void * m_Data = nullptr;
    };


    export void MapFile(const StringView & file_name,
        Function<void(MappedFile &&)> && callback);
    export void UpdateFileLoads();
    export void SyncAllFileLoads();
}
