#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>

#define M 1800
#define N 2200
#define DATA_TYPE float
#define DATA_PRINTF_MODIFIER "%.2f "
#define TEST_REPEAT_TIME 1

void init_array(int m, int n, DATA_TYPE A[M * N], DATA_TYPE x[N]) {
    DATA_TYPE fn = (DATA_TYPE)n;
    
    for (int i = 0; i < n; i++)
        x[i] = 1 + (i / fn);
        
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            A[i * n + j] = (DATA_TYPE)((i + j) % n) / (5 * m);
        }
    }
}

void print_array(int n, DATA_TYPE y[N]) {
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) std::cout << "\n";
        std::cout << y[i] << " ";
    }
    std::cout << "\n";
}

void kernel_atax(int m, int n, DATA_TYPE A[M * N], DATA_TYPE x[N], DATA_TYPE y[N], DATA_TYPE tmp[M]) {
    for (int i = 0; i < n; i++) 
        y[i] = 0;
        
    for (int i = 0; i < m; i++) {
        tmp[i] = 0.0;
        for (int j = 0; j < n; j++)
            tmp[i] += A[i * n + j] * x[j]; 
            
    }
    
    for (int j = 0; j < n; j++){
        for (int i = 0; i < m; i++) {
            y[j] += A[i * n + j] * tmp[i];
        }
    }
}

int submain() {

    auto start1 = std::chrono::high_resolution_clock::now();

    int m = M;
    int n = N;

    DATA_TYPE A[M * N];
    DATA_TYPE x[N];
    DATA_TYPE y[N];
    DATA_TYPE tmp[M];

    init_array(m, n, A, x);

    kernel_atax(m, n, A, x, y, tmp);

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
