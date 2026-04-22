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

unsigned char TestImageData[] =
{
#embed "cs-black-000.png"
};

auto TestDeferredImage = LoadDeferredImageEmbedded(TestImageData);


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

    WindowInitInfo window_init_info;
    if (WindowRef window_ref = CreateWindow_GetRef(window_init_info))
    {
        window_ref.SetContent(MakeWidget<TestWidget>());

        RunUntilAllWindowsClosed();
    }
    else
    {
        FatalPrint("Failed to create window");
    }

    Cleanup();
    return 0;
}
