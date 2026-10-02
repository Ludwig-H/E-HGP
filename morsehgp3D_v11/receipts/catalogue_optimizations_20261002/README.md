# Catalogue : ablations exactes cache J2 / tri indirect

Capture `optimizations2`, source exécutée
`df069960a0e109ea15c94115537576f540f4de1b`. Session close : `completed`,
worker0, arrêt de la bonne génération certifié, résultats vérifiés, clés
OS Login et privée retirées, verrou libéré. L'archive originale unique est
conservée avec reçu filtré, matrice, supplément ASan18, manifeste des
entrées et rapport des mesures. Aucun octet LiDAR n'est ajouté au dépôt.

Le lecteur est **LIVE** : le reçu brut local indiqué dans `receipt.json`
est obligatoire. Code0 signifie cohérence des preuves ; une campagne
en échec reste non conforme. Aucun exécutable natif ni service cloud
n'est appelé. Le tar est lu par `extractfile`, jamais extrait.

```sh
python3 morsehgp3D_v11/receipts/catalogue_optimizations_20261002/check.py
python3 -O morsehgp3D_v11/receipts/catalogue_optimizations_20261002/check.py
python3 morsehgp3D_v11/receipts/catalogue_optimizations_20261002/check_selftest.py
python3 -O morsehgp3D_v11/receipts/catalogue_optimizations_20261002/check_selftest.py
```

Qualification observée : **2 115/2 115** portes de matrice, **139/139**
ASan/UBSan18 (`num;index;tower`), 210 mutants tués avec causes recoupées
JUnit/LastTest, dont deux refus de construction explicitement attendus.
Clang absent est l'exception déclarée. Les vingt portes cache/tri exigées
sont présentes dans chaque configuration native applicable. Les profils,
les drapeaux et hashes de compilation sont reliés aux exécutables mesurés.
La qualification ne porte pas sur les changements FULL ultérieurs.

**36/36 essais K5 réussis**, aucune omission ni divergence ; six groupes
de comparaison sont complets. Les modes sont0=aucune optimisation,
2=tri indirect,1=cache J2,3=les deux. Chaque mode est indépendant ; un échec
de baseline ne justifie aucune omission d'un autre mode. Le calendrier
comprend24 essais LiDAR/W48,6 LiDAR/mode3/W8 et6 synthétiques/mode3/W48.
Timeout enfant15s, budget campagne700s. Durée observée484,138s, dont
118,428s cumulées de processus natifs et359,697s de décodage Python.

Temps de l'API **catalogue K5**, en secondes, un processus neuf par cellule :

| Trame sans sol | Profil | W48 mode0 | W48 tri | W48 cache | W48 les deux | W8 les deux |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | u21 | 4,418 | 3,646 | 4,104 | 3,239 | 3,809 |
| 08/000000 | u24 | 4,699 | 3,726 | 4,329 | 3,274 | 3,864 |
| 08/000100 | u21 | 3,024 | 2,360 | 2,824 | 2,115 | 3,077 |
| 08/000100 | u24 | 3,138 | 2,376 | 2,940 | 2,178 | 3,160 |
| 08/000200 | u21 | 4,006 | 3,110 | 3,730 | 2,790 | 3,037 |
| 08/000200 | u24 | 4,279 | 3,134 | 4,020 | 2,861 | 3,127 |

Les uniformes8k/16k/32k, mode3/W48, donnent u21 :0,426/0,946/1,876s ;
u24 :0,439/0,939/1,974s. Pas de répétitions permettant d'estimer la
variabilité ; les modes ont été joués dans l'ordre fixe0,2,1,3. Aucune
borne asymptotique ni durée FULL ne découle de ces observations.

L'API chronométrée inclut deux passes, tri et assemblage des sorties.
Cloud, création du Pool, entrées/sorties, normalisation Python et retrait
du sol sont séparés. Les pics publiés sont des réservations Buffer,
Cloud vivant compris, pas le RSS. Les trois sous-nuages entiers sont sur
la même grille1mm et dans un domaine18bits, calculés en21/24bits ; les
39885/35551/45845 sites de la même séquence08 ne constituent pas plusieurs
séquences. Ni float32 brut, ni segmentation, ni GPU ni contrat200ms acquis.

`source_contract.json` et `source_df/` épinglent les cinq scripts réellement
exécutés. Le lecteur recompute calendrier et comparaisons, recoupe
intentions/argv, statut processus, flux JSON, compteurs q4/J2, égalités
cache demandes=évaluations+hits, temps et tailles exactes du codec.
Les ratios de temps ne remplacent pas l'égalité des sorties : hash
sémantique et travail géométrique sont comparés entre profils/modes/W,
le hash brut uniquement à profil égal.

**Aucun des36 gros payloads canoniques n'est archivé** : leurs hashes sont
des résultats enregistrés, pas des empreintes recalculées par ce lecteur.
La branche de décodage d'un payload réellement présent est vérifiée sur
un petit binaire synthétique dans l'auto-test. Celui-ci conserve dix
témoins positifs, dont délais/signaux, sortie invalide après décodage,
interruption avant/après lancement et calendrier incomplet. Il refuse
61 corruptions en mémoire ; cela ne réaudite pas tous les helpers
historiques importés. Lectures normal/−O concordantes, preuve compacte
`check_selftest.json`. Les captures antérieures sont inchangées.
