#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <bulk/bulk.hpp>
#include <bulk/backends/mpi/mpi.hpp>
#include <iostream>

#define DEFAULT_K 5
#define DEFAULT_NITERS 20

struct point_t {
  double x;
  double y;
};

double get_time() {
  struct timeval tv;
  gettimeofday(&tv, (struct timezone *)0);
  return ((double)tv.tv_sec + (double)tv.tv_usec / 1000000.0);
}

void read_points(std::string filename, bulk::partitioned_array<point_t,1,1>& p, int n, int displacement) {
  std::ifstream infile{filename};
  double x, y;
  int i = 0;
  // go to the line of the file indicated by displacement and read n values
  infile.seekg(std::ios::beg);
  for (int currLineNumber = 0; currLineNumber < displacement; ++currLineNumber)
      infile.ignore(std::numeric_limits<std::streamsize>::max(), infile.widen('\n'));

  while (infile >> x >> y) {
    if (i >= n) {
        break;
    }
    p.local({i}).x = x;
    p.local({i}).y = y;
    i++;
  }
}

void write_result(std::string filename, int niters, point_t *result, int k) {
  std::ofstream outfile{filename};
  for (int iter = 0; iter < niters + 1; ++iter) {
    for (int i = 0; i < k; ++i) {
      outfile << iter << ' ' << result[iter * k + i].x << ' '
              << result[iter * k + i].y << '\n';
    }
  }
}

void init_centroids(bulk::coarray<point_t> &centroids, int k, int numprocs) {
  for (int i = 0; i < k; ++i) {
      double x = rand() % 100;
      double y = rand() % 100;
      for (int t = 0; t < numprocs; ++t) {
          centroids(t)[i] = {x, y};
      }
  }
}

double euclid_dist(point_t p, point_t q) {
    return ((p.x - q.x) * (p.x - q.x))
                + ((p.y - q.y) * (p.y - q.y)) ;
}

template<typename T, typename Func>
void reduce_coarray(bulk::coarray<T>& coarray, Func f, int size, int root, bulk::world& world) {
    std::vector<bulk::future<T[]>> futcounts; 
    if (world.rank() == root) {
        for (int p = 0; p < world.active_processors(); ++p) {
            if (p != root)
                futcounts.push_back(coarray(p)[{0, size}].get());
        }
    }

    world.sync();

    if (world.rank() == root) {
        for (auto it = futcounts.begin(); it != futcounts.end(); ++it) {
            for (int i = 0; i < size; ++i) {
                f(coarray[i], (*it)[i]);
            }
        }
    }
}

// the k-means algorithm
void k_means(int niters,
             bulk::partitioned_array<point_t,1,1>& points,
             bulk::coarray<point_t> &centroids,
             int* assignment,
             bulk::coarray<int>& countassignment,
             bulk::coarray<point_t>& sumassignment,
             bulk::coarray<bool>& changedassign,
             bulk::var<bool>& stop,
             point_t *result, 
             int local_size,
             int k,
             bulk::world& world) {
    auto myrank = world.rank();
    auto numprocs = world.active_processors();

    for (int iter = 0; iter < niters; ++iter) {

        bool changed = false;

        // assignment step (locally)
        for (int i = 0; i < local_size; ++i) {
            //double min_dist = DBL_MAX;
            double min_dist = euclid_dist(points.local({i}), centroids[assignment[i]]);
            for (int j = 0; j < k; ++j) {
                double dist = euclid_dist(points.local({i}), centroids[j]);

                if (dist < min_dist) {
                    min_dist = dist;
                    assignment[i] = j;
                    changed = true;
                }
            }
        }

        changedassign(0)[myrank] = changed;
        
        // sum up counts and assignment sum values (locally)
        for (int i = 0; i < k; ++i) {
            countassignment[i] = 0;
            sumassignment[i] = {0, 0};
        }

        // update centroids
        for (int i = 0; i < local_size; ++i) {
            sumassignment[assignment[i]].x += points.local({i}).x;
            sumassignment[assignment[i]].y += points.local({i}).y;
            countassignment[assignment[i]]++;
        }

        // communicate counts and assignment sum values
        reduce_coarray(countassignment, [](int& lhs, int rhs) { lhs += rhs; }, k, 0, world);
        reduce_coarray(sumassignment, [](point_t& lhs, point_t rhs) { lhs.x += rhs.x; lhs.y += rhs.y; }, k, 0, world);

        // update step: recalculate new centroids, broadcast new centroids
        if (myrank == 0) {
            // Did the assignments change?
            bool changed = false;
            for (int i = 0; i < numprocs; ++i) {
                if (changedassign[i]) {
                    changed = true;
                    break;
                }
            }

            if (!changed)
                stop.broadcast(true);

            for (int i = 0; i < k; ++i) {
                if (countassignment[i] != 0) {
                    centroids[i].x = sumassignment[i].x / countassignment[i];
                    centroids[i].y = sumassignment[i].y / countassignment[i];
                }
                result[(iter+1) * k + i].x = centroids[i].x;
                result[(iter+1) * k + i].y = centroids[i].y;
            }

            // broadcast new centroids
            for (int t = 0; t < numprocs; ++t) {
                for (int i = 0; i < k; ++i) {
                    centroids(t)[i] = {centroids[i].x, centroids[i].y};
                }
            }
        }

        world.sync();

        if (stop)
            return;
    }
}

int main(int argc, const char *argv[]) {
  // Handle input arguments
  if (argc < 3 || argc > 5) {
    printf("Usage: %s <input file> <num points> <num centroids> <num iters>\n",
           argv[0]);
    return EXIT_FAILURE;
  }
  const char *input_file = argv[1];
  const int n = atoi(argv[2]);
  const int k = (argc > 3 ? atoi(argv[3]) : DEFAULT_K);
  const int niters = (argc > 4 ? atoi(argv[4]) : DEFAULT_NITERS);

  // Spawn SPMD block
  bulk::mpi::environment env;
  env.spawn(env.available_processors(), [n,k,niters,input_file](auto &world) {
      // Get own ranks and number of processors
      int myrank = world.rank();
      int numprocs = world.active_processors();

      // Simple block partitioning of points, the points are parsed later
      // on from the given input file in a distributed way.
      bulk::block_partitioning<1,1> part({n}, {numprocs});

      // Allocate partitioned array for points
      bulk::partitioned_array<point_t,1,1> points(world, part);

      // Get local size of array and position in global index space
      auto local_size = part.local_size(myrank)[0];
      auto displacement = part.global({0}, myrank)[0];

      // Every processor reads a certain part of the input file
      read_points(input_file, points, local_size, displacement);

      // Centroids are stored in a coarray
      bulk::coarray<point_t> centroids(world, k);

      // Rank 0 initiates centroids and broadcasts them to ALL ranks
      if (myrank == 0) {
          srand(1234);
          init_centroids(centroids, k, numprocs);
      }

      // Convergence check
      bulk::coarray<bool> changedassign(world, myrank == 0 ? numprocs : 0);
      bulk::var<bool> stop(world);
      stop = false;

      if (myrank == 0) {
          for (int i = 0; i < numprocs; ++i)
              changedassign[i] = false;
      }

      world.sync(); // Centroids now available at all ranks

      // Result vector, only allocated and initialized locally at rank 0
      point_t *result = nullptr;
      if (myrank == 0) {
          result = static_cast<point_t *>(malloc((niters + 1) * k * sizeof(point_t)));
          // Rest initialized with -1
          for (int i = 0; i < (niters + 1) * k; ++i) {
              result[i].x = -1.0;
              result[i].y = -1.0;
          }

          // First result vector
          for (int i = 0; i < k; ++i) {
              result[i].x = centroids[i].x;
              result[i].y = centroids[i].y;
          }
      }

      // Allocate and initialize rest of required coarrays 
      int *assignment = static_cast<int *>(malloc(local_size * sizeof(int)));
      bulk::coarray<int> countassignments(world, k);
      bulk::coarray<point_t> sumassignments(world, k);

      for (int i = 0; i < local_size; ++i)
          assignment[i] = 0;


      if (world.rank() == 0) {
          world.log("Running with %d processes...", numprocs);
          world.log("Executing k-means clustering with %d iterations, %d points, and %d "
         "centroids...\n", niters, n, k);
      }

      world.sync();

      double runtime = get_time();
      k_means(niters, points, centroids, assignment, countassignments, sumassignments, changedassign, stop, result, local_size, k, world);
      // Add possibly required additional parameters
      //k_means(niters, points, centroids, assignment, result, n, k);
      world.sync();
      runtime = get_time() - runtime;
      if (world.rank() == 0) {
          printf("Time Elapsed: %f s\n", runtime);
          // Verify the results by visualization. Result contains space for the
          // position of the centroids of the initial configuration and at every
          // iteration of the algorithm. For debugging purposes, the array is set to
          // values outside the domain.
          write_result("result.out", niters, result, k);
      }

      free(assignment);
      free(result);

  });
  
  return EXIT_SUCCESS;
}
