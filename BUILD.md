# Build ONNX Runtime

# Supported dev environments

| OS          | Supports CPU | Supports GPU| Notes                              |
|-------------|:------------:|:------------:|------------------------------------|
|Windows 10   | YES          | YES         |Must use VS 2017 or the latest VS2015|
|Windows 10 <br/> Subsystem for Linux | YES         | NO        |         |
|Ubuntu 16.x  | YES          | YES         |                            |
|Ubuntu 17.x  | YES          | YES         |                            |
|Ubuntu 18.x  | YES          | UNKNOWN     | No CUDA package from Nvidia|
|Fedora 24    | YES          | YES         |                            |
|Fedora 25    | YES          | YES         |                            |
|Fedora 26    | YES          | YES         |                            |
|Fedora 27    | YES          | YES         |                            |
|Fedora 28    | YES          | NO          |Cannot build GPU kernels but can run them |

* Red Hat Enterprise Linux and CentOS are not supported.
* Clang 7.x is not supported. You may use Clang 6.x.
* GCC 4.x and below are not supported. If you are using GCC 7.0+, you'll need to upgrade eigen to a newer version before compiling ONNX Runtime.

OS/Compiler Matrix:

| OS/Compiler | Supports VC  | Supports GCC     |  Supports Clang |
|-------------|:------------:|:----------------:|:---------------:|
|Windows 10   | YES          | Not tested       | Not tested      |
|Linux        | NO           | YES(gcc>=5.0)    | YES             |

ONNX Runtime python binding only supports Python 3.x. Please use python 3.5+.

# Build
Install cmake-3.10 or better from https://cmake.org/download/.

Checkout the source tree:
```
git clone --recursive https://github.com/onnx/onnxruntime
cd onnxruntime
./build.sh for Linux (or ./build.bat for Windows)
```
The build script runs all unit tests by default.

The complete list of build options can be found by running `./build.sh (or ./build.bat) --help`

# Integration Tests
To run onnx model tests on Linux,

1. Install docker
2. (optional) Run "export AZURE\_BLOB\_KEY=<secret_value>". You can get the key by executing "az storage keys list --account-name lotus" if you have Azure CLI 2.0 installed or just ask chasun@microsoft.com for that.
3.  Run tools/ci\_build/vsts/linux/run\_build.sh.

For Windows, please follow the README file at onnxruntime/test/onnx/README.txt

# Additional Build Flavors
The complete list of build flavors can be seen by running `./build.sh --help` or `./build.bat --help`. Here are some common flavors.

## Windows CUDA Build
ONNX Runtime supports CUDA builds. You will need to download and install [CUDA](https://developer.nvidia.com/cuda-toolkit) and [CUDNN](https://developer.nvidia.com/cudnn).

ONNX Runtime is built and tested with CUDA 9.0 and CUDNN 7.0 using the Visual Studio 2017 14.11 toolset (i.e. Visual Studio 2017 v15.3).
CUDA versions up to 9.2 and CUDNN version 7.1 should also work with versions of Visual Studio 2017 up to and including v15.7, however you may need to explicitly install and use the 14.11 toolset due to CUDA and CUDNN only being compatible with earlier versions of Visual Studio 2017.

To install the Visual Studio 2017 14.11 toolset, see <https://blogs.msdn.microsoft.com/vcblog/2017/11/15/side-by-side-minor-version-msvc-toolsets-in-visual-studio-2017/>

If using this toolset with a later version of Visual Studio 2017 you have two options:

1. Setup the Visual Studio environment variables to point to the 14.11 toolset by running vcvarsall.bat prior to running cmake
   - e.g.  if you have VS2017 Enterprise, an x64 build would use the following command
`"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" amd64 -vcvars_ver=14.11`

2. Alternatively if you have CMake 3.12 or later you can specify the toolset version in the "-T" parameter by adding "version=14.11"
   - e.g. use the following with the below cmake command
`-T version=14.11,host=x64`

CMake should automatically find the CUDA installation. If it does not, or finds a different version to the one you wish to use, specify your root CUDA installation directory via the -DCUDA_TOOLKIT_ROOT_DIR CMake parameter.

_Side note: If you have multiple versions of CUDA installed on a Windows machine and are building with Visual Studio, CMake will use the build files for the highest version of CUDA it finds in the BuildCustomization folder.  e.g. C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\VC\VCTargets\BuildCustomizations\. If you want to build with an earlier version, you must temporarily remove the 'CUDA x.y.*' files for later versions from this directory._

The path to the 'cuda' folder in the CUDNN installation must be provided. The 'cuda' folder should contain 'bin', 'include' and 'lib' directories.

You can build with:

```
./build.sh --use_cuda --cudnn_home /usr --cuda_home /usr/local/cuda (Linux)
./build.bat --use_cuda --cudnn_home <cudnn home path> --cuda_home <cuda home path> (Windows)
```

## MKL-DNN
To build ONNX Runtime with MKL-DNN support, build it with `./build.sh --use_mkldnn --use_mklml`

## OpenBLAS
### Windows
Instructions how to build OpenBLAS for windows can be found here https://github.com/xianyi/OpenBLAS/wiki/How-to-use-OpenBLAS-in-Microsoft-Visual-Studio#build-openblas-for-universal-windows-platform.

Once you have the OpenBLAS binaries, build ONNX Runtime with `./build.bat --use_openblas`

### Linux
For Linux (e.g. Ubuntu 16.04), install libopenblas-dev package
`sudo apt-get install libopenblas-dev` and build with `./build.sh --use_openblas`

## OpenMP
```
./build.sh --use_openmp (for Linux)
./build.bat --use_openmp (for Windows)
```

## Build with Docker on Linux
Install Docker: `https://docs.docker.com/install/`

### CPU
```
cd tools/ci_build/vsts/linux/docker
docker build -t onnxruntime_dev --build-arg OS_VERSION=16.04 -f Dockerfile.ubuntu .
docker run --rm -it onnxruntime_dev /bin/bash
```

### GPU
If you need GPU support, please also install:
1. nvidia driver. Before doing this please add 'nomodeset rd.driver.blacklist=nouveau' to your linux [kernel boot parameters](https://www.kernel.org/doc/html/v4.17/admin-guide/kernel-parameters.html).
2. nvidia-docker2: [Install doc](`https://github.com/NVIDIA/nvidia-docker/wiki/Installation-(version-2.0)`)

To test if your nvidia-docker works:
```
docker run --runtime=nvidia --rm nvidia/cuda nvidia-smi
```

Then build a docker image. We provided a sample for use:
```
cd tools/ci_build/vsts/linux/docker
docker build -t cuda_dev -f Dockerfile.ubuntu_gpu .
```

Then run it
```
cd ~/src
git clone https://github.com/onnx/onnxruntime
docker run --runtime=nvidia -v ~/src/ONNX Runtime:/data/lotus --rm -it cuda_dev /bin/bash
mkdir build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug /data/lotus/cmake -Donnxruntime_ENABLE_PYTHON=ON -DPYTHON_EXECUTABLE=/usr/bin/python3 -Donnxruntime_USE_CUDA=ON -Donnxruntime_CUDNN_HOME=/usr/local/cudnn-7.0/cuda
ninja
```

## Build with Docker (CPU) on Windows
Register a docker account at [https://www.docker.com/](https://www.docker.com/)

Download Docker for Windows: `https://store.docker.com/editions/community/docker-ce-desktop-windows`

Install Docker for Windows. Share local drive to Docker, open Docker Settings->Shared Drives, share the disk drive to docker. This is used to mount the local code to docker instance.

Run powershell command to build docker

```
cd .\ONNX Runtime\tools\ci_build\vsts\linux\ubuntu16.04
docker login
docker build -t lotus-ubuntu16.04 .
docker run -it --rm --name lotus-cpu -v [LocalPath]/Git/ONNX Runtime:/home/lotusdev/ONNX Runtime lotus-ubuntu16.04 /bin/bash
source /usr/local/miniconda3/bin/activate lotus-py35
python /home/lotusdev/ONNX Runtime/tools/ci_build/build.py --build_dir /home/lotusdev/ONNX Runtime/build/Linux --config Debug --skip_submodule_sync --enable_pybind
```
Run command below if the conda environment `lotus-py35` does not exist
```
/usr/local/miniconda3/bin/conda env create --file /home/lotusdev/ONNX Runtime/tools/ci_build/vsts/linux/Conda/conda-linux-lotus-py35-environment.yml --name lotus-py35 --quiet --force
```