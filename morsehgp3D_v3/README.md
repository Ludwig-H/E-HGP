# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser de mosaïque de Delaunay d'ordre
supérieur. Le profil traité est le nuage quantifié u16; aucune conclusion n'est
étendue au nuage réel antérieur à la quantification.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, le p95
`warm_e2e<100 ms` est la cible principale et `warm_e2e<1 s` la cible
secondaire. Aucun chemin exact actuel n'est qualifié sous l'une ou l'autre.

Le verdict lié au `HEAD` et au worktree est tenu uniquement dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Ne déduire aucun état
live d'une note datée, d'un message de commit ou du seul passage d'un CTest.

## Contrat visé

Deux sorties sont distinctes :

| sortie | contenu | portée actuelle |
| --- | --- | --- |
| Gamma exhaustif enregistré | facettes, cofaces, incidences silencieuses, lots, `coverage_log` et ses `coverage_delta`, et verticales | oracle borné; l'implémentation exhaustive actuelle n'est pas une route 50 k |
| `hgp_reduced_normalized_h0_v3` | composantes horizontales exactes, niveaux exacts et unions des `PointId`, après quotient certifié des blocs H0 inertes | candidat non reçu et non revendiqué publiquement |

Une boule H0-inerte peut porter de vraies incidences Gamma. Une tombstone du
quotient horizontal ne prouve ni l'absence d'un support, ni l'absence d'une
incidence, ni une application verticale. Les verticales sont hors du contrat
horizontal et demandent leur propre spécification.

Le SLO officiel de la section 14.4 du
[`TEST_PLAN_MORSEHGP3D.md`](../docs/TEST_PLAN_MORSEHGP3D.md) porte sur
`BenchmarkOutputContract-v1` : dix forêts, applications verticales, lots et
certificat minimal sont matérialisés avant la fin de `warm_e2e`. Une mesure du
seul payload horizontal v3 appartient donc à une série diagnostique distincte;
même sous une seconde, elle ne ferme pas ce SLO.

## Faits établis

- À `k=1`, les partitions strictes et fermées sont celles du single linkage;
  une route EMST/Boruvka exacte peut éviter tout catalogue Morse d'ordre
  supérieur.
- Pour une boule avec `p` points strictement intérieurs et un support propre
  positif de taille `q`, les ordres `1<=k<=p+q-2` sont des continuations H0
  sans fusion ni nouveau `PointId`.
- À `K=10`, les seuils de témoins des supports q2/q3/q4 sont `10/9/8`. Cette
  preuve autorise seulement une tombstone horizontale avec resolver latent.
- `q_min` est la plus petite arité de provenance Morse prouvée. `q_cert` est le
  maximum des arités effectivement exhibées et rejouées pour la même boule,
  sans preuve d'absence d'un support plus grand.
- Le fast principal d'un lot multiple exige `q<=k+1`, une vraie
  `CarrierClosure` et des carriers stricts résolus dans le snapshot pré-lot.
  `q>k+1` reste au fallback.
- `prefix-all` est exact relativement à la `GeneratorTable` fournie; il ne
  prouve jamais que cette table est géométriquement complète.

Les contre-fixtures exactes du dépôt réfutent la réduction du graphe point au
K-graphe de Gabriel brut proposée dans le manuscrit. Elles interdisent
d'utiliser ce graphe, un RNG d'ordre fini, une cascade low-rank ou le résiduel
q2 comme source complète des supports q3/q4.

`smax=11` borne le contenu de chaque record fermé, jamais le nombre d'arêtes
Gabriel incidentes à un point. Le kissing number 12 ne s'applique pas : dans
l'espace euclidien, le degré est arbitraire même dans un bucket de rang fermé
fixé. Sur la grille u16 finie, seuls les caps triviaux `n-1` et `2^48-1`
subsistent; deux constructions à treize voisins réfutent déjà le cap 12 aux
rangs exacts 2 et 11. Leur preuve est durable; le statut de leur porte
exécutable appartient exclusivement à l'audit live. Sous un modèle de Poisson
homogène 3D sans bord, le degré moyen
jusqu'à `smax=11` vaut 80; c'est une baseline, pas un cap ni une garantie de
temps. La preuve est dans
[`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](audits/AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md).

## Architecture candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : EMST/Boruvka exact
  |-> q2 : Yao48/LBVH strict + classifieur terminal et census fermé
  `-> q3/q4 : center-cover de blocs complet et fail-open
       -> banque Jung--Yao + groupes de Helly terminaux
       -> centres q3 et niveaux shallow q4 dans le disque médiateur
       -> BallActivation/tombstones streamées et RLE par BallKey
       -> resolver latent, fast/fallback et lots atomiques
       -> composantes, verticales et payload officiel nommé
```

Cette architecture possède un prior art mécanique dans la ligne enregistrée :
LBVH Morton/Yao48 CUDA tuilé, classifieur `count--scan` multi-rang sous son
ancien contrat fermé et falsificateur P1a q4. Les décisions q2 ne sont pas
compatibles : l'ancien prune admet une égalité radiale et son classifieur peut
s'arrêter sur dix contacts, tandis que v3 exige dix intérieurs stricts et un
census fermé complet. Les motifs structurels et transactionnels d'ownership,
de tuiles, d'epochs, de lease/reprise/backpressure, de ledger et de
`count--scan` à offsets 64 bits sont des différentiels à réécrire puis à
requalifier. Les décisions sémantiques, layouts, ABI et juges enregistrés ne
sont ni une autorité v3 ni une preuve de SLO. Leur inventaire est dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md).

Le self-join q2 de diagnostic reste un oracle/falsificateur ou un second prune
tant que ses compteurs complets ne battent pas la route Yao/LBVH. Son prune q2
ne retire jamais une ancre q3/q4.

La preuve locale q2 combine un supremum `U4`, un infimum `L4`, des témoins
distincts et une partition exacte des paires. Sa réception logicielle, ses
mutants et ses insuffisances ne sont pas dupliqués ici : voir le verdict live.
Les compteurs historiques à 50 k atteignent déjà 53 à 724 millions de visites
`L4` et 86 millions à 1,36 milliard de tests ponctuels pour q2 seul. Le reçu
brut est dans
[`scale_counters_raw.txt`](receipts/selfjoin_q2_20260811/scale_counters_raw.txt).
Cette route reste très loin du jalon d'une seconde avant census, q3/q4 et
fold.

Le cœur universel de Jung fournit une suppression supérieure exacte, distincte
de q2 : pour une paire distincte certifiée arête maximale d'un support propre
positif, neuf `PointId` q3 ou huit q4 distincts satisfaisant le prédicat strict
certifient toutes les sphères admissibles dans le disque de centres. Pour une
ancre et un témoin fixes, une borne entière par les huit coins certifie
uniformément un nœud AABB de cibles sans rescan par paire. Cette propriété est
prouvée; la banque, son parcours et sa gate restent à construire. Le certificat
ponctuel de Helly exploite les offsets des
demi-plans sur le disque : chaque crédit possède un sous-groupe de trois
identifiants au plus, et neuf ou huit groupes disjoints ferment la lane
correspondante. Contrairement au certificat plus étroit par enveloppe convexe,
Helly n'exige pas que chaque membre soit diamétral strict. Un greedy qui échoue
reste fail-open.

La profondeur fermée de demi-boule et son noyau angulaire partagé restent des
falsificateurs exacts complémentaires. Les mesures live refusent leur collecte
complète par paire dans le chemin chaud : elle coûte davantage qu'elle
n'élimine après le cœur. Cœur, groupes de Helly, profondeur et center-cover par
patches gardent des sorts et des compteurs séparés. Les preuves et limites sont
dans
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](audits/NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md)
et
[`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](audits/NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md).
Le statut précis des composants et de leurs portes reste exclusivement dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Le self-join d'ancres couvre implicitement toutes les paires et redémarre ses
recherches de témoins à la racine. L'audit live classe cette route rouge avant
CUDA : le résiduel est plus mince que le travail, mais les deux pentes
successives de visites dépassent 1,35 dans les huit séries diagnostiques et
l'extrapolation la plus favorable reste en milliards à 50 k. Ce diagnostic ne
remplace pas la porte contractuelle aux tailles `12 500/25 000/50 000`; les
valeurs, hashes et limites exactes restent uniquement dans l'audit live.

## Invariants industriels

- Aucun tableau global de paires, tuples, cellules, faces, cofaces ou
  incidences n'est construit dans le chemin produit.
- Un oracle exhaustif borné falsifie ou recertifie le produit; il ne devient
  jamais son architecture par défaut.
- Le chemin industriel exact n'a aucun budget configurable : il produit
  l'objet complet ou échoue sur une ressource physique réelle.
- Count, fill et consommation portent la même identité. Une insuffisance de
  ressource refuse atomiquement; elle ne tronque aucune sortie.
- Toute égalité géométrique reste dans la branche conservée. Pour l'oracle de
  cellules, la partition exacte est `beta>R_q(C)` contre `beta<=R_q(C)`.
- Une proposition flottante peut ordonner le travail; seul un prédicat exact
  et rejouable autorise un prune.
- Exactitude, réduction hiérarchique, performance et statut public sont quatre
  décisions séparées.

La section 1.1 de la spécification fixe le chemin produit sans budget
configurable. Un cap diagnostique peut refuser, mais ne peut jamais publier un
préfixe comme objet complet.

## Prochain ordre de travail

1. Conserver le générateur, les self-joins, le sidecar borné et les ancres comme
   portes locales ou oracles. Fermer les identités persistantes et les juges
   vraiment indépendants encore ouverts, sans promouvoir le rescan en route
   50 k.
2. Réemployer les motifs de lease, ledger et `count--scan` de la ligne
   enregistrée, sans copier ses layouts binary64 ni ses décisions de rang
   fermé. Remplacer les recherches par ancre par des banques Yao strictes en
   antichaînes de nœuds dans l'enveloppe tuilée `O(B*48*K)`. Le certificat à
   l'autre extrémité reste une optimisation facultative, seulement si sa banque
   est déjà dans la tuile ou un cache borné. Fermer ensuite q2 par un census
   résident multi-ordre avec offsets 64 bits.
3. Porter et requalifier le falsificateur q4 mass-only `P15-HOCUDA-P1a` : partition
   triangulaire implicite des paires, 64 patches de centres, seuil de huit
   témoins par antichaînes de sous-arbres, range-query collective, ledger
   `pruned_mass+microtile_mass=C(n,2)` et aucune arène globale de paires. Cette
   tranche n'émet aucune ancre et ne prouve pas la complétude de P1. Son
   certificat exact emploie des coins rationnels à l'échelle seize et un juge
   bijectif indépendant; il est spécifié dans
   [`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](audits/NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md).
4. Sur les seules ancres admises, mesurer séparément cœur de Jung, Helly,
   composition cœur--profondeur et profondeur terminale. Le gain marginal doit
   payer collecte et tri; toute ambiguïté retombe fail-open.
5. Recevoir `BallActivation`, census, resolver, fold et reconstruction des
   verticales contre Gamma exhaustif borné. Installer le harness du payload
   officiel avant toute optimisation GPU.
6. Pour P1a seulement, fermer le différentiel hôte à `n=32`, puis, dans la même
   session G4 gardée, exécuter la parité native, `n=32` sous Compute Sanitizer
   et le profil 50 k direct, sans taille intermédiaire ni retry. Pour les autres
   routes de source, appliquer la gate de compteurs à
   `12 500/25 000/50 000`. Toute route produit complète admise se mesure ensuite
   sur G4 avec build, transferts, source, certification, dix forêts,
   verticales, lots, certificat minimal et retour hôte dans le même p95
   `warm_e2e`.

## Construction des juges

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
```

Ces commandes valident des portes locales. Elles ne qualifient ni la source,
ni la performance, ni le statut public.

## Autorités

- [`PROPOSITION.md`](PROPOSITION.md) : architecture, preuves conditionnelles et
  conditions d'admission.
- [`audits/README.md`](audits/README.md) : index des audits et reçus.
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : seul verdict
  live.
- [`../docs/SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat.
- [`../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) : statut des preuves.

GCP non utilisé.
