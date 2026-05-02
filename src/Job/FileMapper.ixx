
module;

#include <cstddef>
#include <cstdint>
#include <thread>
#include <semaphore>
#include <functional>
#include <coroutine>

export module YT:FileMapper;

import :Types;
import :JobTypes;
import :MultiProducerMultiConsumer;
import :MultiProducerSingleConsumer;
import :Coroutine;

namespace YT
{
    export class MappedFile
    {
    public:
        MappedFile() = default;

        explicit MappedFile(const StringView & file_name) noexcept;

        MappedFile(const MappedFile&) = delete;
        MappedFile(MappedFile && rhs) noexcept;

        MappedFile & operator=(const MappedFile&) = delete;
        MappedFile & operator=(MappedFile && rhs) noexcept;

        ~MappedFile();

        [[nodiscard]] Span<const std::byte> GetData() const noexcept;

    private:

        int m_FD = -1;

        std::size_t m_Size = 0;
        void * m_Data = nullptr;
    };

    export class FileMapper final
    {
    public:

        static constexpr std::size_t NumThreads = Threading::NumFileMapperThreads;

        FileMapper() noexcept;

        FileMapper(const FileMapper&) = delete;
        FileMapper(FileMapper&&) = delete;
        FileMapper& operator=(const FileMapper&) = delete;
        FileMapper& operator=(FileMapper&&) = delete;

        ~FileMapper();

        void MapFile(const StringView & file_name, Function<void(MappedFile &&)> && callback) noexcept;

        void PushCoro(CoroBase * coro) noexcept;

        void Update() noexcept;
        void SyncAllFileLoads() noexcept;

        static void PushDeferred(Function<void()> && func) noexcept;
        static void CallDeferred() noexcept;

    private:

        void RunThread(int thread_index) noexcept;

    private:

        std::atomic_bool m_Running = true;
        std::atomic_int m_Requests = 0;
        std::atomic_int m_Responses = 0;

        std::array<std::thread, NumThreads> m_Threads;
        std::counting_semaphore<> m_Semaphore;

    private:
        struct InputData
        {
            CoroBase * m_Coro = nullptr;
            String m_FileName;
            Function<void(MappedFile &&)> m_Callback;
        };

        struct OutputData
        {
            MappedFile m_File;
            Function<void(MappedFile &&)> m_Callback;
        };

        MultiProducerMultiConsumer<InputData, 1024> m_InputQueue;
        MultiProducerSingleConsumer<OutputData, 1024> m_OutputQueue;

    private:

        struct DeferredLoad
        {
            Function<void()> m_Callback;
            DeferredLoad * m_Next = nullptr;
        };

        static DeferredLoad * s_DeferredLoad;
    };

    FileMapper g_FileMapper;

    export Coro<MappedFile, ThreadContextType::FileMapper> MapFileAsync(const StringView & file_name)
    {
        co_return MappedFile(file_name);
    }
}
