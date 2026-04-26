#include <iostream>
#include <vector>
#include <cstdlib>
#include <emscripten/emscripten.h>

using namespace std;
using namespace chrono;

using DATA_TYPE = double;

void init_array(int ni, int nj, int nk, int nl, int nm,
                vector<vector<DATA_TYPE>>& A,
                vector<vector<DATA_TYPE>>& B,
                vector<vector<DATA_TYPE>>& C,
                vector<vector<DATA_TYPE>>& D) {
    for (int i = 0; i < ni; i++)
        for (int j = 0; j < nk; j++)
            A[i][j] = (DATA_TYPE)((i * j + 1) % ni) / (5 * ni);
    for (int i = 0; i < nk; i++)
        for (int j = 0; j < nj; j++)
            B[i][j] = (DATA_TYPE)((i * (j + 1) + 2) % nj) / (5 * nj);
    for (int i = 0; i < nj; i++)
        for (int j = 0; j < nm; j++)
            C[i][j] = (DATA_TYPE)(i * (j + 3) % nl) / (5 * nl);
    for (int i = 0; i < nm; i++)
        for (int j = 0; j < nl; j++)
            D[i][j] = (DATA_TYPE)((i * (j + 2) + 2) % nk) / (5 * nk);
}

void print_array(int ni, int nl, const vector<vector<DATA_TYPE>>& G) {
    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nl; j++) {
            if ((i * ni + j) % 20 == 0) cout << endl;
            cout << G[i][j] << " ";
        }
    }
    cout << endl;
}

void kernel_3mm(int ni, int nj, int nk, int nl, int nm,
                vector<vector<DATA_TYPE>>& E,
                const vector<vector<DATA_TYPE>>& A,
                const vector<vector<DATA_TYPE>>& B,
                vector<vector<DATA_TYPE>>& F,
                const vector<vector<DATA_TYPE>>& C,
                const vector<vector<DATA_TYPE>>& D,
                vector<vector<DATA_TYPE>>& G) {
    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nj; j++) {
            E[i][j] = 0.0;
            for (int k = 0; k < nk; ++k)
                E[i][j] += A[i][k] * B[k][j];
        }
    }
    for (int i = 0; i < nj; i++) {
        for (int j = 0; j < nl; j++) {
            F[i][j] = 0.0;
            for (int k = 0; k < nm; ++k)
                F[i][j] += C[i][k] * D[k][j];
        }
    }
    for (int i = 0; i < ni; i++) {
        for (int j = 0; j < nl; j++) {
            G[i][j] = 0.0;
            for (int k = 0; k < nj; ++k)
                G[i][j] += E[i][k] * F[k][j];
        }
    }
}

int main(int argc, char** argv) {

        auto start = std::chrono::high_resolution_clock::now();

        int ni = 1600;
        int nj = 1800;
        int nk = 2000;
        int nl = 2200;
        int nm = 2400;

   
        vector<vector<DATA_TYPE>> A(ni, vector<DATA_TYPE>(nk));
        vector<vector<DATA_TYPE>> B(nk, vector<DATA_TYPE>(nj));
        vector<vector<DATA_TYPE>> C(nj, vector<DATA_TYPE>(nm));
        vector<vector<DATA_TYPE>> D(nm, vector<DATA_TYPE>(nl));
        vector<vector<DATA_TYPE>> E(ni, vector<DATA_TYPE>(nj));
        vector<vector<DATA_TYPE>> F(nj, vector<DATA_TYPE>(nl));
        vector<vector<DATA_TYPE>> G(ni, vector<DATA_TYPE>(nl));

    
        init_array(ni, nj, nk, nl, nm, A, B, C, D);

    
        kernel_3mm(ni, nj, nk, nl, nm, E, A, B, F, C, D, G);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;


        std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;
    

        return 0;
}
