# D6 de la session K : relecture corrigée des bruts — 8 octobre 2026

**Les 126 sorties de sondes sont admises par le lecteur corrigé `e37fd8935`, sans rejouer les moteurs.**
Le rapport originel reste inchangé : commande D6 au code **1**, 63 prises et 66 alertes. Aucun contrat FULL,
choix de profil D6 ou domaine numérique entier n'est clos ici. Cadre : exploration v12 hors registre,
CPU de référence sur la session G4 du constructeur, `public_status=not_claimed`.

Archive de résultats K `5f8d64c1…`, 229 876 octets ; rapport et sources Git épinglés dans `capture.json`.
Inventaire avant lecture, puis ouverture uniquement des membres JSON/journaux D6 ; aucun jeu de coordonnées,
export binaire de catalogue, construction, moteur natif ou appel GCP lu/exécuté par cet audit.
La [provenance extérieure et la fermeture de la Session](../session_k_provenance/README.md) sont contrôlées
séparément : arrêt certifié à 03:03:03.040 UTC, source déclarée `c9ac60f20`, sans hashes binaires ni CMakeCache
rapatriés. Ce reçu D6 ne remplace pas cette distinction de provenance.

## Cause du refus d'origine

Le paquet `c9ac60f20` utilisait encore `pilote_d6.py` SHA `48f40fd6…`. Sa relecture reproduit exactement
les 126 résumés archivés et les 66 alertes : 63 catalogues admis, **63 sorties Gc refusées** malgré leurs codes
de sonde déclarés zéro. Le nouveau schéma Gc possède six scalaires et trois tableaux de diagnostics supplémentaires.
Le correctif livré en `e37fd8935`, SHA `bb1de862…`, est celui déjà contre-éprouvé dans
[d6_livraison_stricte](../d6_livraison_stricte/README.md) ; ses portes historiques ne sont pas rejouées ici.

Les trois alertes « exports du catalogue différents » sont une conséquence du filtre de `checks` : toutes les
prises sont retirées après refus G ; l'ensemble `bodies` devient vide. Les neuf hashes de corps déclarés pour
les trois profils à ×1 sont en fait égaux par trame. **Ces hashes sont repris du rapport, pas recalculés** :
le pilote a supprimé les exports après leur lecture. Cette limite n'empêche pas de recalculer les temps des bruts.

Avec le lecteur corrigé : 63 C + 63 G admis, cinq passes par sonde, **630 passes dont 504 chaudes**, zéro écart
selon les contrôles D6 écrits. Les entiers, statuts, profils, K5, W48, feuille 24, passes, schémas CPU/Gc, nombres de
sites, digests et sorties finales sont contrôlés. La cohorte contient les trois trames × sept couples
`(21,1),(21,8),(24,1),(24,8),(32,1),(32,8),(32,2048)` × trois tours, dans l'ordre tournant demandé.
La médiane des passes 2–5 et les ratios appariés par trame/tour sont recalculés indépendamment des agrégats.

Le code extérieur **1** et `group_closed=1`, sans troncature ni destruction résiduelle, sont archivés pour
la commande D6. Les 126 codes zéro individuels sont **déclarés dans le rapport du pilote** : pas des attestations
externes par sonde. L'audit de Session distingue source déclarée et preuve de compilation ; ce lecteur ne fabrique
aucun hash de binaire ni chaîne de construction absente.

## Mesures reconstituées

Temps en millisecondes : médiane des trois médianes chaudes de prise. Ratios : médiane des trois rapports
appariés au même tour, donc pas nécessairement quotient des deux temps affichés. Les minima/maxima des trois
ratios sont conservés dans la capture ; ce ne sont pas des intervalles de confiance.

| Trame | C u21 ×1 | G u21 ×1 | C u24/u21 ×1 | G u24/u21 ×1 | C u32/u21 ×1 | G u32/u21 ×1 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 325,201 | 53,201 | 1,00446 | 1,00321 | 1,01039 | 1,05039 |
| ng01 | 279,140 | 42,356 | 1,00039 | 1,00071 | 1,00561 | 1,04337 |
| ng02 | 328,427 | 48,444 | 1,00184 | 1,06796 | 1,00884 | 1,11175 |

Le seul élargissement du type à coordonnées inchangées coûte peu sur C dans ces prises ; G présente déjà
une différence de 6,8 % pour u24/ng02 et de 4,3–11,2 % pour u32. Ce sont deux étages chronométrés séparément,
avec préparation du nuage hors du mur. **Ne pas additionner leurs médianes pour inventer une latence FULL.**
Le seuil produit D6 de moins de 3 % reste ouvert : P, raccord et T/M/V/R ne sont pas mesurés ici, ni le
catalogue CUDA de FULL. Voir [la limite de préparation déjà établie](../../audit_d6_preparation_20261007/README.md).

À u32 ×2048, les médianes C sont **958,189 / 810,614 / 954,462 ms**, soit **×2,946 / ×2,904 / ×2,906** face
à u21 ×1 ; G vaut **59,081 / 48,093 / 52,842 ms**, soit ×1,113 / ×1,133 / ×1,092. Les diagnostics mettent en
évidence une piste concrète : les classes de feuilles sont identiques entre u21 ×1 et u32 ×1, mais ×2048 rend
`leaves_narrow=0` et `leaves_exact=123497/99671/115572`, contre `1/3/0` à ×1. Ces valeurs sont stables sur les
passes et les tours. `region_line_fallbacks` reste zéro : ne pas appeler ce changement un repli de lignes.
Il s'agit d'une association mesurée avec le changement de classes arithmétiques, pas d'un gain ou coût causal isolé.

## Portée mathématique et numérique

À ×1, même quantification et mêmes coordonnées ; seules les configurations compilées 21/24/32 changent.
Les empreintes de résolution issues des JSON sont égales entre ces profils et tours, par trame. Les corps C
déclarés au premier tour sont égaux après exclusion de l'en-tête. G n'émet son empreinte et ses ordres qu'à
la **dernière passe** ; aucune identité géométrique des cinq passes n'est inventée.

×8 et ×2048 sont des homothéties des entiers déjà quantifiés, pas des réacquisitions à une grille plus fine.
Pour α positif, les puissances des boules sont multipliées par α² ; intérieurs, coquilles et incidences sont
conservés. Pour ces puissances de deux, les clés Morton sont décalées de 3 log₂α bits : l'ordre des sites est
conservé sans dépassement de domaine. Les comptes d'objet C/G sont effectivement invariants dans les sorties.
Ces comptes n'identifient toutefois pas toutes les associations : la preuve sémantique sous homothétie reste
plus faible qu'une comparaison après normalisation des coordonnées. Voir le
[lemme et ses limites](../../audit_d6_20261007/math/README.md).

L'admission de ×8 en u21 implique, sous le contrôle de domaine du producteur, que les coordonnées originales
sont <2¹⁸ ; ×2048 reste alors <2²⁹. C'est une **inférence des profils admis**, sans scan des coordonnées par
l'auditeur. Cette campagne ne touche donc pas les deux bords de tout le domaine u32 ; elle ne remplace ni les
portes extrêmes ni le banc distinct de translation. Aucun nouveau détail des retours originaux n'est créé.

## Reproduction

```sh
python3 -B CHEMIN_DU_RECU/check.py --repo DEPOT --archive RESULTS_TAR_GZ --check
python3 -B -O CHEMIN_DU_RECU/check.py --repo DEPOT --archive RESULTS_TAR_GZ --check
```

Les deux sorties sont identiques au champ `result` de `capture.json`. Le lecteur vérifie les pins, reproduit
les refus originaux, admet les mêmes octets avec le lecteur corrigé, contrôle couverture et chronos Gc, puis
recalcule les médianes/ratios. L'archive est vérifiée avant/après. Aucun ancien reçu ou rapport n'est réécrit.
