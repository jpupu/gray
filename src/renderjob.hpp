#ifndef _RENDERJOB_HPP_
#define _RENDERJOB_HPP_

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "film.hpp"

class Scene;

namespace threaded_render {

class Job;
class TaskDesc;
class Task;
class Worker;

class Job
{
public:
    Job (int threads, const Scene& scene, Film& film);
    void add_task (const TaskDesc&);
    void finish ();

    void set_callback (std::function<void(const Task&)> cb);

public:
    const Scene& scene;
    Film& film;
    std::mutex mtx;
    std::condition_variable prod_cv;

    /// Called by task itself.
    void task_finished (const Task&);

    std::vector<int> seeds;
private:
    bool wait_for_finish;
    std::vector<std::shared_ptr<Worker>> workers;

    Worker* find_worker (int state);
    bool any_pending () const;
    std::function<void(const Task&)> task_done_cb;
};

class TaskDesc
{
public:
    int xofs, yofs;
    int xres, yres;
    int spp;
};

class Task : public TaskDesc
{
public:
    Job* job;
    std::unique_ptr<Film> film;

    Task () {}
    Task (Job*, const TaskDesc& desc);

    void render ();
};

class Worker
{
public:
    Job* job;
    enum {
        IDLE,
        INPUT_READY,
        WORKING,
        QUIT
    };
    std::thread th;
    Task task;
    std::atomic<int> state;
    std::condition_variable cv;

    Worker (Job* job);
    void loop ();

};


} // namespace threaded_render

#endif // _RENDERJOB_HPP_
