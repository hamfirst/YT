

#include <cstdint>

import YT;


YT::PSOHandle g_PSOHandle;

uint8_t g_TestTriangle_VS[] =
{
#embed "../Shaders/bin/TestTriangle.spv"
};

uint8_t g_TestFrag_FS[] =
{
#embed "../Shaders/bin/TestFrag.spv"
};

class TestWidget : public YT::Widget<TestWidget>
{

public:
    virtual void OnDraw(YT::Drawer & drawer) override
    {
        drawer.DrawRaw(g_PSOHandle, 3);
    };
};

int main()
{
    YT::ApplicationInitInfo init_info
    {
        .m_ApplicationName = "YTTest"
    };

    if(!YT::Init(init_info))
    {
        YT::Print("YT::Init failed");
        return 1;
    }

    YT::RegisterShader(g_TestTriangle_VS);
    YT::RegisterShader(g_TestFrag_FS);

    YT::PSOCreateInfo pso_create_info;
    pso_create_info.m_VertexShader = g_TestTriangle_VS;
    pso_create_info.m_FragmentShader = g_TestFrag_FS;
    g_PSOHandle = YT::CreatePSO(pso_create_info);

    YT::WindowInitInfo window_init_info;
    window_init_info.m_AlphaBackground = false;

    if (YT::WindowRef window_ref = YT::CreateWindow_GetRef(window_init_info))
    {
        window_ref.SetContent(YT::MakeWidget<TestWidget>());

        YT::RunUntilAllWindowsClosed();
    }
    else
    {
        YT::FatalPrint("Failed to create window");
    }

    YT::Cleanup();
    return 0;
}
