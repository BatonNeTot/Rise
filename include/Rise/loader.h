//☀Rise☀
#ifndef rise_loader_h
#define rise_loader_h

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace Rise {
    
    class Loader {
    public:

        Loader(uint32_t threadCount);
        ~Loader();

        template <class T>
        void AddJob(T&& job) {
            std::unique_lock<std::mutex> ul(_queueLock);
            _workQueue.emplace(std::forward<T>(job));
            _queueCond.notify_one();
        }

    private:

        void ThreadLoop();

        std::queue<std::function<void()>> _workQueue;
        std::mutex _queueLock;
        std::condition_variable _queueCond;
        std::vector<std::thread> _jobThreads;
        bool _destroying = false;

    };

}

#endif /* rise_loader_h */
