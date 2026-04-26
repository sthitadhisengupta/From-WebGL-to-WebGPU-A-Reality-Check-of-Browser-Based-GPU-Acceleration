#include <iostream>
#include <cmath>
#include <chrono>

#define W 4096
#define H 2160
#define TEST_REPEAT_TIME 1

using DATA_TYPE = float;

void init_array(int w, int h, DATA_TYPE& alpha, DATA_TYPE imgIn[W * H], DATA_TYPE imgOut[W * H]) {
    alpha = 0.25; 

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            imgIn[i * H + j] = static_cast<DATA_TYPE>((313 * i + 991 * j) % 65536) / 65535.0f;
        }
    }
}

void print_array(int w, int h, DATA_TYPE imgOut[W * H]) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if ((i * h + j) % 20 == 0) std::cout << "\n";
            std::cout << imgOut[i * H + j] << " ";
        }
    }
    std::cout << std::endl;
}


void kernel_deriche(int w, int h, DATA_TYPE alpha, DATA_TYPE imgIn[W * H], DATA_TYPE imgOut[W * H], DATA_TYPE y1[W * H], DATA_TYPE y2[W * H]) {
    DATA_TYPE xm1, tm1, ym1, ym2;
    DATA_TYPE xp1, xp2;
    DATA_TYPE tp1, tp2;
    DATA_TYPE yp1, yp2;

    DATA_TYPE k, a1, a2, a3, a4, a5, a6, a7, a8, b1, b2, c1, c2;

    k = (1.0 - exp(-alpha)) * (1.0 - exp(-alpha)) / (1.0 + 2.0 * alpha * exp(-alpha) - exp(2.0 * alpha));
    a1 = a5 = k;
    a2 = a6 = k * exp(-alpha) * (alpha - 1.0);
    a3 = a7 = k * exp(-alpha) * (alpha + 1.0);
    a4 = a8 = -k * exp(-2.0 * alpha);
    b1 = pow(2.0, -alpha);
    b2 = -exp(-2.0 * alpha);
    c1 = c2 = 1.0;

    for (int i = 0; i < w; i++) {
        ym1 = ym2 = xm1 = 0.0;
        for (int j = 0; j < h; j++) {
            int idx = i * H + j;
            y1[idx] = a1 * imgIn[idx] + a2 * xm1 + b1 * ym1 + b2 * ym2;
            xm1 = imgIn[idx];
            ym2 = ym1;
            ym1 = y1[idx];
        }
    }

    for (int i = 0; i < w; i++) {
        yp1 = yp2 = xp1 = xp2 = 0.0;
        for (int j = h - 1; j >= 0; j--) {
            int idx = i * H + j;
            y2[idx] = a3 * xp1 + a4 * xp2 + b1 * yp1 + b2 * yp2;
            xp2 = xp1;
            xp1 = imgIn[idx];
            yp2 = yp1;
            yp1 = y2[idx];
        }
    }

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = i * H + j;
            imgOut[idx] = c1 * (y1[idx] + y2[idx]);
        }
    }

    for (int j = 0; j < h; j++) {
        tm1 = ym1 = ym2 = 0.0;
        for (int i = 0; i < w; i++) {
            int idx = i * H + j;
            y1[idx] = a5 * imgOut[idx] + a6 * tm1 + b1 * ym1 + b2 * ym2;
            tm1 = imgOut[idx];
            ym2 = ym1;
            ym1 = y1[idx];
        }
    }

    for (int j = 0; j < h; j++) {
        tp1 = tp2 = yp1 = yp2 = 0.0;
        for (int i = w - 1; i >= 0; i--) {
            int idx = i * H + j;
            y2[idx] = a7 * tp1 + a8 * tp2 + b1 * yp1 + b2 * yp2;
            tp2 = tp1;
            tp1 = imgOut[idx];
            yp2 = yp1;
            yp1 = y2[idx];
        }
    }


    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = i * H + j;
            imgOut[idx] = c2 * (y1[idx] + y2[idx]);
        }
    }
}

int submain() {
    auto start1 = std::chrono::high_resolution_clock::now();

    int w = W;
    int h = H;

    DATA_TYPE alpha;
    DATA_TYPE imgIn[W * H];
    DATA_TYPE imgOut[W * H];
    DATA_TYPE y1[W * H];
    DATA_TYPE y2[W * H];

    init_array(w, h, alpha, imgIn, imgOut);

    kernel_deriche(w, h, alpha, imgIn, imgOut, y1, y2);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;

    return 0;
}

int main() {
    for (int i = 0; i < TEST_REPEAT_TIME; ++i)
        submain();
    return 0;
}