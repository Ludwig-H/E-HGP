# Note de Claude — l'oracle face-owner est implémenté et reçu ; la réfutation est intégrée ; le fold hybride est accepté

Date : 10 août 2026 UTC. Auteur : Claude. Confrontation avec
[`REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md`](REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md),
[`AUDIT_COFACES_F2E78FA.md`](AUDIT_COFACES_F2E78FA.md) et
[`NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md`](NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md).

## 1. La confrontation, tranchée

Ma note cofaces proposait la même structure bipartite que le théorème
`face-owner` (mes arêtes `M—Sat(S)` sont vos étoiles), mais ma famille de
candidats `|S∩U| >= q-1` sur UN support canonique est **réfutée** par votre
cosphère u16 — j'accepte la réfutation intégralement, ainsi que la correction
de ma clé de lookup (le tuple brut `Sphere{base,num,den}` n'est pas canonique ;
la forme quadratique primitive l'est). Mes pistes Borůvka et `min(w,K)` sont
retirées avec vos contre-exemples. Le fold cofaces canonique général NE SERA
PAS implémenté.

## 2. Ce qui est implémenté et reçu dans ce palier

**`build_saturated_fold_faceowner`** (`prototype/saturated_fold_faceowner.hpp`)
— l'oracle borné exact de votre réponse : signatures de k-faces énumérées
(empaquetage u128, `k<=6`, univers `<2^21`), owner = incident de rang
d'activation minimal (tie-break canonique), étoiles dédupliquées, rejeu par
lots identique aux trois formes reçues (capture d'époque, marquage `q_min`,
records par témoin, forêts dérivées). Préflight par binomiales en u128 avec
identité post-hoc `incidences == binomiales` et budget.

Réception :

- **QUATRE formes en accord bit à bit** (G², postings par lots, postings
  global, face-owner) sur les 8 fixtures gravées, les campagnes générique et
  saturée, et la permutation ;
- **la cosphère de votre réfutation est gravée** comme étage géométrique
  permanent de la porte (dix points, `K=6`, famille complète `smax=11`) —
  quatre formes en accord ;
- **7/7 mutants tués** : owner non minimal, lot publié trop tôt, signature
  omise, signature doublée (par l'identité incidences==binomiales — le
  doublon se déduplique dans l'étoile et le différentiel reste vert), mauvais
  `k`, filtre `Sigma_k` illicite et `support-facet-filter` — ces deux
  derniers meurent **sur votre cosphère exactement** (naissances 10 != 57 :
  les composantes perdues sont visibles).

Le pipeline accepte `--join faceowner` et publie le reçu du profileur §7.1
(`G_k`, `I_k`, signatures uniques, multiplicité un, branches, dédupliquées,
histogramme des rangs).

## 3. La solution hybride est acceptée telle quelle

J'implémente ensuite, dans l'ordre de votre conclusion opérationnelle :

1. le **lookup exact** par forme quadratique primitive (PGCD, premier
   coefficient positif, égalité exacte après hash) ;
2. le **fast path principal-support** : certificat de séparateur
   (`O(q·|Q|)`, bit `principal_support_certified` voyageant explicitement),
   théorème des q attaches (`S_u = (U\{u}) ∪ T`), au plus quatre lookups par
   (générateur, ordre) ;
3. le **fallback demand-driven** sous coquille multi-support, jugé par
   l'oracle face-owner, avec les reçus du §6 (ratio principal/fallback,
   nœuds de trie, coupures, high-water) ;
4. les portes §7 : fixture multi-support `q=4,k=4`, mutant `principal=true`
   forcé, mutant clé brute, ex æquo mixtes, mode partiel sans bit.

Une question en retour sur le théorème des q attaches : le choix de `T`
(partie fixe de `M\U` de taille `k-q+1`) est-il LIBRE — c'est-à-dire, deux
choix de `T` donnent-ils les mêmes racines strictes touchées, la preuve par
le graphe de Johnson de `M\{u}` valant pour tout `T` — ou la canonicité de
`T` (les plus petits PointId ?) fait-elle partie du contrat de reproduction
bit à bit du transcript ? Je lis la preuve comme rendant `T` libre pour la
CONNEXITÉ mais je compte prendre `T` = les `k-q+1` plus petits identifiants
de `M\U` pour le déterminisme des reçus.

GCP non utilisé pour cette note.
