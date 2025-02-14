# Use an official Python 3.12-slim Linux image
FROM python:3.12-slim

# Set Python environment variables explicitly
ENV PYTHON_ROOT_DIR=/usr/local
ENV PYTHON_EXECUTABLE=/usr/local/bin/python3
ENV PYTHON_INCLUDE_DIR=/usr/local/include/python3.12
ENV PYTHON_LIBRARY=/usr/local/lib/libpython3.12.so

# Set ASan options (detect leaks, etc.)
ENV ASAN_OPTIONS=detect_leaks=1

# Clear any interfering Python environment variables
ENV VIRTUAL_ENV=
ENV CONDA_PREFIX=
ENV CONDA_DEFAULT_ENV=
ENV PYTHONHOME=
ENV PYTHONPATH=

# Install system build dependencies including clang for ASan support.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    cmake \
    ninja-build \
    git \
    libspdlog-dev \
    libfmt-dev \
    libgtest-dev \
    python3-dev \
    python3-pip && \
    rm -rf /var/lib/apt/lists/*

# Upgrade pip and install pybind11 (both via pip and system package)
RUN python3 -m pip install --upgrade pip pybind11 && \
    apt-get update && apt-get install -y python3-pybind11 && \
    rm -rf /var/lib/apt/lists/*

# Set working directory and copy the repository into the image
WORKDIR /managym
COPY . /managym

# Configure and build the project with AddressSanitizer flags
RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DPYBIND11_PYTHON_VERSION=3.12 \
    -G Ninja .. && \
    ninja

# Copy the updated entrypoint script and make it executable
COPY entry.sh /managym/entry.sh
RUN chmod +x /managym/entry.sh

# Set entrypoint to run our tests
ENTRYPOINT ["/managym/entry.sh"]
