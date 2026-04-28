
module;

#include <cstddef>
#include <cstdint>
#include <thread>
#include <semaphore>
#include <functional>


#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

module YT:FileMapperImpl;

import :Types;
import :FileMapper;
import :MultiProducerMultiConsumer;
import :MultiProducerSingleConsumer;

namespace YT
{
    FileMapper::DeferredLoad * FileMapper::s_DeferredLoad = nullptr;

    MappedFile::MappedFile(const StringView & file_name) noexcept
    {
        // 1. Open the file
        int fd = open(file_name.data(), O_RDONLY);
        if (fd == -1)
        {
            return;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1)
        {
            close(fd);
            return;
        }

        void* mapped_data = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);

        if (mapped_data == MAP_FAILED)
        {
            close(fd);
            return;
        }

        m_FD = fd;
        m_Size = sb.st_size;
        m_Data = mapped_data;
    }

    MappedFile::MappedFile(MappedFile && rhs) noexcept
    {
        m_FD = rhs.m_FD;
        m_Data = rhs.m_Data;
        m_Size = rhs.m_Size;
        rhs.m_FD = -1;
        rhs.m_Data = nullptr;
        rhs.m_Size = 0;
    };

    MappedFile & MappedFile::operator=(MappedFile && rhs) noexcept
    {
        if (m_Data != nullptr)
        {
            munmap(m_Data, m_Size);
        }

        if (m_FD >= 0)
        {
            close(m_FD);
        }

        m_FD = rhs.m_FD;
        m_Data = rhs.m_Data;
        m_Size = rhs.m_Size;
        rhs.m_FD = -1;
        rhs.m_Data = nullptr;
        rhs.m_Size = 0;
        return *this;
    }

    MappedFile::~MappedFile()
    {
        if (m_Data != nullptr)
        {
            munmap(m_Data, m_Size);
        }

        if (m_FD >= 0)
        {
            close(m_FD);
        }
    }

    [[nodiscard]] Span<const std::byte> MappedFile::GetData() const noexcept
    {
        return { static_cast<const std::byte *>(m_Data), m_Size };
    }

    FileMapper::FileMapper() noexcept :
        m_Semaphore(0)
    {
        int thread_index = 0;
        for (std::thread & thread : m_Threads)
        {
            thread = std::thread([this, thread_index]()
            {
                RunThread(thread_index);
            });

            ++thread_index;
        }
    }

    FileMapper::~FileMapper()
    {
        m_Running = false;

        m_Semaphore.release(NumThreads);

        for (std::thread & thread : m_Threads)
        {
            thread.join();
        }
    }

    void FileMapper::MapFile(const StringView & file_name, Function<void(MappedFile &&)> && callback) noexcept
    {
        InputData input_data
        {
            .m_FileName = String(file_name),
            .m_Callback = std::move(callback)
        };

        while (!m_InputQueue.TryEnqueue(std::move(input_data)))
        {
            std::this_thread::yield();
        }

        ++m_Requests;
        m_Semaphore.release();
    }

    void FileMapper::Update() noexcept
    {
        OutputData output_data;
        while (m_OutputQueue.TryDequeue(output_data))
        {
            output_data.m_Callback(std::move(output_data.m_File));
        }
    }

    void FileMapper::SyncAllFileLoads() noexcept
    {
        while (true)
        {
            Update();

            if (m_Requests.load() == m_Responses.load())
            {
                break;
            }

            std::this_thread::yield();
        }
    }

    void FileMapper::PushDeferred(Function<void()> && func) noexcept
    {
        DeferredLoad * deferred_load = new DeferredLoad{ std::move(func), s_DeferredLoad };
        s_DeferredLoad = deferred_load;
    }

    void FileMapper::CallDeferred() noexcept
    {
        while (s_DeferredLoad != nullptr)
        {
            DeferredLoad * deferred_load = s_DeferredLoad;
            s_DeferredLoad = deferred_load->m_Next;

            deferred_load->m_Callback();

            delete deferred_load;
        }
    }

    void FileMapper::RunThread(int thread_index) noexcept
    {
         while (m_Running)
         {
             m_Semaphore.acquire();

             InputData input_data;
             if (m_InputQueue.TryDequeue(input_data))
             {
                 OutputData output_data
                 {
                     .m_File = MappedFile{ input_data.m_FileName },
                     .m_Callback = std::move(input_data.m_Callback)
                 };

                 while (!m_OutputQueue.Emplace(std::move(output_data)))
                 {
                     std::this_thread::yield();
                 }

                 ++m_Responses;
             }
         }
    }
}
