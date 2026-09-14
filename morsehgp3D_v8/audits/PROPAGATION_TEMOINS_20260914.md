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
propagation, pour le mode à blocs et pour la bibliothèque inchangée
(28 vérifications). Les décisions d'un
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
| Uniforme | 8 000 | 3.19 → 1.44 (×0.45) | ×0.61 | ×0.67 | 3.85 → 2.89 (×0.75) | ×0.86 | ×0.24 | 6.4 / 9.8 / 10.4 |
| Uniforme | 16 000 | 7.35 → 3.24 (×0.44) | ×0.57 | ×0.63 | 9.37 → 6.79 (×0.72) | ×0.83 | ×0.22 | 16.1 / 25.0 / 25.8 |
| Uniforme | 32 000 | 17.33 → 6.99 (×0.40) | ×0.53 | ×0.60 | 20.91 → 14.54 (×0.70) | ×0.80 | ×0.21 | 37.7 / 57.7 / 57.6 |
| Terrain mince | 8 000 | 0.94 → 0.50 (×0.53) | ×0.62 | ×0.68 | 0.60 → 0.47 (×0.79) | ×0.86 | ×0.21 | 0.8 / 1.3 / 1.4 |
| Terrain mince | 16 000 | 1.89 → 1.00 (×0.53) | ×0.62 | ×0.67 | 1.27 → 1.01 (×0.79) | ×0.86 | ×0.21 | 1.8 / 2.8 / 3.1 |
| Terrain mince | 32 000 | 4.02 → 2.09 (×0.52) | ×0.61 | ×0.66 | 2.75 → 2.18 (×0.79) | ×0.86 | ×0.20 | 4.0 / 6.2 / 6.7 |
| Huit amas | 8 000 | 29.73 → 28.96 (×0.97) | ×0.98 | ×0.98 | 1.84 → 1.58 (×0.86) | ×0.95 | ×0.28 | 2.3 / 3.5 / 4.3 |
| Huit amas | 16 000 | 116.75 → 114.42 (×0.98) | ×0.98 | ×0.98 | 5.25 → 4.25 (×0.81) | ×0.91 | ×0.25 | 7.2 / 11.1 / 12.9 |
| Huit amas | 32 000 | 460.08 → 453.50 (×0.99) | ×0.98 | ×0.98 | 13.27 → 10.17 (×0.77) | ×0.87 | ×0.23 | 20.1 / 31.1 / 34.4 |
| Deux rangées | 8 000 | 16.09 → 16.09 (×1.00) | ×1.00 | ×1.00 | 0.07 → 0.07 (×1.00) | ×1.00 | ×0.20 | 0.0 / 0.1 / 0.1 |
| Deux rangées | 16 000 | 64.18 → 64.18 (×1.00) | ×1.00 | ×1.00 | 0.14 → 0.14 (×1.00) | ×1.00 | ×0.20 | 0.1 / 0.2 / 0.2 |
| Deux rangées | 32 000 | 160.37 → 160.37 (×1.00) | ×1.00 | ×1.00 | 0.28 → 0.28 (×1.00) | ×1.00 | ×0.20 | 0.2 / 0.3 / 0.3 |

Les compteurs sont déterministes ; les temps sont ceux d'une machine
partagée et la copie porte un descripteur de tâche plus lourd que
l'original, ce que la colonne `copy` isole. Ce prototype n'est pas un
moteur : il ne réordonne pas le proposeur, ne partage aucun bloc Z
pendant la descente et conserve le redémarrage à la racine ; il montre
seulement que les crédits partiels perdus valent un gain substantiel sur
le résidu et sur le nombre de rectangles émis, sans coût discret
supplémentaire.

## 2 bis. Blocs Z certifiés le long de la descente

Question ouverte du constructeur : « efficacité d'un bloc Z certifié
pendant la descente (max avec les échantillons, pas somme emboîtée) ». Le
mode `blocks` de la copie ajoute, à la propagation ci-dessus, le test de
chaque bloc frère du chemin de descente par les bornes exactes de trois
boîtes (`h_minimum`, puis `xi_bounds` pour q3/q4) ; un bloc certifié est
crédité comme plage de rangs, et ni les rangs hérités ni les échantillons
de la fenêtre situés dans un bloc crédité ne sont comptés une seconde fois.
Les blocs frères d'un même chemin sont deux à deux disjoints ; un bloc qui
chevauche un bloc déjà crédité de la même voie est écarté. Les blocs
inclus dans un facteur sont exclus par leurs plages de rangs.

| Famille | n | Résidu q2 (M) `propagate` → `blocks` | q3 (rapport à `ref`) `propagate` → `blocks` | q4 | Rectangles émis (M) `propagate` → `blocks` | Tests de blocs par recherche | Temps (s) `propagate` / `blocks` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Uniforme | 8 000 | 1.44 → 1.17 (×0.45 → ×0.37) | ×0.61 → ×0.53 | ×0.67 → ×0.62 | 2.89 → 2.73 | 9.5 | 10.4 / 13.9 |
| Uniforme | 16 000 | 3.24 → 2.76 (×0.44 → ×0.38) | ×0.57 → ×0.51 | ×0.63 → ×0.58 | 6.79 → 6.46 | 10.4 | 25.8 / 35.3 |
| Uniforme | 32 000 | 6.99 → 5.82 (×0.40 → ×0.34) | ×0.53 → ×0.46 | ×0.60 → ×0.54 | 14.54 → 13.61 | 11.4 | 57.6 / 78.6 |
| Terrain mince | 8 000 | 0.50 → 0.43 (×0.53 → ×0.46) | ×0.62 → ×0.53 | ×0.68 → ×0.61 | 0.47 → 0.45 | 9.5 | 1.4 / 1.8 |
| Terrain mince | 16 000 | 1.00 → 0.83 (×0.53 → ×0.44) | ×0.62 → ×0.51 | ×0.67 → ×0.59 | 1.01 → 0.95 | 10.5 | 3.1 / 4.0 |
| Terrain mince | 32 000 | 2.09 → 1.79 (×0.52 → ×0.45) | ×0.61 → ×0.51 | ×0.66 → ×0.58 | 2.18 → 2.05 | 11.5 | 6.7 / 8.8 |
| Huit amas | 8 000 | 28.96 → 28.83 (×0.97 → ×0.97) | ×0.98 → ×0.98 | ×0.98 → ×0.98 | 1.58 → 1.55 | 9.7 | 4.3 / 5.8 |
| Huit amas | 16 000 | 114.42 → 114.13 (×0.98 → ×0.98) | ×0.98 → ×0.97 | ×0.98 → ×0.98 | 4.25 → 4.14 | 10.6 | 12.9 / 17.5 |
| Huit amas | 32 000 | 453.50 → 452.71 (×0.99 → ×0.98) | ×0.98 → ×0.98 | ×0.98 → ×0.98 | 10.17 → 9.71 | 11.5 | 34.4 / 47.1 |
| Deux rangées | 8 000 | 16.09 → 16.09 (×1.00 → ×1.00) | ×1.00 → ×1.00 | ×1.00 → ×1.00 | 0.07 → 0.07 | 9.7 | 0.1 / 0.1 |
| Deux rangées | 16 000 | 64.18 → 64.18 (×1.00 → ×1.00) | ×1.00 → ×1.00 | ×1.00 → ×1.00 | 0.14 → 0.14 | 10.7 | 0.2 / 0.2 |
| Deux rangées | 32 000 | 160.37 → 160.37 (×1.00 → ×1.00) | ×1.00 → ×1.00 | ×1.00 → ×1.00 | 0.28 → 0.28 | 11.7 | 0.3 / 0.5 |

Résultat : un gain supplémentaire réel mais modeste sur le résidu, payé
par environ dix tests de blocs par recherche (autant que la descente
elle-même). Restreindre les tests aux frères proches du milieu (distance
au milieu au plus deux ou quatre fois celle de l'enfant choisi) divise
les tests par douze mais ne garde que 2 à 4 % du gain : les blocs
utiles sont ceux, éloignés du chemin, qui tombent dans la lentille. La
force brute exacte donne 0 rejet non sûr pour ce mode sur les quatre
familles à n = 600. Ce prototype ne dit rien du coût de census évité par
le résidu supplémentaire retiré ; c'est cette comparaison, front plus
census, qui doit trancher.

## 3. Ce que j'en conclus pour le pilote

1. Transmettre les témoins certifiés (identifiants, par voie, avec
   exclusion des doublons) est sûr par restriction et rentable ; c'est le
   schéma A2 de `WSPD_Q2_Q3_Q4.md` §9, dont le constructeur demandait la
   contre-lecture : la présente mesure en est une première.
2. Les blocs Z certifiés pendant la descente apportent un complément
   modeste au prix d'autant de tests que la descente ; leur intérêt dépend
   du coût de census évité, à mesurer avec le raccord en cours. Le vrai
   gisement restant est le proposeur lui-même : 71 % des recherches
   n'aboutissent pas faute de proposition.
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
mêmes compteurs que le reçu sur ses exécutions rejouées.
