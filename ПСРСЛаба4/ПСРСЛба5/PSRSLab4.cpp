#include <stdio.h>
#include<random>
#include<time.h>
#include <mpi.h>
#define MATR_ROWS 1000
#define MATR_COLS 1000
#define MATR_SIZE 1000000
double ** get_matrix(int n, int m) {//заполнение матрицы случайными значениями
	srand(time(NULL));
	double **matrix = new double*[n];
	for (int i = 0; i < n; i++) {
		matrix[i] = new double[m];
		for (int j = 0; j < m; j++) {
			matrix[i][j] = (double)rand() / RAND_MAX * (100 + 100) - 100;
		}
	}
	return matrix;
}
void matrix_multiple_parallel(double** res, double** matrix1, double** matrix2,//результат и матрицы
	int curr_str, int curr_col,int end_str,int end_col) {//ограничения конкретного процесса
	for (int i = curr_str; i <= end_str; i++) {
		for (int j = curr_col; j <= end_col; j++) {
			for (int k = 0; k < MATR_COLS; k++) {
				res[i][j] += matrix1[i][k] * matrix2[k][j];
			}
		}
	}
}
double ** zero_fill(int rows,int cols) {//получение нулевых матриц для результатов
	double **mas = new double*[rows];
	for (int i = 0; i < rows; i++) {
		mas[i] = new double[cols];
		for (int j = 0; j < cols; j++) {
			mas[i][j] = 0;
		}
	}
	return mas;
}

int main(int argc, char* argv[])
{
	system("chcp 1251 > Nul");
	double**matrix1 = get_matrix(MATR_ROWS, MATR_COLS); //матрицы
	double**matrix2 = get_matrix(MATR_ROWS, MATR_COLS);
	double**result = zero_fill(MATR_ROWS, MATR_COLS); //результат работы потоков
	double **res = zero_fill(MATR_ROWS, MATR_COLS);//конечный массив
	int num_of_proc,status,rank_of_proc; //кол-во процессов, статус инициализации, номер текущего процесса	

	int rows_for_proc, n = MATR_ROWS, p = MATR_COLS, //переменные для распределения строк/столбцов по процессам
		curr_str = -1,	curr_col = -1,end_str = -1,end_col = -1;//переменные-ограничители для процесса		

	if ((status = MPI_Init(&argc, &argv)) != 0) {
		return EXIT_FAILURE;
	
	}

	MPI_Comm_size(MPI_COMM_WORLD, &num_of_proc);//получаем из окружения число процессов
	MPI_Comm_rank(MPI_COMM_WORLD, &rank_of_proc);//получаем из окружения номер процесса
	for(int i=0;i<MATR_COLS;i++)
		MPI_Bcast(matrix1[i], MATR_ROWS, MPI_DOUBLE, 0, MPI_COMM_WORLD); //передача матриц всем процессам
	for (int i = 0; i < MATR_COLS; i++)
		MPI_Bcast(matrix2[i], MATR_ROWS, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	for (int i = 0; i < MATR_COLS; i++)
		MPI_Bcast(result[i], MATR_ROWS, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	rows_for_proc = n * p / num_of_proc; //расчёт показателей процесса
	if (rows_for_proc == 0) {//число процессов больше размера матрицы
		if (rank_of_proc < n * p) {
			curr_str = rank_of_proc / p;
			curr_col = rank_of_proc % p;
			end_str = (rank_of_proc + 1) / p;
			end_col = (rank_of_proc + 1) % p;
		}
	}
	else {//число процессов меньше размера матрицы
		curr_str = (rank_of_proc * rows_for_proc) / p;
		curr_col = (rank_of_proc * rows_for_proc) % p;
		end_str = (rank_of_proc * rows_for_proc + rows_for_proc - 1) / p;
		end_col = (rank_of_proc * rows_for_proc + rows_for_proc - 1) % p;
	}


	if (rank_of_proc == num_of_proc - 1) {//корректировка параметров последнего процесса
		end_str = n - 1;
		end_col = p - 1;
	}

	double time = MPI_Wtime();//отсчёт времени с помощью процессонезависимого таймера
	try {
		matrix_multiple_parallel(result, matrix1, matrix2, curr_str , curr_col, end_str, end_col);
	}
	catch (std::exception ex) {
		printf("Процесс №%d не задействован", rank_of_proc);
	}
	MPI_Barrier(MPI_COMM_WORLD);//синхронизация всех процессов в окружении
	
	for (int i = 0; i < MATR_ROWS; i++) //сборка результата на основе данных из процессов
		MPI_Reduce(res[i], result[i], MATR_COLS, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	
	time = MPI_Wtime() - time;
	if(rank_of_proc==0)
		printf("Parallel algorithm - %f s.\n",time);//вывод времени
	
	MPI_Finalize();

	for (int i = 0; i < MATR_ROWS; i++)
		delete[] matrix1[i];
	delete[] matrix1;
	for (int i = 0; i < MATR_ROWS; i++)
		delete[] matrix2[i];
	delete[] matrix2;
	return EXIT_SUCCESS;
}