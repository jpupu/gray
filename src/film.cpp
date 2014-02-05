#include "film.hpp"
#include "lodepng.h"
extern "C" {
#  include "rgbe.h"
}
// #include <cmath>
#include <cstdint>
#include <memory>
using std::auto_ptr;

Film::Film (int xres, int yres)
    : xres(xres), yres(yres), data(xres*yres)
{ }

void Film::add_sample (float x, float y, const Spectrum& s)
{
    int xi = clamp((int)(x*xres), 0, xres-1);
    int yi = clamp((int)(y*yres), 0, yres-1);
    data[xi + yi*xres].add(s, 1.0);
}

void Film::merge (const Film& film, int xofs, int yofs)
{
    for (int y = 0; y < film.yres; y++) {
        for (int x = 0; x < film.xres; x++) {
            Pixel& dst = data[xofs + x + (yofs+ y)*xres];
            const Pixel& src = film.data[x+y*film.xres];
            dst.add(src.L, src.weight);
        }
    }
}


void Film::save (const char* filename)
{
    save_png(filename);
}

void Film::save_png (const char* filename)
{
    tone_mapping();
    std::vector<uint8_t> rgb(xres*yres*3);

    for (int y = 0; y < yres; y++) {
        for (int x = 0; x < xres; x++) {
            int i = x + y*xres;
            int o = x + (yres-1-y)*xres;
            Spectrum L = data[i].tonemapped;
            L = clamp(L*255.0f, Spectrum(0), Spectrum(255));
            for (int k = 0; k < 3; k++) {
                rgb[o*3+k] = L[k];
            }
        }
    }

    lodepng_encode24_file(filename, &rgb[0], xres, yres);
}

void Film::save_rgbe (const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    RGBE_WriteHeader(fp, xres, yres, NULL);
    for (int y = yres-1; y >= 0; y--) {
        for (int x = 0; x < xres; x++) {
            Spectrum v = data[x+y*xres].normalized();
            RGBE_WritePixels(fp, &v.x, 1);
        }
    }
    fclose(fp);
}

void Film::save_float (const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    fwrite(&xres, 4, 1, fp);
    fwrite(&yres, 4, 1, fp);
    for (int y = 0; y < yres; y++) {
        for (int x = 0; x < xres; x++) {
            Spectrum v = data[x+y*xres].normalized();
            fwrite(&v, sizeof(Spectrum), 1, fp);
        }
    }
    fclose(fp);
}

void Film::load_float (const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    fread(&xres, 4, 1, fp);
    fread(&yres, 4, 1, fp);
    data.resize(xres*yres);
    for (int y = 0; y < yres; y++) {
        for (int x = 0; x < xres; x++) {
            Spectrum v;
            fread(&v, sizeof(Spectrum), 1, fp);
            data[x+y*xres].L = v;
            data[x+y*xres].weight = 1;
        }
    }
    fclose(fp);
}

void Film::tone_mapping ()
{
    // Erik Reinhard
    // Photographic Tone Reproduction for Digital Images
    // (Reinhard '02)

    int N = xres*yres;

    for (int ch = 0; ch < 3; ch++) {

        double sum(0.0);
        double Lmin = 999999;
        double Lmax = 0;
        for (auto& d : data) {
            double Lw = d.normalized()[ch]*10;
            sum += log(0.00001 + Lw);
            if (Lw < Lmin) Lmin = Lw;
            if (Lw > Lmax) Lmax = Lw;
        }
        // Log average luminance, Lw
        double Lw = exp(sum /double(N));

        printf("Range of values: %f -- %f\n", Lmin, Lmax);
        printf("Dynamix range:   %d:1\n", (int)(Lmax/Lmin));

        // the image key value 
        const double alpha(0.18);
        const double Lwhite = Lmax;
        /// the following are from Erik Reinhard:
        /// Parameter Estimation for Photographic Tone Reproduction
        // const double alpha = 0.18 * pow(4, (2*log2(Lw)-log2(Lmin)-log2(Lmax)) / (log2(Lmax)-log2(Lmin)));
        // const double Lwhite = 1.2 * pow(2, log2(Lmax) - log2(Lmin) - 5);
        const double Lwhite2 = Lwhite*Lwhite;
        const double gamma = 2.2;
        for (auto& d : data) {
            double L = alpha / Lw * d.normalized()[ch];

            double Ld = L * (1 + L / Lwhite2) / (1 + L);

            d.tonemapped[ch] = pow(Ld, 1/gamma);

        }
    }

}

