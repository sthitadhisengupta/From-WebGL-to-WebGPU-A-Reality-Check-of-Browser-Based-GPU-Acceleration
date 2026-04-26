#include <iostream>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <emscripten/emscripten.h>


typedef double DATA_TYPE;


void init_array(int ni, int nj, int nk, int nl,
                DATA_TYPE &alpha, DATA_TYPE &beta,
                std::vector<std::vector<DATA_TYPE>> &A,
                std::vector<std::vector<DATA_TYPE>> &B,
                std::vector<std::vector<DATA_TYPE>> &C,
                std::vector<std::vector<DATA_TYPE>> &D) {
    alpha = 1.5;
    beta = 1.2;

    for (int i = 0; i < ni; i++)
        for (int j = 0; j < nk; j++)
            A[i][j] = (DATA_TYPE)((i * j + 1) % ni) / ni;

    for (int i = 0; i < nk; i++)
        for (int j = 0; j < nj; j++)
            B[i][j] = (DATA_TYPE)(i * (j + 1) % nj) / nj;

    for (int i = 0; i < nj; i++)
        for (int j = 0; j < nl; j++)
            C[i][j] = (DATA_TYPE)((i * (j + 3) + 1) % nl) / nl;

    for (int i = 0; i < ni; i++)
        for (int j = 0; j < nl; j++)
            D[i][j] = (DATA_TYPE)(i * (j + 2) % nk) / nk;
}

void print_array(int ni, int nl, const std::vector<std::vector<DATA_TYPE>> &D) {
    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nl; j++) {
            if ((i * ni + j) % 20 == 0) std::cout << "\n";
            std::cout << D[i][j] << " ";
        }
    }
    std::cout << std::endl;
}

void kernel_2mm(int ni, int nj, int nk, int nl,
                DATA_TYPE alpha, DATA_TYPE beta,
                std::vector<std::vector<DATA_TYPE>> &tmp,
                const std::vector<std::vector<DATA_TYPE>> &A,
                const std::vector<std::vector<DATA_TYPE>> &B,
                const std::vector<std::vector<DATA_TYPE>> &C,
                std::vector<std::vector<DATA_TYPE>> &D) {

    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nj; j++) {
            tmp[i][j] = 0.0;
            for (int k = 0; k < nk; ++k) {
                tmp[i][j] += alpha * A[i][k] * B[k][j];
            }
        }
    }

    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nl; j++) {
            D[i][j] *= beta;
            for (int k = 0; k < nj; ++k) {
                D[i][j] += tmp[i][k] * C[k][j];
            }
        }
    }
}

int main(int argc, char** argv) {


    auto start = std::chrono::high_resolution_clock::now();


    int ni = 180;
    int nj = 190;
    int nk = 210;
    int nl = 220;

    DATA_TYPE alpha;
    DATA_TYPE beta;



    std::vector<std::vector<DATA_TYPE>> A(ni, std::vector<DATA_TYPE>(nk));
    std::vector<std::vector<DATA_TYPE>> B(nk, std::vector<DATA_TYPE>(nj));
    std::vector<std::vector<DATA_TYPE>> C(nj, std::vector<DATA_TYPE>(nl));
    std::vector<std::vector<DATA_TYPE>> D(ni, std::vector<DATA_TYPE>(nl));
    std::vector<std::vector<DATA_TYPE>> tmp(ni, std::vector<DATA_TYPE>(nj));

    init_array(ni, nj, nk, nl, alpha, beta, A, B, C, D);

    kernel_2mm(ni, nj, nk, nl, alpha, beta, tmp, A, B, C, D);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}
