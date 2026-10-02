# Audit indépendant v11 — état courant et raccord FULL

2 octobre 2026. Dernière qualification recoupée : **MEB/census `25792084e`**,
publication `9a5fd6a61`. Index/numérique : `e8520481d` ; catalogue mesuré :
`ffc2ff95f`. Filtres de centres/FullDomain publiés à `7f1922c77`, qualification
native encore à venir. Cellules/localisation : WIP figé séparément, pas qualifié.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Aucun build, test produit ni GCP lancé par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les historiques et copies closes sont dans [les reçus](../receipts/audit_independant_20261002/README.md).

**La MEB et son raccord au census sont qualifiés aux trois profils. FULL natif,
GPU, contrat de temps et plusieurs dizaines de millions de points restent ouverts.**
Le catalogue LiDAR/K5 mono u21 qualifié prend encore 19,777–24,962 s ; K10 expire.
Priorité utile : conserver toutes les incidences, résoudre les traces à leur date,
dédupliquer les racines globales, puis construire les plateaux et verticales.

## Qualification MEB acquise, échecs conservés

[Recoupe MEB3](../receipts/audit_independant_20261002/meb_requalification_review_15/README.md) :
**1 266/1 266** portes, complément ASan18 **55/55** séparé ; Release B18,
ASan24, TSan21, profils21/24 et poison passent. Clang absent. 141 mutations :
136 par code, trois par ligne, deux refus de compilation core attendus ; les dix
mutants tower meurent par juge. Ce nombre ne se confond pas avec les portes CTest.
Paquet exact aux 2 578 fichiers Git257, manifeste d'archive complet, flags/hashes
binaires enregistrés, fermeture ciblée G4 et huit replays Python normal/−O recoupés.
Aucune certification nouvelle d'isolation temporelle ni de GPU.

**18/18 essais**, six entrées entières × trois profils, une répétition.
Les 864 premières requêtes sur parties choisies donnent 216 census complets et
648 saturés ; les six comparaisons interprofils sont identiques. Pour les trois
LiDAR sans sol u21, les sommes de 48 MEB valent 1,116–1,178 ms ; les wrappers
MEB+census 1,377–1,777 ms. Ce sont des requêtes artificielles, pas des descentes.
Le wrapper répète les calculs : 170 352 présentations initiales, **340 704 réelles**
avec cette répétition. Sa mémoire comprend les deux populations encore vivantes.
Processus LiDAR 49,833–67,324 ms, dont scan témoin 44,064–61,467 ms ; ce scan
partage num::side. L'oracle Fraction indépendant porte sur les petites fixtures.
Cloud/index sont chronométrés séparément ; préparation des parties payée dans le
processus, décodage Python séparé. Ne pas additionner les intervalles au wrapper.

[MEB1 échoué](../receipts/audit_independant_20261002/meb_qualification_review_13/README.md)
reste inchangé : source ab04, 1 254/1 266 +53/55, quatorze échecs uniquement IO,
zéro benchmark lancé et18 non joués. Le [correctif257](../receipts/audit_independant_20261002/meb_campaign_contract_review_12/README.md)
garde le Point renvoyé par anchor() avant d'emprunter ses coordonnées ;
[règle C++20 de durée de vie](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2012r0.pdf).
La garde « centre dans le domaine » était correcte ; aucun défaut mathématique
MEB établi. Conserver le diagnostic enfant complet lors d'un prochain refus IO.
MEB2 est un refus de capacité sans worker ni nouvelle génération certifiée ;
son reçu shutdown_uncertified et la vérification externe de l'ancienne cible
arrêtée restent distincts du succès MEB3.

## Numérique et frontières : lecture favorable, gardes ciblées

[Bornes par présentation](../receipts/audit_independant_20261002/native_arity_bounds_review_6/README.md) :
q1/q2/q4 natifs jusqu'à B24 ; q4 <72M⁵ pour tous les intermédiaires, M=2^B.
Q3 reste large en B21/24 : une annulation finale ne borne pas son premier produit.
Le tag porte l'arité construite, jamais qmin global. Level reste exact non réduit.
Le candidat q4 différé est qualifié à ffc : positivité→owner→I/U→S*→admission→Level.
Sur LiDAR, environ 99,7 % des niveaux candidats évités, **pas** autant du temps ;
les deux passes géométriques et pics mémoire restent identiques.

[MEB≤12](../receipts/audit_independant_20261002/meb_math_port_review_12/README.md) :
au plus793 présentations ; support positif **et inclusion de toute F** certifient
la MEB par M1. Un parcours arité puis tuple peut s'arrêter au premier accepté et
conserver le canonique local. La baseline exhaustive est maintenant qualifiée ;
arrêt anticipé et Q4Candidate→positivité→inclusion→materialize restent des pistes
à requalifier, sans gain acquis. Un préfixe q3 obtus peut porter un q4 positif.
La limite12 porte sur F, jamais sur la coquille ; une éventuelle coface13/K12
exigerait une voie distincte. Le support local n'est pas S* global.

[CenterRegion publié](../receipts/audit_independant_20261002/center_region_published_review_15/README.md) :
code numérique et DFS7f identiques à la [preuve14](../receipts/audit_independant_20261002/center_region_contract_review_14/README.md).
SAT exact de la fermeture, contacts hi=2^B conservés ; owner demi-ouvert décidé
séparément. Les triplets obtus prolongent q4. Le rejet degenerate est sûr **dans
ce DFS affinement indépendant** avec q2 séparé, pas une API générale disjoint.
864 droites/432 paires Fraction normal/−O, sans nouvelle qualification native.
Intermédiaires cubiques : i64 B18, i128 **avant** multiplication B21/24.
Le nouveau témoin séparé seulement par k=2 est correct. Une garde native explicite
B21 reste utile : a=0,b=(m,m,0),c=(m,0,m),Q=[0,1]³,m=2^B−1 donne
2m³−2m²>INT64MAX. Les entrées21 annoncées n'exercent pas ce dépassement ; le
code élargit déjà correctement. Dépasser i64 ne prouve pas seul une mort géométrique
de mutant : viser aussi budget/sanitizer. Tests/mutants déclarés attendent G4.

## Propriétaire FULL, recherche et refus

[FullDomain publié](../receipts/audit_independant_20261002/full_domain_published_review_15/README.md) :
index, catalogue et table possédés ensemble ; index transféré au **seul succès
final**. Le régime unitaire est certifié une fois ; les poids conservés par Cloud
sont explicitement refusés par ce moteur. Lookup égalité exacte des quatre SiteIdx,
padding compris ; hash seul ne décide rien. Un miss du support local ne prouve
pas l'absence globale. Canonicaliser depuis toute U du même Cloud ; un hit identifie
la boule, pas le nœud vivant à toutes les dates.

Pour B boules,C=pow2ceil(2B), table retenue4C octets ; C<=2^33 et offsets u64.
Sous pilote unique et réservations préexistantes stables, le pic propre reste Fcat :
l'assemblage a déjà payé Rcat+16B+4P, supérieur à Rcat+4C pour B>0. Une limite
juste sous ce dernier total refuserait le catalogue en amont, sans tester la table.
La nouvelle porte domain_fault mesure les allocations puis injecte effectivement
la dernière allocation système de table ; index/vues, budget et récupération
sont vérifiés. Les portes couvrent aussi collisions, padding, moves, résultats
coexistants, refus pondéré et lectures concurrentes. **Présentes et relues, non
exécutées par cet audit ; qualification G4 du contexte à venir.**

Les spans de stockage suivent un move ; une référence à l'objet index source
reste liée à l'objet vidé. Fixer durée de vie **et déplacement** du domaine jusqu'au
join ou à la consommation des résultats empruntés. L'admission commune des sorties
parallèles reste à organiser ; la concurrence qualifiée du census utilisait des
budgets privés. La cohérence des jobs ne découle pas du seul partage immuable.

## Cellules et localisation en cours : deux fixtures utiles

[Lecture WIP16](../receipts/audit_independant_20261002/full_locator_contract_review_16/README.md),
copies avant calcul, sans exécution native : le miss local reprend le census global
au seuil k puis S* ; l'absence positive n'est un invariant que si p+q<=K+1.
Le triangle avec p2/q3/K3 reste légitimement absent malgré census complet.
**Sur hit catalogue, complete peut avoir p>=k** : X={0,2,4,6},K3,k2,F={0,6}
rend la population complète connue p2. Ne déduire ni p<k ni fenêtre d'événement du
seul kind/hit ; le futur caller doit lire p ou distinguer la provenance du complet.

Les cellules classent la séparabilité par MEB(A)<lambda ; cette MEB(A) ne remplace
jamais la MEB(I union A) pour descendre. Elles stockent des traces exhaustives,
pas les composantes locales ou globales. Les six sommets d'un octaèdre unitaire
ont, à t=k=2, **12 traces strictes mais un seul morceau local** ; à t3, huit/huit.
Carré antipodal t2 : quatre/quatre ; tétraèdre strict t3 : quatre/quatre.
Résoudre à la date puis dédupliquer les racines avant le plateau est indispensable.

Deux passes exactes, capacités séparées et allocations des seules traces acceptées
sont favorables à la lecture. C(150,10)=1 169 554 298 222 310 reste représentable,
mais deux passages seraient impraticables si cette voie était atteinte : contrôle
analytique, pas observation LiDAR. Un quotient/raffinement **exhaustif** devra avoir
sa preuve et ses portes ; aucun plafond silencieux de coquille n'est permis.
Le pool WIP est synchrone ; ni son raccord ni la forêt FULL ne sont qualifiés ici.

## Temps catalogue, mémoire et contrats LiDAR

[Dernier banc catalogue](../receipts/audit_independant_20261002/q4_qualification_review_9/README.md),
source ffc : quinze succès K5, dix-huit délais30s, trois omissions ; K10 expire.
Trois LiDAR u21 sans sol : 19,777/24,962/23,101 s pour 08/100,0,200 ; uniforme
8k/16k 7,743/16,345 s,32k expire. Un essai, une séquence, CPU mono leaf16 ;
API deux passes/tri/sorties, hors masque/Cloud/sérialisation. Aucun temps FULL.
Rapports anciens/nouveaux1,033–1,053 non appariés, sans gain statistique établi.

[Pic reconstitué](../receipts/audit_independant_20261002/q4_cost_attribution_review_9/README.md) :
U+max(W+T+E,E+F), réservations coexistantes comptées. À LiDAR08/200 B21,
326,510 Mo au pic,154,031 Mo retenus ; réduire DFS seul ne réduit pas le pic
si les deux populations dominent. Buffer n'est pas RSS. Préflight sur nombres
réels de boules,niveaux,incidences et sorties retenues, jamais seuls les sites.

[Dimensionnement30/50 M](../receipts/audit_independant_20261002/index_capacity_port_review_10/README.md) :
leaf8,8 388 607/16 777 215 nœuds, profondeur23/24. Sous ABI Node=40 et B21,
Cloud+index+borne de huit sorties vaut2,136/3,671 Go, hors catalogue/FULL,
entrées,scratch/RSS. Une réponse complète peut contenir n IDs, pas O(K) ;
T résultats retenus coûtent jusqu'à4Tn octets d'IDs. Index construit une fois ;
census au pire O(n) par requête, nombre de requêtes FULL non borné. Aucune
allocation massive ni extrapolation de croissance acquise ; priorité trame entière.

[Précision physique](../receipts/audit_independant_20261002/precision_mapping_review_6/README.md) :
x=o+hq et a=h²beta. B seul ne change pas h ; mêmes XYZ/IDs à1mm dans18/21/24.
Un vrai affinement repart des coordonnées d'origine avec masque/repère/IDs communs,
collisions et fusions publiées. Portée par axe à1mm :262,143/2 097,151/16 777,215m.
La stabilité FULL en rayon ne stabilise pas premier-cover/LCA ; garder la couverture
dynamique et ses ambiguïtés avant toute projection et comparaison à HDBSCAN.
