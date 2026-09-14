# Premier front v8 : lentille, proposeur et propagation des témoins

14 septembre 2026, après **da366f7f**. Auditeur indépendant B.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Le premier front réel (`src/wspd/front.cpp`, mode `MidpointSamples`)
rejette des voies sur des produits non séparés, mais son résultat négatif
est net : à 32k uniforme, s=8, Kmax 10, 63,5 millions de recherches de
témoins pour 954 millions de pas d'index, et un temps de 4,9 s (Pure) à
37,4 s (Samples). Cette note mesure, sur une copie instrumentée de ce
front, où passe cette dépense et ce que rapporterait la transmission des
témoins certifiés du parent à ses enfants. Le constructeur a relu deux de
mes propositions ; la première (test de lentille) est ici réfutée par la
mesure, la seconde (propagation) est chiffrée.

## 1. Le test de lentille n'est pas le levier

Un témoin universel d'un produit A×B existe seulement si la lentille,
intersection des boules diamétrales ouvertes de toutes les paires, est non
vide ; cela se teste exactement en maximisant `h_minimum(A, B, {z})` sur z
(fonction concave séparable par axe ; maxima aux sommets des paraboles et
à leurs intersections, calcul en rationnels). La copie évalue ce test avant
chaque recherche et compare avec l'issue de la recherche réelle :

| Famille (32k, s=8, Kmax 10) | Recherches | Lentille vide | Recherches sans aucun rejet | Rejets partiels | Rejets complets |
| --- | ---: | ---: | ---: | ---: | ---: |
| Uniforme | 63 522 490 | 0,4 % | 71 % | 12 % | 17 % |
| Huit amas | 34 889 004 | 0,6 % | 76 % | 11 % | 12 % |
| Terrain mince | 7 227 826 | 1,0 % | 77 % | 9 % | 12 % |
| Deux rangées | 573 944 | 0 % | 92 % | 4 % | 4 % |

Aucune recherche sur lentille vide n'a rejeté quoi que ce soit
(condition nécessaire confirmée), mais elles ne représentent que 0 à 1 %
des recherches. Le constructeur avait raison d'ajouter qu'une lentille
non vide peut ne contenir aucun site : ici, 71 à 92 % des recherches
portent sur des produits à lentille non vide et ne rejettent rien. La
dépense vient donc du proposeur (une descente unique, au plus Kmax rangs
autour d'une feuille) et non de produits sans espoir. Ma phrase sur les
« paires frères à lentille vide » était trop générale ; elle est corrigée
dans la note des verrous.

## 2. Propager les témoins certifiés du parent aux enfants

Un témoin z certifié universel sur les boîtes du parent (`h_minimum > 0`,
et pour q3/q4 `3·h_min² > Ξ_max`, `2·h_min² > Ξ_max`) le reste sur toute
paire de boîtes enfants incluses : les bornes ne font que se resserrer, et
z reste hors des facteurs enfants. La copie transmet donc à chaque enfant,
par voie, les rangs des témoins déjà certifiés (au plus Kmax par voie,
donc au plus dix rangs de 32 bits par voie dans la tâche), démarre le
compte de la voie à ce nombre, et n'accorde un nouveau crédit à un rang
proposé que s'il n'est pas déjà dans la liste de cette voie. Un rang
crédité pour q2 seulement reste testé pour q3/q4 chez l'enfant, où la
borne Ξ plus serrée peut désormais le qualifier. Rien d'autre ne change :
même proposeur, mêmes seuils, mêmes prédicats.

Sûreté et inclusion. Chaque rejet reste certifié par des témoins distincts
universels sur les boîtes du produit ; le ledger de masse par voie du
front passe. Une force brute exacte sur tous les sites (chaque paire
rejetée doit avoir au moins `h_q` témoins W_q distincts hors d'elle-même)
donne **0 rejet non sûr** sur les quatre familles à n = 600, Kmax 10, et
sur uniforme/huit amas à n = 900 pour Kmax 1, 2, 3, 5, pour la copie avec
propagation comme pour la bibliothèque inchangée. Les décisions d'un
produit ne dépendant que de ses boîtes, tout rejet du front d'origine est
aussi obtenu avec propagation : le résidu avec propagation est inclus dans
le résidu d'origine, voie par voie.

Résultats à Kmax 10, s=8, graine 3, mode `MidpointSamples` (reçu
[PROPAGATION_CHECKS.json](propagation_temoins_20260914/PROPAGATION_CHECKS.json) ;
`ref` = bibliothèque du constructeur inchangée, `copy` = copie sans
propagation, qui reproduit exactement les compteurs de `ref`, `propagate`
= copie avec propagation) :

| Famille | n | Résidu q2 (M paires) `ref` → `propagate` | q3 | q4 | Rectangles émis (M) | Visites | Certificats évalués | Temps (s) `ref` / `copy` / `propagate` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Uniforme | 8 000 | 3.19 → 1.44 (×0.45) | ×0.61 | ×0.67 | 3.85 → 2.89 (×0.75) | ×0.86 | ×0.24 | 6.1 / 8.5 / 9.1 |
| Uniforme | 16 000 | 7.35 → 3.24 (×0.44) | ×0.57 | ×0.63 | 9.37 → 6.79 (×0.72) | ×0.83 | ×0.22 | 15.5 / 21.7 / 22.7 |
| Uniforme | 32 000 | 17.33 → 6.99 (×0.40) | ×0.53 | ×0.60 | 20.91 → 14.54 (×0.70) | ×0.80 | ×0.21 | 37.2 / 52.2 / 50.8 |
| Terrain mince | 8 000 | 0.94 → 0.50 (×0.53) | ×0.62 | ×0.68 | 0.60 → 0.47 (×0.79) | ×0.86 | ×0.21 | 0.8 / 1.2 / 1.2 |
| Terrain mince | 16 000 | 1.89 → 1.00 (×0.53) | ×0.62 | ×0.67 | 1.27 → 1.01 (×0.79) | ×0.86 | ×0.21 | 1.8 / 2.5 / 2.6 |
| Terrain mince | 32 000 | 4.02 → 2.09 (×0.52) | ×0.61 | ×0.66 | 2.75 → 2.18 (×0.79) | ×0.86 | ×0.20 | 4.0 / 6.0 / 6.3 |
| Huit amas | 8 000 | 29.73 → 28.96 (×0.97) | ×0.98 | ×0.98 | 1.84 → 1.58 (×0.86) | ×0.95 | ×0.28 | 2.2 / 3.1 / 3.8 |
| Huit amas | 16 000 | 116.75 → 114.42 (×0.98) | ×0.98 | ×0.98 | 5.25 → 4.25 (×0.81) | ×0.91 | ×0.25 | 7.1 / 9.4 / 11.0 |
| Huit amas | 32 000 | 460.08 → 453.50 (×0.99) | ×0.98 | ×0.98 | 13.27 → 10.17 (×0.77) | ×0.87 | ×0.23 | 19.1 / 26.9 / 29.5 |
| Deux rangées | 8 000 | 16.09 → 16.09 (×1.00) | ×1.00 | ×1.00 | 0.07 → 0.07 (×1.00) | ×1.00 | ×0.20 | 0.0 / 0.1 / 0.1 |
| Deux rangées | 16 000 | 64.18 → 64.18 (×1.00) | ×1.00 | ×1.00 | 0.14 → 0.14 (×1.00) | ×1.00 | ×0.20 | 0.1 / 0.1 / 0.1 |
| Deux rangées | 32 000 | 160.37 → 160.37 (×1.00) | ×1.00 | ×1.00 | 0.28 → 0.28 (×1.00) | ×1.00 | ×0.20 | 0.2 / 0.3 / 0.3 |

Les compteurs sont déterministes ; les temps sont ceux d'une machine
partagée et la copie porte un descripteur de tâche plus lourd que
l'original, ce que la colonne `copy` isole. Ce prototype n'est pas un
moteur : il ne réordonne pas le proposeur, ne partage aucun bloc Z
pendant la descente et conserve le redémarrage à la racine ; il montre
seulement que les crédits partiels perdus valent un gain substantiel sur
le résidu et sur le nombre de rectangles émis, sans coût discret
supplémentaire.

## 3. Ce que j'en conclus pour le pilote

1. Transmettre les témoins certifiés (identifiants, par voie, avec
   exclusion des doublons) est sûr par restriction et rentable ; c'est le
   schéma A2 de `WSPD_Q2_Q3_Q4.md` §9, dont le constructeur demandait la
   contre-lecture : la présente mesure en est une première.
2. Les blocs Z certifiés pendant la descente (max avec les échantillons,
   jamais somme emboîtée) sont le levier suivant, puisque 71 % des
   recherches n'aboutissent pas faute de proposition : un bloc entièrement
   dans la lentille crédite d'un coup sa population.
3. Le test de lentille exact ne mérite pas sa place dans le chemin chaud ;
   il reste un outil d'audit.

## 4. Reproduction

```bash
mkdir -p /tmp/pinned && git archive da366f7f morsehgp3D_v8/src morsehgp3D_v8/bench | tar -x -C /tmp/pinned
cmake -S /tmp/pinned/morsehgp3D_v8 -B /tmp/pinned_build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && cmake --build /tmp/pinned_build --target mhgp8_p0
python3 -B morsehgp3D_v8/audits/propagation_temoins_20260914/run_propagation.py --lib /tmp/pinned_build/libmhgp8_p0.a --src-root /tmp/pinned/morsehgp3D_v8 --build-dir /tmp/propagation_build --output /tmp/propagation.json
python3 -B -O morsehgp3D_v8/audits/propagation_temoins_20260914/run_propagation.py --lib /tmp/pinned_build/libmhgp8_p0.a --src-root /tmp/pinned/morsehgp3D_v8 --build-dir /tmp/propagation_build --sizes 8000 --output /tmp/propagation_O.json
```

Compiler contre une extraction de `da366f7f`, pas contre le worktree
partagé : le constructeur y modifie déjà `front.hpp` (masque de voies
demandé), et l'édition de lien échoue alors contre la bibliothèque
épinglée. Le reçu épingle `src/wspd/front.{cpp,hpp}` d'origine (blobs de
da366f7f), la copie, la bibliothèque liée, et conserve le diff
copie/source intégral. Le rejeu `python3 -O` aux tailles 8k rend les
mêmes compteurs que le reçu sur ses 36 exécutions.
