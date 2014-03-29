#ifndef _TIMER_HPP_
#define _TIMER_HPP_

#include <chrono>
#include <ostream>

class Timer
{
public:
    Timer () { start(); }

    void start ()
    { t0 = std::chrono::high_resolution_clock::now(); }

    void stop ()
    {
        t1 = std::chrono::high_resolution_clock::now();
        d = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
    }

    float seconds () const
    {
        return d.count();
    }

private:
    friend std::ostream& operator<< (std::ostream& os, const Timer& tt);
    std::chrono::high_resolution_clock::time_point t0;
    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::duration<double> d;
};

std::ostream& operator<< (std::ostream& os, const Timer& tt)
{
    float t = tt.d.count();
    int h = (int)(t / 3600);
    t -= h * 3600;
    int m = (int)(t / 60);
    t -= m * 60;
    // int s = (int)t;
    // t -= s;
    // int ms = (int)(t*1000);
    if (h > 0) os << h << "h";
    if (m > 0) os << m << "m";
    os.precision(3);
    os.setf( std::ios::fixed, std:: ios::floatfield );
    os << t << "s";
    return os;
}

#endif // _TIMER_HPP_
