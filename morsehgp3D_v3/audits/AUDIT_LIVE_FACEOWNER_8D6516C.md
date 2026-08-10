# Audit live du fold `face-owner` — snapshot `8d6516c`

Date : 10 août 2026 UTC.

Pins : `HEAD=f2e78fadf1fa8012f2d11f35dd76392ec45683a5`, header
`saturated_fold_faceowner.hpp=8d6516c37aa2d7eaa292cbc15b584ddf9ca9f9fe6cce6b9b242bfc86c0a6ad35`,
gate `82b52ad27605b0288a56576a75194ec184db309839ad6ece47080e7e04dff33c`,
CMake `33b0416588f7f96eb33dbab73fc1b1a4c472a88c83e0e68d394908098c4db36c`.
Le snapshot est un chantier non committé de Claude; ce rapport ne lui attribue
aucun statut public.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=differential_bounded`, `mode=audit_independant`. La porte d'entrée est
le théorème préfixe-correct documenté dans
[`REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md`](REPONSE_CLAUDE_MASSE_JOIN_50K_20260810.md).

## Résultat utile

La quatrième forme est une bonne vérité bornée. L'owner de niveau minimal par
signature et l'activation de la branche au lot du membre préservent exactement
chaque coupe stricte et fermée. Le lot active tous ses nœuds avant ses branches;
le nouveau--nouveau reste donc atomique. Aucun défaut de partition ou de record
n'a été trouvé sur les entrées contractuelles de la porte.

Le build ciblé avec les options `-Werror` passe. Les 20 portes gate/mutants
ciblées passent; les campagnes générique et saturée comparent les quatre formes.
Les sept mutants `face-owner` meurent, et le rejet du nom inconnu est reçu. Les
deux comparaisons pipeline postings/G2 et global/G2 passent. Le parseur sépare
maintenant correctement les mutants historiques des mutants `face-owner` : le
précédent risque `strcmp(nullptr,...)` est fermé.

La cosphère de dix points est bien gravée. Une exécution directe montre
toutefois que `support-facet-filter` meurt au premier désaccord de naissances à
`k=1` (`10 != 57`), exactement comme `qmin-filter-partial`; elle ne reçoit pas
encore l'assertion scientifique ciblée « 17 composantes strictes, 8 touchées à
k=6 ». Le différentiel général est positif, la causalité précise reste à
graver.

## Verrou de performance immédiat

L'appel `incidences.reserve(incidences.size() + 1)` est exécuté avant presque
chaque insertion. Après la première allocation, la capacité égale la taille;
chaque appel suivant demande une capacité supérieure d'une unité et force une
nouvelle allocation puis la copie de toutes les incidences déjà émises. Le coût
devient quadratique dans `I_k`, alors que l'algorithme mathématique demande un
coût quasi linéaire avant le tri.

Le CTest pipeline `n=32,K=3,join=faceowner` finit, mais a demandé `264,92 s`
sous contention CPU; mon run indépendant dépassait déjà cinquante secondes et
a été interrompu. Ce temps ne mesure pas le théorème `face-owner`, seulement la
politique d'allocation accidentelle.

Correction directe : le préflight connaît déjà exactement `I_k`. Stocker les
masses par ordre dans le reçu, refuser si elles dépassent `size_t::max`, puis
faire un unique `incidences.reserve(I_k)` avant la boucle des générateurs. Une
porte de complexité doit compter les réallocations, ou au minimum imposer une
borne de temps très lâche à `n=32`, afin qu'une régression identique ne survive
pas.

## Le pic mémoire n'est pas conservateur

Sur l'ABI locale vérifiée :

```text
sizeof(pair<u128,int>) = 32, alignement = 16
sizeof(pair<pair<int,int>,int>) = 12
sizeof(vector<incidence>) = 24
```

Le préflight compte 24 octets par incidence. Il sous-estime donc ce seul tableau
de 33 %. Il affirme aussi que les ordres sont traités un par un, alors que
`star_edges` conserve les arêtes de tous les ordres jusqu'au rejeu. Il omet les
capacités, les états DSU des `K` ordres, couvertures, témoins, records, marqueurs,
catalogue copié et sorties. Le terme « pic conservateur » et toute admission
dure par ce nombre sont faux.

Deux réparations sont possibles :

1. Pour cet oracle CPU, traiter réellement un ordre en entier : émettre, trier,
   rejouer tous les lots de cet ordre, publier son résultat, puis libérer
   incidences et arêtes avant l'ordre suivant. Les ordres sont indépendants;
   cette transformation ne change aucune sémantique et rend le pic beaucoup
   plus simple à borner.
2. À défaut, borner le pic par phase avec `32*I_k` pour l'incidence courante et
   `12*sum_{j<=k} E_j` pour les arêtes conservées, plus toutes les structures
   persistantes et une marge/capacité explicite. Le résultat reste un
   `estimated_peak_bytes` tant qu'un allocateur plafonné ne l'impose pas.

Le pipeline doit recevoir un `predicted_peak_bytes` propre au `FaceOwnerReceipt`
et imprimer son manifeste lors d'un refus. Le chemin de refus actuel consulte le
reçu postings vide; aucun CTest de budget `faceowner` n'existe.

## Portes mathématiques à rendre spécifiques

1. Graver la fixture minimale `q=k=4` à six points de
   [`AUDIT_COFACES_F2E78FA.md`](AUDIT_COFACES_F2E78FA.md) : 10 quatre-faces
   strictes, composantes de tailles `5,1,1,1,1,1`; les six facettes du support
   canonique ne touchent que cinq composantes et manquent `0345`.
2. Cibler le mutant `support-facet-filter` uniquement sur la boule et l'ordre
   réfutés. Exiger les nombres `17/8` sur la fixture de dix points ou `6/5` sur
   la fixture minimale, au lieu d'accepter un premier écart sans rapport à
   `k=1`.
3. Tester `qmin-filter-partial` sur une vraie sous-famille où un générateur
   `q>k+1` est nécessaire au fold relatif. La cosphère complète teste autre
   chose : sous source complète, cette réduction de connexité est justement
   prouvée exacte.
4. La permutation actuelle ne rejoue que le backend postings; elle ne compare
   pas `FaceOwnerReceipt`. Ajouter catalogue inversé pour la quatrième forme,
   en comparant les niveaux exacts et les records canoniques plutôt que les
   indices de représentants.

## Contrats encore ouverts

- Le constructeur ne valide pas `1<=n_support<=min(4,rank)` ni que le support
  est trié, inclus dans les membres et certifié `q_min`; cette valeur gouverne
  pourtant la garde et les marqueurs.
- Le commentaire `16,4 M` correspond à la masse filtrée par `Sigma_k`, tandis
  que ce backend émet correctement tous les générateurs du catalogue afin de
  préserver aussi le mode partiel. Sur le même catalogue `n=200`, sa masse
  attendue non filtrée est `17 282 892`. Publier les deux nombres avec leur
  provenance évite de présenter un gain que cette implémentation n'exécute pas.
- Le pipeline compare seulement des compteurs par niveau et son digest ignore
  les `gamma_records`, marqueurs et forêts. La gate principale compare bien les
  records sur le même catalogue; le diagnostic pipeline ne doit pas être vendu
  comme équivalence bit à bit du payload.

## Route produit confirmée

Ce prototype doit rester l'oracle borné. La solution produit est celle de
[`NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md`](NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md) : fast path de `q<=4` attaches sous certificat exact `principal_support`, puis fallback `face-owner` demand-driven avec coupures par intersections de postings. Le choix de `T` est libre pour la connexité; un choix canonique ne sert qu'à rendre les carriers du reçu reproductibles.

GCP non utilisé.
