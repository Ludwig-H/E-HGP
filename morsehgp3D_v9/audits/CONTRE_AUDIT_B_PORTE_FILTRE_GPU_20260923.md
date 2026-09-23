# Port du filtre témoin GPU : porte utile, pas encore budget de tour

23 septembre 2026. Contre-audit B en lecture seule, cadre
`exploration_v9_hors_registre`, `quantized_u18_input_only`,
`public_status=not_claimed`. Aucun GCP lancé ici. Sources :
[R11 G4 CPU](../receipts/g4_tower_r11_20260923/README.md), son
[`SUMMARY.json`](../receipts/g4_tower_r11_20260923/SUMMARY.json), le
[profil local des micro-leviers](../receipts/q34_micro_levers_20260923/README.md),
et le chantier GPU **non commité** lu dans `build/v9-open-worktree`.

## Budget séquentiel observé

Les meilleurs essais `q2_jobs_by_mass=ON` de R11 sont des tours CPU G4
sur des trames **sans sol**, toutes de la séquence 08, grille 1 mm,
s8/W48, `complete_relative`. On soustrait seulement la durée q3/q4 de
`chain_s` ; le reliquat contient notamment q2, census, FULL et autres
coûts de chaîne. Il n'est pas une prédiction de vitesse du GPU.

| Trame | K | Chaîne (s) | q3/q4 (s) | Chaîne − q3/q4 (s) |
| --- | ---: | ---: | ---: | ---: |
| 08/000100 | 5 | 2,537 | 1,663 | 0,874 |
| 08/000000 | 5 | 3,210 | 2,148 | **1,062** |
| 08/000200 | 5 | 3,517 | 2,411 | **1,106** |
| 08/000100 | 10 | 7,682 | 4,447 | **3,235** |
| 08/000000 | 10 | 10,364 | 6,246 | **4,118** |
| 08/000200 | 10 | 10,526 | 6,578 | **3,948** |

Ainsi, **si l'expérience ne remplace que le filtre et conserve le reste
des phases et leur séquentialité**, elle ne peut suffire à la seconde :
sur deux trames K5 le reliquat dépasse déjà 1 s même en annulant tout
q3/q4 ; à K10 le reliquat dépasse 3 s partout. Ce n'est pas une borne
d'impossibilité pour une architecture différente qui superposerait ou
refondrait ces phases. Le contrat final concerne de surcroît les
**trames brutes entières**, plusieurs séquences et la tour complète, pas
ces seules trames sans sol.

La porte annoncée par D — tous les masques de filtre rectangles/paires
de 08/000000/K5 sans cache en au plus 0,1 s, transferts inclus — est un
bon test de viabilité **du noyau isolé**. Le profil local K5 donne
15,8 % du CPU q3/q4 au DFS des paires, 12,0 % à celui des rectangles et
5,2 % à l'ordre des enfants appelé par ces DFS : environ 33 % pour ce
chemin, sous l'hypothèse que l'ordre appartient intégralement aux deux
filtres. Les 40 % de « filtrage » cités dans la coordination incluent
notamment le front WSPD (5,0 %), qui n'est pas dans le port annoncé.
Le temps CPU local
ne se convertit pas mécaniquement en temps mur G4. Même un passage de cette
porte impose donc de mesurer une chaîne intégrée appariée, et son échec
seul n'exclurait pas un filtre mieux groupé/à index plus cohérent.

Le ledger de R11 pour 08/000000/K5 compte **3 133 819 requêtes
rectangles** et **23 686 751 paires développées** : la population
« sans cache » annoncée représente donc 26 820 570 masques, soit
**268 millions de masques/s** pour 0,1 s, transferts inclus. Le cache
actif de la chaîne n'effectue que **7 162 322 recherches de paires** ;
il écarte les **16 524 429** autres. Ainsi la population sans cache
évalue un débit maximal volontairement défavorable au GPU, et son
résultat ne se convertit pas directement en accélération face au CPU
**cache ON**. Les comptes proviennent de
[`vm/probe_0.stdout`](../receipts/g4_tower_r11_20260923/vm/probe_0.stdout)
(`ledger.witness_rect_queries`, `expanded_pairs`,
`witness_pair_queries`, `witness_cache_rejected_pairs`).

## Port encore exploratoire au 23 septembre

Dans la première lecture du chantier, `src/gpu/witness_filter.hpp` transpose les bornes
entières, crédits stricts et exclusions du filtre CPU dans une fonction
hôte/device ; aucune divergence arithmétique concrète n'a été trouvée
sous les préconditions u18 et index certifié. Mais
`src/gpu/filter_runner.hpp` n'avait **pas encore d'implémentation CUDA** :
ni noyau, ni scan, ni transferts, ni temps G4, ni masques GPU publiés.
La porte hôte `tests/gpu/witness_filter_port_gate.cpp` compare des
rectangles synthétiques et des paires échantillonnées, sans construire
`FilterInput` ni exercer le rangement rang→coordonnée ou la compaction.

Points à fermer avant tout statut GPU :

1. Le futur lanceur doit valider `node_count>0`, les pointeurs, `K∈[1,10]`,
   masques, IDs de nœuds, plages/rangs et la durée de vie des buffers :
   `filter()` lui-même lit `nodes[0]` sous des préconditions non vérifiées.
   Faire remonter `stack_failure=0xff` en refus, jamais comme masque.
2. Comparer **chaque** masque device au CPU sur les mêmes requêtes, puis
   comparer les candidats restants, le catalogue exact et le condensé
   FULL d'une chaîne intégrée. Le cache CPU existant évite déjà de
   nombreuses recherches de paires ; le coût de la population sans cache
   ne représente pas automatiquement le gain intégrable.
3. Publier séparément préparation/index, upload, rectangles, scan,
   paires, download, synchronisations, pic mémoire, occupation et temps
   de chaîne. Le meilleur des répétitions à contexte chaud et le premier
   passage froid doivent rester distincts ; la porte 0,1 s annoncée
   inclut les transferts et toute la population. Le chronomètre utile
   va de la **production des requêtes** à la **consommation des réponses**,
   pas seulement d'un lot déjà matérialisé à un autre ; comparer au CPU
   cache ON et OFF sur les mêmes requêtes.
4. Ne pas matérialiser sans borne une sortie `pair_masks` par paire : elle
   coûte au moins `P` octets sur l'hôte **et** sur l'appareil si les deux
   copies coexistent, où `P` est le nombre de paires développées et peut
   être quadratique. Le `FlatNode` copié vaut 40 octets ; un arbre binaire
   d'environ `2n` nœuds coûte déjà environ `80n` octets (≈4 Go à 50 M
   sites), plus `12n` octets de coordonnées ordonnées (≈0,6 Go), hors
   index CPU, arènes et masques. Pour les dizaines de millions, prévoir
   des tuiles/batches bornés et des offsets 64 bits avec refus explicite
   de débordement ; mesurer le pic réel avant promesse de résidence.

Verdict : porte de fidélité hôte utile ; **backend GPU et contrat non
qualifiés**. La priorité d'architecture reste la réduction du travail
q3/q4 et la parallélisation de l'aval, pas une interprétation du seul
filtre comme solution de tour.

## Relecture du nouveau lanceur CUDA mutable, vers 13 h 05 UTC

Le développeur a depuis ajouté `src/gpu/filter_runner.cu`, un stub et
`bench/gpu_filter_probe.cpp` dans son **worktree non commité**. La sonde
compare maintenant, par conception, tous les masques du GPU aux masques
CPU avec/sans cache et les nombres de visites. Elle n'a toujours aucun
reçu d'exécution CUDA ; CMake n'enregistre qu'un CTest positif du port
**hôte**, plus un refus d'argument du probe. Cette section est un
préflight du WIP, non une qualification publiée.

- `run_filters` ne valide pas `FilterInput` : `rect_a/b` peuvent indexer
  hors `nodes`, un rang peut dépasser `rank_points`, et `K=0` invalide les
  seuils. `DeviceBuffer::allocate` multiplie `count*sizeof(T)` sans test ;
  `3*rank_count` et la conversion de `rect_count` vers `int` pour CUB
  ne sont pas bornées. Un préflight hôte doit refuser ces cas **avant**
  un lancement device ou une allocation.
- Le scan des masses et la recherche du dernier offset de rectangle
  sont cohérents en lecture statique, y compris les rectangles vides.
  Leur coût est toutefois à mesurer : `pair_kernel` effectue une
  recherche binaire indépendante dans environ 3,13 M offsets pour
  **chacune** des 23,69 M paires de R11/000000/K5, soit environ
  0,5 milliard d'itérations de recherche avant les DFS. Un découpage
  par tuiles de rectangles/paires éviterait ce terme `P log R` et
  bornerait les buffers sans changer les masques.
- Les événements couvrent bien les copies H2D/D2H **de chaque passe**,
  le scan et les noyaux, mais `total_ms` conserve la meilleure passe
  chaude après création du contexte et allocations initiales ;
  `first_total_ms` garde le premier passage. Index/front, construction
  de requêtes, copies CPU de référence, intégration catalogue et FULL
  sont hors de ces durées. Publier le premier passage, la passe chaude,
  le mur complet du probe et le mur de chaîne distinctement.
- Des événements CUDA créés avant une erreur ne sont détruits qu'au
  succès ; corriger cette fuite de ressource. Le coût et la compilation
  du `__int128` device restent à constater sur G4, sans conclusion
  avant le build et un test positif.
