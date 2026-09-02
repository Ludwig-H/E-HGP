# Réponse à la conception C6 — anneau et couture hôte

Date : 2 septembre 2026. Question jugée : `17b6dbea` ; ce pin ajoute une note
de conception, aucun code C6.

```text
phase=exploration_v6_hors_registre
backend=cuda_g4
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

GCP non utilisé. Cette réponse donne un **GO de conception borné**, pas un GO
de session ni une réception d'implémentation. La bonne cible est de supprimer
la matérialisation globale, puis de paralléliser et recouvrir la couture tout
en gardant `gpu_wire_v1` et l'objet aval identiques.

## Correction structurante : séparer les leases avant de doubler le device

La première réponse était trop catégorique : deux slots à **lease unique** ne
peuvent pas porter simultanément `pack(k+1)`, device(k) et `rebuild(k-1)`, mais
deux paires dont IN et OUT ont des leases séparés le peuvent. `pack(k+1)` écrit
IN[p] pendant que `rebuild(k-1)` lit OUT[p] ; seul le futur D2H vers OUT[p]
doit attendre `rebuild_done`.

Le premier jalon C6a recommandé est donc plus simple que deux flux complets :

- deux tampons IN hôte épinglés et deux tampons OUT hôte épinglés, avec leases
  séparés ;
- un seul flux CUDA et un seul jeu de buffers device, donc un seul lot device
  à la fois ;
- recouvrement hôte `pack(k+1)` / device(k) / hôte `rebuild(k-1)`.

Cela attaque les 6,75 s de couture hôte observées sans doubler immédiatement
la VRAM, les flux et la preuve pour 154 ms de kernels. Deux flux et deux zones
device restent un palier ultérieur : ils ne sont justifiés que si une mesure
montre qu'il faut aussi recouvrir H2D, kernels et D2H de lots distincts.

Chaque ressource porte au minimum `{epoch, base_global, nb}` : IN est libéré
après `h2d_done`, le jeu device après `d2h_done`, OUT seulement après
`rebuild_done`. Le D2H suivant vers un OUT réutilisé attend explicitement son
lease ; aucune fin CUDA ne vaut implicitement fin de lecture hôte.

Chaque lot suit l'ordre logique
`FREE→PACKING→READY→H2D→KERNELS→D2H→READY_HOST→VALIDATED→REBUILT→FREE`.
H2D, remplissage éventuel, kernels, D2H et événement d'un lot restent dans le
même flux non défaut. Le wrapper de lancement porte explicitement ce flux,
même si les corps des kernels restent inchangés. Si un palier à deux lots
device est ouvert plus tard, il exigera alors deux zones device disjointes.

La retraite reste strictement ordonnée par `base_global`, jamais par ordre de
fin CUDA. Il faut ici fermer une contradiction préexistante : `GPU.md` et le
commentaire CMake disent « toutes les sorties validées avant toute
reconstruction », tandis que C5 valide chaque record puis le reconstruit dans
un temporaire avant de valider le suivant. C6 doit versionner la règle retenue.
La règle utile est : lot entièrement validé avant toute lecture/reconstruction
de ce lot, lots antérieurs conservés dans des temporaires invisibles, et toute
corruption d'un lot tardif jette l'ensemble. Une fixture de corruption tardive
doit donc prouver zéro `Survivor`, `BallData` ou `ExpandStats` visible.

La première erreur ferme l'admission, draine le travail déjà lancé, choisit
l'erreur selon une règle globale déterministe, libère par RAII et refuse
l'opération entière. `survivants`, `BallData` et compteurs sémantiques
`ExpandStats` ne deviennent visibles qu'après le swap terminal ; la télémétrie
partielle d'échec garde un contrat séparé.

## Réponse aux quatre verrous

### 1. Sentinelles

Conserver le préremplissage hôte dans C6a afin d'isoler le changement
d'ordonnancement. `k_fill_sentinels` est acceptable comme C6b distinct : il
remplit tous les champs à chaque epoch, dans le flux du lot et avant les deux
kernels. La porte composée `skip-fill + skip-ball-write` doit partir d'un
état stale déterministe après réutilisation de slot, pas d'une allocation
indéterminée. Une porte dédiée fill-only/readback tue `skip-fill`,
`skip-ball-write` reste la dent simple de la frontière, et la scène composée
prouve leur interaction. Tester au moins trois lots pour forcer le wrap d'un
anneau de profondeur deux.

### 2. Chronomètres

Ne pas faire fermer l'étage par la somme `pack + attente + rebuild`. Sous
recouvrement, les travaux hôte et device se chevauchent, et CUDA events et
`steady_clock` ne partagent pas une origine permettant une partition globale
à ±0,4 ms.

Publier en entiers :

- `stage_wall_ns` sur l'horloge hôte ;
- une partition externe disjointe `setup + pipeline_wall + finalize` ;
- des `pack_thread_ns` et `rebuild_thread_ns` si l'on somme les durées des
  ouvriers, et `host_wait_device_ns`, tous explicitement **non additifs** au
  mur. Si pack/rebuild désignent plutôt une enveloppe murale, les nommer
  `*_wall_envelope_ns` ;
- H2D/kernels/D2H en `*_event_us` avec une règle de quantification déclarée :
  les événements CUDA fournissent un flottant en millisecondes, pas des
  nanosecondes exactes partageant l'origine de `steady_clock`.

Ne pas publier `overlap_saved_ns` : la somme des temps ouvriers ne mesure pas
un gain causal de recouvrement. Un indicateur ultérieur demanderait une formule
et des domaines d'horloge explicitement spécifiés. Le juge C6 doit être
versionné ; sans jeton C6, la grammaire C5 reste inchangée.

### 3. Stub différé

Ne pas rendre `cuda_stub.hpp` différé : sa sémantique séquentielle reste
l'oracle hôte C2--C5. Ajouter un backend ou modèle différé **C6 séparé**, qualifié
d'auto-test du scheduler seulement. Il doit forcer fins inversées, wrap de
slot, tail, erreur précoce et tardive, et tuer au moins
`reuse-before-event`, `completion-order-merge`, `wrong-epoch/base` et
`publish-prefix`. Il ne prouve ni device ni absence de course.

La parité réelle ×5 à 50k prouve l'objet sur ce point, pas la concurrence.
La garde device doit aussi exercer plusieurs rotations, tailles de tail et
ordres d'achèvement, avec les outils CUDA appropriés ; aucun verdict de
performance ne vient du stub.

### 4. `resize`

Le mesurer seulement lorsque cette stratégie existe réellement. `reserve()`
n'augmente pas `size()` et ne permet ni des écritures indexées hors taille, ni
des `push_back` concurrents sûrs. Avec l'API actuelle, une reconstruction
parallèle exige `resize`, un stockage non initialisé correctement géré, ou
des vecteurs privés suivis d'une concaténation qui coûte copie et résidence.
Le premier jalon peut donc garder un rebuild séquentiel avec
`reserve(nb_total)` comme borne conservatrice, ou compter en deux passes avant
une réserve exacte, et n'ouvrir cette arène qu'après mesure.

De même, deux appels hôte qui créent chacun `threads` travailleurs peuvent
sur-souscrire la G4. Pack et rebuild partagent un scheduler persistant borné
à 48 travailleurs, ou restent séquentiels entre eux au premier jalon tout en
se recouvrant avec le device.

## Contrats à conserver et claims à resserrer

- Prévalider tous les produits de tailles avant allocation, notamment
  `nb×112`, les sorties et le nombre de lots ; écritures par offsets disjoints,
  aucun `push_back` partagé.
- Exiger des allocations hôte réellement épinglées, alignées, possédées par
  RAII et toutes acquises avant la première admission ; aucun repli silencieux
  vers du pageable. Graver séparément octets logiques, padding, pic pinned et
  pic device.
- Conserver `cand_idx = base + gid`, l'ordre global, les digests et tous les
  rejets de C5. Comparer sur le même pin le `GpuBallIn` concaténé octet par
  octet, puis l'objet sémantique, les digests, cartes et totaux ; records de
  chronométrie C5/C6 ne sont évidemment pas bit-identiques.
- Corriger ou supprimer d'abord les constantes inutilisées
  `kWirePrefilterOutBytes=12` et `kWireCensusOutBytes=92` : le contrat et les
  copies effectifs portent 9 et 91 octets, soit 100 par boule. Il ne faut pas
  dimensionner un slab sur deux autorités concurrentes.
- Ne pas affirmer que « les 48 fils sont oisifs » sans mesure d'utilisation.
  Les temps cités mêlent par ailleurs médiane et une répétition ; choisir une
  statistique unique pour le baseline apparié.
- Les 1,0–1,4 s et −21 % restent des hypothèses : pack et rebuild peuvent être
  bornés par la bande passante mémoire.
- C6 peut déplacer le mur de résidence en supprimant plusieurs Gio de staging
  global, tout en ajoutant buffers épinglés et jeux device. Mesurer RSS,
  `VmLck`, VRAM et pics par phase avant de le classer comme palier de débit
  seulement. La baseline d'échelle reste donc prioritaire.

C6 est en amont du fold : libérer ses buffers et flux avant celui-ci, puis
ajouter une porte croisée `C6 × layout {classic, csr}` sur digests, cartes,
totaux, storage kind, offsets et `csr_fallback=0`. Pour la mesure causale de
C6, garder le layout fixe ; aucun gain KeyCSR n'est hérité.

## Séquence de livraison conseillée

1. Baseline d'échelle, puis contrat des leases, budgets et erreurs.
2. Encodeur pur à offsets fixes, prévalidation et `pack == append` sur tails,
   bords et plusieurs nombres de fils.
3. Backend différé C6 séparé et mutants d'ordonnancement ; garder
   `cuda_stub.hpp` séquentiel.
4. C6a CUDA : sentinelles hôte, deux IN + deux OUT aux leases séparés, un flux
   et un jeu device, rebuild séquentiel, parité CPU/C5/C6 et rotations, sans
   claim de performance.
5. Seulement si les mesures le demandent : reconstruction parallèle, deuxième
   flux/jeu device puis fill device, chacun comme facteur isolé.
6. Campagne appariée sur un même pin, bras CPU, C5 et C6 contrebalancés.

Cette décomposition aide C6 à progresser sans rouvrir l'objet mathématique ni
faire porter au premier commit la preuve simultanée du wire, du scheduler, de
la concurrence, de la mémoire et du gain.
