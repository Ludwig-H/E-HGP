FROM nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20260716T000000Z

LABEL org.opencontainers.image.title="MorseHGP3D CUDA 12.9 sm_120 build environment"
LABEL org.opencontainers.image.description="Reproducible MorseHGP3D Phase 3 build and GPU qualification environment"
LABEL org.opencontainers.image.source="https://github.com/Ludwig-H/E-HGP"

RUN apt-get update --snapshot "${UBUNTU_SNAPSHOT}" \
    && apt-get install --yes --no-install-recommends --snapshot "${UBUNTU_SNAPSHOT}" \
        ca-certificates \
        cmake \
        g++ \
        git \
        libboost-dev \
        ninja-build \
        python3 \
        python3-dev \
        python3-hypothesis \
        python3-pytest \
    && rm -rf /var/lib/apt/lists/*

RUN nvcc --version | grep -F "release 12.9" \
    && cmake --version \
    && ninja --version \
    && python3 --version

ENV CUDAARCHS="120-real"
ENV CUDA_MODULE_LOADING="EAGER"
ENV MORSEHGP3D_CUDA_IMAGE_REF="nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"

WORKDIR /workspace/morsehgp3d

CMD ["cmake", "--workflow", "--preset", "cuda-release"]
