#include <iostream>
#include <cmath>
#include <chrono>

#define TMAX 1000
#define NX 2000
#define NY 2600
#define SCALAR_VAL(x) (x)
#define TEST_REPEAT_TIME 1

typedef float DATA_TYPE;

void init_array(int tmax, int nx, int ny,
                DATA_TYPE ex[NX][NY], DATA_TYPE ey[NX][NY], DATA_TYPE hz[NX][NY], DATA_TYPE _fict_[TMAX]) {
    for (int i = 0; i < tmax; i++) {
        _fict_[i] = static_cast<DATA_TYPE>(i);
    }
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ex[i][j] = static_cast<DATA_TYPE>(i * (j + 1)) / nx;
            ey[i][j] = static_cast<DATA_TYPE>(i * (j + 2)) / ny;
            hz[i][j] = static_cast<DATA_TYPE>(i * (j + 3)) / nx;
        }
    }
}

void print_array(int nx, int ny,
                 DATA_TYPE ex[NX][NY], DATA_TYPE ey[NX][NY], DATA_TYPE hz[NX][NY]) {
    std::cout << "ex array:" << std::endl;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            if ((i * nx + j) % 20 == 0) std::cout << std::endl;
            std::cout << ex[i][j] << " ";
        }
    }
    std::cout << std::endl;

    std::cout << "ey array:" << std::endl;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            if ((i * nx + j) % 20 == 0) std::cout << std::endl;
            std::cout << ey[i][j] << " ";
        }
    }
    std::cout << std::endl;

    std::cout << "hz array:" << std::endl;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            if ((i * nx + j) % 20 == 0) std::cout << std::endl;
            std::cout << hz[i][j] << " ";
        }
    }
    std::cout << std::endl;
}

void kernel_fdtd_2d(int tmax, int nx, int ny,
                    DATA_TYPE ex[NX][NY], DATA_TYPE ey[NX][NY], DATA_TYPE hz[NX][NY], DATA_TYPE _fict_[TMAX]) {
    for (int t = 0; t < tmax; t++) {
        for (int j = 0; j < ny; j++) {
            ey[0][j] = _fict_[t];
        }
        for (int i = 1; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                ey[i][j] -= SCALAR_VAL(0.5) * (hz[i][j] - hz[i - 1][j]);
            }
        }
        for (int i = 0; i < nx; i++) {
            for (int j = 1; j < ny; j++) {
                ex[i][j] -= SCALAR_VAL(0.5) * (hz[i][j] - hz[i][j - 1]);
            }
        }
        for (int i = 0; i < nx - 1; i++) {
            for (int j = 0; j < ny - 1; j++) {
                hz[i][j] -= SCALAR_VAL(0.7) * (ex[i][j + 1] - ex[i][j] + ey[i + 1][j] - ey[i][j]);
            }
        }
    }
}

int submain(int argc, char** argv) {
    auto start1 = std::chrono::high_resolution_clock::now();
 
    int tmax = TMAX;
    int nx = NX;
    int ny = NY;

    DATA_TYPE ex[NX][NY];
    DATA_TYPE ey[NX][NY];
    DATA_TYPE hz[NX][NY];
    DATA_TYPE _fict_[TMAX];

    init_array(tmax, nx, ny, ex, ey, hz, _fict_);

    kernel_fdtd_2d(tmax, nx, ny, ex, ey, hz, _fict_);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;


    return 0;
}

int main(int argc, char** argv) {
    for (int i = 0; i < TEST_REPEAT_TIME; ++i)
        submain(argc, argv);
    return 0;
}
