# Audit du reçu G4 de masse résiduelle — `8f2ad6d`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin audité : `HEAD=8f2ad6d6cbc4a6703b0a2f064ed453b3d1c32df0`,
commit `measure the residual on G4, and find that coverage and slope go
opposite ways`.

Artefacts versionnés :

- `receipts/residuel_dominance_g4_20260813/README.md=7be71095...` ;
- `receipts/residuel_dominance_g4_20260813/residuel_brut.txt=40c8cb08...` ;
- `gcp-migration/session_residuel_dominance_g4.sh=9cddf21d...` ;
- `directional_dominance_probe.cpp=4116e788...` ;
- `directional_dominance.hpp=2e33685d...` ;
- `cloud_families.hpp=b7c2f961...` ;
- `CMakeLists.txt=676331c0...`.

## Verdict

Les douze comptes, les identités de masse, les pourcentages et les pentes sont
arithmétiquement corrects. Ils établissent un diagnostic utile : comme
**sparsifieur terminal de `PairId` q4**, la dominance est rouge sur trois des
quatre familles mesurées. Ils ne mesurent ni le travail ni la taille d'un front
factorisé et ne le refusent donc pas.

Trois corrections sont nécessaires :

1. La phrase « la famille où le certificat ferme le moins a la meilleure
   pente » est fausse : `eight_clusters` ferme le moins, tandis que `uniform`
   a la meilleure pente. Les quatre séries ne prouvent aucune loi inverse
   structurelle entre couverture et pente.
2. Le besoin « environ 150 paires par record, soit `12 x 12` » n'est pas
   déduit du reçu. Il suppose un front linéaire, oublie le coefficient de la
   masse et choisit une seule famille. Une équation de gate exacte le remplace
   ci-dessous.
3. Le reçu est incomplet comme preuve reproductible et son script possède deux
   défauts de contrôle : `pipefail` manque dans les pipelines distants, et un
   échec de l'arrêt ciblé est masqué par `|| true`. L'exécution réelle a bien
   compilé CUDA et la VM est bien arrêtée, mais les témoins versionnés ne
   suffisent pas à le prouver seuls.

Le statut reste `NO-GO` G4 et aucun SLO n'est qualifié.

## 1. Ce qui est effectivement mesuré

Pour chaque famille, un seul seed (`3`) et une seule exécution à chacune des
tailles `12 500/25 000/50 000` calculent le nombre de `PairId` q4 non ordonnés
qui ne sont fermés dans aucune des deux orientations. Le probe parcourt encore
`n(n-1)` relations et conserve trois bitsets triangulaires ; la machine G4 ne
sert ici qu'à exécuter douze processus CPU mono-thread en parallèle.

Il n'y a :

- ni kernel mesuré, ni temps, ni warmup, ni répétition temporelle, ni p95 ;
- ni `node_visits`, ni records de front, ni octets, ni high-water ;
- ni source résiduelle, owner, RLE, census, fold ou payload ;
- ni oracle sur les douze grandes exécutions.

Le compte est déterministe pour les octets et le seed annoncés ; une répétition
temporelle n'est pas requise pour vérifier son arithmétique. Elle serait requise
pour tout claim de latence, absent ici.

## 2. Recalcul indépendant

Pour chaque ligne, `residual+closed=C(n,2)` est exact. Les pentes non arrondies
sont :

| famille | pente `12,5 -> 25 k` | pente `25 -> 50 k` | fraction fermée à `50 k` |
| --- | ---: | ---: | ---: |
| `uniform` | `1,294171907` | `1,220345340` | `84,203177 %` |
| `eight_clusters` | `1,545278888` | `1,470041828` | `69,384354 %` |
| `terrain` | `1,447441881` | `1,473280523` | `92,442326 %` |
| `scanline_overlap_multiecho` | `1,606752204` | `1,567559567` | `86,013745 %` |

Le seuil `1,35` refuse donc bien le seul objet « catalogue des `PairId`
résiduels » sur `eight_clusters`, `terrain` et `scanline_overlap_multiecho`.
`uniform` est vert pour ce seul compteur sémantique.

## 3. Couverture et pente : l'identité correcte

Poser `T_n=C(n,2)`, `R_n` la masse résiduelle et `r_n=R_n/T_n` sa fraction.
Sur un doublement :

$$e_R(n,2n)=\log_2\left(\frac{R_{2n}}{R_n}\right)=\log_2\left(\frac{T_{2n}}{T_n}\right)+\log_2\left(\frac{r_{2n}}{r_n}\right).$$

Le premier terme vaut exactement
`log2(2(2n-1)/(n-1))`, très près de deux. La pente du résiduel mesure donc la
**vitesse de décroissance de la fraction résiduelle**, et non son niveau.

Les ratios `r_2n/r_n` observés sont :

| famille | `12,5 -> 25 k` | `25 -> 50 k` |
| --- | ---: | ---: |
| `uniform` | `0,613066` | `0,582495` |
| `eight_clusters` | `0,729622` | `0,692561` |
| `terrain` | `0,681783` | `0,694117` |
| `scanline_overlap_multiecho` | `0,761383` | `0,740992` |

Une famille peut avoir une petite fraction résiduelle mais la réduire lentement,
ou l'inverse. Les données présentes montrent que couverture et pente ne sont
pas interchangeables ; elles ne prouvent ni anticorrélation générale, ni
« saturation » causale. En particulier, `terrain` ferme davantage que
`scanline_overlap_multiecho` et possède pourtant une meilleure pente.

## 4. Cahier des charges exact du front factorisé

Poser `F_n` le nombre de records physiques et `a_n=R_n/F_n` leur masse moyenne.
Alors, sans hypothèse asymptotique :

$$e_F=e_R-e_a,\qquad e_a=\log_2\left(\frac{a_{2n}}{a_n}\right).$$

Pour satisfaire `e_F<=1,35`, la factorisation moyenne doit croître au moins
par le facteur `2^(e_R-1,35)` lorsque `e_R>1,35`. Le reçu donne donc la gate
suivante :

| famille | facteur minimal `12,5 -> 25 k` | facteur minimal `25 -> 50 k` |
| --- | ---: | ---: |
| `uniform` | `1` | `1` |
| `eight_clusters` | `1,145` | `1,087` |
| `terrain` | `1,070` | `1,089` |
| `scanline_overlap_multiecho` | `1,195` | `1,163` |

Si l'objectif diagnostique est un nombre de records linéaire, remplacer `1,35`
par `1`. Les facteurs requis deviennent respectivement :

| famille | facteur linéaire `12,5 -> 25 k` | facteur linéaire `25 -> 50 k` |
| --- | ---: | ---: |
| `uniform` | `1,226` | `1,165` |
| `eight_clusters` | `1,459` | `1,385` |
| `terrain` | `1,364` | `1,388` |
| `scanline_overlap_multiecho` | `1,523` | `1,482` |

Cette gate est directement mesurable : publier `R_n`, `F_n` et `a_n`, puis
vérifier l'identité. Elle ne fixe pas artificiellement une forme carrée aux
rectangles.

Un niveau absolu exige un budget explicite. À `50 k`, si `F_n<=n`, la masse
moyenne minimale serait `R_n/n`, soit environ `3949/7654/1889/3496` paires par
record dans l'ordre du tableau. Si `F_n<=n^1,35` avec coefficient un, elle
serait `90/173/43/79`. Le nombre `150` du reçu ne correspond à aucun de ces
budgets et ne doit pas devenir un plancher CTest.

La vraie gate reste simultanée : deux pentes de `F_n`, `node_visits`, octets,
copies, construction des banques, travail du consommateur et high-water doivent
être `<=1,35`. Une masse moyenne croissante ne compense pas un autre compteur
dominant rouge.

## 5. Provenance et sécurité de session

Le dossier versionné ne contient que le résumé et dix-neuf lignes extraites.
Il manque :

- pin du worktree envoyé, hash du tar et manifeste des sources ;
- commande et code de sortie par run, les douze sorties complètes et leurs
  hashes ;
- hash/Build ID de l'ELF, versions CMake/nvcc et sortie CUDA complète ;
- handoff, génération, preuves des deux coupe-circuits et arrêt ciblé ;
- journal des 105 tests et statut GMP.

Le journal temporaire local a pu être retrouvé après le commit :
`session.log=63522c6f...`, `handoff.json=778560ca...`, `v3.tgz=dd6bd988...`.
Il relie les sources pertinentes aux hashes pincés plus haut et confirme
factuellement : CUDA 12.9/nvcc 12.9.41, cible `mhgp3v_anchor_device` construite,
génération `2026-08-13T05:18:23.244-07:00`, deux coupe-circuits armés, arrêt
gardé réussi. Il indique aussi que GMP est absent et que les sept tests rouges
sont les tests cœur non exécutables ou leurs wrappers parce que leur cible
n'avait pas été construite. Ce journal contient des métadonnées externes et
n'est pas versionné ; cette récupération ne transforme pas le reçu minimal en
preuve autonome.

Un contrôle GCP **en lecture seule** de l'auditeur a retrouvé exactement
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` en état
`TERMINATED`, `g4-standard-48`, `SPOT`, action `STOP`,
`maxRunDuration=3600`, dernier démarrage identique et dernier arrêt
`2026-08-13T05:31:41.482-07:00`. Aucune ressource n'a été démarrée ou mutée par
l'auditeur. Ce contrôle actuel corrobore l'arrêt ; il ne remplace pas un reçu
immuable de la session.

### Deux défauts du script à réparer avant réemploi

1. Les commandes distantes utilisent `set -e` mais pas `set -o pipefail`.
   `cmake ... | tail`, `cmake --build ... | tail` et `ctest ... | tail`
   observent le statut de `tail`. `CUDA_COMPILE=OK` peut donc être imprimé
   malgré un échec du producteur. Cette exécution a réellement réussi selon le
   journal complet, mais la ligne `OK` n'est pas un témoin reçu.
2. Le trap est armé seulement après le retour de `start_and_verify` et le parse
   du handoff. Surtout, `stop_and_verify ... | tee ... || true` masque tout
   échec de certification d'arrêt et restitue le code antérieur. Le wrapper
   peut donc sortir zéro sans avoir certifié `TERMINATED`.

Réparation demandée à Claude, sans ambiguïté :

- armer le cleanup avant le démarrage et lui faire lire le handoff dès qu'il
  existe ;
- rendre un échec de `stop_and_verify` bloquant, en conservant la génération
  attendue ;
- employer `set -euo pipefail` à distance ou capturer `PIPESTATUS[0]` ;
- stocker le log complet dans un fichier puis l'afficher, au lieu de faire
  porter l'autorité par `tail` ;
- enregistrer chaque PID de run, attendre chaque code et écrire `rc` dans le
  reçu ;
- versionner un manifeste sanitizé de provenance, jamais une clé SSH privée ni
  un journal contenant des secrets.

Le commentaire « ne modifie aucune garde » est aussi impropre : le script
appelle explicitement le setter de `maxRunDuration`. Cette mutation est
autorisée et sûre si elle est vérifiée ; elle doit simplement être décrite.

## 6. Décision positive

Le reçu ne doit être ni rejeté en bloc ni promu. Il devient :

- un diagnostic CPU/G4 de masse q4, seed 3, arithmétiquement cohérent ;
- une réfutation empirique supplémentaire du sparsifieur terminal `PairId` sur
  trois familles ;
- une calibration des facteurs de coalescence que le futur `RectFront` doit
  mesurer ;
- aucun reçu de compilation CUDA tant que le témoin scriptural n'est pas
  réparé/rejoué, même si le journal perdu confirme cette exécution ;
- aucune preuve de pente physique, de latence, d'e2e ou de SLO.

Le prochain jalon reste le walking skeleton déjà transmis : DFS unique pour
les 432 seuils, `RectKey` résiduel, `SymmetricAnd` paresseux, puis une tranche
q4 régulière jusqu'au fold dans le même jalon. Ajouter à sa sortie
`record_mass_sum`, `record_count`, `mean_mass`, `max_mass` et la distribution
p50/p95/p99 des masses. La gate compare alors les facteurs observés au tableau
ci-dessus et mesure simultanément visites, octets et high-water.

GCP consulté uniquement en lecture seule par l'auditeur ; aucune mutation ni
session démarrée par lui.
