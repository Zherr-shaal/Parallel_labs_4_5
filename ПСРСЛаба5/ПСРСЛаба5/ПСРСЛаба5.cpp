// ПСРСЛаба5.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <random>
#include <iostream>
#include <ctime>
#include "mpi.h"
#define SIZE_MATRIX 500



double * get_Slau (int dim) {//заполнение СЛАУ случайными значениями
	srand(NULL);
	double* res = new double[dim*dim];
	for (int i = 0; i < dim; i++) {
		for (int j = 0; j < dim; j++)
			res[i * dim + j] = (double)rand() / RAND_MAX * (100 + 100);
	}
	return res;
}


int main(int argc, char* argv[]) {

	double time;//время

	int rank_of_proc, num_of_proc; //номер процесса и число процессов

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank_of_proc);
	MPI_Comm_size(MPI_COMM_WORLD, &num_of_proc);

	int current_num_of_row = SIZE_MATRIX / num_of_proc; //количество строчек на процесс


	double* matrix = get_Slau(SIZE_MATRIX); //получение матрицы коэффициентов СЛАУ

	double* current_matrix = new double[SIZE_MATRIX * current_num_of_row]; //матрица для частей СЛАУ в процессах

	MPI_Scatter(matrix, SIZE_MATRIX * current_num_of_row, MPI_DOUBLE, current_matrix, //передача данных в процессы   
		SIZE_MATRIX * current_num_of_row, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	double* rows = new double[SIZE_MATRIX * current_num_of_row];//матрица для получения строк в процессе

	if (rank_of_proc == 0) {
		time = MPI_Wtime();//отсчёт времени
	}

	for (int i = 0; i < (rank_of_proc * current_num_of_row); i++) {//редукция строк
		MPI_Bcast(rows, SIZE_MATRIX, MPI_DOUBLE, i / current_num_of_row, MPI_COMM_WORLD); //передача матрицы для результата всем процессам

		for (int j = 0; j < current_num_of_row; j++) {
			for (int k = i + 1; k < SIZE_MATRIX; k++) {
				current_matrix[j * SIZE_MATRIX + k] -= current_matrix[j * SIZE_MATRIX + i] * rows[k]; //выполнение редукции
			}
			current_matrix[j * SIZE_MATRIX + i] = 0; //обнуление элементов под главной диагональю
		}
	}

	int current_col;//текущий столбец
	for (int i = 0; i < current_num_of_row; i++) {//Нормировка строк 
		current_col = rank_of_proc * current_num_of_row + i; //расчёт столбца для процесса
		for (int j = current_col; j < SIZE_MATRIX; j++) {
			current_matrix[i * SIZE_MATRIX + j] /= current_matrix[i * SIZE_MATRIX + current_col];//выполнение нормировки
		}

		current_matrix[i * SIZE_MATRIX + current_col] = 1; //элемент главной диагонали равен 1

		memcpy(rows, current_matrix + (i * SIZE_MATRIX), SIZE_MATRIX * sizeof(double)); //копирование результатов применения метода Гаусса во временную переменную

		MPI_Bcast(rows, SIZE_MATRIX, MPI_DOUBLE, rank_of_proc, MPI_COMM_WORLD); //передача изменённых строк для дальнейшей обработки

		for (int j = i + 1; j < current_num_of_row; j++) {//редукция для изменившихся строк
			for (int k = current_col + 1; k < SIZE_MATRIX; k++) {
				current_matrix[j * SIZE_MATRIX + k] -= current_matrix[j * SIZE_MATRIX + i] * rows[k];
			}
			current_matrix[j * SIZE_MATRIX + current_col] = 0; //обнуление элементов под главной диагональю
		}
	}



	MPI_Barrier(MPI_COMM_WORLD); //синхронизация

	if (rank_of_proc == 0) {
		time = MPI_Wtime() - time;
	}

	MPI_Gather(current_matrix, SIZE_MATRIX * current_num_of_row, MPI_DOUBLE, matrix, //сбор результата из нескольких процессов
		SIZE_MATRIX * current_num_of_row, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	if (rank_of_proc == 0) {//печать результата
		std::cout << "Size = " << SIZE_MATRIX << "\nTime: " << time << "second" << std::endl;
	}

	MPI_Finalize();



	delete[] matrix;
	delete[] current_matrix;
	delete[] rows;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
