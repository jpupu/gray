timer.cpp

class Timer
{
public:
    Timer ();

    void start ();
    void stop ();

    float seconds () const;
    std::string to_string () const;

private:
    chrono::high_resolution_clock::time_point t0;
    chrono::high_resolution_clock::time_point t1;
    chrono::duration<double> d;
};