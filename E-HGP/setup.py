from setuptools import setup, find_packages, Extension
from Cython.Build import cythonize
import numpy as np

# OpenMP support flags for GCC on Linux
extra_compile_args = ["-fopenmp", "-O3"]
extra_link_args = ["-fopenmp"]

extensions = [
    Extension(
        "e_hgp._core",
        ["e_hgp/_core.pyx"],
        include_dirs=[np.get_include()],
        language="c++",
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args
    )
]

setup(
    name="e_hgp",
    version="0.1.0",
    packages=find_packages(),
    ext_modules=cythonize(extensions, compiler_directives={'language_level': '3'}),
    install_requires=[
        "numpy>=1.20",
        "scipy>=1.7",
        "scikit-learn>=1.0",
    ],
    description="Entropy-Regularized GPU-friendly Hypergraph Percolation",
    author="Louis Hauseux",
    license="MIT",
)
