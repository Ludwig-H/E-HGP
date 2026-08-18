# Reçu — campagne locale n=8000 v2 (cœur de seed + parallélisme 4 fils)

Date : 18 août 2026. Moteur : `1d6fd06` (cœur de seed aplati,
génération et aval parallèles, plafond transactionnel). Conteneur
4 vCPU / 15 Go. Un run à la fois, `--threads=4`, RSS échantillonné sur
le PID DIRECT du probe (le bug du wrapper de la campagne v1 — RSS
invalides — est corrigé : les pics ci-dessous sont les premiers pics
mémoire FIABLES de ces runs). Référence : campagne v1 du 17 août
(`campagne_locale_n8000_20260817/`, 1 fil, moteur pré-cœur).

## Tableau comparatif (s=8, seed=3, n=8000)

| famille, smax | v1 | **v2** | facteur | événements | RSS pic v2 |
|---|---|---|---|---|---|
| uniform, 11 | 343 s | **145 s** | ×2,4 | 3 126 158 = | 7,73 Go |
| terrain, 11 | 162 s | **24 s** | ×6,8 | 605 870 = | 1,57 Go |
| scanline_overlap, 11 | 697 s | **50 s** | ×14 | 1 094 377 = | 2,68 Go |
| eight_clusters, 6 | 1 427 s | **50 s** | ×28,5 | 533 284 = | 1,00 Go |
| eight_clusters, 11 | **jamais terminé** (> 5 400 s) | **205 s** | > ×26 | **2 658 325 (première mesure)** | 6,53 Go |

Toutes les cellules comparables sont IDENTIQUES au compte près
(événements, boules uniques) : les optimisations n'ont pas changé
l'objet, elles ont changé son coût de découverte.

## La cellule historique : eight_clusters smax=11

Le cas dur de la v1 (« ne finit pas en 90 minutes », run À RISQUE du
plan G4) TERMINE en **3 min 25 s** : 2 658 325 événements, 2 723 973
boules uniques, 16 518 960 fusions, 1 723 449 nœuds. Décomposition :
`t_gen = 142,6 s` (dont cœur de seed 69,4 s CPU cumulé —
**80 758 279 seeds tués**, 3,695 milliards de sites examinés),
`t_fold = 44,2 s`, `t_census = 8,5 s`, `t_prefiltre = 5,6 s`. La
génération sur covers denses reste le poste dominant de cette famille ;
le fold arrive second (voir la question d'audit « internes du fold »).

## Conséquences pour la campagne G4

1. Le run À RISQUE n'existe plus : la grille complète des 28 runs du
   protocole est praticable confortablement — sur 48 vCPU, chaque
   cellule descend encore (le protocole épinglé actuel tourne à 1 fil ;
   l'amender pour `--threads` est un choix d'audit à acter avant
   lancement, ou une campagne v2 re-épinglée).
2. Les pics RSS fiables calibrent la concurrence pilotée par la
   mémoire : 7,7 Go (uniform smax=11) → sur 180 Go de G4,
   la concurrence prudente de 8 est très en dessous du plafond réel.
3. n=16000/32000 restent hors de portée locale (RAM) — G4.

`public_status=not_claimed` — mesure du coût de découverte ; la valeur
contractuelle attend la décision `product` (voir
`REPONSE_CLAUDE_POISSON_Q2_ET_CONTRAT_PRODUIT`).
