# Annexe de la proposition C sur la phase A

26 septembre 2026, auditeur C. Pièces de
[`PROPOSITION_C_PHASE_A_20260926.md`](../PROPOSITION_C_PHASE_A_20260926.md).
Base `f44a8db03`. GCP non utilisé. Aucun fichier du produit n'a été
modifié.

## Contenu

- `phaseA_profile.diff` : instrumentation de `order_lots`.
  - Elle ajoute, par ordre, des minuteries par étape, un échantillonnage
    rdtsc des lots (un sur 16) et des compteurs.
  - Elle s'applique sur `main` (`git apply --check -p1`) et compile sous
    `-Werror`.
  - Un essai local sur les 1 500 sites du préflight R22 a imprimé les
    lignes `PROFA` avec des condensés inchangés.
  - Aucune trame entière n'a encore été mesurée : l'hôte local manquait
    de CPU.
  - Mesures : temps CPU de fil pour l'ordre, la préparation et la
    boucle ; rdtsc pour les autres étapes.
  - Les compteurs sont toujours actifs.
  - Réglages : `MHGP9_PROFA_STRIDE` et `MHGP9_PROFA_DISCARD` ; sortie
    sur stderr, lignes `PROFA`.
- `analyze.py` : lecture des lignes `PROFA`.
- `lanes_understand.json` : sorties de la piste de spécification
  (spécification, dépendances, modèle de coût, leviers L1–L13 et P0–P5)
  et de la piste de profil (échec documenté, comptes déterministes).
- `lanes_verify.json` : verdicts des trois sceptiques (sûreté des
  leviers, preuve de P1, coût et priorités).
- `toys/` : modèle réduit Python des vérifications combinatoires de L3,
  L5, L10 et L12.
- `toys/p1_sim.py`, `toys/p1_toy.cpp` et leurs sorties : simulation et
  transcription C++ du protocole P1.
  - Mode sûr : compte validé en acquisition et publication, case par
    ordinal ; TSan ne signale rien, et les 40 exécutions sur 40 sont
    identiques.
  - Mode relâché : TSan signale les écritures du validateur.
  - Couverture temporelle mince : sous TSan, 1 362 indices acceptés sur
    100 608 facettes. La couverture des branches vient du modèle Python
    adverse, et aucun vérificateur de modèle (GenMC, CDSChecker) n'a été
    lancé.
  - Mutant sans test `prior_count` : il survit aux exécutions
    naturelles.

## Correction d'attribution

`lanes_understand.json` et `lanes_verify.json` sont les sorties brutes
des agents. Par endroits, elles attribuent à C le relevé de comptes
`phase_a_20260923/` et l'ablation des têtes physiques. Ils sont de B
(attribution probable, selon l'index et le canal du 23 septembre à
07 h 21 UTC). Ils ont été commités à 06 h 59 UTC, avant l'arrivée de C.
La note principale fait foi.
