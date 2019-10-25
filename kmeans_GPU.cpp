#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/time.h>

#define DEFAULT_K 5
#define DEFAULT_NITERS 20

struct points_t {
	double x;
	double y;
};

double get_time() {
	struct timeval tv;
	gettimeofday(&tv, (struct timezone*)0);
	return ((double)tv.tv_sec + (double)tv.tv_usec / 1000000.0);
}

void read_points(std::string filename, points_t* p, int n) {
	std::ifstream infile{ filename };
	double x, y;
	int i = 0;
	while (infile >> x >> y) {
		if (i >= n) {
			printf(
				"WARNING: more points in input file '%s' than read: stopping after "
				"%d lines\n",
				filename.c_str(), i);
			return;
		}
		p[i].x = x;
		p[i].y = y;
		i++;
	}
}

void write_result(std::string filename, int niters, points_t* result, int k) {
	std::ofstream outfile{ filename };
	for (int iter = 0; iter < niters + 1; ++iter) {
		for (int i = 0; i < k; ++i) {
			outfile << iter << ' ' << result[iter * k + i].x << ' '
				<< result[iter * k + i].y << '\n';
		}
	}
}

void init_centroids(points_t* centroids, int k) {
	for (int i = 0; i < k; ++i) {
		centroids[i].x = rand() % 100;
		centroids[i].y = rand() % 100;
	}
}


__global__ void k_means(points_t* points, points_t* centroids, int* assignment) {
	int iter;
	int j;
	int tid = blockDim.x * blockIdx.x + threadIdx.x;
	for (iter = 0; iter < NO_ITER; ++iter) {
		if (tid < n) {
			double optimal_dist = DBL_MAX;
			// Calculate Euclidean distance to each centroid and
			determine the closest mean
				for (j = 0; j < K; ++j) {
					double dist = (points[tid].x - centroids[j].x) *
						(points[tid].x - centroids[j].x) +
						(points[tid].y - centroids[j].y) *
						(points[tid].y - centroids[j].y);
					if (dist < optimal_dist) {
						optimal_dist = dist;
						assignment[tid] = j;
					}
				}
		}
		// Calculate new positions of centroids
		if (tid < k) {
			int count = 0;
			double sum_x = 0.0;
			double sum_y = 0.0;
			for (int j = 0; j < n; ++j) {
				if (assignment[j] == tid) {
					sum_x += points[j].x;
					sum_y += points[j].y;
					count++;
				}
			}
			if (count != 0.0) {
				centroids[tid].x = sum_x / count;
				centroids[tid].y = sum_y / count;
			}
			result[(iter + 1) * k + tid].x = centroids[tid].x;
			result[(iter + 1) * k + tid].y = centroids[tid].y;
		}
	}
}

__global__ void calc_distances(point_t* points, point_t* centroids,
	int* assignment, int n, int k) {
	int tid = blockDim.x * blockIdx.x + threadIdx.x;
	if (tid < n) {
		double optimal_dist = DBL_MAX;
		for (int j = 0; j < k; ++j) {
			double dist = (points[tid].x - centroids[j].x) *
				(points[tid].x - centroids[j].x) +
				(points[tid].y - centroids[j].y) *
				(points[tid].y - centroids[j].y);
			if (dist < optimal_dist) {
				optimal_dist = dist;
				assignment[tid] = j;
			}
		}
	}
}

__global__ void update_centroids(int iter, point_t* points,
	point_t* centroids, int* assignment,
	point_t* result, int n, int k) {
	int tid = blockDim.x * blockIdx.x + threadIdx.x;
	if (tid < k) {
		int count = 0;
		double sum_x = 0.0;
		double sum_y = 0.0;
		for (int j = 0; j < n; ++j) {
			if (assignment[j] == tid) {
				sum_x += points[j].x;
				sum_y += points[j].y;
				count++;
			}
		}
		if (count != 0.0) {
			centroids[tid].x = sum_x / count;
			centroids[tid].y = sum_y / count;
		}
		result[(iter + 1) * k + tid].x = centroids[tid].x;
		result[(iter + 1) * k + tid].y = centroids[tid].y;
	}
}

int main(int argc, const char* argv[]) {
	// Handle input arguments
	if (argc < 3 || argc > 5) {
		printf("Usage: %s <input file> <num points> <num centroids> <num iters>\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char* input_file = argv[1];
	const int n = atoi(argv[2]);
	const int k = (argc > 3 ? atoi(argv[3]) : DEFAULT_K);
	const int niters = (argc > 4 ? atoi(argv[4]) : DEFAULT_NITERS);

	// Allocate memory for GPU
	point_t* d_points = 0; 
	point_t* d_centroids = 0;
	point_t* d_result = 0;
	int* d_assignments = 0; 
	cudaMalloc((void**)& d_points, N * sizeof(point_t));
	cudaMalloc((void**)& d_centroids, K * sizeof(point_t));
	cudaMalloc((void**)& d_assignments, N * sizeof(int));
	cudaMalloc((void**)& d_result, (niters + 1) * k * sizeof(point_t));
	// Copy data to GPU
	double runtime_all = get_time();
	cudaMemcpy(d_points, h_points, N * sizeof(point_t), cudaMemcpyHostToDevice);
	cudaMemcpy(d_centroids, h_centroids, K * sizeof(point_t), cudaMemcpyHostToDevice);
	cudaMemcpy(d_assignments, h_assignments, N * sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(d_result, result, (niters + 1) * k * sizeof(point_t), cudaMemcpyHostToDevice);
	
	srand(1234);
	read_points(input_file, points_soa, n);
	init_centroids(centroids, k);

	// Verify the results by visualization. Result contains space for the
	// position of the centroids of the initial configuration and at every
	// iteration of the algorithm. For debugging purposes, the array is set to
	// values outside the domain.

	printf("Executing k-means clustering with %d iterations, %d points, and %d "
		"centroids...\n", niters, n, k);

	// Apply k-means algorithm
	double runtime_kernel = get_time();
	k_means <<<(n + THREADSPERBLOCK - 1) / THREADSPERBLOCK, THREADSPERBLOCK >>> 
		(d_points, d_centroids, d_assignment d_result, n, k);
	cudaDeviceSynchronize();
	runtime_kernel = get_time() - runtime_kernel;
	// Copy results back to host
	cudaMemcpy(result, d_result, (niters + 1) * k * sizeof(point_t), cudaMemcpyDeviceToHost);
	runtime_all = get_time() - runtime_all;

	printf("Time Elapsed: %f s\n", runtime_all);

	// Verify the results by visualization
	write_result("result.out", niters, result, k);
	
	// Free memory
	cudaFree(d_result);
	cudaFree(d_assignment);
	cudaFree(d_centroids);
	cudaFree(d_points);

	return EXIT_SUCCESS;
}
