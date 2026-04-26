#include <iostream>
#include <cmath>
#include <chrono>

#define NQ 220
#define NR 250
#define NP 270
#define TEST_REPEAT_TIME 1

using namespace std;

typedef double DATA_TYPE;

void init_array(int nr, int nq, int np,
                DATA_TYPE A_flat[NR * NQ * NP],
                DATA_TYPE C4_flat[NP * NP]) {
    int i, j, k;

    for (i = 0; i < nr; i++)
        for (j = 0; j < nq; j++)
            for (k = 0; k < np; k++)
                A_flat[i * (nq * np) + j * np + k] = static_cast<DATA_TYPE>((i * j + k) % np) / np;

    for (i = 0; i < np; i++)
        for (j = 0; j < np; j++)
            C4_flat[i * np + j] = static_cast<DATA_TYPE>((i * j) % np) / np;
}


void print_array(int nr, int nq, int np,
                 DATA_TYPE A_flat[NR * NQ * NP]) {
    int i, j, k;

    for (i = 0; i < nr; i++)
        for (j = 0; j < nq; j++)
            for (k = 0; k < np; k++) {
                if ((i * nq * np + j * np + k) % 20 == 0)
                    cout << "\n";
                cout << A_flat[i * (nq * np) + j * np + k] << " ";
            }
}

void kernel_doitgen(int nr, int nq, int np,
                    DATA_TYPE A_flat[NR * NQ * NP],
                    DATA_TYPE C4_flat[NP * NP],
                    DATA_TYPE sum[NP]) {
    int r, q, p, s;

    for (r = 0; r < nr; r++)
        for (q = 0; q < nq; q++) {
            for (p = 0; p < np; p++) {
                sum[p] = 0.0;
                for (s = 0; s < np; s++)
                    sum[p] += A_flat[r * (nq * np) + q * np + s] * C4_flat[s * np + p];
            }
            for (p = 0; p < np; p++)
                A_flat[r * (nq * np) + q * np + p] = sum[p];
        }
}

int submain(int argc, char** argv) {
    
    auto start1 = std::chrono::high_resolution_clock::now();

    int nr = NR;
    int nq = NQ;
    int np = NP;

    DATA_TYPE A_flat[NR * NQ * NP];
    DATA_TYPE sum[NP];
    DATA_TYPE C4_flat[NP * NP];

    init_array(nr, nq, np, A_flat, C4_flat);

    kernel_doitgen(nr, nq, np, A_flat, C4_flat, sum);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    for (int i = 0; i < TEST_REPEAT_TIME; ++i)
        submain(argc, argv);
    return 0;
}
