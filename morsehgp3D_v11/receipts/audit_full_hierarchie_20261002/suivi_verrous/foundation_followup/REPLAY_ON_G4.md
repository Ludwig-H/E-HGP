# Témoin à rejouer sur G4 par le développeur

Non exécuté par cette relecture. Utiliser la racine v11 exactement épinglée pour la qualification et copier le petit `fake_abnormal_stop.py` sur la VM. La commande ci-dessous reproduit l'imbrication de `mhgp11_expect_abnormal_stop`, sans configurer ni construire de projet.

```bash
# Remplacer les deux chemins par ceux de la capture G4.
gate_root=/chemin/vers/morsehgp3D_v11
signal_probe=/chemin/vers/fake_abnormal_stop.py
cmake_bin=$(command -v cmake)
python_bin=$(command -v python3)
"$cmake_bin" "-DCMD=$cmake_bin" -DNARGS=6 \
  "-DARG0=-DCMD=$python_bin" -DARG1=-DNARGS=1 \
  "-DARG2=-DARG0=$signal_probe" -DARG3=-DEXPECTED=0 \
  -DARG4=-P "-DARG5=$gate_root/cmake/run_expect.cmake" \
  -DEXPECTED=1 "-DEXPECT_LINE=run_expect_verdict arret_anormal" \
  -P "$gate_root/cmake/run_expect.cmake"
```

Le code déduit de la capture auditée est **0**, alors que la sonde est sortie normalement avec le code **3**. L'issue exigée après réparation est un **refus de la porte**, donc non zéro. Conserver stdout, stderr, code et hashes des wrappers/probe exécutés. Compléter par le contrôle positif à vrai signal déjà présent et les contrôles ordinaires 0/3 sans ligne imitée. Ce protocole n'est pas un reçu d'exécution.
