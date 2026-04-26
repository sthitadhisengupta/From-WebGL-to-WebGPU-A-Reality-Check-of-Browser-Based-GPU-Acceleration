#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;
using namespace chrono;

using DATA_TYPE = double;

void init_array(int n, DATA_TYPE& alpha, DATA_TYPE& beta, vector<vector<DATA_TYPE>>& A, vector<vector<DATA_TYPE>>& B, vector<DATA_TYPE>& x) {
    alpha = 1.5;
    beta = 1.2;
    for (int i = 0; i < n; i++) {
        x[i] = static_cast<DATA_TYPE>(i % n) / n;
        for (int j = 0; j < n; j++) {
            A[i][j] = static_cast<DATA_TYPE>((i * j + 1) % n) / n;
            B[i][j] = static_cast<DATA_TYPE>((i * j + 2) % n) / n;
        }
    }
}

void print_array(int n, const vector<DATA_TYPE>& y) {
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) cout << endl;
        cout << y[i] << " ";
    }
    cout << endl;
}

void kernel_gesummv(int n, DATA_TYPE alpha, DATA_TYPE beta, const vector<vector<DATA_TYPE>>& A, const vector<vector<DATA_TYPE>>& B, vector<DATA_TYPE>& tmp, const vector<DATA_TYPE>& x, vector<DATA_TYPE>& y) {
    for (int i = 0; i < n; i++) {
        tmp[i] = 0.0;
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            tmp[i] += A[i][j] * x[j];
            y[i] += B[i][j] * x[j];
        }
        y[i] = alpha * tmp[i] + beta * y[i];
    }
}

int main(int argc, char** argv) {
    
    auto start = chrono::high_resolution_clock::now();

    int n = 2800;

    DATA_TYPE alpha;
    DATA_TYPE beta;
    vector<vector<DATA_TYPE>> A(n, vector<DATA_TYPE>(n));
    vector<vector<DATA_TYPE>> B(n, vector<DATA_TYPE>(n));
    vector<DATA_TYPE> tmp(n);
    vector<DATA_TYPE> x(n);
    vector<DATA_TYPE> y(n);

    init_array(n, alpha, beta, A, B, x);

    kernel_gesummv(n, alpha, beta, A, B, tmp, x, y);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "Execution time: " << elapsed.count() << " seconds" << endl;

    return 0;
}
