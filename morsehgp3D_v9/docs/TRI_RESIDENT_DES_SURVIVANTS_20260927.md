# Survivants q3/q4 : trier sur le GPU avant le retour au CPU

27 septembre 2026. Exploration v9 hors registre, grille 1 mm/u18,
`public_status=not_claimed`. Prototype publié, moteur inchangé.

## Pourquoi ce changement

Le [premier raccord résident](RACCORD_RESIDENT_Q34_20260927.md) gardait
l'index géométrique sur le GPU, mais rapatriait les survivants de chaque
vague, puis rétablissait leur ordre sur le CPU. Sur la trame sans sol ng00,
son passage G4 historique W48 mesurait environ 36 ms pour cet ordre CPU
et 21 ms pour les téléchargements. Ce sont des coûts de l'ancien prototype,
pas des gains garantis du nouveau ni des temps FULL.

Le [nouveau prototype](../audits/b_q34_resident_survivors_20260927/README.md)
conserve les survivants sur le GPU jusqu'au tri. Il ne conserve **pas toutes
les paires candidates** : seulement les survivants déjà trouvés et les
buffers de la vague en cours. La taille des vagues ne limite pas la recherche.

Les étapes sont simples : accumuler les survivants, trier leurs identifiants
originaux sur 64 bits avec leurs données attachées, vérifier leur ordre
strict, puis télécharger une seule sortie compacte de 12 octets par
survivant. Les trois noyaux de filtrage géométrique restent inchangés.
Un doublon est refusé, jamais effacé pour masquer une erreur de couverture.

## Ce qui est effectivement validé

La qualification locale R1 ferme 26 commandes : Release, Clang
ASan/UBSan/LSan et compilation CUDA 12.9. Les trois binaires passent les
tests **portables**, avec les mêmes sorties que la référence native.
Le binaire CUDA n'a pas encore exécuté cette variante sur un GPU.

Le corpus comprend 85 lots, 1 360 appels appariés, 18 cas propres au
collecteur et 33 refus. Deux mutants compilés sont détectés : tri tronqué
aux 32 bits bas et données détachées de leur identifiant. Les cas comprennent
sorties vides, vagues clairsemées, réordonnancements, replis complets,
K1/2/5/10 et s8/10/12. Les grandes clés sont testées réellement en portable ;
leur injection dans le vrai tri device reste une prochaine porte distincte.

Les sources, dépendances de compilation, outils CUDA, archives liées,
binaires et commandes sont épinglés. Les lecteurs LIVE normaux et `-O`
passent. Le [contre-audit](../audits/b_q34_resident_survivors_review_20260927/README.md)
précise la couverture et les limites. Ce n'est pas un environnement
entièrement hermétique ni une nouvelle qualification de concurrence.

Précision de traçabilité après contre-audit : R1 n'épinglait pas le
contenu du fichier de paramètres indirects des deux unités CUDA.
Ses tests Release/San, qui n'utilisent pas ce fichier, restent établis ;
la compilation CUDA est observée, mais sa preuve d'options exactes est
partielle. Voir l'[addendum précis](../audits/b_q34_resident_survivors_review_20260927/RESPONSE_FILES.md).
Les anciens reçus ne sont pas réécrits : le prochain comparatif G4 aura
une nouvelle compilation avec ces fichiers fermés avant/après.

## Mémoire et complexité

L'accumulation croît avec S survivants et Q emplacements de vague, en
O(S+Q), sans tableau global de taille P ou E. Sa croissance géométrique
évite de réserver Q places supplémentaires pour chaque vague non vide.
Après séparation des clés et données, les doubles buffers du tri occupent
40 S octets, plus le scratch réellement demandé par CUB. Le chevauchement
avec l'ancien buffer est compté dans le pic d'allocations device.

Ce changement ne réduit pas le nombre E de requêtes géométriques ; leurs
compteurs restent identiques. Il ne prouve donc pas que le générateur
devienne sous-quadratique en nombre de points. Aucune nouvelle campagne
8k/16k/32k, coupes LiDAR ou multi-scènes n'est attribuée à cette tranche.

## Suite et décision d'intégration

La [porte dédiée aux grandes clés](../audits/b_q34_survivors_device_gate_20260927/README.md)
est maintenant compilée dans une capture autonome : dix commandes,
24 cas hôte, lecteurs normal/−O et huit falsifications passent.
Elle ferme aussi les fichiers de paramètres CUDA et de lien, sans réparer
rétroactivement R1. Son mode `--cuda` reste à exécuter sur G4 : les tests
hôte ne constituent pas cette exécution et ses compteurs device sont nuls.

Faire une porte CUDA réelle, puis une comparaison appariée avec l'ancien
raccord sur G4, sur la même trame entière, avec mêmes P/E/S et sorties.
Mesurer préparation, allocations, copies, tri, validation, conversion et
destructions. Les champs de durée emboîtés ne se somment pas librement.

Le [comparateur dédié](../audits/b_q34_survivors_compare_20260927/README.md)
est maintenant compilé : 13 commandes closes, six unités de compilation,
1 088 dépendances pré-épinglées, options indirectes/objets/archives fermés.
Sa petite porte portable passe 52 lots et 208 opérateurs appariés ; lecteurs
normal/−O contre-relus. Aucune exécution GPU à cette étape. Le protocole
prévoit ABBA puis BAAB dans des processus distincts, pour W4 et W48, avec
des propriétaires neufs à chaque passage. Seul le premier appel du processus
paie un premier contexte CUDA ; le premier appel de chaque implémentation
est aussi distingué. Chaque sortie est comparée puis détruite avant la
suivante. Les chronos G4 et leur comparaison restent à produire.

La sortie finale est encore un tableau hôte : le passage d'un propriétaire
GPU directement à S3 reste à développer. Ce prototype ne remplace ni les
autres étapes q3/q4 ni la construction FULL. Aucun gain G4 ou contrat
100 ms nouveau n'est acquis ; GCP non utilisé pour cette qualification.
