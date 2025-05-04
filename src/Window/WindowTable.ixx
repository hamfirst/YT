module;

#include <cstdint>
#include <random>
#include <optional>

module YT:WindowTable;

import :Types;
import :WindowTypes;
import :WindowResource;

namespace YT
{
    export class WindowTable final
    {
    public:

        WindowHandleData AllocateWindowHandle(WindowResource && window_resource)
        {
            for(int index = m_LastFreeWindowIndex; index < MaxWindows; ++index)
            {
                if(m_Windows[index].m_GUID == 0)
                {
                    uint64_t randomness = m_RandomEngine();

                    WindowHandleData new_handle = {};
                    new_handle.m_GUID = randomness;
                    new_handle.m_Index = index;

                    m_Windows[index].m_GUID = new_handle.m_GUID;
                    m_Windows[index].m_WindowResource = std::move(window_resource);

                    m_LastFreeWindowIndex = index + 1;
                    m_NumWindows++;
                    return new_handle;
                }
            }

            throw Exception("WindowTable::AllocateWindowHandle failed - max window count exceeded");
        }

        bool ReleaseWindowHandle(WindowHandleData handle)
        {
            if(handle.m_GUID == 0)
            {
                return false;
            }

            if(handle.m_Index >= MaxWindows)
            {
                return false;
            }

            if(m_Windows[handle.m_Index].m_GUID != handle.m_GUID)
            {
                return false;
            }

            m_Windows[handle.m_Index].m_WindowResource.reset();
            m_Windows[handle.m_Index].m_GUID = 0;
            m_NumWindows--;

            return true;
        }

        [[nodiscard]] OptionalPtr<WindowResource> ResolveHandle(WindowHandleData handle)
        {
            if(handle.m_GUID == 0)
            {
                return nullptr;
            }

            if(handle.m_Index >= MaxWindows)
            {
                return nullptr;
            }

            if(m_Windows[handle.m_Index].m_GUID != handle.m_GUID)
            {
                return nullptr;
            }

            return &m_Windows[handle.m_Index].m_WindowResource.value();
        }

        [[nodiscard]] OptionalPtr<const WindowResource> ResolveHandle(WindowHandleData handle) const
        {
            if(handle.m_GUID == 0)
            {
                return nullptr;
            }

            if(handle.m_Index >= MaxWindows)
            {
                return nullptr;
            }

            if(m_Windows[handle.m_Index].m_GUID != handle.m_GUID)
            {
                return nullptr;
            }

            return &m_Windows[handle.m_Index].m_WindowResource.value();
        }

        template <typename Visitor>
        void VisitAllValidHandles(Visitor && visitor)
        {
            for(int index = 0; index < MaxWindows; ++index)
            {
                if(m_Windows[index].m_GUID != 0)
                {
                    WindowHandleData handle = {};
                    handle.m_GUID = m_Windows[index].m_GUID;
                    handle.m_Index = index;

                    visitor(handle);
                }
            }
        }

        [[nodiscard]] int GetNumWindows() const
        {
            return m_NumWindows;
        }

    private:
        static constexpr int MaxWindows = 1024;

        struct Window
        {
            uint64_t m_GUID = 0;
            Optional<WindowResource> m_WindowResource;
        };

        Window m_Windows[MaxWindows];
        int m_LastFreeWindowIndex = 0;
        int m_NumWindows = 0;

        std::random_device m_RandomDevice;
        std::mt19937_64 m_RandomEngine { m_RandomDevice() };
    };
}

