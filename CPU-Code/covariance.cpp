#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <emscripten/emscripten.h>
#include <chrono>

constexpr int M = 2600;
constexpr int N = 3000;

using DATA_TYPE = double;

void init_array(int m, int n,
                DATA_TYPE &float_n,
                std::vector<std::vector<DATA_TYPE>> &data) {
    float_n = static_cast<DATA_TYPE>(n);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            data[i][j] = static_cast<DATA_TYPE>(i * j) / M;
        }
    }
}

void print_array(int m,
                 const std::vector<std::vector<DATA_TYPE>> &cov) {
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < m; ++j) {
            if ((i * m + j) % 20 == 0) std::cout << std::endl;
            std::cout << std::fixed << std::setprecision(2) << cov[i][j] << " ";
        }
    }
    std::cout << std::endl;
}

void kernel_covariance(int m, int n,
                       DATA_TYPE float_n,
                       std::vector<std::vector<DATA_TYPE>> &data,
                       std::vector<std::vector<DATA_TYPE>> &cov,
                       std::vector<DATA_TYPE> &mean) {
    for (int j = 0; j < m; ++j) {
        mean[j] = 0.0;
        for (int i = 0; i < n; ++i) {
            mean[j] += data[i][j];
        }
        mean[j] /= float_n;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            data[i][j] -= mean[j];
        }
    }

    for (int i = 0; i < m; ++i) {
        for (int j = i; j < m; ++j) {
            cov[i][j] = 0.0;
            for (int k = 0; k < n; ++k) {
                cov[i][j] += data[k][i] * data[k][j];
            }
            cov[i][j] /= (float_n - 1.0);
            cov[j][i] = cov[i][j];
        }
    }
}

int main() {

    auto start = std::chrono::high_resolution_clock::now();

    int n = N;
    int m = M;

    DATA_TYPE float_n;
    std::vector<std::vector<DATA_TYPE>> data(n, std::vector<DATA_TYPE>(m));
    std::vector<std::vector<DATA_TYPE>> cov(m, std::vector<DATA_TYPE>(m));
    std::vector<DATA_TYPE> mean(m);

    init_array(m, n, float_n, data);

    kernel_covariance(m, n, float_n, data, cov, mean);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}
