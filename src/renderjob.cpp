#include "renderjob.hpp"
#include "gray.hpp"
#include "util.hpp"
#include <iostream>

namespace threaded_render {

Job::Job (const Scene& scene, Film& film)
    : scene(scene), film(film)
{
    // Initialize workers.
    for (int i = 0; i < 3; i++) {
        workers.push_back(std::make_shared<Worker>(this));
    }
}

void Job::add_task (const TaskDesc& desc)
{
    // Get idle Worker.
    std::unique_lock<std::mutex> lck(mtx);
    Worker* w;
    while ((w=find_worker(Worker::IDLE)) == nullptr) prod_cv.wait(lck);

    // Assign work.
    w->task = Task(this, desc);

    // Start worker.
    w->state = Worker::INPUT_READY;
    lck.unlock();
    w->cv.notify_all();
}

void Job::finish ()
{
    wait_for_finish = true;

    std::cout << "Waiting until all threads are done...\n";
    std::unique_lock<std::mutex> lck(mtx);
    while (any_pending()) prod_cv.wait(lck);
    lck.unlock();

    // cons_cv.notify_all();
    // consumer_thread.join();
    for (auto& w : workers) {
        std::cout << "Stopping thread " << w->th.get_id() << "..." << std::endl;
        w->state = Worker::QUIT;
        w->cv.notify_all();
        w->th.join();
    }
}

Worker* Job::find_worker (int state)
{
    for (auto& w : workers) {
        if (w->state == state) return w.get();
    }
    return nullptr;
}

bool Job::any_pending () const
{
    for (const auto& w : workers) {
        if (w->state != Worker::IDLE) return true;
    }
    return false;
}

////

Task::Task (Job* job, const TaskDesc& desc)
    : TaskDesc(desc),
    job(job),
    film(new Film(xres, yres))
{ }

void Task::render ()
{
    std::unique_ptr<SurfaceIntegrator> surf_integ(SurfaceIntegrator::make());

    Camera* cam = job->scene.camera.get();
    std::cout << xofs << ", " << yofs << std::endl;
    for (int ly = 0; ly < yres; ly++) {
        for (int lx = 0; lx < xres; lx++) {
            int gx = xofs + lx;
            int gy = yofs + ly;
            srand(gx + gy * job->film.xres);
            for (int s = 0; s < spp; s++) {
                float dx = frand();
                float dy = frand();

                float flx = (lx+dx) / xres;
                float fly = (ly+dy) / yres;
                float fgx = (gx+dx) / job->film.xres;
                float fgy = (gy+dy) / job->film.yres;

                Ray ray(cam->generate_ray(fgx, fgy,
                                          frand(), frand()));

                Spectrum L = surf_integ->Li(ray, &job->scene);
                film->add_sample(flx, fly, L);
            }
        }
    }   
}

void Task::merge_output ()
{
    // Process result.
    job->film.merge(*film, xofs, yofs);
}

////

Worker::Worker (Job* job)
    : job(job), state(IDLE)
{
    th = std::thread(&Worker::loop, this);
}

void Worker::loop ()
{
    while (true) {
        std::unique_lock<std::mutex> lck(job->mtx);
        // while (state != INPUT_READY && state != QUIT) cv.wait(lck);
        while (state != INPUT_READY && state != QUIT) {
            cv.wait(lck);
            std::cout << "woke " << std::this_thread::get_id() << " state " << state << std::endl;
        }
        if (state == QUIT) return;
        state = WORKING;
        lck.unlock();

        task.render();

        lck.lock();
        task.merge_output();
        state = IDLE;
        lck.unlock();
        job->prod_cv.notify_all();
        // job->cons_cv.notify_all();
    }
}

} // namespace threaded_render













#if 0

RenderTask::RenderTask (RenderJob* job,
                        int xofs, int yofs,
                        int xres, int yres,
                        int spp)
    : job(job),
    xofs(xofs), yofs(yofs),
    xres(xres), yres(yres),
    spp(spp)
{ }

void RenderTask::render ()
{
    // Allocate film here as a form of lazy initialization,
    // in case we have lots of tasks just lying around.
    film.reset( new Film(xres, yres) );

    std::unique_ptr<SurfaceIntegrator> surf_integ(SurfaceIntegrator::make());

    Camera* cam = job->scene->camera.get();

    for (int ly = 0; ly < yres; ly++) {
        for (int lx = 0; lx < xres; lx++) {
            int gx = xofs + lx;
            int gy = yofs + ly;
            srand(gx + gy * job->film.xres);
            for (int s = 0; s < spp; s++) {
                float dx = frand();
                float dy = frand();

                float flx = (lx+dx) / xres;
                float fly = (ly+dy) / yres;
                float fgx = (gx+dx) / job->film.xres;
                float fgy = (gy+dy) / job->film.yres;

                Ray ray(cam->generate_ray(fgx, fgy,
                                          frand(), frand()));

                Spectrum L = surf_integ->Li(ray, job->scene);
                film->add_sample(flx, fly, L);
            }
        }
    }   
}


/////////////////////////////////////////////////////////////////////

RenderJob::RenderJob (Scene* scene, Film& film)
    : scene(scene), film(film)
{
    for (int i = 0; i < 3; i++) {
        workers.emplace_back();
    }
}

void RenderJob::add_task (RenderTask task)
{
    // Get idle RenderThread.
    std::unique_lock<std::mutex> lck(mtx);
    RenderThread* w;
    while ((w=find_worker(RenderThread::IDLE)) == nullptr) prod_cv.wait(lck);

    // Assign work.
    w->task = task;

    // Start worker.
    w->state = RenderThread::INPUT_READY;
    lck.unlock();
    w->cv.notify_all();
}

void RenderJob::finish ()
{
    wait_for_finish = true;
    cons_cv.notify_all();
    consumer_thread.join();
    for (RenderThread& w : workers) {
        w.state = RenderThread::QUIT;
        w.cv.notify_all();
        w.th.join();
    }
}

RenderThread* RenderJob::find_worker (int state)
{
    for (RenderThread& w : workers) {
        if (w.state == state) return &w;
    }
    return nullptr;
}

bool RenderJob::any_pending ()
{
    for (RenderThread& w : workers) {
        if (w.state != RenderThread::IDLE) return true;
    }
    return false;
}

void RenderJob::consumer ()
{
    while (true)
    {
        // Get finished worker.
        std::unique_lock<std::mutex> lck(mtx);
        RenderThread* w;
        while (any_pending() && (w=find_worker(RenderThread::OUTPUT_READY)) == nullptr) cons_cv.wait(lck);
        if (!any_pending()) break;

        // Process result.
        film.merge(*w->task.film, w->task.xofs, w->task.yofs);

        // Make worker available.
        w->state = RenderThread::IDLE;
        lck.unlock();
        prod_cv.notify_all();
    }
}

///////////////////////////////////////////////////////////

RenderThread::RenderThread (RenderJob* job)
    : job(job), state(IDLE)
{
    th = std::thread(&RenderThread::loop, this);
}

void RenderThread::loop ()
{
    while (true) {
        std::unique_lock<std::mutex> lck(job->mtx);
        while (state != INPUT_READY && state != QUIT) cv.wait(lck);
        if (state == QUIT) return;
        state = WORKING;
        lck.unlock();

        task.render();

        lck.lock();
        state = OUTPUT_READY;
        lck.unlock();
        job->cons_cv.notify_all();
    }
}
#endif