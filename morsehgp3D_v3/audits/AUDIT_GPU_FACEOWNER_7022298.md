# Audit du premier kernel GPU `face-owner` à `7022298`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_local_and_g4_spot_attempted`,
`profile=device_faceowner_oracle_k_le_6`, `mode=audit_and_bounded_qualification`,
`public_status=not_claimed`.

Snapshot : `HEAD=origin/main=70222980a82de418e91e6fa791382047067c22eb`,
worktree initial propre. Empreintes :

- `faceowner_device_kernel.cu=e4c83aa20cec97a15cd5315bf6c8d72694112df44cf17b9a4e75ad07cf3dd028`;
- `faceowner_device.hpp=909394ff8a990a749e7a8574c3a13451c94121456ed06aaf44af2981b9e4ba33`;
- `faceowner_device_qualification.cpp=03eabfb4233ab5da94b881fdddb58f0cf7625b084112269683a1f2ff90058527`;
- `saturated_fold_faceowner.hpp=436b88ebe0881d024c41222a379e5081640ed668e6fd09db86323a96d79def5f`;
- `postings_join_gate.cpp=2614dfa9a49b4c26a6e382eed0f6d02e9735eac4d477c68025381cd601a11012`;
- `CMakeLists.txt=5be3a08e6e0e79ea97463baad40a06b4144572c4305f100814cef8ca3ee65d49`.

## Verdict

Le kernel constitue une **bonne cinquième vérité de calcul bornée** pour le
flux `face-owner` : il émet les incidences de `k`-faces, les groupe, choisit
l'owner minimal en `(activation_rank,generator)`, produit les branches
d'étoile et les trie/déduplique avant comparaison arête par arête au CPU. Ce
n'est pas encore le fold hybride GPU produit : il matérialise toute la masse
`I_k`, s'arrête à `k<=6` et laisse le replay DSU au CPU.

La mathématique du job valide est positive. L'unranking lexicographique a été
rejoué exhaustivement jusqu'à `rank=20` puis aux frontières jusqu'à
`rank=32,k=6`, soit 198 922 cas, sans écart. Le scan inclusif des têtes puis la
soustraction de un donne les groupes zéro-based attendus. Le minimum packed
`(activation_rank,generator)` reproduit exactement le tie-break CPU et la date
`batch_of(member)` est correcte puisque l'owner est actif au plus tard au même
lot.

Le verdict d'intégration reste **NO-GO produit et NO-GO mémoire** : les entrées
ne sont pas validées avant déréférencement, le manifeste VRAM sous-estime les
allocations, le CTest CUDA est mal conditionné et les ressources CUDA ne sont
pas transactionnelles. Une G4 ne doit pour l'instant servir qu'à compiler et
falsifier ce petit oracle, jamais à annoncer un backend hybride ou un palier
50 k.

## Réception CPU locale

Une configuration Release neuve avec `MHGP3V_ENABLE_CUDA=OFF` compile le
nouveau harnais sous `-Werror`. La porte d'absence CUDA passe. Les sept mutants
`face-owner` passent 7/7 et les campagnes fixtures, générique et saturée passent
3/3 avec la nouvelle permutation. Le script de registre rend toujours
`Validated 20 implementation phases and their gates`.

La nouvelle porte de permutation est bien orientée : elle compare niveaux
rationnels exacts, témoins, saturés marquants et champs de reçu invariants; elle
exclut à juste titre les tentatives d'union et branches dédupliquées dépendantes
du tie-break. Il lui reste à appeler le comparateur sémantique commun, à comparer
tous les niveaux, puis à exiger `identities_ok` et les totaux invariants.

`collect_edges` est un payload de qualification utile mais doit rester hors du
chemin produit : il conserve `sum_k E_k` dans le reçu et copie le vecteur de
l'ordre courant, annulant temporairement le bénéfice mémoire du rejeu ordre par
ordre. Une porte CPU doit d'abord établir : mêmes folds et reçus cœur avec
`collect_edges=false/true`, `edges_k.size()==K`, flux trié/unique, cardinal par
ordre égal à `deduplicated_branches_k` et somme égale aux unions tentées.

## Blocage 1 — contrat d'entrée device

Le kernel lit `incidence_offsets.back()` avant toute validation. Il faut
refuser avant le premier appel CUDA si l'une des propriétés suivantes manque :

- pointeurs de sortie valides et capacité d'erreur positive;
- `k` dans `1..6`, même lorsque la masse est nulle;
- tailles `G`, `G` ou `G+1` exactes des tableaux;
- offsets non négatifs, monotones, dernier offset cohérent avec les buffers;
- `rank` dans `k..32` pour tout générateur émis et différence d'offset égale à
  la binomiale `C(rank,k)` calculée en entier vérifié;
- membres strictement triés, uniques et dans `0..2^21-1`;
- `activation_rank` permutation valide, lots monotones avec ce rang et owner
  actif au plus tard que chaque membre;
- `total`, conversions `size_t`, nombre de blocs et limite de grille vérifiés.

Une masse nulle ne doit jamais court-circuiter ces contrôles. Le kernel peut
ensuite utiliser une boucle grid-stride, ce qui supprime la dépendance à une
conversion non bornée du nombre de blocs.

## Blocage 2 — le manifeste VRAM n'est pas une borne

Le harnais admet avec `56*I + 4*M`. Les allocations explicitement simultanées
valent pourtant au moins les entrées plus `76*I + 16*S`, où `I` est le nombre
d'incidences et `S` celui de signatures. Dans le cas `S=I`, cela atteint
environ `92*I` **avant** les workspaces Thrust, l'allocateur, le contexte et la
sortie hôte. `device_bytes` oublie notamment le second buffer d'arêtes, les deux
sorties par groupe et tous les temporaires; il doit être nommé estimation et ne
jamais décider l'admission.

Deux réparations sont possibles :

1. requêter les workspaces CUB exacts, calculer toutes les tailles en entier
   vérifié, allouer depuis une arène plafonnée et publier son high-water;
2. simplifier le pipeline : stocker `activation_rank` dans `Incidence`, trier
   directement `(key,activation_rank,generator)`, prendre la première incidence
   du groupe comme owner, puis produire `kept` par transform-`copy_if`. Cela
   supprime `candidates`, `reduce_by_key`, ses deux sorties, `raw_edges` et
   `keep`; le pic explicite tombe vers `44*I + 4*S` avant le workspace de
   tri/scan.

La seconde forme réduit aussi le nombre de points de défaillance et rend le
mutant owner plus lisible.

## Blocage 3 — transaction CUDA et CMake

- Les créations, enregistrements, synchronisations et mesures des événements
  CUDA ne sont pas tous contrôlés. Plusieurs retours précoces et l'exception
  Thrust fuient les quatre événements. Employer des wrappers RAII et publier le
  résultat seulement après la dernière synchronisation réussie.
- Vérifier que le nombre de sorties de `reduce_by_key` vaut exactement `S`, si
  cette étape est conservée.
- `device_bytes` et les temps doivent distinguer allocation, H2D, émission,
  tri, réduction, D2H et replay CPU. Le chiffre live exclut H2D, D2H et DSU et
  ne peut pas être comparé directement au temps du fold CPU complet.
- `mhgp3v_faceowner_device_reject_absent` est enregistré
  inconditionnellement. Sous CUDA, une exécution saine fera donc échouer le
  CTest qui attend le code 2. Le test appartient à la branche `NOT CUDA`; la
  branche CUDA doit enregistrer le nominal et `--force-drop-edge 1` attendu en
  code 1, avec un plancher d'incidences et d'arêtes non nul.

## Portes G4 minimales

Après ces corrections, la première session G4 doit rester courte :

1. build `sm_120` et preflight Blackwell vert;
2. petites fixtures couvrant owner différent du plus petit handle, groupe
   final, clé haute à `k=6`, même signature dans plusieurs lots et
   déduplication multi-signature;
3. campagne générique et cosphère, flux CPU/device identiques arête par arête;
4. mutant `drop-edge` tué, puis mutants owner, shift de 21 bits, dernier groupe
   omis, lot de l'owner substitué et `unique` supprimé;
5. répétitions byte-à-byte, `compute-sanitizer memcheck` puis `racecheck` sur une
   petite entrée;
6. rejeu des arêtes device dans le DSU CPU et comparaison du fold, des records
   et marqueurs, sans utiliser le flux CPU pour ce second replay.

Le premier kernel produit, distinct, devrait accélérer le fallback compteur ou
les intersections de postings demandées par l'hôte. Il rend des handles
incidents réels; l'hôte conserve le DSU, le pruning dynamique et le commit
atomique du lot. Cette voie n'énumère pas toutes les `k`-faces.

## Tentative G4 et sécurité

Une tentative autorisée a utilisé exclusivement
`gcp-migration/start_and_verify.sh` sur
`devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`, cible vérifiée
`g4-standard-48`, `SPOT`, label `project=e-hgp`, action `STOP` et
`maxRunDuration=3600`. Le démarrage a été refusé par GCE avant création d'une
nouvelle génération : quota régional
`PREEMPTIBLE_NVIDIA_RTX_PRO_6000_GPUS` déjà consommé.

Le contrôle post-échec certifie la cible exacte `TERMINATED`, avec sa génération
inchangée du 8 août 2026. Une autre cible labellisée,
`ehgp-blackwell-spot-ai1a` dans `europe-west4-ai1a`, était `RUNNING`; elle a été
signalée seulement, jamais arrêtée ni utilisée. Aucun benchmark GPU n'a donc
été exécuté et aucun coût de VM nouveau n'a été engagé par cette tentative.

