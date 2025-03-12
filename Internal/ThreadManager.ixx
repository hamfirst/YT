module;

#include <functional>
#include <mutex>
#include <thread>

import YT.Types;

export module YT.ThreadManager;

namespace YT
{
    class JobList
    {
    public:
        void PushJob(Function<void()> && function)
        {
            // Attempt to find a free slot
            for(Job & job : m_JobList)
            {
                JobState state = JobState::Empty;
                if(job.m_State.compare_exchange_strong(state, JobState::Allocated))
                {
                    job.m_Function = std::move(function);
                    job.m_State = JobState::ReadyToRun;
                    return;
                }
            }

            // If no free slot was found, use the backup list
            std::scoped_lock<Mutex> lock(m_BackupJobListMutex);
            m_BackupJobList.emplace_back(std::forward<Function<void()>>(function));
        }

        bool FindAndProcessJob()
        {
            // Try to find a job in the list
            for(Job & job : m_JobList)
            {
                JobState state = JobState::ReadyToRun;
                if(job.m_State.compare_exchange_strong(state, JobState::Running))
                {
                    job.m_Function();
                    job.m_Function = {};
                    job.m_State = JobState::Empty;
                    return true;
                }
            }

            // If no job was found, try to grab one from the backup list
            std::scoped_lock<Mutex> lock(m_BackupJobListMutex);

            if(!m_BackupJobList.empty())
            {
                std::invoke(m_BackupJobList.back());
                m_BackupJobList.pop_back();
                return true;
            }

            return false;
        }

    private:
        enum class JobState : uint8_t
        {
            Empty,
            Allocated,
            ReadyToRun,
            Running,
        };

        struct Job
        {
            Function<void()> m_Function;
            std::atomic<JobState> m_State { JobState::Empty };
        };

        static constexpr uint32_t QueueSize { 16 };
        std::array<Job, QueueSize> m_JobList;

        Mutex m_BackupJobListMutex;
        Vector<Function<void()>> m_BackupJobList;
    };

    export class ThreadManager
    {
        friend void InitThreadManager(const ApplicationInitInfo & init_info);
        friend void CleanupThreadManager();

    private:
        explicit ThreadManager(const ApplicationInitInfo & init_info) :
            m_Semaphore{ 0 },
            m_MainThreadId{ std::this_thread::get_id() }
        {
            uint32_t num_threads = std::clamp(init_info.m_ThreadPoolSize, 1UL, 8UL);

            m_RunningMask = (1 << num_threads) - 1;

            for(uint32_t thread_index = 0; thread_index < num_threads; ++thread_index)
            {
                m_Threads.emplace_back(&ThreadManager::ThreadMain, this, thread_index);
            }
        }

    public:

        ThreadManager(const ThreadManager&) = delete;
        ThreadManager& operator=(const ThreadManager&) = delete;
        ThreadManager(ThreadManager&&) = delete;
        ThreadManager& operator=(ThreadManager&&) = delete;
        
        ~ThreadManager()
        {
            m_Exit = true;

            for(Thread & thread : m_Threads)
            {
                m_Semaphore.release();
            }

            for(Thread & thread : m_Threads)
            {
                thread.join();
            }
        }

    public:

        void PushParallelJob(Function<void()> && function)
        {
            m_ParallelJobList.PushJob(std::forward<Function<void()>>(function));
            m_Semaphore.release();
        }

        void RunAllParallelJobs()
        {
            if(!WarnCheck(std::this_thread::get_id() == m_MainThreadId))
            {
                return;
            }

            while(true)
            {
                if(m_Semaphore.try_acquire())
                {
                    m_ParallelJobList.FindAndProcessJob();
                }
                else
                {
                    if(m_RunningBitfield.load() == 0)
                    {
                        return;
                    }
                }
            }
        }

        void PushMainThreadJob(Function<void()> && function)
        {
            m_MainThreadJobList.PushJob(std::forward<Function<void()>>(function));
        }

        bool RunAllMainThreadJobs()
        {
            if(!WarnCheck(std::this_thread::get_id() == m_MainThreadId))
            {
                return false;
            }

            bool ran_anything = false;
            while(m_MainThreadJobList.FindAndProcessJob())
            {
                // Just keep on workin them jobs
                ran_anything = true;
            }

            return ran_anything;
        }

    private:


        void ThreadMain(uint32_t thread_index)
        {
            SetThreadRunning(thread_index, true);

            while(true)
            {
                if(!m_Semaphore.try_acquire())
                {
                    SetThreadRunning(thread_index, false);
                    m_Semaphore.acquire();
                    SetThreadRunning(thread_index, true);
                }

                if(m_Exit.load())
                {
                    return;
                }

                m_ParallelJobList.FindAndProcessJob();
            }
        }

        void SetThreadRunning(uint32_t thread_index, bool is_running)
        {
            uint64_t thread_bit = 1ULL << thread_index;

            while(true)
            {
                uint64_t old_mask = m_RunningBitfield.load();
                uint64_t new_mask = is_running ?
                    old_mask | thread_bit :
                    old_mask & ~thread_bit;

                if(m_RunningBitfield.compare_exchange_strong(new_mask, old_mask))
                {
                    break;
                }
            }
        }

    private:
        Vector<Thread> m_Threads;
        Semaphore m_Semaphore;

        std::atomic_uint64_t m_RunningBitfield { 0 };
        uint64_t m_RunningMask { 0 };
        std::atomic_bool m_Exit { false };

        std::thread::id m_MainThreadId;

        JobList m_ParallelJobList;
        JobList m_MainThreadJobList;
    };

    std::unique_ptr<ThreadManager> g_ThreadManager;

    void InitThreadManager(const ApplicationInitInfo & init_info)
    {
        g_ThreadManager = std::unique_ptr<ThreadManager>(new ThreadManager(init_info));
    }

    void CleanupThreadManager()
    {
        g_ThreadManager.reset();
    }

}
