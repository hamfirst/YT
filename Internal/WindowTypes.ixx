module;

#include <cstdint>

export module YT.WindowTypes;

namespace YT
{
    export class WindowHandleData final
    {
    public:
        constexpr WindowHandleData() noexcept
        {
            m_Index = 0;
            m_GUID = 0;
        }

        WindowHandleData(const WindowHandleData& other) noexcept = default;
        WindowHandleData(WindowHandleData&& other) noexcept = default;
        WindowHandleData& operator=(const WindowHandleData& other) noexcept = default;
        WindowHandleData& operator=(WindowHandleData&& other) noexcept = default;

        bool operator==(const WindowHandleData& other) const noexcept = default;
        bool operator!=(const WindowHandleData& other) const noexcept = default;

        operator bool() const noexcept
        {
            return m_GUID != 0;
        }

        static uint64_t ConvertToBits(WindowHandleData handle) noexcept
        {
            uint64_t * ptr = reinterpret_cast<uint64_t *>(&handle);
            return *ptr;
        }
        static void * ConvertToPtr(WindowHandleData handle) noexcept
        {
            void ** ptr = reinterpret_cast<void **>(&handle);
            return *ptr;
        }

        static WindowHandleData CreateFromBits(uint64_t bits) noexcept
        {
            WindowHandleData * handle = reinterpret_cast<WindowHandleData*>(&bits);
            return *handle;
        }

        static WindowHandleData CreateFromPtr(void * ptr) noexcept
        {
            WindowHandleData * handle = reinterpret_cast<WindowHandleData*>(&ptr);
            return *handle;
        }

        uint64_t m_Index : 10;
        uint64_t m_GUID : 54;
    };

    static_assert(sizeof(WindowHandleData) == sizeof(uint64_t), "YT::WindowHandle should be 64 bits long");

}