module;

#include <experimental/meta>

module YT:WidgetRegistry;

import :Types;
import :Widget;

namespace YT
{
    struct WidgetClass
    {
        void (*m_UpdateAll)(double delta_time) = nullptr;
        Vector<WidgetBase*> (*m_GetAllWidgets)() = nullptr;
    };

    class WidgetRegistry
    {
    public:

        template<typename T>
        void RegisterWidgetClass()
        {

        }
    };
}