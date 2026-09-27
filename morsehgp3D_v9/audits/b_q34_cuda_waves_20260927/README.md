# Vagues q34 CUDA — prototype isolé, 27 septembre 2026

État : **porte portable locale passée ; CUDA non compilé et non exécuté ici**.
Ni accélération GPU, ni gain net S2, ni contrat FULL ne sont acquis par ces
reçus. Aucun moteur modifié, aucun GCP utilisé par cet auteur.

## Ce qui change

Port explicite du décodeur `b_q34_arena_waves_20260927/waves.hpp` et de son
arène collective figés au commit `70168cc3b`. Les prédicats GPU exacts sont
**inclus sans modification** depuis `src/gpu/witness_filter.hpp` ; les nœuds
proviennent de `gpu::flatten_nodes`. Le patron de gestion des allocations
CUDA est adapté explicitement de `src/gpu/filter_runner.cu`, sans appel de
`run_filter_batch` et sans ses tableaux de taille P.

Chaîne mesurée proposée : préparation CPU → copie privée POD → transfert
→ vagues Q (décodage, filtre exact, scan, dispersion) → rapatriement des S
survivantes → tri par ordinal original → sortie native ordonnée.

- `Prepared` reste inchangé, propriétaire immuable du nuage et des plans.
  `Snapshot` conserve ce propriétaire **et copie** les tableaux dans son
  stockage privé ; aucune adoption de tampon mutable de l'appelant.
  Sa vue et ses pointeurs sont empruntés immuables pendant l'appel synchrone,
  pas une API de géométrie brute non fiable.
- P désigne les paires logiques natives après filtre de rectangle ; E les
  requêtes physiques résiduelles après Pool. Aucun tableau indexé par P/E.
  Descripteurs, produits, préfixes et ordinals globaux restent `uint64_t`.
- Les bandes recalculent le masque depuis les crédits de classe A et du
  rang groupé B. Les rectangles non planifiés gardent leur produit complet.
  Les crédits ne seedent jamais le census : le filtre ponctuel repart de zéro.
- Le décodeur retrouve l'ordinal de la paire dans les rectangles d'origine,
  y compris les trous des rectangles fermés. Le tri final conserve exactement
  l'ordre/endpoints/masques avant S3. Rejet logique q = Pq − Eq + rejets Eq.
- Seule une vague est bornée par `min(Q demandé, INT_MAX)` pour le scan CUB.
  Le découpage épuise E ; ce n'est pas un plafond de recherche. Si E=0,
  aucune vague ni scan vide n'est lancé. E>0/S=0 est testé séparément.
- `stack_failure=0xff` ou un échec de décodage positionne le drapeau global
  d'échec ; aucune sortie n'est publiée. Ce marqueur n'est jamais une voie
  survivante. Tous les buffers device sont libérés avant le retour normal ;
  RAII les libère aussi aux exceptions. Pas de reprise partielle annoncée.

## Qualification locale close

`receipts/q34_cuda_waves_20260927/r2` : 31 commandes ; GCC Release et Clang
ASan/UBSan/LSan ; vérification du header portable en C++17 avec les deux
compilateurs ; 85 cas et 340 consommations par build, capacités 1, 7, 257,
et 2^31+17. K1/2/5/10, s8/10/12 ; uniformes/terrains/amas/rangées, facteurs
permutés, bandes et replis mélangés, sortie vide avec E non nul.

Le gate dénombre par build 101 320 requêtes, 39 868 survivantes, 63 plans,
1 411 replis, 179 paires éliminées par Pool, 312 réductions de masque par
crédits, 1 668 survivantes du repli, 376 désordres avant tri. Ces comptes
sont ceux du **corpus**, pas d'une trame LiDAR. Les réductions de voies sont
comparées au masque du rectangle, pas seulement au masque 6.

Une fixture arithmétique décode le dernier élément d'un produit 2^32 et
son ordinal supérieur à 2^32 ; elle n'alloue pas ce produit et n'est pas
une mesure de passage à l'échelle. Refus EOF et overflow contrôlés.
Seize refus CLI/stub (8 par build) vérifient notamment les grands K/s,
signes, suffixes, dépassements et CUDA absent. Deux mutants compilés en
Release sont tués causalement : masque 6 (`physical_counts`) ; ordinal
groupé (`survivors_order_mask`). Les macros mutantes sont absentes du
build normal et du futur build CUDA.

Première tentative `r1` conservée : échec de compilation, alias `u32` non
qualifié dans le lecteur u18. Sa version de `probe.cpp` est archivée dans
`r1/sources/` ; le correctif est `using cw::u32`. Pas de défaut géométrique
déduit de cet échec. Sources et builds r1/r2 restent intacts après capture.

Builds : `/workspaces/E-HGP/build/v9-audit-q34-cuda-waves-20260927-r2_release`
et suffixe `_sanitize`. Le runner local réutilise des archives générateur
épinglées, compile ses propres sondes et pinne aussi le `.cu` **non compilé**.
Les lecteurs rejugent les sources/binaires/commandes/sorties en normal et
avec Python `-O`, sans assertions désactivables ni recompilation.

## Interface pour la qualification G4

Mini-CMake autonome : `MHGP9_SOURCE_ROOT` désigne `morsehgp3D_v9` du
snapshot. Par défaut il compile les 25 sources du générateur explicitement
énumérées ; `MHGP9_GEN_LIBRARY` n'est qu'une option de réemploi local.
`MHGP9_CUDA_WAVES_ENABLE_CUDA=ON` compile `runner.cu` en CUDA17, architecture
120, CUB du toolkit, runtime statique ; les unités hôtes restent C++20.
Sans cette option le stub refuse toute exécution device. Aucun toolkit
n'a été installé localement ; la compilation autonome/CUDA reste à juger
avant toute mesure réelle G4.

Exécutable `mhgp9_q34_cuda_waves` :

```text
--gate
--gate --cuda
--frame PATH --cuda --q=262144 --k=5 --s=8
```

La porte device appelle le vrai CUDA pour Q7 et Q257 sur les mêmes cas,
ainsi que Q1 sur les petites masses ; le test portable Q>INT_MAX ne qualifie
pas un scan device géant. Elle compare chaque sortie au filtre CPU natif,
et ses compteurs physiques au chemin portable des mêmes prédicats GPU.
La mesure lit un fichier u32 little-endian/u18 XYZ, pas le binaire float32
SemanticKITTI original. Elle publie n, hash géométrique, K/s/Q, appareil,
P/E/S, digest et comparaison intégrale avec le filtre natif W4 ; préparation
W4 séparément. Le contrôleur doit lier ce fichier au manifeste sans sol
1 mm et exécuter la porte avant la trame ng00. Ces commandes ne calculent
ni catalogue, ni q3/q4 census, ni FULL.

## Coûts publiés et limites

`cuda_runner` comprend initialisation du runtime, allocation/upload,
chaque vague, compaction, téléchargement, tri/conversion CPU et destruction
des buffers internes. Il **n'inclut pas** `preparation` ni `snapshot` : les
ajouter pour discuter du remplacement S2. `reference` paie le filtre natif
de contrôle ; ce n'est pas du temps du nouveau moteur. `total` inclut aussi
lecture, index, front, contrôle et destruction finale des objets hôtes.

`waves_including_count_download` inclut les petits D2H de compteurs et des
deux derniers éléments du scan ; `survivor_download_allocate` inclut seulement
croissance du vecteur S et rapatriement des survivantes. `download_bytes`
compte **les deux**. Ces durées ne doivent donc pas être interprétées comme
un débit global D2H. Le tri CPU et la conversion sont encore scalaires.

`device_bytes` compte les buffers explicitement alloués : snapshot device,
deux tableaux Edge[Q], flags[Q], positions[Q], scratch CUB et compteurs.
Pas le contexte/runtime CUDA. `snapshot_array_bytes` compte les capacités
de ses tableaux, pas tout le processus. Préparation conservée, snapshot,
réallocations transitoires de S, copie native finale et référence de contrôle
coexistent : ce n'est pas un pic RAM complet. Le vecteur rapatrié ne contient
que S survivantes (avec croissance amortie), pas E emplacements cachés.
La mémoire supplémentaire suit O(n + R + F + bandes + Q + S), avec la
préparation CPU payée ; aucun résultat sous-quadratique global n'en découle.
