//import_std

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <array>
#include <string>
#include <format>
#include <memory>
#include <vector>
#include <coroutine>
#include <atomic>
#include <mutex>
#include <type_traits>
#include <utility>

import glm;

import YT;

using namespace YT;


DeferredImageLoad TestDeferredImage("../assets/cs-black-000.png");


class TestWidget : public Widget<TestWidget>
{
public:
    virtual void OnDraw(YT::Drawer & drawer) override
    {
        double t = GetApplicationTime();
        drawer.DrawQuad(glm::vec2(0, 0 + glm::sin(t) * 100), glm::vec2(100, 100), glm::vec4(1, 0, 0, 1), TestDeferredImage);
        drawer.DrawQuad(glm::vec2(50, 50), glm::vec2(100, 100), glm::vec4(1, 1, 1, 1));
        drawer.DrawQuad(glm::vec2(100, 100), glm::vec2(100, 100), glm::vec4(0, 0, 1, 1));
    };
};

BackgroundTask<int> TestCoro()
{
    MappedFile file = co_await MapFileAsync("../shaders/TestTriangle.vert");

    PrintStr(file.AsStringView());
    co_return 42;
}

int main()
{
    ApplicationInitInfo init_info
    {
        .m_ApplicationName = "YTTest"
    };

    if (!Init(init_info))
    {
        FatalPrint("YT::Init failed");
        return ENODEV;
    }

    Vector<WindowRef> windows;
    for (int i = 0; i < 1; ++i)
    {
        WindowInitInfo window_init_info;
        if (WindowRef window_ref = CreateWindow_GetRef(window_init_info))
        {
            window_ref.SetContent(MakeWidget<TestWidget>());
            windows.emplace_back(std::move(window_ref));
        }
        else
        {
            FatalPrint("Failed to create window");
        }
    }

    RunUntilAllWindowsClosed();

    Cleanup();
    return 0;
}
