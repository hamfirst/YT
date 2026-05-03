module;

#include <memory>
#include <chrono>

module YT:InitImpl;

import :Types;
import :Init;
import :Wait;
import :JobManager;
import :WindowManager;
import :RenderManager;
import :QuadRender;
import :BackgroundTaskManager;
import :FileMapper;
import :FontManager;

namespace YT
{
    bool Init(const ApplicationInitInfo& init_info) noexcept
    {
        InitWait();

        g_InitTime = std::chrono::high_resolution_clock::now();

        SetCurrentThreadContext(ThreadContextType::Main);

        if (!JobManager::CreateJobManager())
        {
            FatalPrint("Failed to create JobManager");
            return false;
        }

        if (!FileMapper::CreateFileMapper())
        {
            FatalPrint("Failed to create FileMapper");
            return false;
        }

        if (!BackgroundTaskManager::CreateBackgroundTaskManager())
        {
            FatalPrint("Failed to create BackgroundTaskManager");
            return false;
        }

        if (!FontManager::CreateFontManager())
        {
            FatalPrint("Failed to create FontManager");
            return false;
        }

        if (!WindowManager::CreateWindowManager(init_info))
        {
            FatalPrint("Failed to create WindowManager");
            return false;
        }

        if (!RenderManager::CreateRenderManager(init_info))
        {
            FatalPrint("Failed to create RenderManager");
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
        g_RenderManager->CleanupImmediately();

        g_RenderManager.reset();
        g_WindowManager.reset();
    }

}
