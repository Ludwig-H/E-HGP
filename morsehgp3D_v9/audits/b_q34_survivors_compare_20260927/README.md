# Comparaison appariée du résident S2 — préparation G4

27 septembre 2026. Harnais séparé, aucun changement du moteur ni des deux
implémentations gelées. **Aucune mesure G4 de ce comparateur pour l'instant.**
La référence de performance est le prototype résident af369c44 ; le candidat
est son port de sortie S résident publié en 6af40d886. Ce n'est pas une
comparaison au filtre GPU natif du moteur et ce n'est pas une tour FULL.

## Même travail, propriétaires distincts

Le lecteur u32/18 bits, le hash d'entrée et le digest natif sont portés
explicitement depuis
[le petit harnais résident](../b_q34_filtered_resident_20260927/probe.cpp).
Le nuage/index et le vecteur ordonné de rectangles sont construits une seule
fois : vrai front natif `MidpointSamples`, K5/s8, masque 6, **un worker**.
Aucun front/census n'est recopié. La trame entière sans sol utilise le fichier
1 mm figé du protocole ; ni float32 ni u16/2 cm ne sont raccordés implicitement.

Un oracle CPU natif W4 est calculé une fois, avant le premier appel CUDA.
Il est conservé identiquement pendant les quatre passages, hors chronos des
opérateurs. Chaque retour est comparé champ par champ : endpoints orientés,
masques et ordre de S, tous les masques rectangles, masses et rejets natifs.
La comparaison entre passages est donc exacte par cet oracle commun, pas
une simple égalité de hashes. Entre passages, P/E/S par voie, les huit
compteurs physiques, les 22 compteurs de préparation géométrique et les 15
compteurs d'arène doivent être identiques. Le digest est publié en plus.

Chaque passage crée sa propre session et son propre Prepared : aucun plan
d'une session n'est injecté dans une autre, aucun pointeur GPU n'est partagé.
Après consommation, Prepared, session, décision et buffers privés sont
détruits. Seul le résultat retourné demeure jusqu'à sa comparaison ; il est
ensuite détruit **avant le passage suivant**. Il n'y a donc ni accumulation
de quatre sorties S, ni réutilisation cachée de l'index GPU d'un autre passage.

## Le premier contexte n'est payé qu'une fois par processus

CLI de trame :

```text
mhgp9_survivors_compare --frame PATH --workers 4 --first baseline --cuda
mhgp9_survivors_compare --frame PATH --workers 4 --first candidate --cuda
```

W vaut 4 ou 48 ; K5/s8 et Q=Qr=262144 sont fixes dans ce premier protocole.
`--first baseline` donne ABBA ; `--first candidate` donne BAAB. Il faut les
deux **processus distincts**, pour chaque largeur retenue, afin que chaque
implémentation soit aussi essayée en première position. Aucun contexte CUDA
n'est ouvert par le lecteur, le front ou l'oracle CPU.

`first_cuda_call` vaut vrai seulement à la position 0.
`first_implementation_call` vaut vrai aux positions 0 et 1 : les noyaux de
chaque implémentation peuvent eux-mêmes être chargés paresseusement.
**Les quatre passages ne sont pas quatre démarrages CUDA froids.** Les
opérateurs possèdent tous des allocations neuves, mais le contexte reste
vivant entre eux. Le harnais n'appelle pas `cudaDeviceReset` et ne soustrait
pas l'initialisation pour fabriquer un chrono warm.

## Bornes chronométriques

Chaque ligne publie :

- `adapter` : création de session, init éventuelle, copies/index GPU, filtre
  rectangles, arène, consommation/tri/transport/conversion, destruction des
  propriétaires privés ; la sortie native est alors retenue.
- `consume_outer` : borne réelle autour de `Session::consume` ; `consume`
  est la mesure interne. Les champs détaillés restent emboîtés.
- `comparison` : contrôle complet au même oracle, hors `adapter`.
- `native_output_release` : destruction S + masques + label du résultat,
  payée avant le prochain passage.
- `adapter_plus_native_release_noncontiguous` : somme explicitement
  **non contiguë**, comparaison exclue. Ce n'est pas une nouvelle borne mur.
- `operation_observed` et `diagnostic_overhead` : fenêtre incluant contrôles,
  hashes et collecte des diagnostics, distinguée du coût de l'opérateur.

Lecture/index/front/oracle communs et leur destruction finale sont publiés
séparément. `common_times_ms.total` comprend ces phases et les quatre
passages : **ce total de banc n'est le coût d'aucun opérateur ni d'une tour**.
Le front W1 demeure à payer pour une exécution complète ; il ne disparaît
pas parce qu'il est partagé dans un benchmark.

Les sous-champs ne s'additionnent pas uniformément : le portable inclut
l'ajout dans les vagues ; le CUDA candidat sépare append/split/radix/validation.
`order_convert`, `pair_release` et certains `output_*` peuvent être emboîtés.
Comparer les bornes englobantes avant de chercher l'attribution des gains.

## Mémoire et contexte des résultats

Les capacités des décisions, Prepared, arènes temporaires, sorties natives,
transferts et pics device exposés sont publiés par passage. Le candidat
ajoute son ledger de sortie (croissance, copies, coexistence ancien/nouveau,
split, double buffer radix et scratch). La baseline n'exporte pas ce ledger :
`output_memory_available=false` et ses champs ne sont pas inventés à zéro.
Le pic hôte ancien de son vecteur de survivants n'est pas mesuré par cette API.

`array_peak_bytes` portable et CUDA ne possèdent pas le même périmètre ; voir
[le contrat du candidat](../b_q34_resident_survivors_20260927/README.md).
Les capacités ne sont ni le RSS, ni le pic complet du processus (qui garde
l'oracle et l'index communs), ni une somme de maxima non simultanés.

## Portes et compilation

`--gate` exécute de petites fixtures appariées en portable.
`--gate --cuda` exécute la même couture géométrique et rectangle/paires sur
GPU, contre l'oracle natif, dans un processus distinct des trames. La
[porte high-u64](../b_q34_survivors_device_gate_20260927/README.md) reste
également requise : elle teste directement les bits hauts, doublons et pics
du collecteur, que de petits lots géométriques ne peuvent pas produire.

CMake : option `MHGP9_COMPARE_ENABLE_CUDA=ON`, cible
`mhgp9_survivors_compare`, et cible incluse explicitement
`mhgp9_survivors_device_gate` dans le sous-dossier `device_gate`.
La bibliothèque `resident_gen` est reconstruite à partir de la liste source
gelée quand `MHGP9_GEN_LIBRARY` n'est pas fourni ; l'archive immuable locale
peut être utilisée pour les petits tests hors cloud. La cible historique
`mhgp9_q34_filtered_resident` est configurée mais exclue de ce build.

[Capture locale r1](checks/r1/summary.json) close PASS : 13 commandes,
compilation CUDA 12.9 neuve des deux exécutables, six unités de traduction
et 1088 dépendances pré-épinglées. Options indirectes `.rsp`, flags de
compilation, objets `.o`, dépendances `.o.d`, archives réellement résolues
au lien et binaires sont fermés. L'inventaire avant build est conservé et
lié aux sorties des commandes `-M`. Aucune qualification de l'ancien build
CUDA n'est transférée. Il ne s'agit pas d'un environnement système hermétique.

Les seuls appels des binaires dans cette capture sont `--gate` portable,
`--host-gate` du collecteur et trois refus CLI causaux. Le gate portable
donne 52 lots, 208 opérateurs, 26 576 requêtes physiques, 22 408 sorties
dans le corpus, plans/replis et ordre non vacuels. Ses deux compteurs de
premiers appels CUDA valent zéro. Ce n'est ni une exécution GPU ni une
mesure LiDAR. Le build est immuable :
`/workspaces/E-HGP/build/v9-survivors-compare-20260927-r1_cuda`.

Le protocole G4 devra fermer une **autre** compilation fraîche, exécuter
la porte high-u64 `--cuda` puis le comparatif `--gate --cuda`, avant les
quatre processus de trame (deux ordres × W4/W48). La porte CUDA appariée
compte un seul premier appel CUDA et deux premiers appels d'implémentation
sur l'ensemble de ses 52 lots, pas un premier contexte par lot.

## Décision de port ultérieure

Un gain contre le résident af369 ne suffira pas pour activer ce chemin dans
le moteur. Il restera à le comparer au **chemin GPU natif actuel**, dont le
chrono S2 historique d'environ 101 ms n'est pas une mesure appariée de ce
harnais. Celui-ci paie notamment une préparation d'arène CPU et plusieurs
vagues de filtre rectangles. Un tri plus rapide peut donc laisser le coût
complet moins bon que le chemin moteur. Dans ce cas, publier le prototype
sans l'activer ; les suites structurelles sont préparation des facteurs GPU
et résidence vers S3, pas une promesse de contrat FULL par soustraction des
temps de tri historiques.
