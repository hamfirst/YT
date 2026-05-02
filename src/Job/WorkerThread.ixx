
module;

export module YT:WorkerThread;

import :WorkerThreadQueue;

namespace YT
{
    WorkerThreadQueue g_MainThreadQueue(ThreadContextType::Main);

}
