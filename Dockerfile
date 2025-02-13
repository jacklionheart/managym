# Use the official Python 3.12-slim base image
FROM python:3.12-slim

# Set environment variables first, being very explicit about paths
ENV PYTHON_ROOT_DIR=/usr/local
ENV PYTHON_EXECUTABLE=/usr/local/bin/python3
ENV PYTHON_INCLUDE_DIR=/usr/local/include/python3.12
ENV PYTHON_LIBRARY=/usr/local/lib/libpython3.12.so

# Clear any potential interfering Python-related env vars
ENV VIRTUAL_ENV=
ENV CONDA_PREFIX=
ENV CONDA_DEFAULT_ENV=
ENV PYTHONHOME=
ENV PYTHONPATH=

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    libspdlog-dev \
    libfmt-dev \
    libgtest-dev \
    python3-dev \
    python3-pip && \
    rm -rf /var/lib/apt/lists/*

# Install pybind11 both via pip and as a system package
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install pybind11 && \
    apt-get update && \
    apt-get install -y python3-pybind11 && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /managym

# Copy the repository into the container
COPY . /managym

# Build the project
RUN rm -rf build && mkdir build && cd build && \
    cmake \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DPYBIND11_PYTHON_VERSION=3.12 \
    -G Ninja .. && \
    ninja

RUN ctest --output-on-failure