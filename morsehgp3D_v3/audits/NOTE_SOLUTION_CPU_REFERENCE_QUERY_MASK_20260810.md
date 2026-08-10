# Noyau CPU de référence pour le futur `query_mask`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=bounded_combinatorial_gate`, `mode=implementation_blueprint`,
`public_status=not_claimed`.

## Décision proposée à Claude

Avant un kernel GPU sparse, écrire un petit noyau CPU combinatoire qui ne
réutilise ni les postings, ni l'unranking device, ni les représentants DSU.
Il devient la vérité indépendante de chaque job, du quotient tardif et du
replay. Le premier jalon interroge tout le lot actif; il est exact relativement
au catalogue sans sidecar de complétude. Le masque sparse n'est ouvert qu'après
réception de ses certificats.

## Domaine exact d'un ordre

Pour l'ordre `k`, le domaine visible est
`V_k={a<visible_end : rank(a)>=k}`. Les générateurs de rang inférieur à `k` ne
figurent ni dans les postings, ni dans les masses, ni dans le DSU de cet ordre.
Poser `Q=query_mask` et `R=V_k sans Q`.

Le noyau valide avant tout calcul : membres triés/uniques, bornes des handles,
`Q` inclus dans le lot courant et dans `V_k`, aucun futur, arithmétique de
combinaisons vérifiée, caps de travail et arêtes fast/forest réellement au
seuil.

## API minimale

```cpp
struct ReferenceBatch {
  std::vector<std::vector<PointId>> generators_in_activation_order;
  ActivationId current_begin;
  ActivationId visible_end;
  Bitset query_mask;
  std::vector<WitnessedEdge> staged_real_edges;
  std::vector<WitnessedEdge> r_forest_edges;
  std::vector<ActivationId> frozen_component_min;
};

OwnerResult reference_owner(const ReferenceBatch&, int k, OwnerMode, Limits);
PairResult reference_pairs(const ReferenceBatch&, int k, PairMode,
                           const CoverSpecs&, Limits);
QuotientResult late_quotient(const ReferenceBatch&, const RawEdges&);
ReplayResult validate_replay_against_full_graph(const ReferenceBatch&, int k,
                                                const RawEdges&);
```

`HandleId` peut être brouillé dans les tests; seul l'indice d'activation porte
l'ordre. Les modes owner sont `GlobalMinAllQuery` et `PreferRCertified`. Les
modes de paires sont `CountDirected`, `CountMaskCanonical`, `CoverDirected` et
`CoverMaskCanonical`.

## Indépendance algorithmique

L'owner CPU énumère les combinaisons lexicographiques avec
`next_combination`, puis teste chaque générateur visible par `std::includes`.
Il ne partage ni CSR, ni clé packée, ni unranking avec CUDA.

Le count/cover calcule `std::set_intersection` directement sur les membres. Il
ne réutilise ni RLE ni postings. Pour cover, il vérifie `1<=t<=k`, `A` inclus
dans `M` et `|A|=rank(M)-k+t`, énumère les `t`-signatures de `A`, unique les
candidats, puis recalcule toujours `|M intersection N|`.

Cette indépendance fait de la porte une vraie falsification plutôt qu'une
seconde implémentation du même bug.

## Owner exact

En tout-requête, exiger `Q=V_k intersect lot_courant`. Pour chaque tâche
`(M,F)`, choisir le minimum d'activation visible contenant `F` et rendre
`(M,combination_rank,F,carrier,Q|R)`. L'étoile conserve exactement la clique de
chaque signature.

En mode `PreferRCertified`, valider d'abord que `r_forest_edges` ne contient que
de vraies arêtes `R--R` et que sa partition est exactement celle du graphe
direct induit par `R`. Ne jamais obtenir cette partition en restreignant un DSU
qui a utilisé un chemin par `Q`. L'ancre d'une signature est le plus petit
carrier dans `R` s'il existe, sinon le plus petit dans `Q`.

Le minimum global sous masque partiel est faux : une requête précoce et un fast
omis plus tardif partageant seuls une signature laissent la requête attachée à
elle-même. La préférence pour `R` rend l'arête réelle vers le fast.

## Count, cover et quotient tardif

Le mode canonique possède une paire si le candidat est hors `Q` ou si son
ActivationId est inférieur à celui de la requête. Ainsi query--ancien et
query--fast apparaissent une fois, query--query une fois par l'extrémité tardive
et `R--R` zéro fois. Le mode dirigé conserve les deux orientations
query--query.

Le seuil est décidé sur le **vrai candidat**. Ensuite seulement, grouper par
`(query,frozen_component_min[candidate])` et garder le plus petit candidat réel
ainsi que le plus petit témoin. Une projection des postings ou candidats vers
la racine avant intersection invente des voisins.

Le merge du quotient est global après tous les chunks. Un quotient local peut
rendre deux carriers de la même racine ou perdre le minimum canonique. Les
groupes déjà dans la composante scratch de la requête restent dans le reçu et
sont comptés comme pruning dynamique au replay.

## Transaction et chunks

Le snapshot `visible_end`, le masque, les labels canoniques et la forêt `R`
restent gelés pendant tout le lot. Le count ne coupe jamais les contributions
d'un même CandidateId; le cover ne coupe jamais une vérification de candidat.
Une erreur au dernier chunk jette le lot entier. Aucun cache ni label n'est
rafraîchi entre chunks.

Sorties à comparer au GPU :

- owner : exactement une réponse par tâche, self inclus avant filtrage;
- count/cover : flux trié au seuil avant quotient;
- quotient : vrais handles incidents et témoins;
- replay : partitions après chaque lot, records et marqueurs;
- reçus : domaine `V_k`, masses par mode, chunks, dernier élément et digests.

## Résultats de falsification hors dépôt

Un prototype temporaire a confronté ce contrat à des graphes directs :

- 12 000 états aléatoires all-query owner, zéro écart;
- 12 000 états aléatoires `prefer-R`, zéro écart;
- 24 000 états count dirigé/canonique, zéro écart;
- 24 000 états cover, zéro écart;
- 156 000 contrôles de découpage, zéro écart;
- univers exhaustif de trois points : 1 029 états all-query, 2 667 états
  masqués et 11 088 choix cover `(A,t)`, zéro écart;
- 80 000 contrôles supplémentaires avec domaine `V_k`, zéro écart.

Le minimum global utilisé sous masque partiel diverge dans 1 063 états de la
campagne. Ce résultat négatif reçoit la nécessité de séparer les deux modes.
La preuve combinatoire reste l'autorité; ces campagnes fixent la porte.

## Fixtures minimales permanentes

1. Trois requêtes ex æquo portant `{0,1}` : étoile owner, count dirigé deux
   orientations et canonique une.
2. Requête précoce et fast omis tardif portant seuls `{0,1}` : tue minimum
   global partiel, `N<M` et préférence pour `Q`; ajouter l'orientation inverse.
3. Plusieurs requêtes sans carrier dans `R` : ancre minimum dans `Q`.
4. Futur contenant la signature mais HandleId plus petit : doit être exclu.
5. Générateur de rang inférieur à `k` : aucune contribution à `H`, aux uniques
   cover ou au DSU.
6. Deux anciens dans une même racine dont l'union des membres contient la
   signature, mais aucun handle ne la contient : tue la projection avant
   intersection.
7. Deux sommets `R` reliés seulement par une requête `Q` : tue la fausse
   certification de `G[R]` par restriction du DSU global.
8. Deux contributions count du même candidat dans deux chunks : le RLE local
   échoue; les slabs CandidateId passent.
9. Cover avec un vrai voisin apparaissant une seule fois : tue le seuil RLE
   appliqué au cover; ajouter un faux candidat rejeté par intersection.
10. Même racine rencontrée dans deux slabs : merge global garde le vrai handle
    minimum.
11. Première/dernière combinaison, dernier candidat/chunk, `k=1`, `k=rank`,
    `Q` vide et faute au dernier chunk sans publication.

Mutants obligatoires : mauvais domaine visible, minimum global masqué,
préférence `Q`, coupure du driver à `M`, HandleId au lieu d'ActivationId, futur
inclus, owner sur une signature de taille inférieure à `k`, self omis, RLE
local, RLE cover, racine avant seuil, forêt `R` via `Q`, quotient seulement
local, labels rafraîchis, représentant DSU rendu au lieu du vrai carrier et
publication partielle.

## Ordre de réalisation

1. Graver cette porte CPU combinatoire.
2. Implémenter le GPU **tout-requête** et le comparer réponse par réponse; ce
   mode ne dépend pas encore du sidecar pour sa connexité relative.
3. Ajouter count/cover avec masses propres au mode et quotient tardif.
4. Ouvrir le masque sparse seulement derrière le sidecar et le certificat
   `R`; un certificat absent élargit `Q`, il ne refuse pas un calcul relatif.
5. Mesurer ensuite le dispatcher; aucun taux de cache ne décide l'admission.

Snapshot observé : `HEAD=origin/main=37139de2329c32797815db3fa73130a2e80aeda3`.
Aucun backend `query_mask/prefer-R/count_mask/cover_mask` n'existe encore dans
le prototype ou CMake à ce snapshot.

