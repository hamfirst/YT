#include <cstdint>
#include <cstdio>

#include <glm/glm.hpp>

import YT;

using namespace YT;

PSOHandle g_PSOHandle;

uint8_t g_TestTriangle_VS[] =
{
#embed "../Shaders/bin/TestTriangle.spv"
};

uint8_t g_TestFrag_FS[] =
{
#embed "../Shaders/bin/TestFrag.spv"
};

class TestWidget : public Widget<TestWidget>
{
public:
    virtual void OnDraw(YT::Drawer & drawer) override
    {
        drawer.DrawQuad(glm::vec2(0, 0), glm::vec2(100, 100), glm::vec4(1, 0, 0, 1));
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
        return 1;
    }

    RegisterShader(g_TestTriangle_VS);
    RegisterShader(g_TestFrag_FS);

    PSOCreateInfo pso_create_info;
    pso_create_info.m_VertexShader = g_TestTriangle_VS;
    pso_create_info.m_FragmentShader = g_TestFrag_FS;
    g_PSOHandle = CreatePSO(pso_create_info);

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
