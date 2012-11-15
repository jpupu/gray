#include "Transform.hpp"
#include "film.hpp"

int main (int argc, char* argv[])
{
    Film film(256,256);

    const float X = 1.0/256;
    const float H = X/2;
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 64; x++) {
            film.add_sample(x*X+H, y*X+H, 10.0f*Spectrum((y+x)&1));
        }
        for (int x = 64; x < 128; x++) {
            film.add_sample(x*X+H, y*X+H, 10.0f*Spectrum(y<85?68/255.:y<170?.5:187/255.));
        }
        for (int x = 128; x < 192; x++) {
            film.add_sample(x*X+H, y*X+H, 10.0f*Spectrum(y/255.));
        }
        for (int x = 192; x < 256; x++) {
            film.add_sample(x*X+H, y*X+H, 10.0f*Spectrum((rand()%256)<y));
        }
    }
    film.save("/tmp/test.png");

    return 0;
}
