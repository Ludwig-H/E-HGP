# Contre-audit du reçu de croissance LiDAR local `6fe8ec7a`

**Portée.** Reçu `receipts/lidar_scaling_local_partial_20260923/`, scène
SemanticKITTI 08/000000 **sans sol**, grille entière 1 mm, CPU local partagé,
W8, séparation `s=8`, leviers par défaut du binaire `28f0c284`. K5 comporte
trois sous-nuages emboîtés de 8k/16k/32k et les sept morceaux spatiaux ; K10
ne comporte que les sous-nuages 8k/16k. Aucun GPU/G4, aucune trame brute,
aucune autre scène, aucun essai répété. Tous les JSON conservés indiquent
`complete_relative_to_cross_checked_catalogue` : cela ne certifie pas la
complétude géométrique des clés absentes du catalogue.

## Intégrité et identité vérifiées

- Les **15/15** empreintes de `SHA256SUMS` concordent avec les blobs du commit.
  Les dix lignes de `SUMMARY_s00_k5.json` correspondent aux dix JSON K5
  individuels (tailles, statut, digest, compteurs et temps arrondis).
- Depuis le `full.u32le` v8 de 39 885 sites, le tri décrit par le runner
  reproduit les cinq SHA-256 déclarés pour les entrées emboîtées K5/K10
  (leurs fichiers dérivés ne sont pas archivés). Les sept fichiers de morceaux
  v8 correspondent également aux sept SHA-256 K5. Les moitiés et les quarts
  couvrent chacun les
  39 885 identifiants de sites sans intersection ni manque ; leurs coordonnées
  recollent à celles de `full.u32le`. Ce contrôle porte sur les entrées, pas
  sur une nouvelle exécution HGP ni sur la justesse des sorties.
- `probe_binary.sha256` archive `e305f124…` pour le binaire déclaré. Aucun des
  quatre binaires `mhgp9_tower_probe` locaux retrouvés au moment de l'audit ne
  possède cette empreinte : **pas de rejeu LIVE du binaire original** à partir
  des builds actuellement présents. Le commit source et les sorties JSON
  restent inspectables ; ne pas transformer leur hash en preuve d'un rejeu.

## Croissance effectivement observée

Les exposants `p` sont `log₂(mesure(2n)/mesure(n))` ; ce sont deux ou un
doublements sur **une seule géométrie emboîtée**, pas une borne asymptotique.

| Mesure | K5, 8k→16k | K5, 16k→32k | K10, 8k→16k |
| --- | ---: | ---: | ---: |
| Chrono interne de chaîne, secondes | 5,168→11,196 (`p=1,115`) | 11,196→22,562 (`p=1,011`) | 21,484→37,552 (`p=0,806`) |
| Temps CPU cumulé, secondes | 32,134→74,541 (`p=1,214`) | 74,541→133,201 (`p=0,837`) | 115,437→250,381 (`p=1,117`) |
| Paires développées | 3,648→12,787 M (`p=1,810`) | 12,787→23,073 M (`p=0,852`) | 4,953→15,832 M (`p=1,676`) |
| `core_sites` | 33,606→257,617 M (**`p=2,938`**) | 257,617→349,173 M (`p=0,439`) | 105,034→597,535 M (**`p=2,508`**) |
| `dead_core_uniform_tests` | 56,401→299,823 M (**`p=2,410`**) | 299,823→669,390 M (`p=1,159`) | 226,732→992,882 M (**`p=2,131`**) |
| Boules du catalogue | 342 181→639 102 (`p=0,901`) | 639 102→1 114 470 (`p=0,802`) | 1 567 942→2 830 107 (`p=0,852`) |

Les temps mesurés et plusieurs compteurs croissent sous `n²` sur ces
doublements, mais `core_sites` et `dead_core_uniform_tests` dépassent `n²`
sur le premier doublement **dans les deux K**. Leur chute au second
doublement K5 signale une forte dépendance à la géométrie du recadrage.
Conclure « algorithme sous-quadratique sur LiDAR » serait donc injustifié ;
le diagnostic utile est de poursuivre K10 à 32k, de répéter les mesures,
de comparer plusieurs scènes/séquences et de profiler ces compteurs lourds.
Les sept morceaux diagnostiquent la variation spatiale mais ne fournissent
pas les trois tailles emboîtées à K10 ni une qualification de la trame entière
sur G4.

## Statut du runner après le commit `4530644b`

Ces limites concernent **le reçu partiel v1 ci-dessus**. Le runner v2 publié
ensuite dans `4530644b` refuse une sortie déjà occupée, encode scène/K/s/W/
répétition dans les noms, vérifie le schéma, les options, les ordres et le
statut du probe, conserve les refus avec commande/stdout/stderr, publie les
hashes d'IDs des entrées emboîtées et ajoute la pente du temps interne de
chaîne et les compteurs cachés. Il vérifie aussi les morceaux contre le
manifeste v8. Ces
corrections de code **ne complètent pas rétroactivement** le K10 32k, les
répétitions ou les autres scènes absentes de ce reçu. Leur qualification sur
une campagne v2 reste à examiner séparément ; la priorité de mesure reste
la croissance des compteurs lourds et le coût total sur LiDAR réel.

**Contrat d'entrée encore ouvert dans v2.** `validate_probe` contrôle le
nombre de sites, mais ni `input.hash`, ni `input.format`, ni `input.grid`, ni
`options.tower_static_threads`. Une mutation locale du JSON 8k archivé,
adapté au schéma/champs v12, reste acceptée après substitution indépendante
de `input.hash=0000000000000000`, `input.format=u16le`,
`input.grid=other` ou `tower_static_threads=0` (quatre `True` de
`validate_probe`). Le test juge **le lecteur Python**, pas une sortie
réelle de la sonde v12 ; le binaire normal lit bien le chemin demandé.
Le worker G4 possède déjà `input_fnv(raw)` et compare l'identité complète.
Pour rendre le prochain reçu local autoritatif, calculer ce FNV sur les
octets effectivement fournis, exiger sa correspondance dans le JSON ainsi
que `u32le` et le nombre de threads effectif attendu ; passer explicitement
`--grid=1mm` à la sonde et vérifier ce libellé. Ajouter ces quatre mutations
à une porte du runner. Le SHA-256 du fichier dans la provenance reste utile,
mais ne remplace pas le lien avec l'entrée annoncée par la sonde.

**Verdict :** reçu partiel et intègre, premier signal utile mais pas preuve
de sous-quadraticité dans les régimes LiDAR, ni qualification FULL exacte
globale, ni résultat G4/GPU, ni contrat sous la seconde.
