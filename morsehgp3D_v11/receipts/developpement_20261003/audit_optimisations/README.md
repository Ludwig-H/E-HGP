# Audit des ports performance v11 — 3 octobre 2026

Source fixe479f53f0b, ports ef75dafac/479 contre70e494777 (produit identique
à la baseline895680ff8).99 fichiers produit hachés,31 chemins src modifiés.
[Audit courant](../../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md) et
[invariants](../../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Aucune source ou donnée LiDAR copiée, aucun build/test natif ou cloud nouveau.

- `performance.json` recoupe les35+13 runs locaux, médianes et limites des reçus existants.
- `workspace_admission_model.py` :570 cas, majorant corrigé exact ; W48/4 workspaces/2 blocs aux workers30/31.
- `context_refusal_model.py` :8 branches du contrôle,3 refus étrangers contournés sur hit ; modèle de source, pas C++ exécuté.
- `float_key_model.py` :132012 contrôles Fraction, profils18/21/24 et64 combinaisons d’arrondis des clés/comparaisons ; pas porte FENV native.
- `float_model_initial_failure.*` garde le premier modèle avec une fixture hors budget et son échec. Aucun défaut natif déduit de cette erreur d’audit.
- `proof.json` fixe sources Git, artefacts et26 fichiers des reçus existants ; sans copier leurs captures.

Le cap Buffer reste actif ; le modèle d’admission ne reproduit pas un échec
FULL natif ni une corruption. Les sorties complètes et binaires mesurés
sont absents ; leurs hashes déclarés ne sont pas des rehachages aujourd’hui.
Les0,32s G4 sont une simulation/extrapolation. La dernière qualification G4
reste reuse1 ; les suites locales fast ne transfèrent pas ASan/TSan/profils.

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/audit_optimisations/check.py
python3 -B -O morsehgp3D_v11/receipts/developpement_20261003/audit_optimisations/check.py
```

Le lecteur vérifie les sources Git fixes et hashes, rejoue les deux lecteurs
Python de reçus normal/−O, recalcule les médianes, puis rejoue les trois
modèles indépendants normal/−O. Il ne compile, ne sonde et ne contacte pas GCP.
