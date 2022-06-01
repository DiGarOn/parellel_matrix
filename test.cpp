#include "main.cpp"
#include <chrono>

int main() {

    ofstream out;
	out.open("output.txt");
	if(!(out.is_open())){
		cout << "file not find\n";
		exit(0);
	}


    matrix<int> mat_1, mat_2, mat_3;
    mat_1 = matrix<int>::init_random(100);
    mat_2 = matrix<int>::init_random(100);

    out << "const size\n";
    for(int i = 1; i < 50; i++) {
        set_threads_num(i);

        auto start = std::chrono::high_resolution_clock::now();
        mat_3 = product(mat_1, mat_2);
        auto end = std::chrono::high_resolution_clock::now();

        out << i << " " << std::chrono::duration<double, std::milli>(end - start).count() << "\n";

    }

    set_threads_num(16);
    out << "\nconst thread_num\n";
    for(int i = 1; i < 50; i++) {
        mat_1 = matrix<int>::init_random(i);
        mat_2 = matrix<int>::init_random(i);
 

        auto start = std::chrono::high_resolution_clock::now();
        mat_3 = product(mat_1, mat_2);
        auto end = std::chrono::high_resolution_clock::now();

        out << i << " " << std::chrono::duration<double, std::milli>(end - start).count() << " ";

        start = std::chrono::high_resolution_clock::now();
        mat_3 = mat_1 * mat_2;
        end = std::chrono::high_resolution_clock::now();

        out << " " << std::chrono::duration<double, std::milli>(end - start).count() << "\n";
    }

    return 0;
}