# Audit indépendant v11 — état des fondations

2026-10-02 13:44 UTC. Catalogue exécuté `e6fe34cb0`, publication `3e7b52b43` ;
fondations qualifiées sur G4 à `a97180667`, clôturées à `6a22a9118`.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Sources figées, recoupe d'archives et contrôles autonomes entiers/Fraction ;
aucun nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Catalogue exact relu favorablement et qualifié à e6 ; coût encore trop élevé.**
Leaf16 conserve la sortie déclarée à 8k/K5 et permet trois catalogues LiDAR
sans sol K5 achevés en 20–26 s. FULL est absent ; aucun contrat FULL/GPU/100 ms
ou massif acquis. Aucun nouveau défaut séquentiel établi. Les gains proposés
ci-dessous gardent les coquilles complètes et leurs incidences.

## Qualification fonctionnelle courante

Les [fondations a971](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md)
sont complétées par [catalogue3, source e6](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md).
960/960 portes conformes ; campagne de temps distincte en échec. Les premiers
échecs restent dans leurs reçus, sans encombrer l'état courant.

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

Interruption globale corrigée et hashes exécutables/cache/flags capturés.
Fermeture des groupes de matrice et de banc et arrêt ciblé recoupés.
La quiescence de descendance **entre configurations internes de la matrice**
reste déclarée `isolation=not_certified` ; aucune fuite de VM déduite.

## Temps : réduction observée et déplacement du travail

[Pièces et lecteur autonome clos](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md) :
13 tentatives, sept succès, six délais de processus 30 s et 23 omissions.
Les cinq K10 tentés expirent ; uniforme32k/K5 expire, son K10 n'est pas joué.
Un délai ne donne aucune durée finale du catalogue.

| Catalogue CPU mono achevé, leaf16/K5 | Durée | Pic Buffer, Cloud compris |
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

[Prédicats et bornes](../receipts/audit_independant_20261002/numeric_geometry_review_3/README.md) :
196 contrôles autonomes normal/−O, aucun défaut trouvé jusqu'à B24 ;
centres hors boîte couverts, q3 obtus prolongé en q4 strict conservé.
[Entiers et niveaux](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md) :
narrow et refus transactionnels cohérents, largeurs Level suffisantes.
`Sphere::through` construit une circumsphère, sans certification critique.
Le futur u32 exige de reprendre les expressions natives, pas seulement Level.

[Preuve i128 et niveau tardif](../receipts/audit_independant_20261002/catalogue_native_side_review_5/README.md) :
M=2^B, |v_i|<M, D<24M⁴, |N_i|<24M⁵. La puissance évaluée
D||v||²−2ΣN_i v_i a tous ses produits et sommes partielles <216M⁶.
À B18, <2^116<2^127 : le census peut employer i128 signé, même hors hull.
À B21, un triangle **strictement aigu** donne un premier produit ≥2^127
alors que le résultat final tient ; garder les voies 21/24 larges.
Ces preuves justifient le port, sans qualifier du code encore absent.

Centre/ancre exacts suffisent à propriété, census et canonical_support.
Retarder Level q4 jusqu'à l'émission est possible avec un **type interne
sans niveau**, en préservant le Sphere public cohérent. Aucun Level nul
provisoire publiable ; tout vrai niveau précède count/fill/tri/rangs.
Comparer sorties, refus et travail discret sur G4, puis mesurer les niveaux
réellement évités. Les filtres flottants restent une tranche distincte.

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
si **tout L** est cosphérique et un support positif≤3 existe, aucun q4 de
L n'est canonique : couper l'arité entière et garder les q2/q3. Si qmin4,
sa présentation canonique inclut le premier SiteIdx ; regrouper la famille
évite C(m,4) présentations. La recherche tetra_support actuelle bénéficie
déjà de cette ancre : le gain concerne l'énumération/collecte répétée.
Lorsque le centre appartient à la boîte propriétaire K-certifiée, le
certificat complet donne I=∅ et U=L, sans refaire le census de cette boule.
Ne pas extrapoler à une coquille partielle ou un centre extérieur.
Un triplet collinéaire n'a aucune extension q4 stricte ; pour un triplet
non collinéaire, un quatrième point dans sa boule fermée a un poids q4≤0.
Ces coupures conservent les préfixes obtus utiles. Port, coût de détection,
budgets et refus à qualifier ; aucune hausse de max_leaf prévalidée.

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
