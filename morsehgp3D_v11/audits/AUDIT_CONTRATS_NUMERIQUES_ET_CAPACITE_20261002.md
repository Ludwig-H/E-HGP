# Audit indépendant v11 — état des fondations

2026-10-02. Dernière source exécutée et qualifiée `ffc2ff95f` ; reçus
publiés `8671d6ac3`. Capsules closes jusqu'au plan `d0dc9cd8b` ; lecture
initiale du nouvel index publié `e8520481d`, sans qualification héritée.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only` (défaut), `public_status=not_claimed`.
Sources figées, archives et contrôles autonomes entiers/Fraction ; aucun
nouveau build, test produit ou GCP par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Port q4 différé qualifié : 1 014/1 014 sélections, complément ASan B18
14/14 séparé.** Le banc conserve son échec : trois catalogues LiDAR sans
sol K5 prennent 19,777–24,962 s en B21 ; K10, FULL/GPU/100 ms et massif
restent ouverts. L'index global vient d'être publié à e852 : saturation
stricte ou census complet, sans perdre la coquille. Lecture initiale favorable,
qualification G4 en préparation, aucune nouvelle qualification native acquise.

## Dernière qualification numérique et catalogue close

[Recoupe q4levels1, source ffc](../receipts/audit_independant_20261002/q4_qualification_review_9/README.md) :
paquet identique octet pour octet à Git, 121 entrées d'archive vérifiées,
flags/cache/hashes enregistrés des binaires liés aux profils. Fermeture ciblée de VM et nettoyage
recoupés ; échecs et omissions conservés, aucune nouvelle GCP par l'audit.

| Configuration | Portes conformes | Portée |
| --- | ---: | --- |
| GCC Release B18 | 229/229 | 154 portes de base +75 références Python |
| GCC ASan/UBSan B24 | 154/154 | Références Python exclues |
| GCC TSan B21, profils B21 et B24 | 154/154 chacune | Références Python exclues |
| Poison B21 | 155/155 | Base +porte poison |
| Style B21 ; mutants | 2/2 ; 12/12 | Manifestes/campagnes séparés |

119 verdicts mutants : 114 par code, trois par ligne, deux refus de
construction attendus ; aucun signal/délai/INVALIDE. Modules core78,
num16, cloud16, catalogue9. La configuration mutants est B18, mais sept
mutants numériques déclarent explicitement B21/B24. Clang absent,
aucune qualification Clang. CPU sur G4 ne signifie pas GPU. La coquille qmin4/m5 et le croisement inter-K restent
joués ; celui-ci dans les deux références Python, sans FULL natif.

Complément du même paquet ffc : **12 portes num +2 style**, candidat privé
831 contrôles, power_paths 207 et Fraction 11 838 contrôles dans chacun des
modes normal/−O. Q3 natif B18 reste réellement exercé ; le complément
ne s'ajoute pas au compteur 1 014 de la matrice principale.

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
relus, couverture propre recoupée dans la dernière campagne ffc/B18.
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

[Contrat du raccord index](../receipts/audit_independant_20261002/global_index_contract_review_9/README.md) :
index immuable une fois par Cloud, lignée commune au catalogue et aux jobs.
Le [port e852](../src/index/index.hpp) possède Cloud et nœuds ; déplacement
au seul succès, aucun pointeur vers l'objet appelant. Lecture construction/
census favorable : refus préservent Cloud, plages disjointes, deux parcours
sans coquille allouée en saturation et I/U complets sinon. Il compte les
**sites**, même avec poids ; certifier le régime unitaire une fois au futur
raccord Catalogue/FULL. En asynchrone, posséder Sphere/K et un propriétaire
stable. Réponse discriminée : K SiteIdx distincts stricts prouve p≥K,
**ou** I/U complets et ordonnés avec p<K, pour ce compte de sites.
La coquille n'est pas bornée par K ; un refus ne publie aucun certificat.
Découverte avec témoins bornés et compte scalaire de coquille, puis admission
et remplissage exact si p<K : deux parcours et tri payés, sans capacité TLS
héritée. Ne pas réutiliser L(Q) après sortie du centre de Q. Budget commun :
index/catalogue/FULL, scratch actif **et résultats terminés encore retenus** ;
T sorties complètes ont une enveloppe d'IDs 4Tn, pas O(TK).

[Bornes de census et WIP séparé](../receipts/audit_independant_20261002/global_census_bounds_review_9/README.md) :
pour F(x)=DΣδ²−2ΣNδ sur une boîte fermée, minima quadratiques tenant compte
de zéro et extrema linéaires donnent LB≤F≤UB. Rejet extérieur seulement
LB>0, admission stricte seulement UB<0 ; égalités conservées. **Minimum des
coins invalide** : boule centre(1,1,1)/rayon1 et boîte[0,2]³, coins extérieurs,
centre intérieur. num::Box valide ses deux Points, hi<2^B ; la boîte de centres
demi-ouverte peut avoir hi=2^B et ne lui est pas interchangeable. Les bornes
gardent les budgets d'arité : q3 large B21/24, q4 natif jusqu'à B24, sans
degré dix. Le WIP figé dans la capsule précède e852 ; lecture favorable
du port LB/UB, sans qualification native de ces opérations ou de l'index.
L'option plus serrée
UB*=Σmax(F_axis(lo),F_axis(hi)) reste une possibilité, sans gain acquis.

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
