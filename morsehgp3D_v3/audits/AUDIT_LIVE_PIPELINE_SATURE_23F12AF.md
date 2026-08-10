# Réception constructive — pipeline saturé de `23f12af` à `2b4801c`

Date : 10 août 2026 UTC.

Périmètre : nouveau binaire de mesure `prototype/saturated_pipeline.cpp` et
compaction de `prototype/saturated_fold.hpp`, committés et poussés à
`2b4801c2b2a7fed0e91dfc8aabed1d11998e8787`. Aucun fichier produit n'est
modifié par l'auditeur.

Empreintes du commit :

- `prototype/saturated_pipeline.cpp` SHA-256
  `d0b05653d950b2b8c50e4d798b9fbffc75a39edc1b22ed1e55205342c7ab707b`;
- `prototype/saturated_fold.hpp` SHA-256
  `bf1fdff4a72be726135b64aab6177542089fdc61abcaf71cf4b64b1444599772`;
- `CMakeLists.txt` SHA-256
  `8661ff6cdd9de1a25e508e4a01da64419b7fee7eea87ef4af872d3b5cb45f577`.

> **Correction mathématique du 10 août 2026.** L'absence de croissance de
> couverture ne signifie pas absence de continuation Gamma. La réduction exacte
> est maintenant démontrée par `q_min<=k+1`; voir
> [`NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md`](NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md).
> Le join postings développé ensuite est reçu séparément dans
> [`AUDIT_LIVE_JOIN_POSTINGS_621EE80.md`](AUDIT_LIVE_JOIN_POSTINGS_621EE80.md).

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

Le premier run `--points 200 --smax 11 --max-order 5` sur le sous-snapshot
initial `b454e5c6`/`1be6e58b` n'a produit aucune sortie en plus de 600 s; Claude
l'a ensuite arrêté.
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
pour le pipeline; ce sont les empreintes du commit final.

Cette réponse enlève bien la matérialisation `niveaux * partitions` du chemin
de profilage. Elle rend toutefois le digest courant vacuable : le pipeline
appelle le fold avec `keep_partitions=false`, puis `fold_digest` ne parcourt que
`closed_partitions`, devenues vides. Le digest ne dépend donc plus du nuage. En
attendant un flux compact canonique de deltas, il faut imprimer
`digest diagnostique=INDISPONIBLE`, ou digérer les événements réellement
conservés. Des reçus et un `flush` à la fin de chaque étage permettront aussi
d'attribuer proprement le prochain timeout.

Le second run, sur ce chemin compact committé, a lui aussi dépassé 600 s sans
sortie ni reçu; l'auditeur a terminé uniquement son PID ciblé après vérification
de la ligne de commande. Il n'existe donc aucun chrono `n=200` à créditer. Les
deux portes Gamma fold ont en revanche passé 2/2 avant le commit : elles
reçoivent la non-régression des partitions avec `keep_partitions=true`, pas le
profileur compact ni son échelle.

## Découverte constructive, puis correction : couverture et événement sont orthogonaux

Sur exactement les mêmes entrées, l'oracle Gamma rend :

| entrée | naissances Gamma | fusions Gamma | continuations Gamma | niveaux sujet étrangers |
| --- | ---: | ---: | ---: | ---: |
| générique | 31 | 20 | 87 | 41 |
| saturée | 40 | 24 | 65 | 38 |

Les naissances et fusions concordent exactement. L'excès de continuations du
fold vaut exactement le nombre de niveaux saturés étrangers : `128-87=41` et
`103-65=38`.

La première interprétation de cet écart était incomplète. Un générateur sans
croissance de couverture peut être soit une vraie continuation Gamma, soit une
activation redondante pour l'ordre. Le critère qui les sépare n'est pas le diff
de couverture mais la cardinalité minimale du support : `q_min<=k+1` porte une
face ou coface exacte; `q_min>k+1` est déjà représenté à la coupe stricte sous
source complète.

La correction est donc de séparer :

- `birth`, `continuation` et `multifusion`, décidés par les racines marquées
  avec `q_min<=k+1`;
- `coverage_growth`, payload orthogonal à ces types;
- activation redondante `q_min>k+1`, excluable de `DSU_k` lorsque la source est
  certifiée complète pour l'ordre;
- état `relative_to_certified_subfamily` lorsque cette complétude manque.

Le digest scientifique du transcript doit porter cette distinction. Les
compteurs homonymes actuels ne peuvent pas être publiés comme compteurs Gamma.

### Réception du classifieur de `2b4801c` : la couverture ne suffit pas

Le delta committé tente cette séparation par la seule variation de cardinal de
la couverture de la composante. Les deux commandes petites donnent exactement :

| entrée | naissances | fusions | croissances | dits silencieux | continuations Gamma | niveaux étrangers |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique | 31 | 20 | 0 | 128 | 87 | 41 |
| saturée | 40 | 24 | 0 | 103 | 65 | 38 |

Les identités `128=87+41` et `103=65+38` sont donc toujours visibles, mais le
nouveau compteur appelle **tous** les événements `strict==1` silencieux. Il
engloutit les continuations Gamma vraies, dont la partition et l'union de
PointId peuvent effectivement rester inchangées. Le message « JAMAIS des
continuations Gamma » et le claim du commit ne sont pas reçus.

Le critère structurel est désormais démontré : pour l'ordre `k`, un générateur
porte un événement Gamma si et seulement si `|M|>=k` et `q_min<=k+1`. Plus
fort, la sous-famille ainsi filtrée engendre exactement `Gamma_k` à toutes les
coupes lorsque la source est complète pour l'ordre. La preuve, la garde du cas
`q_min=k+1`, les supports multiples et les contre-exemples partiels sont dans la
note `q_min` liée en tête. Il reste à comparer ce nouveau transcript aux niveaux
exhaustifs avant de remplacer le compteur produit.

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

Sur les deux entrées ci-dessus, le chemin compact imprime exactement le même
`digest diagnostique=12196949897413546625`. Ce n'est pas une collision
accidentelle : avec `keep_partitions=false`, toutes les boucles d'entrée du
digest sont vides et la valeur ne dépend que de `K=3`.

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

Un reçu CPU valide existe à `n=64`, `smax=11`, `K=5`. Commande exacte :
`stdbuf -oL ./mhgp3v_saturated_pipeline --points 64 --smax 11 --max-order 5`,
depuis `build/v3`, code 0. La coordonnée résolue vaut 40 et le statut imprimé est
`partial_refinement`. Le run donne 7 873 générateurs, 62 243 membres, 35 183
niveaux, 1 490 naissances, 1 046 fusions, zéro croissance, 33 181 événements
dits silencieux, 142 125 421 appels de prédicat paire et 37 660 unions;
catalogue 2,599 s, fold 28,685 s, total 31,284 s. Il expose déjà
quantitativement le mur du join. Son digest `8387169000292456873` est la
constante associée à `K=5`, pas un reçu scientifique. Le binaire Release testé
a pour SHA-256
`4f9b231ff98c50729841ceff42a8db3077077f4641ebb072916d73e743f1de71`.

### Portes au commit et réponse live

Au commit `2b4801c`, le binaire n'avait encore aucun CTest, plancher de masse,
digest attendu ni CLI hostile. Le worktree postings pincé ultérieurement ajoute
13 portes postings/CLI, six mutants et un différentiel complet contre le fold
`G^2`; ces tests passent. Ils reçoivent le join et les compteurs internes du
fold, pas encore le prédicat `q_min` ni le transcript Gamma. Le détail positif
et la prochaine porte sont dans l'audit du join lié en tête.

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

## Verdict au commit `2b4801c`

**GO comme instrument CPU de profilage et révélateur de la différence entre
couverture et événement. NO-GO comme transcript Gamma, reçu d'exactitude,
digest scientifique ou qualification 50 k.**

Le binaire est actuellement CPU-only. Tant qu'aucun kernel du join
postings/tri-réduction n'existe et n'est différencié contre cette vérité CPU,
une G4 ne mesurerait que son hôte et ne constituerait pas une qualification GPU.

GCP non utilisé à ce snapshot.
