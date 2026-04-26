#include <iostream>
#include <cmath>
#include <chrono>

#define TSTEPS 1000 
#define N 200  
#define DATA_TYPE double 

void init_array(int n, DATA_TYPE A_flat[N * N * N], DATA_TYPE B_flat[N * N * N]) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                int index = i * (n * n) + j * n + k;
                A_flat[index] = B_flat[index] = (DATA_TYPE)(i + j + (n - k)) * 10 / n;
            }
        }
    }
}

void print_array(int n, DATA_TYPE A_flat[N * N * N]) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                int index = i * (n * n) + j * n + k;
                if (index % 20 == 0) std::cout << "\n";
                std::cout << A_flat[index] << " ";
            }
        }
    }
    std::cout << std::endl;
}

void kernel_heat_3d(int tsteps, int n, DATA_TYPE A_flat[N * N * N], DATA_TYPE B_flat[N * N * N]) {
    for (int t = 1; t <= tsteps; t++) {
        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                for (int k = 1; k < n - 1; k++) {
                    int index = i * (n * n) + j * n + k;
                    int index_xp = (i + 1) * (n * n) + j * n + k;
                    int index_xm = (i - 1) * (n * n) + j * n + k;
                    int index_yp = i * (n * n) + (j + 1) * n + k;
                    int index_ym = i * (n * n) + (j - 1) * n + k;
                    int index_zp = i * (n * n) + j * n + (k + 1);
                    int index_zm = i * (n * n) + j * n + (k - 1);

                    B_flat[index] = 0.125 * (A_flat[index_xp] - 2.0 * A_flat[index] + A_flat[index_xm])
                                  + 0.125 * (A_flat[index_yp] - 2.0 * A_flat[index] + A_flat[index_ym])
                                  + 0.125 * (A_flat[index_zp] - 2.0 * A_flat[index] + A_flat[index_zm])
                                  + A_flat[index];
                }
            }
        }

        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                for (int k = 1; k < n - 1; k++) {
                    int index = i * (n * n) + j * n + k;
                    int index_xp = (i + 1) * (n * n) + j * n + k;
                    int index_xm = (i - 1) * (n * n) + j * n + k;
                    int index_yp = i * (n * n) + (j + 1) * n + k;
                    int index_ym = i * (n * n) + (j - 1) * n + k;
                    int index_zp = i * (n * n) + j * n + (k + 1);
                    int index_zm = i * (n * n) + j * n + (k - 1);

                    A_flat[index] = 0.125 * (B_flat[index_xp] - 2.0 * B_flat[index] + B_flat[index_xm])
                                  + 0.125 * (B_flat[index_yp] - 2.0 * B_flat[index] + B_flat[index_ym])
                                  + 0.125 * (B_flat[index_zp] - 2.0 * B_flat[index] + B_flat[index_zm])
                                  + B_flat[index];
                }
            }
        }
    }
}

int submain() {

    auto start1 = std::chrono::high_resolution_clock::now();

    int n = N;
    int tsteps = TSTEPS;

    DATA_TYPE A_flat[N * N * N];
    DATA_TYPE B_flat[N * N * N];

    init_array(n, A_flat, B_flat);

    kernel_heat_3d(tsteps, n, A_flat, B_flat);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;

    return 0;
}

int main() {
    const int TEST_REPEAT_TIME = 1;
    for (int i = 0; i < TEST_REPEAT_TIME; ++i) {
        submain();
    }
    return 0;
}