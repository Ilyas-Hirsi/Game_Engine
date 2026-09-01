#pragma once

#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
namespace engine {
    class TaskScheduler {
        public:
        TaskScheduler();
        ~TaskScheduler();
        void addTask(std::function<void()>);
        std::atomic<int> task_count;
        void stop();
        template <typename Func>
        void parallel_for(std::size_t begin, std::size_t end, Func&& func, std::size_t chunk_size = 0){
            if (chunk_size == 0) chunk_size = std::max<std::size_t>(1, (end - begin) / (2*threads.size()));
            int chunk_count = (end - begin + chunk_size - 1) / chunk_size;
            std::atomic<std::size_t> remaining = chunk_count;
            for (std::size_t i = begin; i < end; i += chunk_size){
                auto task = [this, i, end, func, chunk_size, &remaining](){
                    for (std::size_t j = i; j < i + chunk_size && j < end; j++){
                        func(j);
                    }
                    remaining.fetch_sub(1);
                };
                addTask(task);
            }
            while (remaining.load() > 0){
                std::unique_lock<std::mutex> lock(tasks_mutex);
                if (!tasks.empty()){
                    auto task = tasks.front();
                    tasks.pop();
                    lock.unlock();
                    task();
                }
                else{
                    lock.unlock();
                    std::this_thread::yield();
                }
            }
        }

        private:
        void workerThread();
        bool running;
        std::vector<std::thread> threads;
        std::mutex tasks_mutex;
        std::condition_variable tasks_cv;
        std::queue<std::function<void()>> tasks;
    };
}