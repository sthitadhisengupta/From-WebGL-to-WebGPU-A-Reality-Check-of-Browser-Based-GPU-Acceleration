#include <iostream>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

using DATA_TYPE = double;

void init_array(int m, int n, vector<vector<DATA_TYPE>>& A,
                vector<DATA_TYPE>& r, vector<DATA_TYPE>& p) {
    for (int i = 0; i < m; i++)
        p[i] = static_cast<DATA_TYPE>(i % m) / m;
    for (int i = 0; i < n; i++) {
        r[i] = static_cast<DATA_TYPE>(i % n) / n;
        for (int j = 0; j < m; j++)
            A[i][j] = static_cast<DATA_TYPE>(i * (j + 1) % n) / n;
    }
}

void print_array(int m, int n, const vector<DATA_TYPE>& s, const vector<DATA_TYPE>& q) {
    cout << "Array s:" << endl;
    for (int i = 0; i < m; i++) {
        if (i % 20 == 0) cout << endl;
        cout << s[i] << " ";
    }
    cout << endl;

    cout << "Array q:" << endl;
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) cout << endl;
        cout << q[i] << " ";
    }
    cout << endl;
}

void kernel_bicg(int m, int n, vector<vector<DATA_TYPE>>& A,
                 vector<DATA_TYPE>& s, vector<DATA_TYPE>& q,
                 const vector<DATA_TYPE>& p, const vector<DATA_TYPE>& r) {
    for (int i = 0; i < m; i++)
        s[i] = 0.0;
    for (int i = 0; i < n; i++) {
        q[i] = 0.0;
        for (int j = 0; j < m; j++) {
            s[j] += r[i] * A[i][j];
            q[i] += A[i][j] * p[j];
        }
    }
}

int main(int argc, char** argv) {

    auto start = chrono::high_resolution_clock::now();

    int m = 1800;
    int n = 2200;

    vector<vector<DATA_TYPE>> A(n, vector<DATA_TYPE>(m));
    vector<DATA_TYPE> s(m), q(n), p(m), r(n);

    init_array(m, n, A, r, p);

    kernel_bicg(m, n, A, s, q, p, r);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "Execution time: " << elapsed.count() << " seconds" << endl;

    return 0;
}
