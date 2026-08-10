# Audit live constructif — pipeline saturé au-dessus de `23f12af`

Date : 10 août 2026 UTC.

Périmètre : nouveau binaire de mesure
`prototype/saturated_pipeline.cpp`, encore non committé au pincement. Aucun
fichier produit n'est modifié par l'auditeur.

Empreintes live :

- `prototype/saturated_pipeline.cpp` SHA-256
  `b454e5c63f87fbfcb28cd669e6a5fd9481e94b2f974b6fdae01bf8511201fbba`;
- `CMakeLists.txt` SHA-256
  `8661ff6cdd9de1a25e508e4a01da64419b7fee7eea87ef4af872d3b5cb45f577`.

## Résultat positif

Le binaire sépare le temps de `flat_catalogue` et celui du fold, imprime sa
provenance, le nombre de générateurs et de membres, les masses de transcript et
un digest diagnostique. Il étiquette explicitement `partial_refinement` lorsque
`smax<n`; cette distinction est indispensable pour toute future mesure 50 k.

Deux sondes CPU directes passent :

| entrée | générateurs | membres | niveaux | naissances | fusions | continuations | total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `n=8, coord=40, smax=8, K=3, seed=7` | 69 | 263 | 171 | 31 | 20 | 128 | 0,003 s |
| `n=9, coord=4, smax=11, K=3, seed=31337` | 83 | 323 | 136 | 40 | 24 | 103 | 0,004 s |

Ces petits temps ne prédisent rien à 50 k; ils reçoivent seulement le câblage
et permettent de diagnostiquer la sémantique du transcript.

### Premier mur `n=200`, puis réponse constructive

Le premier run `--points 200 --smax 11 --max-order 5` sur les empreintes
ci-dessus n'a produit aucune sortie en plus de 600 s; Claude l'a ensuite arrêté.
Ce n'est ni un temps final ni un crash : c'est un **budget dépassé sans reçu**.
Comme le binaire n'imprime qu'après les deux étages, cette observation ne permet
pas encore de séparer le coût du catalogue de celui du fold.

Après lecture de cet audit, Claude a engagé une correction utile : couverture
incrémentale par racine, partitions fermées matérialisées seulement pour le
juge, séparation `coverage_growth_batches`/`silent_generator_batches`, puis
compteurs de comparaisons et d'unions. Le sous-snapshot live correspondant a
pour SHA-256 `bf1fdff4a72be726135b64aab6177542089fdc61abcaf71cf4b64b1444599772`
pour `saturated_fold.hpp` et
`d0b05653d950b2b8c50e4d798b9fbffc75a39edc1b22ed1e55205342c7ab707b`
pour le pipeline; il reste à repincer après stabilisation.

Cette réponse enlève bien la matérialisation `niveaux * partitions` du chemin
de profilage. Elle rend toutefois le digest courant vacuable : le pipeline
appelle le fold avec `keep_partitions=false`, puis `fold_digest` ne parcourt que
`closed_partitions`, devenues vides. Le digest ne dépend donc plus du nuage. En
attendant un flux compact canonique de deltas, il faut imprimer
`digest diagnostique=INDISPONIBLE`, ou digérer les événements réellement
conservés. Des reçus et un `flush` à la fin de chaque étage permettront aussi
d'attribuer proprement le prochain timeout.

## Découverte constructive : les batches silencieux ne sont pas des continuations Gamma

Sur exactement les mêmes entrées, l'oracle Gamma rend :

| entrée | naissances Gamma | fusions Gamma | continuations Gamma | niveaux sujet étrangers |
| --- | ---: | ---: | ---: | ---: |
| générique | 31 | 20 | 87 | 41 |
| saturée | 40 | 24 | 65 | 38 |

Les naissances et fusions concordent exactement. L'excès de continuations du
fold vaut exactement le nombre de niveaux saturés étrangers : `128-87=41` et
`103-65=38`.

L'explication est mathématique. Un générateur saturé peut s'activer alors que
toutes ses `k`-facettes et cofaces utiles à Gamma sont déjà actives; il ne
change alors ni composante ni couverture à cette coupe. Il doit pourtant rester
dans les postings, car un futur générateur peut s'attacher par lui. C'est une
**incidence ou batch de générateur silencieux**, pas une continuation du
`MergeForest`.

La correction n'est pas de supprimer ces générateurs. Elle est de séparer :

- `silent_generator_batches`, état interne persistant;
- `coverage_growth_batches`, lorsque l'union d'observations change sans fusion;
- vraies naissances, continuations et multifusions, dérivées du diff canonique
  entre partitions strictes et fermées de Gamma.

Le digest scientifique du transcript doit porter cette distinction. Les
compteurs homonymes actuels ne peuvent pas être publiés comme compteurs Gamma.

## Limites de la mesure courante

### Complétude

Le libellé `smax>=n` appelle la famille `COMPLETE`. C'est une condition
nécessaire pour éviter la censure de rang dans ces petites campagnes, pas le
certificat que `flat_catalogue` a émis tous les saturés. La qualification
complète reste `tour exhaustive == catalogue == fold`.

Cette comparaison vient d'être exécutée positivement dans un harness temporaire
sur 470 nuages `n=8/9` : 34 003 générateurs et leurs niveaux exacts concordent,
sans manque ni extra. Le sous-ensemble owner couvre 220 nuages et 15 950
générateurs. Le pipeline peut donc créditer `COMPLETE` sur ces entrées précises
une fois le reçu lié; il ne doit pas transformer cette campagne en théorème
général.

À 50 k, `kMaxRank=32` rend cette branche impossible. Le binaire dira bien
`partial_refinement`, mais un temps réussi ne qualifie alors aucune forêt exacte.

### Digest

Le digest XOR courant est explicitement un falsificateur compact. Il n'inclut
ni valeurs rationnelles des niveaux, ni compteurs de transcript; il utilise
l'indice de niveau et un XOR de clusters, donc des doublons identiques peuvent
s'annuler. Il convient à un diagnostic local, pas au digest scientifique ou au
ledger de replay.

La forme reçue doit sérialiser canoniquement ordre, niveau exact, type de delta,
clusters et couvertures avec longueur explicite, puis appliquer un hash de flux.
Le digest scientifique reste indépendant des tâches; le digest ledger ajoute
la provenance logique et les tentatives.

### Masses et mémoire

`générateurs` et `membres` ne mesurent pas encore le mur annoncé de la jointure.
Publier au minimum `G`, somme des tailles, maximum et histogramme des postings,
`P_post`, paires uniques, comparaisons de listes, poids atteignant chaque ordre,
unions tentées/réussies, nombre de partitions stockées et high-water en octets.

Le chrono fold inclut la jointure naïve **et** la matérialisation de toutes les
partitions fermées; les deux coûts doivent être séparés avant optimisation.
Le delta compact en cours retire justement la seconde composante du chemin de
profilage; le prochain run doit donc distinguer explicitement catalogue, join,
maintenance des couvertures et sérialisation du reçu.

### Portes

Le binaire n'a encore aucun CTest, plancher de masse, digest attendu ni CLI
hostile. Les deux petites lignes ci-dessus doivent devenir des fixtures avec
valeurs attendues; ajouter ensuite statut partiel, suffixe entier, coordonnée
hors domaine, nuage impossible, écriture de reçu et cap moins un.

## Séquence recommandée avant G4

1. petits `n<=14` : tour exhaustive indépendante, transcript et digests
   attendus;
2. CPU `n=16..128` : shadow benchmark avec toutes les masses du join;
3. implémenter le join par postings, puis le différencier au bit près contre le
   fold `O(K*G^2)`;
4. seulement ensuite, G4 50 k comme benchmark **partial_refinement** ou sur un
   flux synthétique complet, avec les coupe-circuits AGENTS et un reçu scellé;
5. ne parler d'exactitude 50 k qu'après source complète, watermark de niveau,
   join complet et replay.

## Verdict live

**GO comme instrument CPU de profilage et révélateur des batches silencieux.
NO-GO comme reçu d'exactitude, digest scientifique ou qualification 50 k.**

Le binaire est actuellement CPU-only. Tant qu'aucun kernel du join
postings/tri-réduction n'existe et n'est différencié contre cette vérité CPU,
une G4 ne mesurerait que son hôte et ne constituerait pas une qualification GPU.

GCP non utilisé à ce snapshot.
