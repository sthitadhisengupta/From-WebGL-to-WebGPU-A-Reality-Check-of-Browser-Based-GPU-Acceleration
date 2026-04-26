#include <iostream>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

using DATA_TYPE = double;

void init_array(int n, int m, DATA_TYPE& alpha, DATA_TYPE& beta,
                vector<vector<DATA_TYPE>>& C, vector<vector<DATA_TYPE>>& A, vector<vector<DATA_TYPE>>& B) {
    alpha = 1.5;
    beta = 1.2;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            A[i][j] = static_cast<DATA_TYPE>((i * j + 1) % n) / n;
            B[i][j] = static_cast<DATA_TYPE>((i * j + 2) % m) / m;
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = static_cast<DATA_TYPE>((i * j + 3) % n) / m;
        }
    }
}

void print_array(int n, const vector<vector<DATA_TYPE>>& C) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if ((i * n + j) % 20 == 0) cout << endl;
            cout << C[i][j] << " ";
        }
    }
    cout << endl;
}

void kernel_syr2k(int n, int m, DATA_TYPE alpha, DATA_TYPE beta,
                  vector<vector<DATA_TYPE>>& C, const vector<vector<DATA_TYPE>>& A, const vector<vector<DATA_TYPE>>& B) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            C[i][j] *= beta;
        }
        for (int k = 0; k < m; k++) {
            for (int j = 0; j <= i; j++) {
                C[i][j] += A[j][k] * alpha * B[i][k] + B[j][k] * alpha * A[i][k];
            }
        }
    }
}

int main(int argc, char** argv) {

    auto start = chrono::high_resolution_clock::now();

    int m = 2000;
    int n = 2600;

    DATA_TYPE alpha;
    DATA_TYPE beta;
    vector<vector<DATA_TYPE>> C(n, vector<DATA_TYPE>(n));
    vector<vector<DATA_TYPE>> A(n, vector<DATA_TYPE>(m));
    vector<vector<DATA_TYPE>> B(n, vector<DATA_TYPE>(m));

    init_array(n, m, alpha, beta, C, A, B);

    kernel_syr2k(n, m, alpha, beta, C, A, B);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "Execution time: " << elapsed.count() << " seconds" << endl;

    return 0;
}
