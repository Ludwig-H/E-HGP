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

## Contre-lecture du WIP d'encodeur après `788b22da`

Le nouveau `pack_ball_in` est une bonne brique : offsets little-endian
explicites, produits de tailles vérifiés, écriture disjointe et confrontation
octet pour octet avec `append_ball_in`. Sur le snapshot lu, le témoin direct
passe et les deux mutants stride/slack rendent code 4. Ce lot reste non reçu
tant qu'il est non commité et non raccordé à CMake.

Une brèche précise doit être fermée avant ce pin. Le template
`pack_ball_range` appelle `Source` deux fois, une fois pour valider et une fois
pour écrire, tout en promettant qu'un succès signifie que les boules écrites
ont été validées. Un `Source` mutable peut rendre une clé valide à la première
passe, puis `a=0` à la seconde : le probe rend `kOk` et écrit la clé invalide.
Un chevauchement source/destination non déclaré peut de même corrompre une
boule future ; une scène locale finit jusque dans une division par zéro de
`wire_t1_candidates`. Une callback qui lève à la seconde passe détruit aussi la
promesse « valeur de refus, jamais écriture partielle ».

La fermeture la plus simple respecte le jalon : faire de
`pack_candidate_range` l'API transactionnelle réelle sur un `CandidateSpan`
const, vérifier avant écriture `arity <= smax + 1`, stabilité/non-chevauchement
et toutes les clés, puis écrire directement ce même tableau. Le helper template
reste interne avec préconditions explicites `stable`, `noexcept`, `noalias`, ou
perd son claim transactionnel. Il n'est pas utile d'allouer une copie pour
sauver une généralité que C6 ne consomme pas.

Avant commit, raccorder le témoin code 0, les deux mutants code 4 et un plancher
dans CMake ; ils sont actuellement au registre mais orphelins. Le commentaire
du test doit enfin distinguer les additions/caps réellement exercés du statut
`kByteOverflow` : sur une cible 64 bits, la borne `2^32-1` rend le débordement
de `nb*112` inatteignable. Tester `mul_checked` directement ou annoncer cette
dominance est plus exact que revendiquer une dent absente.

La confrontation partage aussi `wire_threshold` entre le bras `append` et le
bras `pack` : une erreur commune de seuil reste verte. Ajouter quelques attentes
indépendantes, par exemple `h=10,9,8` pour les arités 2,3,4 à `smax=11`, suffit.
La dérivation T1 partagée est déjà jugée par la porte wire historique ; le
rapport doit donc parler d'indépendance de sérialisation, pas de deux encodeurs
entièrement indépendants. Distinguer enfin `kNullSource` de `kNullBuffer`
évitera qu'un refus de source nulle soit rendu comme « tampon nul ».

## Contre-lecture du WIP d'anneau différé

Le modèle est une aide constructive : les leases IN/device/OUT sont séparés,
les fins inversées et corruptions tardives sont exercées, et Claude a déjà
fermé pendant la relecture la transition prématurée kernels→D2H avec
`kernels_done`. En compilation autonome `-Werror` avec `MHGP6_TESTING`, le
nominal passe 21 scènes, 64 lots, 87 rotations, trois queues et 76 fins
inversées ; les cinq mutants rendent chacun code 4 et un plancher forcé code 3.
Ce WIP reste un auto-test hôte non épinglé, sans preuve de course ni de CUDA.

Quatre coutures locales restent à fermer avant son pin :

1. Les fins ne sont pas one-shot. Après `h2d_end`, le ticket reste `H2D` alors
   que son slot IN devient `kNoSlot`; un second appel franchit `expect` puis
   indexe les slots avec cette sentinelle. Le probe ASan/UBSan termine en faute
   mémoire. `kernels_end` est répétable, et `rebuild_end` l'est aussi pour un
   lot achevé hors ordre, resté `REBUILT` après avoir rendu OUT. Des états ou
   drapeaux `*_done`, vérifiés avant tout `slot_of`, et des fixtures de double
   complétion ferment la classe entière.
2. `publish()` accepte un second appel : les deux rendent vrai et le second
   rééchange l'objet vers `pending_`, vidant la sortie. Après reconstruction
   complète, `abort_all()` puis `publish()` réussit de même avec un objet vide.
   Un état terminal explicite doit faire refuser sans mutation tout rappel et
   toute publication après abandon.
3. `fail_device` déduit le rang de l'état mutable au moment du signalement et
   accepte même un lot libre ou jamais admis. Un même échec asynchrone peut donc
   changer de rang selon son timing. Le callback doit capturer lot, epoch,
   étape et base, faire vérifier leur bail actif, puis rejouer les tie-breaks
   étape/code/message dans les deux ordres d'arrivée.
4. `ScopedRelease::~ScopedRelease()` appelle `release()`, qui alloue encore via
   `journal_.push_back`. Un `bad_alloc` peut donc terminer le processus dans le
   destructeur censé rendre le bail. Libérer état et compteurs par un chemin
   strictement non allouant ; préallouer le journal ou le rendre best-effort.

Deux finitions sont P2 : le témoin indépendant de journal doit comparer tout le
tuple `{lot, epoch, base, nb}` au rendu, pas seulement le lot ; le hook
`(u8)(shell_cap + 200)` reboucle sous la borne pour `shell_cap` entre 56 et 64,
donc employer `shell_cap + 1` et graver ce bord. Enfin, ni `lot_ring` ni les
mutants `c6-*` ne sont encore raccordés au CMake et au registre ; le fallback
local convient au WIP, pas au pin.

## Correction structurante : séparer les leases avant de doubler le device

La première réponse était trop catégorique : deux slots à **lease unique** ne
peuvent pas porter simultanément `pack(k+1)`, device(k) et `rebuild(k-1)`, mais
deux paires dont IN et OUT ont des leases séparés le peuvent. `pack(k+1)` écrit
IN[p] pendant que `rebuild(k-1)` lit OUT[p] ; seul le futur D2H vers OUT[p]
doit attendre `rebuild_done`. Si le préremplissage hôte des sentinelles emploie
ce même OUT comme source H2D, son fill et son H2D attendent eux aussi ce lease.
L'alternative est un template épinglé immuable distinct, dont les
`100×lot_effectif` octets doivent alors entrer dans le budget pinned.

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

Trois tickets de lots logiques coexistent donc bien (`k-1`, `k`, `k+1`) : la
profondeur deux porte séparément sur les ressources IN et OUT, pas sur deux
`LotSlot` monolithiques à état unique. Chaque ticket suit l'ordre logique
`FREE→PACKING→READY→H2D→KERNELS→D2H→READY_HOST→VALIDATED→REBUILT→FREE`.
Les FSM et epochs de réutilisation IN/OUT restent distinctes. H2D, remplissage
éventuel, kernels, D2H et événement d'un lot restent dans le
même flux non défaut. Le wrapper de lancement porte explicitement ce flux,
même si les corps des kernels restent inchangés. Si un palier à deux lots
device est ouvert plus tard, il exigera alors deux zones device disjointes.

La retraite reste strictement ordonnée par `base_global`, jamais par ordre de
fin CUDA. Il faut ici lever une ambiguïté préexistante : la formule de `GPU.md`
et du commentaire CMake, « avant toute reconstruction », peut se lire à l'échelle
du lot, tandis que C5 valide chaque record avant de le reconstruire dans un
temporaire. Ce n'est pas une brèche transactionnelle puisque le swap reste
terminal, mais C6 doit versionner la granularité retenue. La règle la plus
simple à juger est : lot entièrement validé avant toute reconstruction de ce
lot, lots antérieurs conservés dans des temporaires invisibles, et toute
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
- des attentes de backpressure séparées, par exemple `wait_in_h2d_ns` et
  `wait_out_rebuild_ns`, afin de distinguer un device affamé d'un simple temps
  d'attente terminal ;
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
La garde device C6a doit aussi exercer plusieurs rotations, tailles de tail et
la dépendance event-before-reuse en FIFO. Avec un flux et un jeu device, les
fins CUDA de lots ne peuvent pas s'inverser : les ordres d'achèvement inversés
restent au modèle différé, puis à la future porte deux flux. Aucun verdict de
performance ne vient du stub.

### 4. `resize`

Le mesurer seulement lorsque cette stratégie existe réellement. `reserve()`
n'augmente pas `size()` et ne permet ni des écritures indexées hors taille, ni
des `push_back` concurrents sûrs. Avec l'API actuelle, une reconstruction
parallèle exige `resize`, un stockage non initialisé correctement géré, ou
des vecteurs privés suivis d'une concaténation qui coûte copie et résidence.
Le premier jalon peut donc garder un rebuild séquentiel avec
`reserve(nb_total)` comme borne conservatrice. Obtenir d'abord une taille
globale exacte en deux passes contredit le streaming mono-passe : il faut
rejouer le device, retenir les OUT de tous les lots, ou garder des vecteurs par
lot puis les concaténer, avec leur coût de copie et de résidence. N'ouvrir cette
variante qu'après mesure.

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
  octet dans une porte bornée, ou son digest calculé en streaming à l'échelle,
  puis l'objet sémantique, les digests, cartes et totaux. Ne pas reconstruire
  ce concaténé dans le chemin produit, ce qui annulerait le gain de résidence ;
  les records de chronométrie C5/C6 ne sont évidemment pas bit-identiques.
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
