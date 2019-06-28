#ifndef RTWE_THREADING_H
#define RTWE_THREADING_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <optional>
#include <cassert>
#include <boost/log/trivial.hpp>

namespace rtwe
{

//
// Constants
//

inline const int HARDWARE_MAX_CONCURRENT_THREADS = static_cast<int>(std::thread::hardware_concurrency());

//
//
//

template <typename Task>
class ThreadPool final
{
public: // Construction / Destruction

    explicit ThreadPool(const int threadCount = HARDWARE_MAX_CONCURRENT_THREADS);

    ~ThreadPool();

public: // Deleted

    ThreadPool(const ThreadPool &) = delete;

    ThreadPool & operator=(const ThreadPool &) = delete;

public: // Interface

    inline void FinishAndJoinAll();

    inline void EnqueueTask(Task task);

    template <typename... Args>
    inline void EmplaceTask(Args && ... args);

private: // Service

    inline std::vector<std::thread> CreateThreads(const int threadCount);

    static inline void ThreadMain(ThreadPool * const pThreadPool);

private: // Members

    std::mutex       m_TasksQueueMutex;
    std::atomic_bool m_MustFinishThreads;

    std::queue<Task>         m_TasksQueue;
    std::vector<std::thread> m_Threads;
};

//
// Construction / Destruction
//

template <typename Task>
ThreadPool<Task>::ThreadPool(const int threadCount):
    m_TasksQueueMutex      (),
    m_MustFinishThreads    (false),
    m_TasksQueue           (),
    m_Threads(CreateThreads(threadCount))
{
    BOOST_LOG_TRIVIAL(info)<< "Created a threadpool with " << threadCount << " threads for tasks of type " << typeid(Task).name();
}

template <typename Task>
ThreadPool<Task>::~ThreadPool()
{
    FinishAndJoinAll();
}

//
// Interface
//

template <typename Task>
inline void ThreadPool<Task>::FinishAndJoinAll()
{
    m_MustFinishThreads = true;

    std::for_each(m_Threads.begin(), m_Threads.end() , std::mem_fn(&std::thread::join));

    BOOST_LOG_TRIVIAL(info) << "All threadpool threads have exited";
}

template <typename Task>
inline void ThreadPool<Task>::EnqueueTask(Task task)
{
    EmplaceTask(std::move(task));
}

template <typename Task>
template <typename... Args>
inline void ThreadPool<Task>::EmplaceTask(Args && ... args)
{
    std::unique_lock tasksQueueLock(m_TasksQueueMutex);

    m_TasksQueue.emplace(std::forward<Args>(args)...);
}

//
// Service
//

template <typename Task>
inline std::vector<std::thread> ThreadPool<Task>::CreateThreads(const int threadCount)
{
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; i++)
        threads.emplace_back(ThreadMain, this);

    return threads;
}

template <typename Task>
inline void ThreadPool<Task>::ThreadMain(ThreadPool * pThreadPool)
{
    assert(pThreadPool != nullptr);

    BOOST_LOG_TRIVIAL(debug)<< "Thread has been created";

    std::optional<Task> task;

    while(!pThreadPool->m_MustFinishThreads)
    {
        {
            std::unique_lock tasksQueueLock(pThreadPool->m_TasksQueueMutex);

            if (!pThreadPool->m_TasksQueue.empty())
            {
                task.emplace(std::move(pThreadPool->m_TasksQueue.front()));

                pThreadPool->m_TasksQueue.pop();
            }
        }

        if (!task.has_value())
            continue; // TODO: Wait on a conditional variable instead of spinning

        (*task)();

        task.reset();
    }

    BOOST_LOG_TRIVIAL(debug)<< "Thread is exiting";
}

} // namespace rtwe

#endif // RTWE_THREADING_H