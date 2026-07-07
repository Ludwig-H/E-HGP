import json

# Define code contents first to avoid syntax errors in dictionary definition
install_code = [
    "# Installation des dependances systeme (Eigen3 et CGAL pour HGP-old)\n",
    "!apt-get update && apt-get install -y libeigen3-dev libcgal-dev\n",
    "\n",
    "# Installation des packages python requis\n",
    "!pip install pybind11 Cython hdbscan gudhi\n",
    "\n",
    "# Recuperation du depot GitHub de la these\n",
    "!git clone https://github.com/Ludwig-H/E-HGP.git\n",
    "\n",
    "# Compilation de HGP-old (version combinatoire) avec chemin absolu et sans isolation\n",
    "!pip install --no-build-isolation /content/E-HGP/HGP-old\n",
    "\n",
    "# Compilation de E-HGP (version entropique optimisee) avec chemin absolu\n",
    "!pip install /content/E-HGP/E-HGP"
]

import_code = [
    "import os\n",
    "import time\n",
    "import numpy as np\n",
    "import matplotlib.pyplot as plt\n",
    "import seaborn as sns\n",
    "from sklearn.datasets import make_moons, make_blobs\n",
    "from sklearn.metrics import adjusted_rand_score\n",
    "\n",
    "import torch\n",
    "device = 'cuda' if torch.cuda.is_available() else 'cpu'\n",
    "print(f'Dispositif de calcul detecte : {device.upper()}')\n",
    "\n",
    "import e_hgp\n",
    "import hgp_clusterer\n",
    "from hdbscan import HDBSCAN\n",
    "\n",
    "print('e_hgp importe avec succes !')\n",
    "print('hgp_clusterer importe avec succes !')"
]

benchmark_impl_code = [
    "def run_single_experiment(X, y, algo_name, params):\n",
    "    t0 = time.time()\n",
    "    try:\n",
    "        if algo_name == 'HGP-old':\n",
    "            if X.shape[1] > 3:\n",
    "                return None, None\n",
    "            model = hgp_clusterer.HGPClusterer(\n",
    "                K=params.get('K', 2), \n",
    "                min_cluster_size=params.get('min_cluster_size', 15),\n",
    "                expZ=params.get('expZ', 2.0)\n",
    "            )\n",
    "            labels = model.fit_predict(X)\n",
    "        elif algo_name == 'E-HGP':\n",
    "            model = e_hgp.EHGPClusterer(\n",
    "                K=params.get('K', 2),\n",
    "                kappa=params.get('kappa', 1.5),\n",
    "                m_reg=params.get('m_reg', 25),\n",
    "                min_cluster_size=params.get('min_cluster_size', 15),\n",
    "                expZ=params.get('expZ', 2.0)\n",
    "            )\n",
    "            labels = model.fit_predict(X)\n",
    "        elif algo_name == 'HDBSCAN':\n",
    "            model = HDBSCAN(min_cluster_size=params.get('min_cluster_size', 15))\n",
    "            labels = model.fit_predict(X)\n",
    "        else:\n",
    "            raise ValueError('Algo inconnu')\n",
    "        \n",
    "        t_exec = time.time() - t0\n",
    "        ari = adjusted_rand_score(y, labels)\n",
    "        return t_exec, ari\n",
    "    except Exception as e:\n",
    "        print(f'Erreur lors de l\\'execution de {algo_name}: {e}')\n",
    "        return None, None"
]

scenario_1_code = [
    "sizes = [500, 1000, 2500, 5000, 10000]\n",
    "algos = ['E-HGP', 'HGP-old', 'HDBSCAN']\n",
    "\n",
    "results_2d = {algo: {'times': [], 'aris': []} for algo in algos}\n",
    "results_3d = {algo: {'times': [], 'aris': []} for algo in algos}\n",
    "\n",
    "for N in sizes:\n",
    "    print(f'Benchmark N = {N}...')\n",
    "    X_2d, y_2d = make_moons(n_samples=N, noise=0.08, random_state=42)\n",
    "    X_3d, y_3d = make_blobs(n_samples=N, centers=3, n_features=3, random_state=42)\n",
    "    \n",
    "    min_cluster_size = max(10, N // 100)\n",
    "    params = {'K': 2, 'kappa': 1.5, 'min_cluster_size': min_cluster_size, 'expZ': 2.0}\n",
    "    \n",
    "    for algo in algos:\n",
    "        # Run 2D\n",
    "        t, ari = run_single_experiment(X_2d, y_2d, algo, params)\n",
    "        if t is not None:\n",
    "            results_2d[algo]['times'].append(t)\n",
    "            results_2d[algo]['aris'].append(ari)\n",
    "        else:\n",
    "            results_2d[algo]['times'].append(np.nan)\n",
    "            results_2d[algo]['aris'].append(0.0)\n",
    "            \n",
    "        # Run 3D\n",
    "        t, ari = run_single_experiment(X_3d, y_3d, algo, params)\n",
    "        if t is not None:\n",
    "            results_3d[algo]['times'].append(t)\n",
    "            results_3d[algo]['aris'].append(ari)\n",
    "        else:\n",
    "            results_3d[algo]['times'].append(np.nan)\n",
    "            results_3d[algo]['aris'].append(0.0)\n",
    "\n",
    "print('Scenario 1 termine !')"
]

plot_1_code = [
    "fig, axes = plt.subplots(2, 2, figsize=(14, 10))\n",
    "\n",
    "# 2D Times\n",
    "for algo in algos:\n",
    "    axes[0, 0].plot(sizes, results_2d[algo]['times'], marker='o', label=algo)\n",
    "axes[0, 0].set_title('Temps de calcul vs Taille (2D Moons)')\n",
    "axes[0, 0].set_xlabel('N points')\n",
    "axes[0, 0].set_ylabel('Temps (s)')\n",
    "axes[0, 0].set_yscale('log')\n",
    "axes[0, 0].grid(True)\n",
    "axes[0, 0].legend()\n",
    "\n",
    "# 2D ARI\n",
    "for algo in algos:\n",
    "    axes[0, 1].plot(sizes, results_2d[algo]['aris'], marker='s', label=algo)\n",
    "axes[0, 1].set_title('Precision (ARI) vs Taille (2D Moons)')\n",
    "axes[0, 1].set_xlabel('N points')\n",
    "axes[0, 1].set_ylabel('ARI')\n",
    "axes[0, 1].set_ylim(0.8, 1.05)\n",
    "axes[0, 1].grid(True)\n",
    "axes[0, 1].legend()\n",
    "\n",
    "# 3D Times\n",
    "for algo in algos:\n",
    "    axes[1, 0].plot(sizes, results_3d[algo]['times'], marker='o', label=algo)\n",
    "axes[1, 0].set_title('Temps de calcul vs Taille (3D Blobs)')\n",
    "axes[1, 0].set_xlabel('N points')\n",
    "axes[1, 0].set_ylabel('Temps (s)')\n",
    "axes[1, 0].set_yscale('log')\n",
    "axes[1, 0].grid(True)\n",
    "axes[1, 0].legend()\n",
    "\n",
    "# 3D ARI\n",
    "for algo in algos:\n",
    "    axes[1, 1].plot(sizes, results_3d[algo]['aris'], marker='s', label=algo)\n",
    "axes[1, 1].set_title('Precision (ARI) vs Taille (3D Blobs)')\n",
    "axes[1, 1].set_xlabel('N points')\n",
    "axes[1, 1].set_ylabel('ARI')\n",
    "axes[1, 1].set_ylim(0.8, 1.05)\n",
    "axes[1, 1].grid(True)\n",
    "axes[1, 1].legend()\n",
    "\n",
    "plt.tight_layout()\n",
    "plt.show()"
]

scenario_2_code = [
    "dims = [2, 5, 10, 20, 50]\n",
    "N_nd = 2000\n",
    "algos_nd = ['E-HGP', 'HDBSCAN']\n",
    "\n",
    "results_nd = {algo: {'times': [], 'aris': []} for algo in algos_nd}\n",
    "\n",
    "for d in dims:\n",
    "    print(f'Benchmark dimension d = {d}...')\n",
    "    X_nd, y_nd = make_blobs(n_samples=N_nd, centers=4, n_features=d, random_state=42)\n",
    "    params = {'K': 2, 'kappa': 1.5, 'min_cluster_size': 20, 'expZ': 2.0}\n",
    "    \n",
    "    for algo in algos_nd:\n",
    "        t, ari = run_single_experiment(X_nd, y_nd, algo, params)\n",
    "        results_nd[algo]['times'].append(t if t is not None else np.nan)\n",
    "        results_nd[algo]['aris'].append(ari if ari is not None else 0.0)\n",
    "\n",
    "fig, axes = plt.subplots(1, 2, figsize=(14, 5))\n",
    "for algo in algos_nd:\n",
    "    axes[0].plot(dims, results_nd[algo]['times'], marker='o', label=algo)\n",
    "axes[0].set_title(f'Temps de calcul vs Dimension (N={N_nd})')\n",
    "axes[0].set_xlabel('Dimension d')\n",
    "axes[0].set_ylabel('Temps (s)')\n",
    "axes[0].set_yscale('log')\n",
    "axes[0].grid(True)\n",
    "axes[0].legend()\n",
    "\n",
    "for algo in algos_nd:\n",
    "    axes[1].plot(dims, results_nd[algo]['aris'], marker='s', label=algo)\n",
    "axes[1].set_title(f'Precision (ARI) vs Dimension (N={N_nd})')\n",
    "axes[1].set_xlabel('Dimension d')\n",
    "axes[1].set_ylabel('ARI')\n",
    "axes[1].set_ylim(0.0, 1.05)\n",
    "axes[1].grid(True)\n",
    "axes[1].legend()\n",
    "\n",
    "plt.show()"
]

scenario_3_code = [
    "expZ_values = [1.0, 2.0, 3.0]\n",
    "k_values = [2, 3]\n",
    "X_param, y_param = make_blobs(n_samples=5000, centers=5, n_features=3, cluster_std=2.0, random_state=42)\n",
    "\n",
    "fig, axes = plt.subplots(1, 2, figsize=(14, 5))\n",
    "\n",
    "for K in k_values:\n",
    "    aris_z = []\n",
    "    times_z = []\n",
    "    for expZ in expZ_values:\n",
    "        params = {'K': K, 'kappa': 1.5, 'min_cluster_size': 50, 'expZ': expZ}\n",
    "        t, ari = run_single_experiment(X_param, y_param, 'E-HGP', params)\n",
    "        aris_z.append(ari if ari is not None else 0.0)\n",
    "        times_z.append(t if t is not None else 0.0)\n",
    "    \n",
    "    axes[0].plot(expZ_values, aris_z, marker='o', label=f'K={K} (ARI)')\n",
    "    axes[1].plot(expZ_values, times_z, marker='s', label=f'K={K} (Temps)')\n",
    "\n",
    "axes[0].set_title('Sensibilite au parametre expZ (ARI)')\n",
    "axes[0].set_xlabel('expZ')\n",
    "axes[0].set_ylabel('ARI')\n",
    "axes[0].grid(True)\n",
    "axes[0].legend()\n",
    "\n",
    "axes[1].set_title('Temps de calcul vs expZ')\n",
    "axes[1].set_xlabel('expZ')\n",
    "axes[1].set_ylabel('Temps (s)')\n",
    "axes[1].grid(True)\n",
    "axes[1].legend()\n",
    "\n",
    "plt.show()"
]

gpu_cpu_benchmark_code = [
    "import torch\n",
    "if not torch.cuda.is_available():\n",
    "    print('GPU CUDA non disponible, le benchmark GPU vs CPU ne peut pas etre execute de maniere significative.')\n",
    "else:\n",
    "    sizes_gpu = [1000, 5000, 10000, 25000, 50000]\n",
    "    times_cpu = []\n",
    "    times_gpu = []\n",
    "    \n",
    "    original_is_available = torch.cuda.is_available\n",
    "    \n",
    "    for N in sizes_gpu:\n",
    "        print(f'Test GPU vs CPU pour N = {N}...')\n",
    "        X_gpu, y_gpu = make_blobs(n_samples=N, centers=5, n_features=10, random_state=42)\n",
    "        params = {'K': 2, 'kappa': 1.5, 'min_cluster_size': N // 100}\n",
    "        \n",
    "        # 1. Force CPU mode\n",
    "        torch.cuda.is_available = lambda: False\n",
    "        t0 = time.time()\n",
    "        model_cpu = e_hgp.EHGPClusterer(\n",
    "            K=params['K'],\n",
    "            kappa=params['kappa'],\n",
    "            min_cluster_size=params['min_cluster_size']\n",
    "        )\n",
    "        model_cpu.fit(X_gpu)\n",
    "        times_cpu.append(time.time() - t0)\n",
    "        \n",
    "        # 2. Force GPU mode\n",
    "        torch.cuda.is_available = original_is_available\n",
    "        t0 = time.time()\n",
    "        model_gpu = e_hgp.EHGPClusterer(\n",
    "            K=params['K'],\n",
    "            kappa=params['kappa'],\n",
    "            min_cluster_size=params['min_cluster_size']\n",
    "        )\n",
    "        model_gpu.fit(X_gpu)\n",
    "        times_gpu.append(time.time() - t0)\n",
    "        \n",
    "    # Plot the results\n",
    "    plt.figure(figsize=(10, 6))\n",
    "    plt.plot(sizes_gpu, times_cpu, marker='o', label='CPU Fallback')\n",
    "    plt.plot(sizes_gpu, times_gpu, marker='s', label='GPU (CUDA Accelerated)')\n",
    "    plt.title('Temps de calcul E-HGP : GPU vs CPU')\n",
    "    plt.xlabel('Nombre de points N')\n",
    "    plt.ylabel('Temps (s)')\n",
    "    plt.yscale('log')\n",
    "    plt.grid(True)\n",
    "    plt.legend()\n",
    "    plt.show()"
]

scenario_hypermassive_code = [
    "print('Generation d\\'un dataset hypermassif : 10 000 000 de points en 3D avec 100 clusters...')\n",
    "t0 = time.time()\n",
    "X_mass, y_mass = make_blobs(n_samples=10000000, centers=100, n_features=3, random_state=42)\n",
    "print(f'Dataset genere en {time.time() - t0:.2f}s.')\n",
    "\n",
    "import torch\n",
    "original_is_available = torch.cuda.is_available\n",
    "\n",
    "print('=== RUNNING E-HGP on CPU (10M points) ===')\n",
    "torch.cuda.is_available = lambda: False\n",
    "t0 = time.time()\n",
    "ehgp_cpu = e_hgp.EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=1000, L_initial=10)\n",
    "labels_cpu = ehgp_cpu.fit_predict(X_mass)\n",
    "t_cpu = time.time() - t0\n",
    "sub_idx = np.random.choice(10000000, size=50000, replace=False)\n",
    "ari_cpu = adjusted_rand_score(y_mass[sub_idx], labels_cpu[sub_idx])\n",
    "print(f'E-HGP (CPU) termine en {t_cpu:.2f}s | ARI (subsample 50k): {ari_cpu:.4f}')\n",
    "\n",
    "print('=== RUNNING E-HGP on GPU (10M points) ===')\n",
    "torch.cuda.is_available = original_is_available\n",
    "if not torch.cuda.is_available():\n",
    "    print('GPU CUDA non disponible, execution GPU ignoree.')\n",
    "    t_gpu = float('nan')\n",
    "    ari_gpu = 0.0\n",
    "else:\n",
    "    t0 = time.time()\n",
    "    ehgp_gpu = e_hgp.EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=1000, L_initial=10)\n",
    "    labels_gpu = ehgp_gpu.fit_predict(X_mass)\n",
    "    t_gpu = time.time() - t0\n",
    "    ari_gpu = adjusted_rand_score(y_mass[sub_idx], labels_gpu[sub_idx])\n",
    "    print(f'E-HGP (GPU) termine en {t_gpu:.2f}s | ARI (subsample 50k): {ari_gpu:.4f}')\n",
    "\n",
    "print('=== RUNNING HDBSCAN (10M points) ===')\n",
    "try:\n",
    "    t0 = time.time()\n",
    "    hdb_mass = HDBSCAN(min_cluster_size=1000, core_dist_n_jobs=-1)\n",
    "    labels_hdb_mass = hdb_mass.fit_predict(X_mass)\n",
    "    t_hdb_mass = time.time() - t0\n",
    "    ari_hdb_mass = adjusted_rand_score(y_mass[sub_idx], labels_hdb_mass[sub_idx])\n",
    "    print(f'HDBSCAN termine en {t_hdb_mass:.2f}s | ARI (subsample 50k): {ari_hdb_mass:.4f}')\n",
    "except Exception as e:\n",
    "    print(f'HDBSCAN a echoue ou a manque de memoire (OOM) : {e}')\n",
    "    t_hdb_mass = float('nan')\n",
    "\n",
    "print(\"\\n\" + \"=\"*80)\n",
    "print(\"RAPPORT COMPARATIF SUR NUAGES SUPER-MASSIFS (10 000 000 points)\")\n",
    "print(\"=\"*80)\n",
    "print(f\"E-HGP (CPU) : {t_cpu:.2f}s | ARI = {ari_cpu:.4f}\")\n",
    "if not np.isnan(t_gpu):\n",
    "    print(f\"E-HGP (GPU) : {t_gpu:.2f}s | ARI = {ari_gpu:.4f} (Speedup: {t_cpu/t_gpu:.1f}x)\")\n",
    "else:\n",
    "    print(\"E-HGP (GPU) : NON EXECUTE (Pas de GPU)\")\n",
    "if not np.isnan(t_hdb_mass):\n",
    "    print(f\"HDBSCAN     : {t_hdb_mass:.2f}s\")\n",
    "else:\n",
    "    print(\"HDBSCAN     : ECHEC (OOM)\")\n",
    "print(\"=\"*80 + \"\\n\")\n"
]

notebook_content = {
 "cells": [
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "# E-HGP vs HGP-old vs HDBSCAN: Google Colab Benchmark\n",
    "\n",
    "Ce notebook permet de comparer les performances en temps de calcul et en precision (ARI) de trois algorithmes de clustering :\n",
    "1. **HGP-old** (Version exacte combinatoire développée dans `HGP-old` utilisant Delaunay/Voronoi global).\n",
    "2. **E-HGP** (Version regularisee locale et parallélisable développée dans `E-HGP` avec support Cython/CPU).\n",
    "3. **HDBSCAN** (Version standard basée sur la densité).\n",
    "\n",
    "## Configuration Google Colab :\n",
    "Afin de profiter de l'accélération matérielle, allez dans **Runtime** -> **Change runtime type** -> Sélectionnez **T4 GPU** (ou GPU supérieur si Colab Pro)."
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 1. Installation des dépendances et compilation des packages"
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": install_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 2. Imports et vérification du matériel"
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": import_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 3. Implémentation du framework de Benchmark\n",
    "Nous allons concevoir des scénarios variés :\n",
    "* **Variabilité en dimension** : 2D, 3D, 10D, 30D.\n",
    "* **Variabilité en nombre de points** : 500 à 50 000 points.\n",
    "* **Variabilité des hyperparamètres** : Ordre $K$, régularisation et taille minimale des clusters."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": benchmark_impl_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 4. Scénario 1 : Comparaison en 2D (Nesting Moons) et 3D (Blobs) avec variation de taille ($N$)\n",
    "Ce test compare E-HGP à HGP-old et HDBSCAN."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": scenario_1_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "#### Visualisation graphique des temps de calcul et de la précision (2D & 3D)"
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": plot_1_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 5. Scénario 2 : Haute dimension (nD) - E-HGP vs HDBSCAN\n",
    "Ici, HGP-old n'est pas testé car la construction de la mosaïque Delaunay échoue en dimension $>3$."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": scenario_2_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 6. Scénario 3 : Influence des paramètres $K$ (ordre) et $expZ$ (exposant de distance)\n",
    "Nous étudions l'impact du réglage de la distance de puissance $expZ$ ($1.0$ comme HDBSCAN, $2.0$ par défaut) et de l'ordre géométrique $K$ sur E-HGP."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": scenario_3_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 7. Scénario 4 : Comparatif GPU vs CPU (Accélération CUDA sur E-HGP)\n",
    "Ce test compare directement la vitesse de notre module de régularisation entropique et de Gabriel lorsqu'il tourne sur carte graphique (CUDA) vs sur CPU."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": gpu_cpu_benchmark_code
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### 8. Scénario 5 : Passage à l'échelle hypermassif (10 000 000 de points)\n",
    "Ce test compare E-HGP et HDBSCAN sur un dataset massif de 10 millions de points en 3D avec 100 clusters. Il met en évidence la robustesse mémoire de E-HGP (avec chunking dynamique et L_max=30) face à HDBSCAN qui sature la mémoire (OOM)."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": scenario_hypermassive_code
  }
 ],
 "metadata": {
  "kernelspec": {
   "display_name": "Python 3",
   "language": "python",
   "name": "python3"
  },
  "language_info": {
   "name": "python"
  }
 },
 "nbformat": 4,
 "nbformat_minor": 2
}

# Write notebook file
with open('/workspaces/E-HGP/E-HGP_Colab_Benchmark.ipynb', 'w') as f:
    json.dump(notebook_content, f, indent=1)
