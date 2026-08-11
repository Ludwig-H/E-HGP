# Audit delta `cbac109` — sidecar et nouvelle source q2

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot

Commit audité : `cbac109a09c2575cdf875b19de1570265bd5bf08`.

Empreintes committées décisives :

- `prototype/validated_hybrid_sidecar.hpp` :
  `d4611eea124d80d1c4ff20a16cd73a7a40a6bb13f22e522a30adf5d921fd819c` ;
- `prototype/sealed_source.hpp` :
  `74ee9f04aa87862f33137655ea0a74498970471fbe6253ef6d1c37058c9529fe` ;
- `prototype/hybrid_fold_validated.hpp` :
  `d01dd4f86d6db7e312be681ddefec1d0f7d88c5e4103f4a62483bc4fdeba55a6` ;
- `prototype/saturated_pipeline.cpp` :
  `4989a31bdb5e20fcedc04034b5fc305ec9d6c0f6fc99b3cd3bb4b9abe4488b56`.

Le worktree ajoute en parallèle une sonde q2 non committée. Elle est auditée
ci-dessous par son empreinte propre et n'est pas confondue avec le commit.

## Verdict

Le delta ferme plusieurs mécanismes fautifs de `9483b1c`, mais le sidecar
n'est toujours pas recevable, même comme juge borné autoritaire. Quatre
contre-résultats indépendants subsistent : le reçu reste forgeable en C++20,
l'unicité des `BallKey` est fausse, une représentation hostile provoque un
overflow signé avant refus et le digest ne lie pas la décision complète.

Même après ces corrections, le pipeline hybride ne peut jamais être la route
50 k : son interface limite `smax` à 32 alors que sa fermeture exige
`smax>=n`. Il demeure un juge borné pour `n<=32`.

## Corrections effectivement présentes

### Reçu et consommation typée

`SourceProducerToken` a un constructeur privé, mais le type est vide et
trivialement copiable. `std::bit_cast<SourceProducerToken>` permet donc de le
fabriquer en C++20 défini, puis d'appeler le constructeur public de
`HybridSourceReceipt`. La clôture nominale n'est pas une clôture de type.

Les modes `hybrid` et `hybrid-prefix` transmettent maintenant
`const ValidatedHybridSidecar&` au wrapper de fold, qui consomme les points,
le catalogue et les drapeaux principaux possédés par ce sidecar.

Cette fermeture de type est un progrès réel. Elle ne prouve pas à elle seule
la complétude du producteur; le fait géométrique reste porté par
`enumeration_completed && rank_bound>=point_count` dans le reçu scellé.

### Débordement et support

La clé ne carre plus les numérateurs de centre dans `i128`. L'égalité de
niveau appelle `sphere_cmp_beta`, qui utilise les largeurs multiprécision du
prédicat exact. Le débordement du carré présent dans `9483b1c` est donc
retiré. La factory ne borne toutefois pas les champs de `Sphere` avant la
normalisation : `nx=INT128_MIN, den=1` atteint encore `-INT128_MIN` dans
`sidecar_gcd` avant tout refus.

La factory compare maintenant `Sphere.support`, `n_support`, le cardinal
minimal recalculé, la boule engendrée par le support déclaré et ses
sous-ensembles de cardinal `q-1`. Cela rejette les supports redondants,
non-générateurs et les champs incohérents couverts par les nouvelles fixtures.
Elle recopie toutefois le support déclaré dans `canonical_support` sans
reconstruire le tie-break coordonné exigé par le contrat.

### Sérialisation

Le digest ne hache plus la structure `CriticalSphere` entière; il retire le
padding et le `double beta`. Les entiers sont néanmoins donnés à FNV par leur
représentation native : l'ordre des octets reste dépendant de l'architecture.
`members_digest` fait de même. Le digest du catalogue contient bien les
supports déclarés, les membres et leur ordre. Le digest final ne sérialise
directement que les digests points/catalogue puis les états `principal`; il ne
lie pas séparément les `RemovalEvidence` calculées, `maximum_order` ou les
fermetures. FNV-1a 64 bits reste collisionnable. Byte order fixé, framing,
version de schéma, contenu complet et SHA-256 contractuel sont donc des portes
obligatoires.

## P0 — reçu frais encore forgeable

Reproduction C++20 indépendante :

1. construire un `SourceProducerToken` par `std::bit_cast` depuis un octet;
2. amputer le catalogue d'un nuage tétraédrique à un singleton;
3. recalculer les deux digests publics;
4. construire le reçu avec `rank_bound=point_count=4` et
   `enumeration_completed=true`;
5. appeler la factory.

Le binaire temporaire `/tmp/mhgp3v_fresh_receipt_attack` termine avec le code
zéro, `sidecar.ok()==true` et
`closure_certified_all_orders()==true`. Cette exécution prouve une forge
fraîche, pas un simple rejeu de digest. Le constructeur du reçu doit être privé
et possédé par le producteur; le payload scellé doit être non trivialement
copiable et idéalement consommé par déplacement.

## P1 — doublon non adjacent de boule concentrique

`ball_index_` est trié par `(centre,index_catalogue)`. La factory compare
ensuite seulement deux entrées adjacentes de même centre avec
`sphere_cmp_beta`. L'ordre des rayons à centre commun reste donc l'ordre du
catalogue, pas l'ordre des niveaux.

Contre-fixture exacte : nuage
`{(1,2,0),(3,2,0),(0,2,0),(4,2,0)}` et catalogue contenant, dans cet ordre,
la boule intérieure de rayon 1, la boule extérieure de rayon 2, puis une copie
de la boule intérieure. Les trois records sont individuellement valides et
saturés. L'ordre `[r1,r2,r1]` sépare les deux copies exactes; la factory rend
pourtant `ok=1` et un refus vide.

Reproduction indépendante :

```text
/tmp/mhgp3v_cbac109_duplicate_ball_probe
ok=1 refusal=
exit=0
```

Le binaire de reproduction est hors dépôt; la fixture minimale doit devenir
un CTest permanent. Correctif : dans chaque groupe de centre, trier aussi les
indices par `sphere_cmp_beta`, avec l'indice seulement comme dernier tie-break,
puis rejeter deux niveaux adjacents égaux. Une vraie clé canonique
centre+niveau multiprécision est l'autre solution. La fixture concentrique
positive doit continuer d'accepter `[r1,r2]`, tandis que `[r1,r2,r1]` doit
refuser.

Le champ `GeneratorCertificate.exact_ball` ne contient désormais que le
centre. Il doit être renommé `exact_center` ou complété par le niveau afin que
son nom ne promette pas une identité qu'il ne porte plus.

## P2 — représentation hostile non refusée avant arithmétique

Une sphère synthétique avec `nx=INT128_MIN` et `den=1` est extérieure au
domaine normal produit à partir de points u16, mais la factory accepte des
`Sphere` publiques et doit la refuser sans comportement indéfini. Sous
UBSan, `/tmp/mhgp3v_hostile_sphere_ubsan` signale à la ligne 257 la négation
non représentable de `INT128_MIN` et termine avec le code un. La factory doit
valider ou normaliser son ABI d'entrée avant tout PGCD; le PGCD lui-même doit
travailler sur des magnitudes non signées.

## Le pipeline hybride reste borné à 32 points

Trois faits du même binaire composent le verrou :

1. la CLI refuse `smax>mhgp::kMaxRank`, soit 32 ;
2. `HybridSourceReceipt::claims_complete_family()` exige
   `rank_bound>=point_count` ;
3. le pipeline refuse le fold si cette fermeture n'est pas certifiée.

Ainsi `hybrid` et `hybrid-prefix` ne peuvent accepter que `n<=32`. Pour un run
accepté, le pipeline construit d'abord un catalogue, puis
`SealedSourceProducer::run` rappelle séquentiellement `flat_catalogue` sur le
même nuage : l'énumération est effectuée deux fois. Le mode parallèle est
explicitement refusé.

Ce design est cohérent comme oracle borné qui recalcule son autorité. Les
commentaires et documents ne doivent plus l'appeler chemin public ou candidat
50 k. Le futur producteur streamé aura son propre domaine complet, ses
identités count/fill et un reçu qui ne dépend pas de `smax>=n`.

## Sonde q2 concurrente : périmètre d'audit

Fichier non committé audité : `prototype/pair_selfjoin_probe.cpp`, SHA-256
`ee44bc469645adcdd86dd92f25698c2ea081d820148278fc83f84726138eba0c`. Cette sonde
partitionne exactement le self-produit non ordonné d'un kd-tree médian CPU.
Pour un bloc de
paires `(A,B)`, elle cherche dix points distincts certifiés strictement dans
toutes les boules diamétrales du bloc par la borne exacte
`sup (w-x) dot (w-y)<0`. Un bloc ainsi certifié est H0-inerte pour q2; les
autres blocs sont divisés jusqu'aux microtuiles. L'identité
`pruned_pairs+microtile_pairs=C(n,2)` ferme la couverture combinatoire.

Le fichier live a depuis divergé de cette empreinte. Les constats et mesures
ci-dessous portent sur `ee44bc...`; le nouveau contenu reste non reçu tant
qu'il n'est pas stabilisé, repincé et rejoué.

Le principe de prune est exact et fail-closed. Il ne constitue encore ni une
source q2 ni une admission : les microtuiles ne calculent pas profondeur,
coquille, `BallKey` ou `BallActivation`, et la recherche de témoins peut
reparcourir l'arbre pour chaque état.

Sa portée est strictement q2. Dix points dans la boule diamétrale d'une paire
ne prouvent pas que cette paire est une mauvaise ancre d'un support q3 ou q4 :
ces points peuvent se trouver hors de la sphère plus grande dont le centre est
décalé dans le plan médiateur. Les paires prunées de cette lane ne peuvent
donc pas être retirées d'une future source d'ancres supérieures.

Fixture u16 permanente proposée : `a=(50,100,100)`, `b=(150,100,100)` et
`z=(100,160,100)`. `ab` est le diamètre du triangle aigu et son circumcentre
est `(100,655/6,100)`. Ajouter les dix témoins
`(51,95,100),(149,95,100),(51,94,100),(149,94,100),(51,93,100),`
`(149,93,100),(52,92,100),(148,92,100),(51,92,100),(149,92,100)`.
Ils sont tous strictement dans la boule diamétrale de `ab`, mais strictement
hors du cercle q3. La lane q2 doit donc pruner `ab`, tandis que toute source q3
doit conserver la même paire comme ancre du support propre `{a,b,z}`.

Deux portes manquent déjà dans le prototype initial :

- `--verify-bruteforce` compare seulement le nombre de paires non inertes au
  nombre de paires en microtuiles. Cette inégalité ne prouve pas l'inclusion :
  une paire non inerte prunée pourrait être compensée par une paire inerte
  conservée. Le rejeu doit identifier chaque paire ou chaque bloc pruné et
  certifier directement ses dix témoins.
- aucun CTest ou mutant n'est encore enregistré pour la partition du
  self-produit, le contact `dot==0`, le dixième témoin, le dernier bloc, les
  coordonnées dupliquées et le budget moins un.

La sonde est automatiquement NO-GO si la majorité des paires atteint les
microtuiles, mais ce seuil de 50 % n'est pas un budget de latence. Même 1 %
représente 12 499 750 paires à 50 k, avant census, sort et fold. L'admission
doit convertir chaque compteur en octets et mesurer le p95 chaud avec
répétitions; un p10 déjà hors budget ne sert que de réfutation.

Les premiers runs Release locaux confirment que l'implémentation n'est pas la
route chaude :

| famille | n | visites de nœuds témoins | paires en microtuiles | part | temps sonde |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain | 1 600 | 4 372 224 | 110 958 | 8,67 % | 0,450 s |
| terrain | 2 400 | 20 855 916 | 144 986 | 5,04 % | 2,066 s |

Ces reruns isolés ne sont ni G4 ni `warm_e2e`, mais la sonde dépasse déjà une
seconde à 2 400 points. L'arbre est un kd-tree médian CPU construit par
`nth_element`, pas un LBVH résident. Chaque état relance la recherche de
témoins depuis la racine. `witness_visits` ne compte pas les tests ponctuels
dans les feuilles, `frontier_max` n'est jamais renseigné et aucun compteur
d'octets/high-water n'est calculé.

Des mesures complémentaires à `n=2400`, un thread et `leaf=8`, sous charge
concurrente, donnent 4,39 % de paires résiduelles sur scanline simple et
14,15 % sur uniforme, pour respectivement 17 667 775 et 60 454 402 visites.
Le run terrain `n=50 000` n'a produit aucun résultat sous un timeout de
60 secondes. Ce timeout refuse l'ordonnanceur CPU courant pour le jalon sous
la seconde; il ne prédit pas un futur kernel G4.

## Portes demandées

1. Tuer la forge fraîche, `[r1,r2,r1]` et `INT128_MIN`, puis réauditer la
   frontière de type et l'index de boules exactes.
2. Reconstruire le tie-break du support, remplacer FNV dans toute décision de
   confiance et lier le certificat complet, pas seulement les états
   principaux.
3. Documenter le pipeline hybride comme oracle `n<=32`, avec double
   énumération, jusqu'à l'arrivée d'une vraie source streamée.
4. Rendre le différentiel q2 non compensable et ajouter ses fixtures/mutants.
5. Mesurer la sonde q2 sur les trois familles avant tout CUDA. Si les
   microtuiles ou les visites de témoins restent quadratiques, abandonner ce
   prune comme route produit et le conserver comme oracle.

GCP non utilisé.
