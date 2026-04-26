#include <iostream>
#include <cmath>
#include <chrono>
#include <emscripten/emscripten.h>

#define N 4000
#define TEST_REPEAT_TIME 1

using namespace std;

typedef float DATA_TYPE;

void init_array(int n, DATA_TYPE r[]) {
    for (int i = 0; i < n; i++) {
        r[i] = static_cast<DATA_TYPE>(n + 1 - i);
    }
}

void print_array(int n, DATA_TYPE y[]) {
    for (int i = 0; i < n; i++) {
        if (i % 20 == 0) cout << "\n";
        cout << y[i] << " ";
    }
    cout << "\n";
}

void kernel_durbin(int n, DATA_TYPE r[], DATA_TYPE y[]) {
    DATA_TYPE z[N]= {0.0f};
    DATA_TYPE alpha;
    DATA_TYPE beta;
    DATA_TYPE sum;

    y[0] = -r[0];
    beta = 1.0;
    alpha = -r[0];

    for (int k = 1; k < n; k++) {
        beta = (1 - alpha * alpha) * beta;
        sum = 0.0;
        for (int i = 0; i < k; i++) {
            sum += r[k - i - 1] * y[i];
        }
        alpha = -(r[k] + sum) / beta;

        for (int i = 0; i < k; i++) {
            z[i] = y[i] + alpha * y[k - i - 1];
        }
        for (int i = 0; i < k; i++) {
            y[i] = z[i];
        }
        y[k] = alpha;
    }
}

int submain() {
    auto start1 = std::chrono::high_resolution_clock::now();
    int n = N;

    DATA_TYPE r[N];
    DATA_TYPE y[N]= {0.0f};

    init_array(n, r);

    kernel_durbin(n, r, y);

    auto end1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed1 = end1 - start1;

	std::cout << "Time taken: " << elapsed1.count() << " seconds" << std::endl;

    return 0;
}

int main() {
    for (int i = 0; i < TEST_REPEAT_TIME; ++i) {
        submain();
    }
    return 0;
}
