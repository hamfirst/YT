
module;

#include <cstddef>
#include <cstdint>
#include <utility>
#include <stdexcept>

export module YT:DeferredImageLoad;

import :Types;
import :ImageReference;

namespace YT
{
    export class DeferredImageLoad;
    DeferredImageLoad * g_DeferredImageLoadHead = nullptr;

    class DeferredImageLoad
    {
    public:
        explicit DeferredImageLoad(const Span<const std::byte> & image_data) noexcept;

        operator const ImageReference & () const noexcept;
        const ImageReference & operator -> () const noexcept;
        const ImageReference & operator * () const noexcept;

    protected:

        friend class RenderManager;

        void Finalize() noexcept;

    private:
        Span<const std::byte> m_ImageData;
        Optional<ImageReference> m_ImageRef;

        DeferredImageLoad * m_Next = nullptr;
    };

    export template <int ArraySize>
    DeferredImageLoad LoadDeferredImageEmbedded(std::byte (&arr)[ArraySize])
    {
        return DeferredImageLoad(Span(&arr[0], ArraySize));
    }

    export template <int ArraySize>
    DeferredImageLoad LoadDeferredImageEmbedded(unsigned char (&arr)[ArraySize])
    {
        return DeferredImageLoad(Span(reinterpret_cast<std::byte *>(&arr[0]), ArraySize));
    }

}
