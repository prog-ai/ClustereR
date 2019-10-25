# ClustereR

Data clustering utility based on K-Means algorithm and adapted to benefit from running on massively parralel systems.

<img src="/utils/ClustereR.png?raw=true">

## Details

Clustering application which makes use of the CUDA/Bulk parralelization for significantly increased performance.
The code has been tested on **Compute Clusters** with nodes having `Intel Xeon Platinum 8160 48x2.1Ghz` and `Nvidia Tesla V100` processing units. 

```shell
+-----------------------------------------------------------------------------+
| NVIDIA-SMI 418.43       Driver Version: 418.43       CUDA Version: 10.1     |
|-------------------------------+----------------------+----------------------+
| GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |
| Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |
|===============================+======================+======================|
|   0  Tesla V100-SXM2...  Off  | 00000000:61:00.0 Off |                    0 |
| N/A   39C    P0    53W / 300W |      0MiB / 16130MiB |      0%   E. Process |
+-------------------------------+----------------------+----------------------+
|   1  Tesla V100-SXM2...  Off  | 00000000:62:00.0 Off |                    0 |
| N/A   40C    P0    55W / 300W |      0MiB / 16130MiB |      3%   E. Process |
+-------------------------------+----------------------+----------------------+
```

## Performance
The **performance boost ~ 231** is significant, but it is worth noting the serial version of the code only used one core of the CPU, so the actual performance gain from using the CUDA acceleration should be arround ~9 in case all 48 cores would have been used.

- Tcpu = 124.785246 s
- Tgpu = 0.607147 s
- Speedup of ~231


## Demo

**Output visualization**

<img src="/utils/kmean.png" alt="k-means" width="1200" align="middle"/>

## Package

- Codebase for whole utility
- Makefile for different targets as explained below
- Small input file with 1000 elements and a large one with 1000000
- R Visualization Scripts contains a setup script `install-prerequisites.sh` and a visualization script `vis.R`

## Clone

- Clone this repo to your local machine using `https://github.com/DumitruHanciu/ClustereR.git`

## Usage

Makefile: Contains the following targets:
- debug: Build for debugging
- release: Build for performance
- run-small: Run kmeans for the small data set
- run-large: Run kmeans for the large data set
- vis-small: Generate cluster.gif for the small data set
- vis-large: Generate cluster.gif for the large data set
- install-vis: Install requirements for the R visualization script
- clean: Clean-up of generated files

```shell
$ make run-small
Executing k-means clustering with
20 iterations, 1000 points, and 5
centroids...
Time Elapsed (kernel): 0.001543 s
Time Elapsed (total): 0.001608 s
```

## How to Contribute

1. Clone repo and create a new branch: `$ git checkout https://github.com/DumitruHanciu/ClustereR -b new_branch`.
2. Make changes and test
3. Submit Pull Request with comprehensive description of changes


## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org)

- **[MIT license](http://opensource.org/licenses/mit-license.php)**
