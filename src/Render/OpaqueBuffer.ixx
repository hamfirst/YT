module;

export module YT:OpaqueBuffer;

namespace YT
{
    export class OpaqueBuffer
    {
    public:
        using DeleterType = void (*)(void *);

        OpaqueBuffer() = default;
        OpaqueBuffer(void * data_ptr, DeleterType deleter) noexcept
            : m_DataPtr(data_ptr), m_Deleter(deleter)
        {

        }

        OpaqueBuffer(const OpaqueBuffer & rhs) = delete;

        OpaqueBuffer(OpaqueBuffer && rhs) noexcept
        {
            m_DataPtr = rhs.m_DataPtr;
            m_Deleter = rhs.m_Deleter;

            rhs.m_DataPtr = nullptr;
            rhs.m_Deleter = nullptr;
        }

        OpaqueBuffer & operator = (const OpaqueBuffer & rhs) = delete;
        OpaqueBuffer & operator = (OpaqueBuffer && rhs) noexcept
        {
            if (m_DataPtr)
            {
                m_Deleter(m_DataPtr);
            }

            m_DataPtr = rhs.m_DataPtr;
            m_Deleter = rhs.m_Deleter;

            rhs.m_DataPtr = nullptr;
            rhs.m_Deleter = nullptr;
            return *this;
        }

        ~OpaqueBuffer()
        {
            if (m_DataPtr)
            {
                m_Deleter(m_DataPtr);
            }
        }

    private:
        void * m_DataPtr = nullptr;
        DeleterType m_Deleter = nullptr;
    };

    template <typename T>
    OpaqueBuffer MakeOpaqueBuffer(T * t, void (*deleter)(T *))
    {
        return OpaqueBuffer(t, reinterpret_cast<void(*)(void *)>(deleter));
    }
}
