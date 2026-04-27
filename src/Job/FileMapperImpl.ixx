
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

import :FileMapper;
import :Types;

namespace YT
{
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


    class FileMapper final
    {
    public:

        static constexpr std::size_t NumThreads = 4;

        FileMapper() noexcept :
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

        FileMapper(const FileMapper&) = delete;
        FileMapper(FileMapper&&) = delete;
        FileMapper& operator=(const FileMapper&) = delete;
        FileMapper& operator=(FileMapper&&) = delete;

        ~FileMapper()
        {
            m_Running = false;

            m_Semaphore.release(NumThreads);

            for (std::thread & thread : m_Threads)
            {
                thread.join();
            }
        }

        void Update() noexcept
        {


        }

        void SyncAllFileLoads() noexcept
        {

        }

    private:

        void RunThread(int thread_index) noexcept
        {
            /*while (m_Running)
            {
                m_Semaphore.acquire();

                InputData input_data;
                if (m_InputQueue.try_dequeue(input_data))
                {
                    m_OutputQueue[thread_index].enqueue(
                        OutputData
                        {
                            .m_File = MappedFile{ input_data.m_FileName },
                            .m_Callback = std::move(input_data.m_Callback)
                        });
                }
            }*/
        }

    private:

        std::atomic_bool m_Running = true;

        std::array<std::thread, NumThreads> m_Threads;
        std::counting_semaphore<> m_Semaphore;

        struct InputData
        {
            String m_FileName;
            Function<void(MappedFile &&)> m_Callback;
        };

        struct OutputData
        {
            MappedFile m_File;
            Function<void(MappedFile &&)> m_Callback;
        };

    };

    FileMapper g_FileMapper;

    void MapFile(const StringView & file_name,
        Function<void(const Span<const std::byte> &)> && callback)
    {

    }

    void UpdateFileLoads()
    {

    }

    void SyncAllFileLoads()
    {

    }
}
