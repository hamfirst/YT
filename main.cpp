

#include <cstdint>

import YT;


YT::PSOHandle g_PSOHandle;

uint8_t g_ScreenSpace_VS[] =
{
#embed "../Shaders/bin/Screenspace.spv"
};

uint8_t g_VertexColor_FS[] =
{
#embed "../Shaders/bin/VertexColor.spv"
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

    YT::RegisterShader(g_ScreenSpace_VS);
    YT::RegisterShader(g_VertexColor_FS);

    YT::PSOCreateInfo pso_create_info;
    pso_create_info.m_VertexShader = g_ScreenSpace_VS;
    pso_create_info.m_FragmentShader = g_VertexColor_FS;
    g_PSOHandle = YT::CreatePSO(pso_create_info);

    YT::WindowInitInfo init_info_window;

    if (YT::WindowRef window_ref = YT::CreateWindow_GetRef(init_info_window))
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
