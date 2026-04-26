#include <iostream>
#include <cmath>
#include <chrono>

#define TSTEPS 1000
#define N 4000
#define DATA_TYPE float
#define TEST_REPEAT_TIME 1

void init_array(int n, DATA_TYPE A[N], DATA_TYPE B[N]) {
    for (int i = 0; i < n; i++) {
        A[i] = ((DATA_TYPE)i + 2) / n;
        B[i] = ((DATA_TYPE)i + 3) / n;
    }
}

void print_array(int n, DATA_TYPE A[N]) {
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) std::cout << "\n";
        std::cout << A[i] << " ";
    }
    std::cout << std::endl;
}

void kernel_jacobi_1d(int tsteps, int n, DATA_TYPE A[N], DATA_TYPE B[N]) {
    for (int t = 0; t < tsteps; t++) {
        for (int i = 1; i < n - 1; i++) {
            B[i] = 0.33333 * (A[i - 1] + A[i] + A[i + 1]);
        }
        for (int i = 1; i < n - 1; i++) {
            A[i] = 0.33333 * (B[i - 1] + B[i] + B[i + 1]);
        }
    }
}


int submain() {

    auto start1 = std::chrono::high_resolution_clock::now();

    int n = N;
    int tsteps = TSTEPS;

    DATA_TYPE A[N];
    DATA_TYPE B[N];

    init_array(n, A, B);

    kernel_jacobi_1d(tsteps, n, A, B);

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
