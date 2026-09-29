# Addendum : sonde CUDA et diagnostic des grands K

29 septembre 2026. Nouvelles lectures des publications CUDA et grands K.
Aucun nouveau calcul GCP, aucune modification du moteur. Seuls des recalculs
de CSV et une réplique hôte minuscule sous UBSan ont été exécutés.

## 1. CUDA : un résultat positif isolé, pas encore un port du moteur

Le reçu `receipts/g4_session7_cuda_probe_20260929` décrit une sonde compilée
depuis `3b3ea7241cf770906e5fa6d632c6f68d803ff2cb`, avec `nvcc -O3 -std=c++17
-arch=sm_120`. Ses 11 hashes publiés concordent. Sa commande est close, code 0,
en 4,176 s ; la fermeture indique la même génération de VM et `TERMINATED`,
avec clé retirée. Aucun nouveau contrôle externe de la VM n'a été effectué ici.

Le comparateur `cmp128` (`bench/g4/cuda_probe.cu:18–23,60–93`) compare
16 777 216 couples de produits signés i64 × i64, dont 2 011 255 égalités,
avec zéro différence face au calcul hôte. Chaque produit i64 × i64 entre bien
dans un entier signé 128 bits : cette partie a un domaine arithmétique valable.
Elle fournit une preuve expérimentale utile sur cette compilation, cette entrée
et ce GPU.

Cela ne couvre pas les prédicats complets, leurs sommes/déterminants, les
opérations I192, les divisions, les comparaisons de niveaux, le traitement des
égalités géométriques, les listes complètes de témoins ou la tour FULL. La sonde
n'inclut aucun noyau du moteur. Elle utilise le device 0 ; ni multi-GPU ni
multi-G4 n'est testé. Le reçu le dit correctement en ouverture ; la phrase
« l'arithmétique exacte de la v10 se porte » doit rester une perspective de
développement, pas une qualification transférée au moteur.

## 2. Le ratio de débit ×2,32 repose sur des débordements signés

Le comparateur valide précédent est distinct des deux boucles de débit.
Dans `cuda_probe.cu:25–46` :

```cpp
// loop64
acc += x * (x ^ t);
x = (x >> 1) ^ acc;
// loop128
acc += i128(x) * (x ^ t);
x = int64_t(acc >> 7) ^ x;
```

L'entrée d'indice 0 vaut obligatoirement `INT64_MIN` (ligne 69).
Pour `loop64`, dès `t=0`, le produit est `2^126`, hors du domaine i64.
Pour `loop128`, avec la conversion usuelle aux 64 bits bas, `t=2` produit la
somme exacte `253887739491777466561334935858653954048`, supérieure à
`2^127−1 = 170141183460469231731687303715884105727`.
Les opérations sont signées et ne sont pas une arithmétique modulaire définie
par le code. On ne peut donc tirer une qualification de débit arithmétique exact
de ces boucles optimisées.

La [réplique hôte](../../../receipts/audit_continu_20260929/timeout/cuda_loops_host_replica.cpp) reprend ces récurrences, sur
quatre itérations et uniquement cette entrée. GCC 13.3 avec
`-fsanitize=undefined -fno-sanitize-recover=undefined` signale le produit i64 à
`t=0` et l'addition i128 à `t=2`, chacun avec code 1 attendu. Le
[reçu](../../../receipts/audit_continu_20260929/timeout/receipt_cuda_loops_host.json) conserve commandes, diagnostics et hashes.
**Ce contrôle hôte ne prétend ni avoir instrumenté le device ni prouver quels
codes machine NVCC a effectivement produits.** Il confirme le défaut de domaine
des expressions recopiées ; la lecture de la source CUDA établit leur présence
dans la sonde de débit.

De plus, `gops` est calculé comme nombre d'itérations divisé par le temps
(`cuda_probe.cu:132–140`), pas comme un compte d'instructions ni de prédicats.
Les deux boucles ont des récurrences différentes. Le ratio 2,32 ne doit pas
dimensionner le port exact des feuilles avant un nouveau test à arithmétique
définie et à charge représentative.

Correction ciblée conseillée : soit employer explicitement des types non signés
pour une sonde modulaire et la nommer ainsi, soit conserver les types signés avec
des bornes prouvées pour toutes les itérations. Pour la décision moteur, mesurer
ensuite les vrais prédicats avec leurs cas limites et oracles. Aucun besoin de
refaire les 16 millions de comparaisons pour invalider les seuls débits ci-dessus.

## 3. Transferts et latence : périmètre des micro-mesures

Les 56,8/56,4 Go/s proviennent de quatre copies synchrones de 256 Mio entre
mémoire épinglée et GPU (`cuda_probe.cu:112–123`). Ils ne comprennent ni
constitution des listes de feuilles, ni allocation/épinglage, ni compactage,
ni rapatriement et fusion des sorties réelles du moteur.
Une règle de trois sous une milliseconde pour quelques dizaines de Mo est
plausible comme **coût de transfert seul**, pas comme budget d'une étape complète.

Les 1,86 µs sont la moyenne de 1 000 lancements, suivis d'une synchronisation
finale (`cuda_probe.cu:125–140`), pas une latence isolée mesurée par
lancement/synchronisation. Enfin, plusieurs retours CUDA intermédiaires sont
ignorés ; un seul `cudaGetLastError` terminal ne remplace pas des contrôles par
allocation, copie, lancement et synchronisation pour une future qualification.
Cela ne démontre aucun échec observé de cette session réussie.

## 4. Grand K : campagne complète, mais seulement pour un témoin au-delà de 10

Les 23 hashes du reçu `receipts/bench_dev_bigk_20260929` concordent.
Les deux CSV contiennent 10 240 et 20 480 lignes : **30 720 lignes distinctes,
384 scènes, exactement 80 configurations par scène, zéro refus déclaré**.
Il y a 128 scènes à chacune des tailles 8k/16k/32k et 48 scènes par famille.
Les deux commandes G4 sont closes avec code 0 ; leurs reçus publient
`TERMINATED` sur les générations correspondantes et clés retirées.

Mais `bench/synthetic/bigk_dev.py:83–100` exécute :

- MR₂-bord à K = 10/16/24/32/48, avec EOM z = 3 ou 6 ;
- sklearn en feuilles à ces mêmes K ;
- **la tour exacte uniquement à K = 10**, cover, EOM z = 6.

L'accord moyen tour/MR₂ observé à K ≤ 10 n'est ni une identité des objets ni une
borne uniforme de leur écart pour K > 10. Ce lot ne mesure donc ni la qualité
ni les coûts du moteur FULL à grand K. Le témoin exact MR₂ utilise par ailleurs
un Prim quadratique (`tests/head/mreach.cpp:24–64`) ; son rôle ici est celui
d'un diagnostic de qualité, pas d'un nouveau moteur sous-quadratique.

## 5. Les conclusions négatives dépendent de la tête et des familles

Recalcul direct des CSV, écarts appariés à K10 pour la même méthode :

| Configuration | Grand K | Écart moyen, 384 scènes | Écart hors `shells`, diagnostic |
| --- | ---: | ---: | ---: |
| MR₂, z6, `asc20_b2` | 16 | −0,083724 | +0,000161 |
| MR₂, z6, `asc20_b2` | 48 | −0,090475 | −0,001379 |
| MR₂, z3, sans remplissage | 32 | +0,004913 | +0,034454 |
| MR₂, z3, sans remplissage | 48 | +0,007622 | +0,037600 |
| sklearn feuilles, `asc20_b2` | 16 | +0,008303 | +0,007356 |

Le retrait des coquilles dans cette colonne est une analyse **post hoc**, pas une
nouvelle moyenne principale et pas une sélection autorisée de scènes. Elle
montre pourquoi une conclusion universelle serait injustifiée : l'échec global
de MR₂/z6 provient presque entièrement de cette famille, alors que certaines
autres configurations progressent. Sur `anisotropic`, MR₂/z3 sans remplissage
gagne +0,045997 à K48 dans ce même diagnostic dev.

La phrase « étendre le moteur exact au-delà de K10 ne se justifie pas pour le
clustering » doit donc être remplacée par une décision d'ingénierie conditionnée
au protocole : **cette grille ne justifie pas à elle seule un chantier immédiat
du moteur**, mais elle ne démontre pas que de grands K ne peuvent pas aider.
Z, le seuil de taille et les règles d'affectation influencent l'effet mesuré ;
deux z fixes ne qualifient pas toutes les têtes possibles à grand K.

Il n'est pas nécessaire de lancer un gros chantier pour le vérifier : figer
d'abord la cible statistique et une règle adaptée sans labels, tester cette règle
sur de nouvelles graines du témoin, puis décider si une extension exacte mérite
son coût. Ne pas utiliser les scènes déjà lues pour annoncer une confirmation
neuve, ni attribuer au moteur grand K des résultats qu'il n'a pas calculés.
