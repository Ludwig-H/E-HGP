# Audit indépendant v11 — état des fondations

2026-10-02. Dernière source exécutée et qualifiée `e8520481d` ; reçus index
publiés `356cbdf88`. Mesures catalogue distinctes, source `ffc2ff95f`.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only` (défaut), `public_status=not_claimed`.
Sources figées, archives et contrôles autonomes entiers/Fraction ; aucun
nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Index global qualifié : 1 149/1 149 sélections, complément ASan B18
36/36 séparé.** Son banc termine 18/18 essais, sans descente ni FULL.
Le banc catalogue q4 conserve son échec : trois catalogues LiDAR sans
sol K5 prennent 19,777–24,962 s en B21 ; K10, FULL/GPU/100 ms et massif
restent ouverts. Prochain raccord : MEB exacte, semis de descente relevés
à leur date, incidences complètes et plateaux N-aires ; trois nouvelles
fixtures exactes donnent les attendus avant ce port. Le WIP MEB est maintenant
en construction ; sa qualification reste distincte de celle de l’index.

## Dernière qualification numérique, catalogue et index close

[Recoupe index1, source e852](../receipts/audit_independant_20261002/index_campaign_review_10/README.md) :
paquet identique octet pour octet à Git, 121 entrées d'archive vérifiées,
flags/cache/hashes enregistrés des binaires liés aux profils. Fermeture ciblée de VM et nettoyage
recoupés ; échecs et omissions conservés, aucune nouvelle GCP par l'audit.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 251/251 | 176 portes de base +75 références Python |
| GCC ASan/UBSan B24 | 176/176 | Références Python exclues |
| GCC TSan B21, profils B21 et B24 | 176/176 chacune | Références Python exclues |
| Poison B21 | 177/177 | Base +porte poison |
| Style B21 ; mutants | 2/2 ; 15/15 | Manifestes/campagnes séparés |

131 verdicts mutants : 126 par code, trois par ligne, deux refus de
construction attendus ; aucun signal/délai/INVALIDE. Modules core78,
num20, cloud16, catalogue9, index8. La configuration mutants est B18, mais neuf
mutants numériques déclarent explicitement B21/B24. Clang absent,
aucune qualification Clang. CPU sur G4 ne signifie pas GPU. La coquille qmin4/m5 et le croisement inter-K restent
joués ; celui-ci dans les deux références Python, sans FULL natif.

Complément du même paquet e852 : **17 portes num +17 index +2 style**.
Index : 845 contrôles natifs, 1 010 requêtes/36 020 contrôles Fraction par
profil normal/−O. Bornes num : 183 contrôles natifs et 391 cas Fraction.
Q3 natif B18 reste exercé ; le complément ne s'ajoute pas aux 1 149.

Groupes fermés entre matrice et banc, sans descendant tué résiduel ;
quiescence **entre configurations internes** encore `isolation=not_certified`.
Les [fondations a971](../receipts/audit_independant_20261002/g4_qualification_review_3/README.md)
et [catalogue e6](../receipts/audit_independant_20261002/catalogue_ablation_review_5/README.md)
restent des captures historiques distinctes, jamais réécrites.

## Temps du catalogue et attribution des coûts

Ces mesures restent celles du port q4 ffc, sans chronométrage FULL nouveau.

33 tentatives : quinze succès K5, dix-huit délais de processus 30 s ;
trois omissions 32k/K10. Dans chaque profil, uniforme32k/K5 et les cinq K10
tentés expirent. Un délai ne donne aucune durée finale. Les cinq cas K5
achevés ont les mêmes digests mathématiques et compteurs géométriques
déclarés en B18/21/24. Sorties retirées de VM : aucun rehash canonique ici.

| Catalogue CPU mono, leaf16/K5, mêmes XYZ/IDs à 1 mm | B18 | B21, défaut | B24 |
| --- | ---: | ---: | ---: |
| Uniforme 8k | 7,180 s | 7,743 s | 7,818 s |
| Uniforme 16k | 15,104 s | 16,345 s | 16,404 s |
| 08/000100 sans sol, 35 551 sites | 18,457 s | 19,777 s | 19,673 s |
| 08/000000 sans sol, 39 885 sites | 23,380 s | 24,962 s | 24,794 s |
| 08/000200 sans sol, 45 845 sites | 21,568 s | 23,101 s | 23,176 s |

Un essai par case, trois trames d'une seule séquence. Les quinze succès
gardent les tailles/SHA canoniques **enregistrés**, comptes et réservations
de profiles1/9df ; binaire différent. Rapports ancien/nouveau 1,033–1,053,
sans comparaison appariée ni preuve de gain stable. L'ancienne ablation
leaf32→16 reste dans son reçu, pas une nouvelle borne générale.
Durée API : deux passes, tri et sorties mémoire ; lecture/Cloud, masque,
préparation, sérialisation et décodage séparés ou exclus. Le décodage
sémantique reste hors API ; Buffer n'est pas RSS.

## Numérique : leviers exacts du prochain port

[Bornes par présentation](../receipts/audit_independant_20261002/native_arity_bounds_review_6/README.md) :
q1/q2/q4 natifs jusqu'à B24 ; q4 <72M⁵ pour toutes les sommes partielles,
M=2^B, sans supposer un centre dans le hull. Q3 <216M⁶ suffit en B18,
pas en B21/24 : un q3 qmin2 peut déborder au premier produit. Le tag
porte l'arité de présentation, jamais qmin. Factories/copieurs et refus
relus, couverture propre recoupée dans la dernière campagne e852/B18.
U32 exigera de reprendre les expressions, pas seulement Level.

Le [port du candidat q4](../receipts/audit_independant_20261002/q4_candidate_port_review_8/README.md)
est conforme à la lecture : ancre/N/D possédés, constructeur privé, tag4,
vue des coefficients interne et synchrone. Les cinq prédicats partagent
leurs noyaux ; Sphere publique reste complète, q2/q3 inchangés, fraction
q4 non réduite et anciens refus conservés. 17 103 contrôles statiques et
rationnels normal/−O, aucune nouvelle exécution native.

Le [raccord catalogue](../receipts/audit_independant_20261002/q4_catalogue_port_review_8/README.md)
garde inside→owner→census→S*→admission→Level→Collector ; q3 obtus continue
vers q4. Aucun niveau provisoire, PGCD ou réancrage ; aucun filtre de
famille porté. Les deux passes matérialisent les seuls supports qmin4
et comparent le ledger complet. Sur succès, q4_levels=#boules qmin4 ;
un refus Collector après matérialisation ne publie aucun catalogue.

[Travail, temps et mémoire du port](../receipts/audit_independant_20261002/q4_cost_attribution_review_9/README.md) :
q4_candidates=C et q4_levels=L4 comptent une passe ; count/fill identiques
donnent **2(C−L4)** niveaux évités par appel. Sur LiDAR, C=42–53 millions,
L4=121–158 milliers : environ 99,7 % évités, sans gain temporel de même
proportion. Les quinze pics Buffer sont inchangés. Ventiler génération,
positivité/census/canonicalisation, tri et assemblage avant d'attribuer
les secondes restantes. L'ancien plan profiles_g4 est retiré à d0dc.

[Identité pour le futur FULL](../receipts/audit_independant_20261002/ball_identity_contract_review_8/README.md) :
le tuple primitif signé (D,−2(Da+N),D||a||²+2N·a) identifie centre/rayon,
sans ID/arité/qmin/Level, dans un même repère et les mêmes unités.
Le terme constant est signé ; les types actuels couvrent les coefficients,
mais PGCD/division exacte/factory/format restent à porter et qualifier.
Cette clé ne réduit pas le Level public. Une circumsphère valide ne donne
ni la criticité ni la MEB ; garder les replis affines et obtus.

[MEB bornée à douze sites](../receipts/audit_independant_20261002/meb_bounded_contract_review_11/README.md) :
au plus 793 présentations d’arités1..4 ; inclusion de toute la partie obligatoire,
replis affines et obtus conservés. Le minimum exact est correct et unique ;
son support positif local peut différer de la première présentation minimisante
et de S* global. Pour une voie filtrant déjà les supports positifs, le premier
candidat contenant toute la partie certifie directement la MEB : arrêt anticipé
possible dans l’ordre arité puis tuple SiteIdx, sans gain natif acquis.
Les [conditions et la limite des cofaces13](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#q1-morceaux-raffinement-et-descente)
sont explicitées ; le Level seul ne constitue jamais une identité globale.

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

[Coquilles nombreuses](../receipts/audit_independant_20261002/catalogue_boundary_work_review_4/README.md) :
présentations répétées mesurées mathématiquement sur 30/150 sites,
refus max_leaf256 à 270 sites ; aucune causalité LiDAR ou chrono natif.

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

[Extension aux centres propriétaires](../receipts/audit_independant_20261002/q4_owner_line_witness_review_8/README.md) :
intersecter la droite c(u)=a+(N+u n)/D avec Q fermé. La puissance d'un
site devient affine en u ; strictement négative aux deux endpoints
certifie un intérieur commun, même hors du plan. Contacts conservés,
restriction à Q indispensable, branches q≤3 exhaustives. Les signes
atteignent 8B+9 bits : 153/177/201, donc 3/3/4 mots larges ; aucune
qualification i128 q4 héritée. 5 807 contrôles Fraction normal/−O, sans
port, feuille LiDAR qualifiée ou gain ; mesurer aussi le scan supplémentaire.

## Propriétaire, capacité et futur parallélisme

[Cloud immuable](../receipts/audit_independant_20261002/cloud_immutable_review_3/README.md) :
stockage privé, entrée stable, restitution du delta et pic absolu reconnus.
Le futur index/tower doit conserver l'identité et la durée de vie du même
Cloud ; le déplacer vide l'objet initial, SiteIdx ne certifie pas son propriétaire.
La préparation à 30 M avec entrées vivantes vaut environ 1,8/2,4 Go
pour B18/21 et B24 ; formule hors index/catalogue/FULL, pas une mesure.
Voir le [dimensionnement complet](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md).

[Port index et capacité](../receipts/audit_independant_20261002/index_capacity_port_review_10/README.md) :
index immuable une fois par Cloud, lignée commune au catalogue et aux jobs.
Le [port e852](../src/index/index.hpp) possède Cloud et nœuds ; déplacement
au seul succès, aucun pointeur vers l'objet appelant. Construction/
census favorables : refus préservent Cloud, plages disjointes, deux parcours
sans coquille allouée en saturation et I/U complets sinon. Il compte les
**sites**, même avec poids ; certifier le régime unitaire une fois au futur
raccord Catalogue/FULL. En asynchrone, posséder Sphere/K et un propriétaire
stable. Réponse discriminée : K SiteIdx distincts stricts prouve p≥K,
**ou** I/U complets et ordonnés avec p<K, pour ce compte de sites.
La coquille n'est pas bornée par K ; un refus ne publie aucun certificat.
Premier parcours comptant jusqu'à K et compte scalaire de coquille, puis
admission/remplissage exact ; sorties déjà croissantes, sans tri ni TLS.
Ne pas réutiliser L(Q) après sortie du centre de Q. Budget commun :
index/catalogue/FULL, scratch actif **et résultats terminés encore retenus** ;
T sorties complètes ont une enveloppe d'IDs 4Tn, pas O(TK).
Pour leaf8, 30/50 M sites donnent 8 388 607/16 777 215 nœuds, profondeur 23/24.
Sous l'ABI Node=40 du banc, Cloud+index+borne de huit sorties vaut 2,136/3,671 Go,
hors catalogue/FULL/entrées/scratch/RSS. Calcul analytique, aucune allocation
massive ni performance extrapolée ; construction O(n) après Cloud.
L'API actuelle ne fournit pas de ticket de comptage pour une admission commune
avant fill parallèle : sérialiser, préadmettre 4Tn ou ajouter un ticket opaque
lié à propriétaire/Sphere/K. Concurrence G4 : budgets privés par fil.

[Banc index1](../receipts/audit_independant_20261002/index_campaign_review_10/README.md) :
18/18 essais, six entrées entières, 64 requêtes choisies par entrée/profil,
1 152 réponses comparées au scan num::side. Ce scan juge le parcours,
pas indépendamment l'arithmétique ; les petites portes utilisent Fraction.
Sur trois LiDAR sans sol B21 : arbre 0,341–0,418 ms, **64** census cumulés
0,621–0,810 ms ; Cloud/factories/scan séparés. Une répétition, une séquence,
aucune MEB/descente/catalogue/FULL dans ce chrono. Construction O(n) après
Cloud, census au pire O(n) par requête : nombre de requêtes FULL non borné.

[Bornes de census et WIP séparé](../receipts/audit_independant_20261002/global_census_bounds_review_9/README.md) :
pour F(x)=DΣδ²−2ΣNδ sur une boîte fermée, minima quadratiques tenant compte
de zéro et extrema linéaires donnent LB≤F≤UB. Rejet extérieur seulement
LB>0, admission stricte seulement UB<0 ; égalités conservées. **Minimum des
coins invalide** : boule centre(1,1,1)/rayon1 et boîte[0,2]³, coins extérieurs,
centre intérieur. num::Box valide ses deux Points, hi<2^B ; la boîte de centres
demi-ouverte peut avoir hi=2^B et ne lui est pas interchangeable. Les bornes
gardent les budgets d'arité : q3 large B21/24, q4 natif jusqu'à B24, sans
degré dix. Le WIP figé dans la capsule précède e852 ; port LB/UB désormais
qualifié dans index1. Les deux contrats de boîtes restent distincts.

[Option lattice exacte](../receipts/audit_independant_20261002/index_lattice_bounds_review_10/README.md) :
arrondir exactement le centre par axe puis clamper donne le minimum sur les
sites entiers de la boîte ; un coin le plus loin donne le maximum continu.
Préparation i128, deux puissances aux budgets existants, aucun N². Le minorant
**discret ne remplace pas le contrat continu de power_bounds**. Contrôles
Fraction aux trois profils seulement, pas de vraie boîte de descente ni gain
natif ; mesurer coût total avant port. MEB/FULL reste prioritaire.

[Deux passes](../receipts/audit_independant_20261002/catalogue_capacity_review_4/README.md) :
pic propre=max(W+T+E,E+F), workspace W, capacités DFS T, émissions/population E,
résultat F. Deux populations coexistent ; ajouter Cloud et réservations antérieures.
Après count, observer T puis admettre E et scratch fill ensemble éviterait
un refus tardif. Les sizeof incluent l'alignement.

[Attribution des quinze pics](../receipts/audit_independant_20261002/q4_cost_attribution_review_9/README.md) :
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
