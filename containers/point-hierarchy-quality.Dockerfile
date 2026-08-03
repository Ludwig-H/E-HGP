ARG MORSEHGP3D_PHASE3_IMAGE_REF
FROM ${MORSEHGP3D_PHASE3_IMAGE_REF}

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20260716T000000Z

LABEL org.opencontainers.image.title="MorseHGP3D point-hierarchy quality environment"
LABEL org.opencontainers.image.description="Pinned scikit-learn comparison layer for the guarded exact quality campaign"
LABEL org.opencontainers.image.source="https://github.com/Ludwig-H/E-HGP"

RUN apt-get update --snapshot "${UBUNTU_SNAPSHOT}" \
    && apt-get install --yes --no-install-recommends --snapshot "${UBUNTU_SNAPSHOT}" \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

COPY containers/point-hierarchy-quality.requirements.txt /tmp/point-hierarchy-quality.requirements.txt
RUN python3 -m venv /opt/morsehgp3d-point-quality \
    && /opt/morsehgp3d-point-quality/bin/python -m pip install \
        --disable-pip-version-check \
        --no-cache-dir \
        --only-binary=:all: \
        --require-hashes \
        --requirement /tmp/point-hierarchy-quality.requirements.txt \
    && rm -f /tmp/point-hierarchy-quality.requirements.txt

ENV PATH="/opt/morsehgp3d-point-quality/bin:${PATH}"
ENV PYTHONDONTWRITEBYTECODE=1

RUN python -c "import joblib,numpy,scipy,sklearn,threadpoolctl; from sklearn.cluster import HDBSCAN; assert joblib.__version__ == '1.5.3'; assert numpy.__version__ == '2.5.1'; assert scipy.__version__ == '1.18.0'; assert sklearn.__version__ == '1.9.0'; assert threadpoolctl.__version__ == '3.6.0'; assert HDBSCAN is not None"

WORKDIR /workspace/morsehgp3d
