#include "Log.h"
#include "TaskScheduler.h"

namespace engine {
    TaskScheduler::TaskScheduler() {
        running = true;
        int tasks = std::thread::hardware_concurrency();
        tasks = std::max(1, tasks);
        for (int i = 0; i < tasks; i++){
            threads.emplace_back(&TaskScheduler::workerThread, this);
        }    
    }
    TaskScheduler::~TaskScheduler() {
        stop();
        for (auto& thread : threads){
            if (thread.joinable()){
                thread.join();
            }
        }
    }
    void TaskScheduler::addTask(std::function<void()> task){
        std::lock_guard<std::mutex> lock(tasks_mutex);
        tasks.emplace(task);
        tasks_cv.notify_one();
    }
    void TaskScheduler::stop(){
        std::lock_guard<std::mutex> lock(tasks_mutex);
        running = false;
        tasks_cv.notify_all();
    }

    void TaskScheduler::workerThread(){
        while (true){
            std::unique_lock<std::mutex> lock(tasks_mutex);
            tasks_cv.wait(lock, [this]{ return !tasks.empty() || !running; });
            if (!running) return;
            auto task = tasks.front();
            task_count.fetch_add(1);
            tasks.pop();
            lock.unlock();
            try{
                task();
                
            } catch (const std::exception& e){
                LogError("TaskScheduler: Exception in task:" + std::string(e.what()) + " in thread: ");
                
            }
            catch (...) {
                LogError("TaskScheduler: Unknown exception in task");
                
            }
            task_count.fetch_sub(1);


        }
    }

    void TaskScheduler::emptyQueue(){
        std::lock_guard<std::mutex> lock(tasks_mutex);
        tasks  = {};
    }
}
