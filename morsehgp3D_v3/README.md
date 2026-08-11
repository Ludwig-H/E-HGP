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
| Gamma/v2 exhaustif | facettes, cofaces, incidences silencieuses, lots, `coverage_delta` et verticales | oracle borné; l'implémentation exhaustive actuelle n'est pas une route 50 k |
| `hgp_reduced_normalized_h0_v3` | composantes horizontales exactes, niveaux exacts et unions des `PointId`, après quotient certifié des blocs H0 inertes | candidat non reçu et non revendiqué publiquement |

Une boule H0-inerte peut porter de vraies incidences Gamma. Une tombstone du
quotient horizontal ne prouve ni l'absence d'un support, ni l'absence d'une
incidence, ni une application verticale. Les verticales sont hors du contrat
horizontal et demandent leur propre spécification.

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

Les contre-exemples du manuscrit interdisent d'utiliser un graphe Gabriel, un
RNG d'ordre fini, une cascade low-rank ou le résiduel q2 comme source complète
des supports q3/q4.

## Architecture candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : EMST/Boruvka exact
  |-> q2 : Yao48/LBVH strict + classifieur terminal et census fermé
  `-> q3/q4 : cœur de Jung à recevoir, profondeur et cover à construire
       -> centres q3 et niveaux shallow q4 dans le disque médiateur
       -> BallActivation/tombstones streamées et RLE par BallKey
       -> resolver latent, fast/fallback et lots atomiques
       -> composantes et payload horizontal normalisé
```

Le self-join q2 actuel reste un oracle/falsificateur ou un second prune tant que
ses compteurs complets ne battent pas la route Yao/LBVH. Son prune q2 ne retire
jamais une ancre q3/q4.

Le commit `1dfe07b` ajoute au self-join q2 `L4`, l'héritage de témoins et une
sortie précoce. Ces transformations ont une preuve mathématique locale, mais
leur intégration n'est pas reçue : aucune gate ne compare encore tous les
sorts et masses à une baseline sans optimisation. Le générateur multi-écho
peut rendre `n+1` ou `n+2` points. Les drivers q2 et ancres remplacent
localement `n` par la taille rendue; cela ferme leur incohérence interne, pas
le contrat partagé de `--points`. Le journal q2 mêle en outre deux binaires et
n'est plus un reçu immuable. Ses segments à 50 k comptent encore 53 à 724
millions de visites `L4` et 86 millions à 1,36 milliard de tests ponctuels pour
q2 seul. Les chronos sous charge ne qualifient aucun gain; voir
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Le cœur universel de Jung fournit une suppression supérieure exacte, distincte
de q2 : pour une paire distincte certifiée arête maximale d'un support propre
positif, neuf `PointId` q3 ou huit q4 distincts satisfaisant le prédicat strict
certifient toutes les sphères admissibles dans le disque de centres. Le
falsificateur `core` de `1dfe07b` est présent, mais non reçu : sa porte partage
les primitives géométriques v2, son rejeu ne vérifie pas encore l'unicité des
handles et sa branche q4 accepte un certificat dégénéré `D^2=U^2=0`. La
profondeur fermée de demi-boule reste un filtre terminal exact complémentaire
à implémenter, sans hypothèse de diamètre sous un support q3/q4 certifié; le
cover par 64 patches reste un troisième schéma conditionnel exact. Ces
certificats ne définissent aucune chaîne d'inclusion entre leurs résiduels. Les
preuves, la provenance, les prédicats et les limites sont dans
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](audits/NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md).

La parcimonie globale des ancres et des arrangements n'est pas prouvée. Le pire
cas q2 est quadratique en sortie, et une recherche de témoins naïve peut être
cubique. Les compteurs à `12 500/25 000/50 000` doivent donc précéder tout port
CUDA.

## Invariants industriels

- Aucun tableau global de paires, tuples, cellules, faces, cofaces ou
  incidences n'est construit dans le chemin produit.
- Un oracle exhaustif borné falsifie ou recertifie le produit; il ne devient
  jamais son architecture par défaut.
- Count, fill et consommation portent la même identité. Une insuffisance de
  ressource refuse atomiquement; elle ne tronque aucune sortie.
- Toute égalité géométrique reste dans la branche conservée. Pour l'oracle de
  cellules, la partition exacte est `beta>R_q(C)` contre `beta<=R_q(C)`.
- Une proposition flottante peut ordonner le travail; seul un prédicat exact
  et rejouable autorise un prune.
- Exactitude, réduction hiérarchique, performance et statut public sont quatre
  décisions séparées.

## Prochain ordre de travail

1. Imposer à tous les consommateurs un contrat unique de cardinalité, de
   préférence exactement `n` points ou un refus fermé, puis recevoir `L4` et
   l'héritage par différentiel baseline, mutants ciblés et extrêmes u16.
2. Mesurer seulement le delta reçu, construire le classifieur terminal et le
   census fermé, puis comparer cette route à Yao48/LBVH sur les mêmes familles.
3. Remplacer le tag de version du sidecar par une identité producteur vérifiée,
   ajouter ses mutants de métadonnées et cibler l'appel du self-test dans la
   factory; conserver ce chemin comme oracle permanent `n<=32`.
4. Corriger et recevoir le falsificateur cœur q3/q4 par juge indépendant,
   certificats distincts et cas dégénérés; implémenter ensuite séparément la
   profondeur de demi-boule et le cover par patches, avec coûts isolés.
5. Recevoir `BallActivation`, census, resolver et fold horizontal contre Gamma
   exhaustif borné.
6. Mesurer seulement ensuite le pipeline complet sur G4 : build, source,
   certification, fold et payload inclus dans le p95 `warm_e2e`.

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
