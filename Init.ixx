module;

#include <exception>
#include <iostream>

import YT.Types;
import YT.WindowManager;
import YT.RenderManager;

export module YT.Init;

namespace YT
{
    export bool Init(const ApplicationInitInfo& init_info) noexcept
    {
        if (!WindowManager::CreateWindowManager(init_info))
        {
            return false;
        }

        if (!RenderManager::CreateRenderManager(init_info))
        {
            return false;
        }

        return true;
    }

    export void RunUntilAllWindowsClosed() noexcept
    {
        while (!g_WindowManager->ShouldExit() && g_WindowManager->HasOpenWindows())
        {
            g_WindowManager->DispatchEvents();
            g_WindowManager->RenderWindows();
            g_WindowManager->WaitForNextFrame();
        }
    }

    export void Cleanup() noexcept
    {
        g_WindowManager->CloseAllWindows();

        g_RenderManager.reset();
        g_WindowManager.reset();
    }
}
