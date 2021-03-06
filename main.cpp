#include <iostream>
#include <fstream>
#include <math.h>
#include <thread>
#include <future>
#include <mutex>
#include <pthread.h>
#include <cstdlib>
#include <initializer_list>

using namespace std;

static int thread_num = 10;

template<typename T>
class matrix {
public:
    int ncols;
    int nrows;
    T ** data;
    void init_from_file(ifstream *); // check
    void print_to_file(ofstream *); // check
    
    static matrix init_random(int size);

    matrix() : ncols(0), nrows(0) {}; // check
    matrix(size_t, size_t); // check
    matrix(ifstream *); // check
    matrix(const matrix<T> &);
    matrix(matrix<T> &&);
    matrix(initializer_list<tuple<int, int, T>>);

    matrix operator * (const matrix&); // check + by async
    matrix operator * (int); // check

    matrix operator + (const matrix&); // check
    matrix operator - (const matrix&); // check

    matrix operator ! (); // check + by async
    matrix transpose_matrix(); // check
    int det_matrix(); //check
    matrix AlgDop();  //check 

    bool operator == (const matrix&); // check 
    bool operator != (const matrix&); // check

    bool operator == (int); // check
    bool operator != (int); // check

    matrix& operator = (const matrix&); // check
    matrix& operator = (matrix &&);

    static matrix init_zero(int); // check
    static matrix init_single(int); // check

    //threads
    friend T prod(const matrix<T> & mat_1, const matrix<T> & mat_2, int i, int j, int l);
};

void set_threads_num(int n) {thread_num = n;}
int get_threads_num() {return thread_num;}

template<typename T>
struct matrix_data {
    const matrix<T> * mat_1;
    const matrix<T> * mat_2;
    int i;
    int j;
    matrix<T> * res;
    int res_i;
    matrix_data() : mat_1(nullptr), mat_2(nullptr), i(0), j(0), res(nullptr), res_i(0) {}
};

template<typename T>
T prod(const matrix<T> & mat_1, const matrix<T> & mat_2, int i, int j) {
    T s = 0;
    for(int k = 0; k < mat_1.ncols; k++) {
        s += mat_1.data[i][k] * mat_2.data[k][j];
    }
    return s;
}

template<typename T>
void * prod_posix(void * d) { // ?????????????????? ?? ????????????????????
    matrix_data<int>* dat = static_cast<matrix_data<int>*>(d);
    int s = 0;
    for(int k = 0; k < dat->mat_1->ncols; k++) {
        s += dat->mat_1->data[dat->i][k] * dat->mat_2->data[k][dat->j];
    }
    //cout << dat->i << " " << dat->j << " " << s << endl;
    dat->res->data[dat->i][dat->j] = s;
    pthread_exit(nullptr);
}

template<typename T>
matrix<T> product_posix(const matrix<T> & mat_1, const matrix<T> & mat_2) {
    if(!(mat_1.ncols == mat_2.nrows)) {
        cout << "u can't multiply this matrices" << endl;
        exit(0);
    }
    int rc;
    pthread_attr_t attr;

    matrix<T> result(mat_1.nrows, mat_2.ncols);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t ** res = new pthread_t*[mat_1.nrows];
    for(int i = 0; i < mat_1.nrows; i++) { res[i] = new pthread_t[mat_2.ncols]; }
    
    //function<void*(void *)> cprod_posix = prod_posix<T>;

    matrix_data<T> ** a = new matrix_data<T> * [mat_1.nrows];
    for(int i = 0; i < mat_1.nrows; i++) { 
        a[i] = new matrix_data<T> [mat_2.ncols]; 
        for(int j = 0; j < mat_2.ncols; j++) {
            a[i][j].mat_1 = &mat_1;
            a[i][j].mat_2 = &mat_2;
            a[i][j].res = &result;
        }
    }

    int val = mat_1.ncols * mat_1.nrows;
    int start_i = 0;
    int start_j = 0;

    while(val >= thread_num) {
        bool flag = false;
        int i;
        int j = start_j;
        for(i = start_i; i < mat_1.nrows; i++) {
            while(j < mat_2.ncols) {
                if(i*mat_1.nrows + j - start_i*mat_1.nrows - start_j + 1 >= thread_num) { 
                    flag = true; 
                    break; 
                }
                a[i][j].i = i;
                a[i][j].j = j;
                rc = pthread_create(&res[i][j], &attr, prod_posix<T>, (void *)&a[i][j]);
                //cout << i << " " << j << endl;
                if (rc) {
                    printf("ERROR; return code from pthread_create() is %d\n", rc);
                    exit(-1);
                }
                j++;
            }
            if(flag) break;
            else j = 0;
        }

        flag = false;
        j = start_j;

        pthread_attr_destroy(&attr);
        for(i = start_i; i < mat_1.nrows; i++) {
            while(j < mat_2.ncols) {
                if(i*mat_1.nrows + j - start_i*mat_1.nrows - start_j + 1 >= thread_num) { 
                    flag = true; 
                    break; 
                }
                rc = pthread_join(res[i][j], nullptr);
                //result.data[i][j] = *(T*)(status);
                j++;
            }
            if(flag) break;
            else j = 0;
        }
        start_i = i;
        start_j = j;
        val -= thread_num;
    }

    bool flag = false;
    for(int i = start_i; i < mat_1.nrows; i++) {
        for(int j = start_j; j < mat_2.ncols; j++) {
            a[i][j].i = i;
            a[i][j].j = j;
            rc = pthread_create(&res[i][j], &attr, prod_posix<T>, (void *)&a[i][j]);
            if (rc) {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }
    }

    int i;
    int j;
    for(i = start_i; i < mat_1.nrows; i++) {
        for(j = start_j; j < mat_2.ncols; j++) {
            rc = pthread_join(res[i][j], nullptr);
        }
    }

    delete[]res;
    return result;
}

template<typename T>
matrix<T> product(const matrix<T> & mat_1, const matrix<T> & mat_2) {
    if(!(mat_1.ncols == mat_2.nrows)) {
        cout << "u can't multiply this matrices" << endl;
        exit(0);
    }
    matrix<T> result(mat_1.nrows, mat_2.ncols);

    int val = mat_1.ncols * mat_1.nrows;
    int start_i = 0;
    int start_j = 0;

    future<T>**res = new future<T>*[mat_1.nrows];
    for(int i = 0; i < mat_1.nrows; i++) {res[i] = new future<T>[mat_2.ncols];}

    while (val >= thread_num) {
        bool flag = false;
        int i;
        int j = start_j;
        for(i = start_i; i < mat_1.nrows; i++) {
            while(j < mat_2.ncols) {
                if(i*mat_1.nrows + j - start_i*mat_1.nrows - start_j + 1 > thread_num) { 
                    flag = true; 
                    break; 
                }
                res[i][j] = async(prod<T>, mat_1, mat_2, i, j);//!!!
                j++;
            }
            if(flag) break;
            else j = 0;
        }
        flag = false;
        j = start_j;
        for(i = start_i; i < mat_1.nrows; i++) {
            while(j < mat_2.ncols) {
                if(i*mat_1.nrows + j - start_i*mat_1.nrows - start_j + 1 > thread_num) { 
                    flag = true; 
                    break; 
                }
                result.data[i][j] = res[i][j].get();
                j++;
            }
            if(flag) break;
            else j = 0;
        }
        start_i = i;
        start_j = j;
        val -= thread_num;
    }

    bool flag = false;
    for(int i = start_i; i < mat_1.nrows; i++) {
        for(int j = start_j; j < mat_2.ncols; j++) {
            res[i][j] = async(prod<T>, mat_1, mat_2, i, j);//!!!
        }
    }

    int i;
    int j;
    for(i = start_i; i < mat_1.nrows; i++) {
        for(j = start_j; j < mat_2.ncols; j++) {
            result.data[i][j] = res[i][j].get();
        }
    }
    delete[]res;
    return result;
}

template<typename T>
T AlgDop_i(matrix<T> mat, int i, int j) {
    matrix<T> c (mat.nrows-1, mat.nrows-1);
    bool flag_k = false;
    for(int k = 0; k < mat.nrows; k++) {
        bool flag_l = false;
        for(int l = 0; l < mat.nrows; l++) {
            if(i == k) {
                flag_k = true;
                continue;
            }
            if(j == l) {
                flag_l = true;
                continue;
            }
            int new_k, new_l;
            new_k = flag_k? k-1: k;
            new_l = flag_l? l-1: l;
            c.data[new_k][new_l] = mat.data[k][l];
        }
    }
    return (pow(-1, i+j) * c.det_matrix());
}

template<typename T>
matrix<T> AlgDop(matrix<T> mat) {
	matrix<T> result(mat.nrows,mat.nrows);
    future<T> **res = new future<T>*[mat.nrows];
    for(int i = 0; i < mat.nrows; i++) { res[i] = new future<T>[mat.ncols]; }

    int val = mat.ncols * mat.nrows;
    int start_i = 0;
    int start_j = 0;

    while(val >= thread_num) { 
        bool flag = false;
        int i;
        int j = start_j;   
        for(i = start_i; i < mat.nrows; i++) {
            while(j < mat.ncols) {
                if(i*mat.nrows + j - start_i*mat.nrows - start_j + 1 > thread_num) { 
                    flag = true; 
                    break; 
                }
                res[i][j] = async(AlgDop_i<T>, mat, i, j);//!!!
                j++;
            }
            if(flag) break;
            else j = 0;
        }
        flag = false;
        j = start_j;
        for(int i = 0; i < mat.nrows; i++) {
            while(j < mat.ncols) {
                if(i*mat.nrows + j - start_i*mat.nrows - start_j + 1 > thread_num) { 
                    flag = true; 
                    break; 
                }
                result.data[i][j] = res[i][j].get();
                j++;
            }
            if(flag) break;
            else j = 0;
        }
    }

    bool flag = false;
    for(int i = start_i; i < mat.nrows; i++) {
        for(int j = start_j; j < mat.ncols; j++) {
            res[i][j] = async(AlgDop_i<T>, mat, i, j);//!!!
        }
    }

    int i;
    int j;
    for(i = start_i; i < mat.nrows; i++) {
        for(j = start_j; j < mat.ncols; j++) {
            result.data[i][j] = res[i][j].get();
        }
    }

    delete[]res;
    return result;
}

template<typename T>
matrix<T> matrix<T>::operator * (const matrix& nmat) {
    if(!(ncols == nmat.nrows)) {
        cout << "u can't multiply this matrices" << endl;
        exit(0);
    }
    matrix<int> res(nrows, nmat.ncols);
    for(int i = 0; i < nrows; i++) {
        for(int j = 0; j < nmat.ncols; j++) {
            T s = 0;
            for(int k = 0; k < ncols; k++) {
                s += data[i][k]*nmat.data[k][j];
            }
            res.data[i][j] = s;
        }
    }

    return res;
}

template<typename T>
matrix<T> back_posix(matrix<T> & mat) {
    if(mat.ncols != mat.nrows) {
        cout << "u can't do it" << endl;
        exit(0);
    }
    int det = mat.det_matrix();
    if(det == 0) {
		cout << "u can't do it" << endl;
        exit(0);
	}
	matrix<T> res(mat.ncols,mat.ncols);

    res = mat.transpose_matrix();
	res = AlgDop_posix(res);
	for(int i = 0; i < mat.ncols; i++) {
		for(int j = 0; j < mat.ncols; j++) {
			res.data[i][j] /= det*1.0;
		}
	}
    
	return res;  	
}

template<typename T>
matrix<T> back(matrix<T> & mat) {
    cout << "here";
    if(mat.ncols != mat.nrows) {
        cout << "u can't do it" << endl;
        exit(0);
    }
    cout << "here";
    int det = mat.det_matrix();
    if(det == 0) {
		cout << "u can't do it" << endl;
        exit(0);
	}
	matrix<T> res(mat.ncols,mat.ncols);
    cout << "here";
    res = mat.transpose_matrix();
	res = AlgDop(res);
    cout << "here";
	for(int i = 0; i < mat.ncols; i++) {
		for(int j = 0; j < mat.ncols; j++) {
			res.data[i][j] /= det*1.0;
		}
	}
    
	return res;  	
}

template<typename T>
void* AlgDop_i_posix(void* d) {
    matrix_data<int>* dat = static_cast<matrix_data<int>*>(d);
    
    matrix<T> c (dat->mat_1->nrows-1, dat->mat_1->nrows-1);
    bool flag_k = false;
    for(int k = 0; k < dat->mat_1->nrows; k++) {
        bool flag_l = false;
        for(int l = 0; l < dat->mat_1->nrows; l++) {
            if(dat->i == k) {
                flag_k = true;
                continue;
            }
            if(dat->j == l) {
                flag_l = true;
                continue;
            }
            int new_k, new_l;
            new_k = flag_k? k-1: k;
            new_l = flag_l? l-1: l;
            c.data[new_k][new_l] = dat->mat_1->data[k][l];
        }
    }
    dat->res->data[dat->i][dat->j] = pow(-1, dat->i+dat->j) * c.det_matrix();
    return (nullptr);
}

template<typename T>
matrix<T> AlgDop_posix(matrix<T> mat) {
	matrix<T> result(mat.nrows,mat.nrows);
    int rc;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t ** res = new pthread_t*[mat.nrows];
    for(int i = 0; i < mat.nrows; i++) { res[i] = new pthread_t[mat.ncols]; }

    matrix_data<T> ** a = new matrix_data<T> * [mat.nrows];
    for(int i = 0; i < mat.nrows; i++) { 
        a[i] = new matrix_data<T> [mat.ncols]; 
        for(int j = 0; j < mat.ncols; j++) {
            a[i][j].mat_1 = &mat;
            a[i][j].res = &result;
        }
    }

	for(int i = 0; i < mat.nrows; i++) {
		for(int j = 0; j < mat.nrows; j++) {
            a[i][j].i = i;
            a[i][j].j = j;
            rc = pthread_create(&res[i][j], &attr, AlgDop_i_posix<T>, (void *)&a[i][j]);
            if (rc) {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
		}
	}

    pthread_attr_destroy(&attr);
    for(int i = 0; i < mat.nrows; i++) {
        for(int j = 0; j < mat.ncols; j++) {
            rc = pthread_join(res[i][j], nullptr);
            //result.data[i][j] = *(T*)(status);
        }
    }
    delete[]res;
    return result;
}

template<typename T>
matrix<T> matrix<T>::operator !() {
    if(ncols != nrows) {
        cout << "u can't do it" << endl;
        exit(0);
    }
    int det = det_matrix();
    if(det == 0) {
		cout << "u can't do it" << endl;
        exit(0);
	}
	matrix<T> res(ncols,ncols);
    
	res = transpose_matrix();
	res = res.AlgDop();
	for(int i = 0; i < ncols; i++) {
		for(int j = 0; j < ncols; j++) {
			res.data[i][j] /= det*1.0;
		}
	}
    
	return res;  	
}

template<typename T>
matrix<T> matrix<T>::operator * (int num) {
    matrix<T> res;
    res = *this;
    for(int i = 0; i < nrows; i++) {
        for(int j = 0; j < ncols; j++) {
            res.data[i][j] *= num;
        }
    }
    return res;
}

template<typename T>
void matrix<T>::init_from_file(ifstream * in) {
	*in >> nrows;
    *in >> ncols;
	
	data = new T * [nrows];
	for(int i = 0; i < nrows; i++) {
		data[i] = new T [ncols];
		for(int j = 0; j < ncols; j++) {
			if(!(*in >> data[i][j])) {
				cout << "?? ???????????????? ??????-???? ???? ??????\n";
				exit(0);
			}
		}
	}
}

template<typename T>
void matrix<T>::print_to_file(ofstream * out) {
    for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++) {
			*out << data[i][j] << " ";
		}
		*out << endl;
	}
	*out << "_" << endl;
}

template<typename T>
matrix<T>::matrix(size_t rows, size_t cols) : nrows(rows), ncols(cols) {
    data = new T * [nrows];
	for(int i = 0; i < nrows; i++) {
		data[i] = new T [ncols];
		for(int j = 0; j < ncols; j++) {
			data[i][j] = 0;
		}
	}
}

template<typename T>
matrix<T>::matrix(ifstream * in) {
    init_from_file(in);
} 

template<typename T>
matrix<T>& matrix<T>::operator = (const matrix& nmat) {
    nrows = nmat.nrows;
    ncols = nmat.ncols;
    data = new T * [nrows];
	for(int i = 0; i < nrows; i++) {
		data[i] = new T [ncols];
		for(int j = 0; j < ncols; j++) {
			data[i][j] = nmat.data[i][j];
		}
	}
    return *this;
}

template<typename T>
matrix<T>& matrix<T>::operator = (matrix && nmat) {
    ncols = nmat.ncols;
    nrows = nmat.nrows;
    data = nmat.data;
    nmat.ncols = 0;
    nmat.nrows = 0;
    nmat.data = nullptr;
    return *this;
}

template<typename T>
matrix<T>::matrix(const matrix<T> & nmat) {
    ncols = nmat.ncols;
    nrows = nmat.nrows;
    data = new T*[nrows];
    for(int i = 0; i < nrows; i++) {
        data[i] = new T[ncols]; 
        for(int j = 0; j < ncols; j++) {
            data[i][j] = nmat.data[i][j];
        }
    }
}


template<typename T>
matrix<T>::matrix(matrix<T> && nmat) : ncols(nmat.ncols), nrows(nmat.nrows), data(nmat.data) {
    nmat.ncols = 0;
    nmat.nrows = 0;
    nmat.data = nullptr;
}

template<typename T>
matrix<T> matrix<T>::operator + (const matrix& nmat) {
    if(!((nrows == nmat.nrows)&&(ncols == nmat.ncols))) {
        cout << "u can't fold this matrices" << endl;
        exit(0);
    }
    matrix<int> res(nrows, ncols);
    for(int i = 0; i < nrows; i++) {
        for(int j = 0; j < ncols; j++) {
            res.data[i][j] = data[i][j] + nmat.data[i][j];
        }
    }
    return res;
}

template<typename T>
matrix<T> matrix<T>::operator - (const matrix& nmat) {
    if(!((nrows == nmat.nrows)&&(ncols == nmat.ncols))) {
        cout << "u can't subtract this matrices" << endl;
        exit(0);
    }
    matrix<int> res(nrows, ncols);
    for(int i = 0; i < nrows; i++) {
        for(int j = 0; j < ncols; j++) {
            res.data[i][j] = data[i][j] - nmat.data[i][j];
        }
    }

    return res;
}

template<typename T>
bool matrix<T>::operator == (const matrix& nmat) {
    if(!((ncols == nmat.ncols)&&(nrows==nmat.nrows))) return false;
    for(int i = 0; i < nrows; i++) {
        for(int j = 0; j < ncols; j++) {
            if(!(data[i][j]==nmat.data[i][j])) return false;
        }
    }
    return true;
}

template<typename T>
bool matrix<T>::operator != (const matrix& nmat) {
    return !(*this == nmat);
}

template<typename T>
bool matrix<T>::operator == (int num) {
    if(ncols == 0 || nrows == 0) return false;
    if(!(ncols == nrows)) return false;
    if(num == 1) {
        for(int i = 0; i < nrows; i++) {
            for(int j = 0; j < ncols; j++) {
                if(i == j) {
                    if(data[i][j] != 1) return false;
                } else {
                    if(data[i][j] != 0) return false;
                }
            }
        }
    } else if(num == 0) {
        for(int i = 0; i < nrows; i++) {
            for(int j = 0; j < ncols; j++) {
                if(data[i][j] != 0) return false;
            }
        }
    } else {
        cout << "u can't do it" << endl;
        exit(0);
    }
    return true;
}

template<typename T>
bool matrix<T>::operator != (int num) {
    if(num == 1) {
        return !(*this == 1);
    } else if(num == 0) {
        return !(*this == 0);
    }
    cout << "u can't do it" << endl;
    exit(0);
}

template<typename T>
matrix<T> matrix<T>::init_zero(int num) {
    matrix<T> res(num,num);
    for(int i = 0; i < num; i++) {
        for(int j = 0; j < num; j++) {
            res.data[i][j] = 0;
        }
    } 
    return res;
}

template<typename T>
matrix<T> matrix<T>::init_random(int size) {
    matrix<T> res(size, size);
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            res.data[i][j] = rand();
        }
    } 
    return res;
}

template<typename T>
matrix<T> matrix<T>::init_single(int num) {
    matrix<T> res(num,num);
    for(int i = 0; i < num; i++) {
        for(int j = 0; j < num; j++) {
            if(i == j) res.data[i][j] = 1;
            else res.data[i][j] = 0;
        }
    } 
    return res;
}

template<typename T>
int matrix<T>::det_matrix() {
	if(nrows == 1){
		return data[0][0];
	} else if(nrows == 2){
		return data[0][0]*data[1][1]-data[0][1]*data[1][0];
	} else { 
		int res = 0;
		int one = 1;
		for (int k = 0; k < nrows; k++) {
            matrix<T> m(nrows-1,nrows-1);
            for (int i = 1; i < nrows; i++) {
                int t = 0;
                for (int j = 0; j < nrows; j++) {
                    if (j == k)
                        continue;
                    m.data[i-1][t] = data[i][j];
                    t++;
                }
            }
            res += one * data[0][k] * m.det_matrix();
            one *= -1; 
        }
        return res;
	} 
} 

template<typename T>
matrix<T> matrix<T>::AlgDop() {
	matrix<T> res(nrows,nrows);
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < nrows; j++) {
			matrix<T> c (nrows-1, nrows-1);
			bool flag_k = false;
			for(int k = 0; k < nrows; k++) {
				bool flag_l = false;
				for(int l = 0; l < nrows; l++) {
					if(i == k) {
						flag_k = true;
						continue;
					}
					if(j == l) {
						flag_l = true;
						continue;
					}
					int new_k, new_l;
					new_k = flag_k? k-1: k;
					new_l = flag_l? l-1: l;
					c.data[new_k][new_l] = data[k][l];
				}
			}
			res.data[i][j] = pow(-1, i+j) * c.det_matrix();
		}
	}
	return res;
}

template<typename T>
matrix<T> matrix<T>::transpose_matrix() {
	matrix<T> c(nrows,nrows);
	for(int i = 0; i < nrows; i++) {
		for(int j = 0; j < nrows; j++) {
			c.data[i][j] = data[j][i]; 
		}
	}
	return c;	
}

//________________________________________________________________________________________________________
template<typename T>
matrix<T>::matrix(initializer_list<tuple<int, int, T>> l) {
    int rows, cols;
    rows = cols = 0; 
    for(auto i : l) {
        if(rows < get<0>(i)) rows = get<0>(i);
        if(cols < get<1>(i)) cols = get<0>(i);
    }
    nrows = rows + 1;
    ncols = cols + 1;
    data = new T*[rows];
    for(int i = 0; i < nrows; i++) {
        data[i] = new T[ncols];
    }   
    for(auto i : l) {
        data[get<0>(i)][get<1>(i)] = get<2>(i);
    }
}

 int main() {
    ifstream in;
	in.open("input.txt");
	if(!(in.is_open())){
		cout << "file not find\n";
		exit(0);
	}

    ofstream out;
	out.open("output.txt");
	if(!(out.is_open())){
		cout << "file not find\n";
		exit(0);
	}

    matrix<int> mat_1 {make_tuple<int, int, int>(0,0,1), make_tuple<int, int, int>(0,1,2),
                        make_tuple<int, int, int>(1,0,3), make_tuple<int, int, int>(1,1,4)};
    mat_1.print_to_file(&out);

    return 0;
}
