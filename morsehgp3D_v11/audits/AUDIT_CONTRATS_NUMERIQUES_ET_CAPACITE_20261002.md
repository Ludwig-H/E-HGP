# Audit indépendant v11 — état des fondations

2026-10-02. Source qualifiée `9df774947`, port numérique `9d639e146`,
complément numérique ASan/UBSan B18 `d77e4b77c`, publié dans `099886ed6`.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only` (défaut), `public_status=not_claimed`.
Sources figées, archives et contrôles autonomes entiers/Fraction ; aucun
nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Nouvelle matrice conforme : 1 002/1 002 portes ; campagne de temps en échec.**
Les trois catalogues LiDAR sans sol K5 prennent 20,551–25,847 s en B21.
K10 expire ; FULL, GPU, 100 ms et massif restent ouverts. Le prochain travail
utile porte sur les rejets q4 avant calcul du niveau et la coexistence des
émissions avec le résultat. Aucun gain de ces propositions encore acquis.

## Qualification numérique et catalogue actuelle

[Recoupe profiles1, source 9df](../receipts/audit_independant_20261002/profiles_capture_review_7/README.md) :
paquet identique octet pour octet à Git, 106 entrées d'archive vérifiées,
flags/cache/binaires liés aux profils. Fermeture ciblée de VM et nettoyage
recoupés ; échecs et omissions conservés, aucune nouvelle GCP par l'audit.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 227/227 | 152 portes de base +75 références Python |
| GCC ASan/UBSan B24 | 152/152 | Références Python exclues |
| GCC TSan B21, profils B21 et B24 | 152/152 chacune | Références Python exclues |
| Poison B21 | 153/153 | Base +porte poison |
| Style B21 ; mutants | 2/2 ; 12/12 | Manifestes/campagnes séparés |

116 verdicts mutants : 111 par code, trois par ligne, deux refus de
construction attendus ; aucun signal/délai/INVALIDE. Modules core78,
num13, cloud16, catalogue9. La configuration mutants est B18, mais quatre
mutants numériques déclarent explicitement B21/B24. Clang absent,
aucune qualification Clang. CPU sur G4 ne signifie pas GPU. La coquille qmin4/m5 et le croisement inter-K restent
joués ; celui-ci dans les deux références Python, sans FULL natif.

[Complément ASan/UBSan B18](../receipts/audit_independant_20261002/profiles_u18_sanitize_review_7b/README.md) :
13/13 portes num et style, source d77 sans changement de src/tests C++.
Q3 natif réellement exercé par power et side, 207 contrôles de voies et deux
oracles Fraction de 7 526 contrôles chacun. Il complète ASan B24 ; aucune
nouvelle matrice entière ni mesure de catalogue/FULL n'en est déduite.

Groupes fermés entre matrice et banc, sans descendant tué résiduel ;
quiescence **entre configurations internes** encore `isolation=not_certified`.
Les [fondations a971](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md)
et [catalogue e6](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md)
restent des captures historiques distinctes, jamais réécrites.

## Temps actuels et attribution des coûts

33 tentatives : quinze succès K5, dix-huit délais de processus 30 s ;
trois omissions 32k/K10. Dans chaque profil, uniforme32k/K5 et les cinq K10
tentés expirent. Un délai ne donne aucune durée finale. Les cinq cas K5
achevés ont les mêmes digests mathématiques et compteurs géométriques
déclarés en B18/21/24. Sorties retirées de VM : aucun rehash canonique ici.

| Catalogue CPU mono, leaf16/K5, mêmes XYZ/IDs à 1 mm | B18 | B21, défaut | B24 |
| --- | ---: | ---: | ---: |
| Uniforme 8k | 7,420 s | 8,066 s | 8,090 s |
| Uniforme 16k | 15,619 s | 16,920 s | 17,076 s |
| 08/000100 sans sol, 35 551 sites | 19,440 s | 20,551 s | 20,508 s |
| 08/000000 sans sol, 39 885 sites | 24,524 s | 25,847 s | 25,915 s |
| 08/000200 sans sol, 45 845 sites | 22,674 s | 24,051 s | 24,094 s |

Un essai par case, trois trames d'une seule séquence. En B18, cinq succès
communs gardent les mêmes tailles/SHA canoniques, comptes et réservations
que leaf16/e6 ; binaire différent. Les différences de temps entre campagnes
ne sont pas une preuve causale ni une comparaison statistique appariée.
L'ablation leaf32→16 reste dans son reçu, pas une nouvelle borne générale.
Durée API : deux passes, tri et sorties mémoire ; lecture/Cloud, masque,
préparation, sérialisation et décodage séparés ou exclus. Le décodage
sémantique prend 4,89–10,17 s supplémentaires ; Buffer n'est pas RSS.

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
le signe natif sûr. Portes Fraction, voie Wide de harnais et nouvelles mutations sont
désormais recoupées dans profiles1 ; leurs profils restent distincts.
B18 reste explicite ; la couverture q3 native ASan est dans le complément
B18 distinct, jamais héritée de la seule configuration B24.

Le [contrat du candidat q4](../receipts/audit_independant_20261002/q4_candidate_contract_review_7/README.md)
précise le niveau tardif : objet privé centre/ancre/N/D, sans Level
provisoire publiable, mêmes prédicats exacts partagés avec Sphere.
Garder **strictly_inside avant propriétaire/census/judged** : le tétraèdre
rectangle peut avoir une circumsphère valide, centre extérieur, p=0 et
m=q=4 ; sans ce test, canonical_support sauté émettrait une fausse boule
critique. Maintenir factories publiques complètes et refus actuels ; vrai
niveau avant count/fill/tri/rangs, calculé dans les deux passes admises.
Q3 garde sa formule réduite, pas un numérateur générique hors budget.
Mesurer constructions et calculs de niveau évités, puis phases sur G4.
La factory actuelle calcule encore le niveau : proposition, aucun gain acquis.

## Profils compilés et précision physique

[Le banc](../receipts/audit_independant_20261002/profile_benchmark_review_6/README.md)
utilise les mêmes XYZ/IDs à 1 mm dans B18/21/24. SHA/cache du binaire qualifié
vérifiés avant mesures ; digest normalisé garde niveaux rationnels, S*, I/U
et rangs. C'est une comparaison d'encodages, pas un oracle géométrique.
API/processus/décodage sont séparés ; refus/délais et checkpoint avant
décodage préservent les essais incomplets. Cinq comparaisons complètes
sont désormais recoupées, sept incomplètes ; aucun changement de h.

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

[Nouveau minorant commun au triplet](../receipts/audit_independant_20261002/q4_level_math_review_7/README.md) :
les témoins distincts **coplanaires et strictement intérieurs au disque**
d'un triplet non collinéaire sont intérieurs à toutes ses extensions q4.
Leur compte c>K−3 suffit à couper les présentations q4, même pour un
triplet obtus. Les branches q≤3 restent exhaustives : qmin peut être moindre.
Exemple : T={(10,5,5),(2,9,5),(2,1,5)}, témoin (5,5,5), quatrième
site (5,5,11). À K3, une présentation q4 est coupée, le q3 reste admis ;
à K4, le q4 redevient admissible. Le compte reste séparé du census q3
(seuil K−2) et des témoins hors plan ; les points frontière ne créditent rien. Sous-ensemble sûr suffit
au rejet, sans seed ni ajout au census final. Réutiliser un parcours existant
et mesurer le coût total ; un nouveau scan par triplet pourrait déplacer le
coût. Quatre sommets dans Q fermé et centre q4 strict dans leur hull
donnent l'owner demi-ouvert, jamais la population I/U.

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

[Attribution des quinze pics](../receipts/audit_independant_20261002/profile_memory_phase_review_7/README.md) :
ils égalent exactement Cloud+E+F, pendant l'assemblage. Pour n retours uniques,
N boules, L niveaux zéro compris, P incidences : U=28n+8,
E=e_B N+4P, F=40N+l_B L+4P+8. Layouts reconstitués et corroborés :
(e_B,l_B)=(96,48)/(104,64)/(112,72) pour B18/21/24, aucune exécution sizeof.
B21−B18 ajoute 8N+16L au pic ; B24−B21 ajoute 8N+8L. À 08/000200/B21,
pic 326,510 Mo, conservé 154,031 Mo, temporaire 172,479 Mo dont seulement
26,059 Mo de population ancienne. Réduire DFS seul ne réduit pas ce pic
si E+F domine encore. Toute suppression de copie doit conserver CSR et
I/U complets après tri ; préflight massif sur N,L,P et coexistence réelle.

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
