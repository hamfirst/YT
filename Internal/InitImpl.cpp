module;

module YT:InitImpl;

import :Types;
import :JobManager;
import :WindowManager;
import :RenderManager;
import :QuadRender;

namespace YT
{
    bool Init(const ApplicationInitInfo& init_info) noexcept
    {
        if (!JobManager::CreateJobManager())
        {
            return false;
        }

        if (!WindowManager::CreateWindowManager(init_info))
        {
            return false;
        }

        if (!RenderManager::CreateRenderManager(init_info))
        {
            return false;
        }

        g_RenderManager->RegisterRenderGlobals();

        if (!QuadRender::CreateQuadRender(init_info))
        {
            return false;
        }

        return true;
    }

    void RunUntilAllWindowsClosed() noexcept
    {
        while (!g_WindowManager->ShouldExit() && g_WindowManager->HasOpenWindows())
        {
            g_WindowManager->DispatchEvents();
            g_WindowManager->RenderWindows();
            g_WindowManager->WaitForNextFrame();
        }
    }

    void Cleanup() noexcept
    {
        g_WindowManager->CloseAllWindows();

        g_RenderManager.reset();
        g_WindowManager.reset();
    }

}
