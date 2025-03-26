module;

module YT:InitImpl;

import :Types;
import :WindowManager;
import :RenderManager;
import :RenderQuad;

namespace YT
{
    bool Init(const ApplicationInitInfo& init_info) noexcept
    {
        if (!WindowManager::CreateWindowManager(init_info))
        {
            return false;
        }

        if (!RenderManager::CreateRenderManager(init_info))
        {
            return false;
        }

        g_RenderManager->RegisterRenderGlobals();

        if (!RenderQuad::CreateRenderQuad(init_info))
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
