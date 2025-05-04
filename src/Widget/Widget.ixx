module;

#include <cstddef>
#include <cstdint>
#include <utility>
#include <concepts>
#include <format>

export module YT:Widget;

import :Types;
import :BlockTable;
import :Drawer;

namespace YT
{
    using WidgetDeleter = void (*)(void *);

    struct WidgetStorageBase
    {
        BlockTableGenerationRefCount * m_GenerationAndRefCountPtr = nullptr;
        BlockTableHandle m_Handle;
    };

    template <typename WidgetClass>
    struct WidgetStorage final : public WidgetStorageBase
    {
        alignas(WidgetClass) std::byte m_Data[sizeof(WidgetClass)];
    };

    export class WidgetBase;
    export struct WidgetBaseInitData;

    export template <typename MyWidgetClass, typename InitDataType = WidgetBaseInitData, typename ParentWidgetClass = WidgetBase>
    class Widget;

    export template <typename WidgetClass>
    class WidgetHandle;

    export template <typename WidgetClass>
    class WidgetRef final
    {
    private:
        explicit WidgetRef(WidgetClass * widget) : m_Widget(widget)
        {
            std::byte * storage_start = reinterpret_cast<std::byte *>(m_Widget) - offsetof(WidgetStorage<WidgetClass>, m_Data);
            auto * storage = reinterpret_cast<WidgetStorage<WidgetClass> *>(storage_start);

            m_Storage = storage;
            m_Widget = reinterpret_cast<WidgetClass *>(&storage->m_Data);
            m_Deleter = [](void * ptr) { auto widget = static_cast<WidgetClass *>(ptr); delete widget; };

            IncRef();
        }

        WidgetRef(WidgetClass * widget, WidgetStorageBase * storage, WidgetDeleter deleter)
        {
            m_Storage = storage;
            m_Widget = widget;
            m_Deleter = deleter;

            IncRef();
        }

    public:
        WidgetRef() noexcept = default;
        WidgetRef(WidgetRef & rhs) noexcept
        {
            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;

            IncRef();
        }

        WidgetRef(WidgetRef && rhs) noexcept
        {
            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;
            rhs.m_Storage = nullptr;
            rhs.m_Widget = nullptr;
            rhs.m_Deleter = nullptr;
        }


        template <typename BaseClass> requires std::derived_from<BaseClass, WidgetClass>
        WidgetRef(const WidgetRef<BaseClass> & rhs)
        {
            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;

            IncRef();
        }

        template <typename BaseClass> requires std::derived_from<BaseClass, WidgetClass>
        WidgetRef(WidgetRef<BaseClass> && rhs)
        {
            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;
            rhs.m_Storage = nullptr;
            rhs.m_Widget = nullptr;
            rhs.m_Deleter = nullptr;
        }

        WidgetRef & operator=(const WidgetRef & rhs) noexcept
        {
            if (this == &rhs)
            {
                return *this;
            }

            DecRef();

            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;

            IncRef();
            return *this;
        }

        WidgetRef & operator=(WidgetRef && rhs) noexcept
        {
            if (this == &rhs)
            {
                return *this;
            }

            DecRef();

            m_Storage = rhs.m_Storage;
            m_Widget = rhs.m_Widget;
            m_Deleter = rhs.m_Deleter;
            rhs.m_Storage = nullptr;
            rhs.m_Widget = nullptr;
            rhs.m_Deleter = nullptr;
            return *this;
        }

        ~WidgetRef()
        {
            DecRef();
        }

        [[nodiscard]] OptionalPtr<WidgetClass> * operator * () noexcept
        {
            return m_Widget;
        }

        [[nodiscard]] OptionalPtr<const WidgetClass> * operator * () const noexcept
        {
            return m_Widget;
        }

        WidgetClass * operator -> () noexcept
        {
            return m_Widget;
        }

        const WidgetClass * operator -> () const noexcept
        {
            return m_Widget;
        }

        explicit operator bool () const noexcept
        {
            return m_Widget != nullptr;
        }

        void Reset() noexcept
        {
            DecRef();

            m_Storage = nullptr;
            m_Widget = nullptr;
        }

        template <typename CastWidgetClass>
        [[nodiscard]] MaybeInvalid<WidgetRef<CastWidgetClass>> Cast() const noexcept
        {
            if constexpr (std::is_base_of_v<CastWidgetClass, WidgetClass>)
            {
                return WidgetRef<CastWidgetClass>(m_Widget, m_Storage);
            }

            if (auto * cast_widget_ptr = dynamic_cast<CastWidgetClass *>(m_Widget))
            {
                return WidgetRef<CastWidgetClass>(cast_widget_ptr, m_Storage);
            }

            return {};
        }

    private:

        void IncRef() const noexcept
        {
            if (m_Storage)
            {
                m_Storage->m_GenerationAndRefCountPtr->IncRefNoValidation();
            }
        }

        void DecRef() const noexcept
        {
            if (m_Storage)
            {
                if (!m_Storage->m_GenerationAndRefCountPtr->DecRefNoValidation())
                {
                    m_Deleter(m_Widget);
                }
            }
        }

        template <typename OtherWidgetClass, typename... Args>
        friend WidgetRef<OtherWidgetClass> MakeWidget(Args &&...) noexcept;

        template <typename OtherWidgetType>
        friend class WidgetRef;

        friend class WidgetHandle<WidgetClass>;

        WidgetStorageBase * m_Storage = nullptr;
        WidgetClass * m_Widget = nullptr;
        WidgetDeleter m_Deleter = nullptr;
    };

    template <typename WidgetClass>
    class WidgetHandle final
    {
    private:
        WidgetHandle(WidgetStorageBase * storage, WidgetClass * widget, WidgetDeleter deleter) :
            m_Storage(storage), m_Widget(widget), m_Deleter(deleter)
        {
            if (m_Storage)
            {
                m_Generation = m_Storage->m_GenerationAndRefCountPtr->GetGeneration();
            }
        }

    public:

        explicit WidgetHandle(WidgetRef<WidgetClass> widget_ref) :
            m_Storage(widget_ref.m_Storage), m_Widget(widget_ref.m_Widget), m_Deleter(widget_ref.m_Deleter)
        {
            if (m_Storage)
            {
                m_Generation = m_Storage->m_GenerationAndRefCountPtr->GetGeneration();
            }
        }

        WidgetHandle() noexcept = default;
        WidgetHandle(const WidgetHandle & rhs) noexcept = default;
        WidgetHandle(WidgetHandle && rhs) noexcept = default;

        WidgetHandle & operator=(const WidgetHandle & rhs) noexcept = default;
        WidgetHandle & operator=(WidgetHandle && rhs) noexcept = default;

        ~WidgetHandle() = default;

        [[nodiscard]] MaybeInvalid<WidgetRef<WidgetClass>> Get() const noexcept
        {
            if (!m_Storage)
            {
                return {};
            }

            if (m_Storage->m_GenerationAndRefCountPtr->IncRef(m_Generation))
            {
                WidgetRef<WidgetClass> ref(m_Widget, m_Storage, m_Deleter);
                m_Storage->m_GenerationAndRefCountPtr->DecRefNoValidation();

                return ref;
            }

            return {};
        }

        template <typename CastWidgetClass>
        [[nodiscard]] MaybeInvalid<WidgetHandle<CastWidgetClass>> Cast() const noexcept
        {
            if constexpr (std::is_base_of_v<CastWidgetClass, WidgetClass>)
            {
                return WidgetHandle<CastWidgetClass>(m_Storage, m_Widget, m_Deleter);
            }

            if (auto * cast_widget_ptr = dynamic_cast<CastWidgetClass *>(m_Widget))
            {
                return WidgetHandle<CastWidgetClass>(m_Storage, cast_widget_ptr, m_Deleter);
            }

            return {};
        }

    private:

        template <typename CastWidgetClass>
        friend class WidgetHandle;

        template <typename MyWidgetClass, typename InitDataType = WidgetBaseInitData, typename ParentWidgetClass = WidgetBase>
        friend class Widget;

        WidgetStorageBase * m_Storage = nullptr;
        WidgetClass * m_Widget = nullptr;
        WidgetDeleter m_Deleter = nullptr;
        uint32_t m_Generation = 0;
    };

    export struct WidgetBaseInitData
    {

    };

    export class WidgetBase
    {
    public:
        struct Types
        {
            using MyWidget = WidgetBase;
            using InitData = WidgetBaseInitData;
            using ParentWidget = void;
        };

    public:

        WidgetBase() = default;
        virtual ~WidgetBase() = default;

        virtual void OnUpdate(double delta_time) { }
        virtual void OnDraw(Drawer & drawer) { }

        virtual void VisitChildren(Function<void(const WidgetBase&)> & callback) { }
    };

    template <typename MyWidgetClass, typename InitDataType = WidgetBaseInitData, typename ParentWidgetClass = WidgetBase>
    class Widget : public ParentWidgetClass
    {
    public:
        struct Types
        {
            using MyWidget = MyWidgetClass;
            using InitData = InitDataType;
            using ParentWidget = ParentWidgetClass;
        };

        static_assert(std::is_base_of_v<typename ParentWidgetClass::Types::InitData, InitDataType>);

    private:

        static BlockTable<WidgetStorage<MyWidgetClass>> & GetWidgetStorage() noexcept
        {
            static BlockTable<WidgetStorage<MyWidgetClass>> s_WidgetStorage;
            return s_WidgetStorage;
        }

        void * operator new (size_t size) noexcept
        {
            if (size != sizeof(MyWidgetClass))
            {
                FatalPrint("Incorrect storage size for widget - check template parameters");
                return nullptr;
            }

            if (auto handle = GetWidgetStorage().AllocateHandle())
            {
                WidgetStorage<MyWidgetClass> * storage = GetWidgetStorage().ResolveHandle(handle);
                storage->m_Handle = handle;
                storage->m_GenerationAndRefCountPtr = GetWidgetStorage().GetGenerationPointer(handle);
                return &storage->m_Data;
            }

            FatalPrint("Failed to allocate memory for Widget");
            return nullptr;
        }

    public:
        void operator delete (void * ptr) noexcept
        {
            auto storage = static_cast<WidgetStorage<MyWidgetClass> *>(ptr);
            GetWidgetStorage().ReleaseHandle(storage->m_Handle);
        }

        template <typename Visitor>
        static void VisitAllHandles(Visitor && visitor)
        {
            GetWidgetStorage().VisitAllHandles([&](const BlockTableHandle & handle)
            {
                WidgetStorage<MyWidgetClass> * storage = GetWidgetStorage().ResolveHandle(handle);

                visitor(
                    WidgetHandle<MyWidgetClass>(storage,
                        reinterpret_cast<MyWidgetClass *>(&storage->m_Data),
                        [](void * ptr) { auto widget = static_cast<MyWidgetClass *>(ptr); delete widget; }));
            });
        }

        template <typename Visitor>
        static void VisitAllWidgets(Visitor && visitor)
        {
            VisitAllHandles([&](WidgetHandle<MyWidgetClass> handle)
            {
                WidgetRef<MyWidgetClass> widget_ref = handle.Get();
                if (MyWidgetClass * widget = *widget_ref)
                {
                    visitor(widget);
                }
            });
        }

    private:

        template <typename WidgetClass, typename... Args>
        friend WidgetRef<WidgetClass> MakeWidget(Args &&...) noexcept;

    };

    export template <typename WidgetClass, typename... Args>
    WidgetRef<WidgetClass> MakeWidget(Args &&... args) noexcept
    {
        return WidgetRef<WidgetClass>(new WidgetClass(std::forward<Args>(args)...));
    }
}
