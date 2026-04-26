#include <iostream>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

using DATA_TYPE = double;

void init_array(int n, vector<DATA_TYPE>& x1, vector<DATA_TYPE>& x2,
                vector<DATA_TYPE>& y_1, vector<DATA_TYPE>& y_2, vector<vector<DATA_TYPE>>& A) {
    for (int i = 0; i < n; i++) {
        x1[i] = static_cast<DATA_TYPE>(i % n) / n;
        x2[i] = static_cast<DATA_TYPE>((i + 1) % n) / n;
        y_1[i] = static_cast<DATA_TYPE>((i + 3) % n) / n;
        y_2[i] = static_cast<DATA_TYPE>((i + 4) % n) / n;
        for (int j = 0; j < n; j++) {
            A[i][j] = static_cast<DATA_TYPE>(i * j % n) / n;
        }
    }
}

void print_array(int n, const vector<DATA_TYPE>& x1, const vector<DATA_TYPE>& x2) {
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) cout << endl;
        cout << x1[i] << " ";
    }
    cout << endl;

    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) cout << endl;
        cout << x2[i] << " ";
    }
    cout << endl;
}

void kernel_mvt(int n, vector<DATA_TYPE>& x1, vector<DATA_TYPE>& x2,
                const vector<DATA_TYPE>& y_1, const vector<DATA_TYPE>& y_2, const vector<vector<DATA_TYPE>>& A) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            x1[i] += A[i][j] * y_1[j];
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            x2[i] += A[j][i] * y_2[j];
        }
    }
}

int main(int argc, char** argv) {

    auto start = chrono::high_resolution_clock::now();

    int n = 4000;

    vector<DATA_TYPE> x1(n), x2(n), y_1(n), y_2(n);
    vector<vector<DATA_TYPE>> A(n, vector<DATA_TYPE>(n));

    init_array(n, x1, x2, y_1, y_2, A);

    kernel_mvt(n, x1, x2, y_1, y_2, A);


    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "Execution time: " << elapsed.count() << " seconds" << endl;

    return 0;
}