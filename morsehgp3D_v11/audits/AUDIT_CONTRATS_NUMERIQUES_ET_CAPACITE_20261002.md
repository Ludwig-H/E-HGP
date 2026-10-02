# Audit indépendant v11 — état des fondations

2026-10-02 14:12 UTC. Port numérique publié `9d639e146`, plan `9df774947` ;
qualification catalogue antérieure `e6fe34cb0`, fondations `a97180667`.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only` (nouveau défaut), `public_status=not_claimed`.
Sources figées, recoupe d'archives et contrôles autonomes entiers/Fraction ;
aucun nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Port numérique 9d relu favorablement ; qualification propre encore attendue.**
Q1/q2/q4 passent en i128 jusqu'à B24, q3 seulement à B18 ; défaut désormais u21.
La qualification et les temps e6 ci-dessous restent ceux des sources antérieures,
entrées 1 mm/u18 : trois catalogues LiDAR K5 en 20–26 s. Aucun gain natif 9d
ni contrat FULL/GPU/100 ms ou massif acquis. Aucun nouveau défaut établi.

## Dernière qualification close, avant le port 9d

[Catalogue3, source e6](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md) :
960/960 portes conformes ; campagne de temps distincte en échec. Les premiers
échecs et les [fondations a971](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md) gardent leurs reçus.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 220/220 | 145 portes de base +75 références Python |
| GCC ASan/UBSan, TSan, B21, B24 | 145/145 chacune | Références Python exclues |
| Poison | 146/146 | Base +porte poison |
| Style ; mutants | 2/2 ; 12/12 | Manifestes/campagnes séparés |

Les 103 mutants de fondations gardent leurs causes distinctes ; les neuf
mutants catalogue sont tués par code, sans signal/délai/compilation ratée.
Le juge natif Gram/Fraction vérifie 378 requêtes, dont 20 refus, 132 505
contrôles et 127 relations métamorphiques par profil, normal/−O.
La coquille qmin4/m5 et le croisement core inter-K ont leurs portes :
ce dernier est joué dans les deux références Python, sans FULL natif.
Clang absent, aucune qualification Clang. CPU sur G4 ne signifie pas GPU.

Hashes exécutables/cache/flags, interruption et fermetures ciblées recoupés.
Quiescence **entre configurations internes** encore `isolation=not_certified` ;
aucune fuite de VM déduite.

## Temps : réduction observée et déplacement du travail

[Pièces et lecteur autonome clos](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md) :
13 tentatives, sept succès, six délais de processus 30 s et 23 omissions.
Les cinq K10 tentés expirent ; uniforme32k/K5 expire, son K10 n'est pas joué.
Un délai ne donne aucune durée finale du catalogue.

| Catalogue CPU mono e6, 1 mm/u18, leaf16/K5 | Durée | Pic Buffer, Cloud compris |
| --- | ---: | ---: |
| Uniforme 8k | 8,240 s, médiane de trois | 133 416 208 octets |
| Uniforme 16k | 17,310 s, un essai | 275 155 856 octets |
| 08/000100 sans sol, 35 551 sites | 20,741 s, un essai | 235 905 260 octets |
| 08/000000 sans sol, 39 885 sites | 26,018 s, un essai | 279 721 668 octets |
| 08/000200 sans sol, 45 845 sites | 24,093 s, un essai | 297 653 548 octets |

À 8k/K5, même binaire Release, mêmes entrées et hash canonique **déclaré**
que [leaf32](../receipts/audit_independant_20261002/catalogue_second_capture_review_4/README.md) :
597 998 boules, 597 987 niveaux, 2 895 136 incidences et pic inchangés.
Fichiers de sortie retirés de VM, pas de rehash ici. 15,478 s / médiane
8,239569411 s = ×1,879 observé, baseline unique et campagnes non appariées.
Par passe, préfixes 144,09→85,49 M et census 61,81→18,53 M ; filtres
28,94→64,53 M et feuilles 12 507→77 934. Ventiler count/fill/tri/assemblage
avant d'attribuer le gain à une phase. Ces deux tailles ne prouvent aucune
borne générale. Les trois trames appartiennent à une seule séquence.
Durée API = deux passes, tri et sorties mémoire ; lecture/Cloud, masque de
sol, préparation et sérialisation distincts ou exclus. Buffer n'est pas RSS.

## Numérique : leviers exacts du prochain port

[Prédicats](../receipts/audit_independant_20261002/numeric_geometry_review_3/README.md)
et [entiers/niveaux](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md)
relus favorablement ; centres hors boîte et préfixes q3 obtus couverts.
`Sphere::through` ne certifie pas la criticité. U32 exigera de reprendre
les expressions natives, pas seulement Level.

[Bornes par présentation](../receipts/audit_independant_20261002/native_arity_bounds_review_6/README.md) :
M=2^B. Pour q4, chaque cross de différences issues de trois points du **même
carré** a une magnitude <M². Cramer donne D<6M³ et |N_j|<9M⁴ ; chacun des
quatre termes power est <18M⁵, chaque somme partielle <72M⁵<2^127 à B24.
Aucune convexité ou annulation supposée. Pour q3, <216M⁶ suffit en B18,
pas en B21/24. Un q3 de boule qmin2 déborde au premier produit en B21 :
le bon critère est l'arité **de présentation**, jamais qmin.

[Contrat du port 9d](../receipts/audit_independant_20261002/native_power_contract_review_6/README.md) :
tag privé fixé par les factories, coefficients/Level cohérents et copies
conservant ce couplage. `power` garde la conversion contrôlée, `side` lit
le signe natif sûr. Portes Fraction, voie Wide de harnais et mutations
supplémentaires sont déclarées ; leur réussite native reste à établir.
B18 reste explicite ; ASan/UBSan B24 n'exerce pas q3 natif B18.

Le [niveau q4 tardif](../receipts/audit_independant_20261002/catalogue_native_side_review_5/README.md)
reste une proposition : type interne sans Level provisoire publiable,
vrai niveau avant count/fill/tri/rangs. La factory actuelle le calcule
encore. Comparer sorties/refus/travail discret puis phases sur G4 ; aucun
port de filtres flottants ou gain acquis par cette seule lecture.

## Profils compilés et précision physique

[Le nouveau banc](../receipts/audit_independant_20261002/profile_benchmark_review_6/README.md)
utilise les mêmes XYZ/IDs à 1 mm dans B18/21/24. SHA/cache du binaire qualifié
vérifiés avant mesures ; digest normalisé garde niveaux rationnels, S*, I/U
et rangs. C'est une comparaison d'encodages, pas un oracle géométrique.
API/processus/décodage sont séparés ; refus/délais et checkpoint avant
décodage préservent les essais incomplets. Aucun résultat de campagne acquis ici.

[Contrat de précision](../receipts/audit_independant_20261002/precision_mapping_review_6/README.md) :
position physique=o+hq, niveau physique=h²β. B seul ne change pas h ; multiplier
les anciens entiers ne restaure aucun détail. Un vrai affinement repart des
coordonnées d'origine, masque/repère/IDs communs, collisions publiées.
À 1 mm, portées par axe u18/u21/u24 : 262,143/2 097,151/16 777,215 m.
Les poids restent conservés et refusés par le catalogue actuel. La stabilité
FULL en rayon ne stabilise pas une attache figée premier-cover/LCA.

## Complétude et coquilles nombreuses

[Élagages et propriété](../receipts/audit_independant_20261002/catalogue_geometry_review_4/README.md) :
listes K-certifiées sur boîtes fermées, égalités conservées, census accepté
complet, propriétaire demi-ouvert unique. Rejeter une présentation ne
supprime pas S*. Le juge compare aussi les boules inertes.

[Témoin atteignable](../receipts/audit_independant_20261002/catalogue_boundary_work_review_4/README.md) :
coquille entière de 30 sites, 1 695 présentations de la boule centrale,
50 850 tests side par passe pour une émission. À 150 sites, 20 822 900
préfixes par passe ; à 270 sites, refus max_leaf=256 requis. Comptes
mathématiques, sans chrono ni causalité établie avec les temps LiDAR.

[Certificats de familles](../receipts/audit_independant_20261002/catalogue_family_contract_review_5/README.md) :
**tout L** cosphérique et support positif≤3 permet de couper tous les q4.
Si qmin4, le support canonique inclut le premier SiteIdx ; regrouper évite
C(m,4) présentations, pas la recherche tetra_support déjà ancrée ainsi.
Dans la boîte propriétaire K-certifiée, le certificat donne I=∅, U=L ;
coquille partielle ou centre extérieur ne le permettent pas. Préfixe
collinéaire, ou quatrième point dans la boule fermée d'un triplet, exclut
q4 strict ; conserver les préfixes obtus utiles. Coût, budgets et refus
à qualifier ; aucune hausse de max_leaf ou gain natif prévalidé.

## Propriétaire, capacité et futur parallélisme

[Cloud immuable](../receipts/audit_independant_20261002/cloud_immutable_review_3/README.md) :
stockage privé, entrée stable, restitution du delta et pic absolu reconnus.
Le futur index/tower doit conserver l'identité et la durée de vie du même
Cloud ; le déplacer vide l'objet initial, SiteIdx ne certifie pas son propriétaire.
Avec quatre buffers d'entrée vivants et n sites uniques, préparation seule :
B18/21=max(48n+H,60n+8), B24=80n+81920. À 30 M, environ 1,8/2,4 Go,
hors index/catalogue/FULL ; formules, aucune allocation géante ou mesure RSS.

[Deux passes](../receipts/audit_independant_20261002/catalogue_capacity_review_4/README.md) :
pic propre=max(W+T+E,E+F), workspace W, capacités DFS T, émissions/population E,
résultat F. Deux populations coexistent ; ajouter Cloud et réservations antérieures.
Après count, observer T puis admettre E et scratch fill ensemble éviterait
un refus tardif. Les sizeof incluent l'alignement.

[Raccord parallèle](../receipts/audit_independant_20261002/catalogue_parallel_contract_review_5/README.md) :
les listes des jobs se chevauchent, leur somme n'est pas bornée par n ;
capacité allouée suivant le parent, workspace suivant **max_leaf**, pas leaf_size.
Frontière et listes doivent rester possédées jusqu'au join. Count/fill par
ordinal, sommes u64 vérifiées et `population_begin` rebasé vers la population
globale ; le tri exact et les plateaux restent globaux. Comptabiliser frontière,
workspaces actifs, DFS et sorties coexistants. Aucun bug séquentiel ni gain
parallèle annoncé ; ces obligations précèdent le port.

Livrer core/cover à K fixé est cohérent et désormais explicite dans P4.
Une hiérarchie commune doit mesurer ses incompatibilités et pertes.
Le [massif](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md) reste secondaire après
le jalon trame. Les WIP ultérieurs ne reçoivent aucune qualification de ces copies.
