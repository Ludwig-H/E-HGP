from setuptools import setup, find_packages

setup(
    name="perg_hgp",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "numpy>=1.20",
        "scipy>=1.7",
        "torch>=2.0",
        "pyyaml",
        "tqdm",
        "scikit-learn>=1.0"
    ],
    description="Progressive Entropic Rank-Gabriel HGP for massive 3D point clouds",
    author="Louis Hauseux",
    license="MIT",
)
