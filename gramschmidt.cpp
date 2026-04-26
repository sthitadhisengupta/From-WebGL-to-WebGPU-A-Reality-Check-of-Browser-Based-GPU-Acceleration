#include <iostream>
#include <cmath>
#include <cstdlib>
#include <chrono>

const int M = 2000;
const int N = 2600;
const int TEST_REPEAT_TIME = 1;
typedef float DATA_TYPE;

void init_array(int m, int n, DATA_TYPE *A, DATA_TYPE *R, DATA_TYPE *Q) {
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++) {
            A[i * N + j] = (((DATA_TYPE)((i * j) % m) / m) * 100) + 10;
            Q[i * N + j] = 0.0;
        }

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            R[i * N + j] = 0.0;
}

void print_array(int m, int n, DATA_TYPE *A, DATA_TYPE *R, DATA_TYPE *Q) {
    std::cout << "R:" << std::endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << R[i * N + j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Q:" << std::endl;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << Q[i * N + j] << " ";
        }
        std::cout << std::endl;
    }
}

void kernel_gramschmidt(int m, int n, DATA_TYPE *A, DATA_TYPE *R, DATA_TYPE *Q) {
    for (int k = 0; k < n; k++) {
        DATA_TYPE nrm = 0.0;
        for (int i = 0; i < m; i++)
            nrm += A[i * N + k] * A[i * N + k];
        
        R[k * N + k] = sqrtf(nrm);
        for (int i = 0; i < m; i++)
            Q[i * N + k] = A[i * N + k] / R[k * N + k];
    
        
        for (int j = k + 1; j < n; j++) {
            R[k * N + j] = 0.0;
            for (int i = 0; i < m; i++)
                R[k * N + j] += Q[i * N + k] * A[i * N + j];
        }
        
        
       for (int j = k + 1; j < n; j++) {
           for (int i = 0; i < m; i++)
                A[i * N + j] = A[i * N + j] - Q[i * N + k] * R[k * N + j];
       }
       
    
    }
}

int submain() {
    auto start1 = std::chrono::high_resolution_clock::now();
    int m = M;
    int n = N;

    DATA_TYPE A[M * N];
    DATA_TYPE R[N * N];
    DATA_TYPE Q[M * N];

    init_array(m, n, A, R, Q);

    kernel_gramschmidt(m, n, A, R, Q);

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