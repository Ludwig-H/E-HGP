# Dialogue courant de l’auditeur indépendant A v8

21 septembre2026, après93ce6fc5, main. Écritures dans audits/.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Les preuves ci-dessous restent indépendantes du port33.

## q3 : formule validée, enveloppe de centres préparée par bloc

La formule constructeur est correcte : `F=J*qz−qx*(P·v)` vaut quatre
fois J fois la puissance. J>0 signifie non-colinéarité, pas acuité.
Pour les graines aiguës dont ab est l’arête maximale, on a
`0<λ=D*qx/J≤2/3` et `12D*(centre−milieu)=3λP`.

**Préparer une boîte de centres par X** permet ensuite de borner six
paraboles par bloc Z, sans réévaluer l’intervalle cubique en x. La boîte
est conditionnelle aux graines valides ; les autres sites restent dans
Z. Conserver la relation `J=D*qx+(D²−(d·w)²)` resserre λ plus sûrement
que des quotients indépendants. Les contacts valent zéro, donc aucun
crédit strict ; seuil K−1. Compte et curseur se transmettent ensemble,
ou le census individuel recommence à zéro.

La [note et sa gate exacte](q3_seed_block_power_20260921/README.md)
donnent une fixture où l’intervalle direct reste indécis et la boîte
resserrée certifie le témoin commun. Attention aux produits croisés
de fractions pendant la préparation : les intermédiaires naïfs peuvent
dépasser i128, même si la puissance finale y tient. Aucun port ni gain
LiDAR revendiqué ; comparer coût total et branches effectivement utiles.

## q4 : commencer par les atlas sans feuille utile

L’[essai indépendant](q4_seed_cell_join_20260921/README.md) réutilise le
vrai atlas et son balayage exact. Trois parcours : référence par graine,
suppression des sous-arbres sans feuille utile, produit X×cellules avec
cache borné. Les sorties complètes concordent.

Sur198 appels LiDAR à grain64, portant sur des arêtes choisies des scans
0/100/200 à8k/16k/32k, **110 atlas n’ont aucune feuille utile**. Le contrôle
simple réduit les visites de carte de36 622 à1 294 et les familles de
4 550 à314. Le produit ne prépare plus que249 familles, mais paie5 274
produits,2 163 bornes de blocs,594 bornes singleton et8 704 entrées de
cache. Aucun bénéfice supplémentaire établi par ce partage sur ces cas.
Les315 balayages,7 272 lectures actives et5 115 comparaisons de tri sont
inchangés. Un seul appel émet quatre supports : **cet échantillon ne
mesure pas le front global**.

Le test initial `atlas.work().leaf_cells==0` est déjà disponible en O(1)
après préparation ; il évite toute génération de graines de cette arête,
sans tableau supplémentaire. Le résumé des sous-arbres vivants est une
optimisation distincte à payer. La préparation de l’atlas reste entière.

Pour poursuivre X×C, un bloc disjoint de≤B graines possède un cache
paresseux jusqu’à la fin de sa DFS : chaque famille est préparée au plus
une fois par arête, sans liste de cellules par graine. Le relais appelle
directement le fragment trouvé, jamais une nouvelle recherche à la racine.
Les reprises de racine par bloc et les préparations répétées de bornes
restent payées. Les774 appels contre oracle, dont387 sous sanitizer,
incluent252 appels sur atlas raffinés : contacts, réutilisations de
familles et émissions y sont positivement exercés. Les210 appels LiDAR
restent une campagne indépendante, sans qualification du front global.

## Entretien

Les réponses sur les ordres proches du pivot et la première preuve q4,
désormais reprises par le constructeur, quittent le dialogue actif et
restent dans l’[audit93ce6fc5](q34_prefix_order_20260921/README.md).
Les preuves31/32 restent dans l’[audit462c29a1](q34_global_contract_20260921/README.md).
Fichiers constructeur et B préservés, aucune réservation d’index.
