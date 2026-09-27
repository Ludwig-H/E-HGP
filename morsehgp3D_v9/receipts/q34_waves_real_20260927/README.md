# Reçus : consommateur q34 par vagues sur entrées réelles

Base `70168cc3b`, CPU mono, grille 1 mm, hors registre, `not_claimed`.
Aucun moteur ou GCP modifié. Voir [l'audit](../../audits/b_q34_waves_real_20260927/README.md)
et [les périmètres chronométrés](../../audits/b_q34_waves_real_20260927/DESIGN.md).

- `qualification/` : 21 commandes PASS, Release/Clang ASan/UBSan/LSan,
  gate de 175 lots exécutée à neuf, six mutations natives et six petits appels du
  nouveau chemin de mesure. Lectures normales/`-O` et huit mutations du
  lecteur closes dans `qualification/checks/`.
- `initial/` : cinq commandes PASS, ng00 entier puis uniforme 8k/16k/32k.
  Lectures normale/`-O` et huit corruptions du lecteur PASS dans ses propres
  `checks/`. Résultats exacts et coûts observés, pas gain statistique stable.
- `cuts/` : sept commandes PASS, six moitiés/quarts capteur ng00, avec
  effectifs et partitions vérifiés. Lectures normale/`-O` et huit corruptions
  du lecteur PASS dans ses propres `checks/`. Son statut vient de ses propres
  reçus, pas de l'initiale ni de la qualification.

Recettes de provenance, chemins neufs requis pour toute nouvelle capture :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/run.py --capture morsehgp3D_v9/receipts/q34_waves_real_20260927/qualification --build-prefix /workspaces/E-HGP/build/v9-audit-q34-waves-real-20260927
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/measure.py --capture morsehgp3D_v9/receipts/q34_waves_real_20260927/initial --qualification morsehgp3D_v9/receipts/q34_waves_real_20260927/qualification --group initial
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/measure.py --capture morsehgp3D_v9/receipts/q34_waves_real_20260927/cuts --qualification morsehgp3D_v9/receipts/q34_waves_real_20260927/qualification --group cuts
```

Les deux builds `v9-audit-q34-waves-real-20260927_{release,sanitize}` sont
épinglés. La campagne réutilise exactement ce binaire Release, sans
recompilation. Sources, archives immuables, configuration, binary, données
et ID maps des sept morceaux sont hachés et refermés ; aucun octet LiDAR
n'est copié dans ces reçus. Les lecteurs sont LIVE, pas autonomes.

Relecture après clôture :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/run.py --readback morsehgp3D_v9/receipts/q34_waves_real_20260927/qualification
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/measure.py --readback morsehgp3D_v9/receipts/q34_waves_real_20260927/initial
python3 -B morsehgp3D_v9/audits/b_q34_waves_real_20260927/measure.py --readback morsehgp3D_v9/receipts/q34_waves_real_20260927/cuts
```

Les mêmes commandes avec `-O` contrôlent les verdicts sans assertions Python.
`check_capture.py --kind qualification|measure` enregistre ces lectures et
les corruptions dans un `checks/` neuf. Une divergence ou erreur arrête
la campagne au premier cas ; intentions, stdout/stderr, codes et fermeture
des groupes restent conservés, sans passage silencieux au cas suivant.

Le build mutable de préflight, distinct, a compilé et testé uniforme64 et
amas64 avant gel. Aucun de ses temps n'est une autorité de résultat.
