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
    // std::condition_variable cons_cv;

    /// Called by task itself.
    void task_finished (const Task&);

private:
    bool wait_for_finish;
    // std::thread consumer_thread;
    std::vector<std::shared_ptr<Worker>> workers;

    Worker* find_worker (int state);
    bool any_pending () const;
    // void consumer ();
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
    // void merge_output ();
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

// class RenderJob;
// class RenderTask;
// class RenderThread;

// class RenderTask
// {
// public:
//     RenderJob* job;
//     int xofs, yofs;
//     int xres, yres;
//     int spp;
//     std::unique_ptr<Film> film;

//     RenderTask (const RenderTask& o)
//         : job(o.job),
//         xofs(o.xofs), yofs(o.yofs),
//         xres(o.xres), yres(o.yres),
//         spp(o.spp),
//         film(nullptr)
//     { }

//     RenderTask& operator=(const RenderTask& o)
//     {
//         job = o.job;
//         xofs = o.xofs;
//         yofs = o.yofs;
//         xres = o.xres;
//         yres = o.yres;
//         spp = o.spp;
//         film.reset();
//         return *this;
//     }

//     RenderTask () {}

//     RenderTask (RenderJob* job,
//                 int xofs, int yofs,
//                 int xres, int yres,
//                 int spp);

//     void render ();
// };

// class RenderJob
// {
// public:
//     Scene* scene;
//     Film& film;
//     std::mutex mtx;
//     std::condition_variable prod_cv;
//     std::condition_variable cons_cv;
//     std::vector<RenderThread> workers;

//     RenderJob (Scene* scene, Film& film);
//     void add_task (RenderTask task);
//     void finish ();

// private:
//     bool wait_for_finish;
//     std::thread consumer_thread;

//     RenderThread* find_worker (int state);
//     bool any_pending ();
//     void consumer ();

// };


// class RenderThread
// {
// public:
//     RenderJob* job;
//     enum {
//         IDLE,
//         INPUT_READY,
//         WORKING,
//         OUTPUT_READY,
//         QUIT
//     };
//     std::thread th;
//     RenderTask task;
//     std::atomic<int> state;
//     std::condition_variable cv;

//     RenderThread (RenderJob* job);
//     void loop ();
// };

} // namespace threaded_render

#endif // _RENDERJOB_HPP_
