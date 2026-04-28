
module;

#include <cstddef>
#include <cstdint>
#include <any>
#include <utility>
#include <stdexcept>

export module YT:DeferredImageLoad;

import :Types;
import :ImageReference;
import :FileMapper;

namespace YT
{
    export class DeferredImageLoad;

    DeferredImageLoad * g_DeferredImageLoadHead = nullptr;

    class DeferredImageLoad
    {
    public:
        explicit DeferredImageLoad(const Span<const std::byte> & image_data) noexcept;
        explicit DeferredImageLoad(const StringView & file_name) noexcept;

        DeferredImageLoad(const DeferredImageLoad &) = delete;
        DeferredImageLoad(DeferredImageLoad &&) = delete;
        DeferredImageLoad & operator=(const DeferredImageLoad &) = delete;
        DeferredImageLoad & operator=(DeferredImageLoad &&) = delete;

        operator const ImageReference & () const noexcept;
        const ImageReference & operator -> () const noexcept;
        const ImageReference & operator * () const noexcept;

    protected:

        friend class RenderManager;

        void Finalize() noexcept;

    private:
        Span<const std::byte> m_ImageData;
        Optional<ImageReference> m_ImageRef;

        Optional<MappedFile> m_MappedFile;

        DeferredImageLoad * m_Next = nullptr;
    };

    export template <int ArraySize>
    DeferredImageLoad LoadDeferredImageEmbedded(const std::byte (&arr)[ArraySize]) noexcept
    {
        return DeferredImageLoad(Span(&arr[0], ArraySize));
    }

    export template <int ArraySize>
    DeferredImageLoad LoadDeferredImageEmbedded(unsigned char (&arr)[ArraySize]) noexcept
    {
        return DeferredImageLoad(CreateByteSpan(arr));
    }
}
