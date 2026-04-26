#include <iostream>
#include <cmath>

#define TSTEPS 1000 
#define N 2800 
#define DATA_TYPE float  
#define SCALAR_VAL(val) val

void init_array(int n, DATA_TYPE A[N][N], DATA_TYPE B[N][N]) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = ((DATA_TYPE)i * (j + 2) + 2) / n;
            B[i][j] = ((DATA_TYPE)i * (j + 3) + 3) / n;
        }
    }
}

void print_array(int n, DATA_TYPE A[N][N]) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << A[i][j] << " ";
            if ((i * n + j) % 20 == 0) std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void kernel_jacobi_2d(int tsteps, int n, DATA_TYPE A[N][N], DATA_TYPE B[N][N]) {
    for (int t = 0; t < tsteps; t++) {
        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                B[i][j] = SCALAR_VAL(0.2) * (A[i][j] + A[i][j-1] + A[i][j+1] + A[i+1][j] + A[i-1][j]);
            }
        }
        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                A[i][j] = SCALAR_VAL(0.2) * (B[i][j] + B[i][j-1] + B[i][j+1] + B[i+1][j] + B[i-1][j]);
            }
        }
    }
}

int submain(int argc, char** argv) {

    auto start1 = std::chrono::high_resolution_clock::now();

    int n = N;
    int tsteps = TSTEPS;

    DATA_TYPE A[N][N];
    DATA_TYPE B[N][N];

    init_array(n, A, B);

    kernel_jacobi_2d(tsteps, n, A, B);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    for (int i = 0; i < 1; ++i) 
        submain(argc, argv);
}
