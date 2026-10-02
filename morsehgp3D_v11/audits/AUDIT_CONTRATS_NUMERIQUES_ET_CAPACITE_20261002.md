# Audit indépendant v11 — état courant et raccord FULL

2 octobre 2026. Dernière qualification recoupée : **CenterRegion/FullDomain `7f1922c77`**.
MEB/census : `25792084e` ; index : `e8520481d`. Catalogue mesuré à 7f, banc
interrompu. Catalogue parallèle/Pool/cellules publiés à `9c883b93f`,
descente datée/diagnostics à `2e3af233f` ; forêt/verticales WIP sur `a7cd34ee2`
relues séparément. Aucune qualification native de ces nouveaux ports acquise ici.
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `mode=audit_v11_full_and_parallel_contracts`,
`public_status=not_claimed`.
Aucun build, test produit ni GCP lancé par cet audit. Deux notes actives :
celle-ci et [les verrous mathématiques](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les historiques et copies closes sont dans [les reçus](../receipts/audit_independant_20261002/README.md).

**La MEB et son raccord au census sont qualifiés aux trois profils. FULL natif,
GPU, contrat de temps et plusieurs dizaines de millions de points restent ouverts.**
Les deux catalogues LiDAR/K5 mono u21 persistés dans le dernier banc prennent
17,374 et 21,734 s ; aucun succès K10 persisté. La campagne complète est en échec.
Priorité utile : conserver toutes les incidences, résoudre les traces à leur date,
dédupliquer les racines globales, puis construire les plateaux et verticales.

## Difficultés actuelles du développeur — diagnostic du 2 octobre

[Recoupe des sessions](../receipts/audit_independant_20261002/developer_blockers_evidence_review_18/README.md) :
parallel1/2 ont refusé le démarrage faute de capacité en zone b, sans worker.
parallel3 a démarré en zone c mais aucun test natif n'a été joué : compilateur,
CMake et CTest absents avant configuration. **Bootstrap maintenant traité** :
tools1/a7 installe GCC 11.4, CMake/CTest 3.22.1 et Make 4.3, puis ferme la cible.
Ce succès d'outillage ne qualifie aucun produit ; reprendre la matrice prévue.

**Coût d'outillage évitable :** le paquet tools1 transporte 112,18 Mo compressés,
142,87 Mo décompressés, dont 141,12 Mo de reçus historiques (98,77 %).
Nos archives d'audit y contribuent aussi. Préparer un paquet de sources/harnais
requis, depuis Git avec sélection et manifeste exhaustifs explicites ; conserver
les preuves séparément. Vérifier les dépendances avant d'exclure un sous-arbre,
sans réécrire les anciens paquets ni leur qualification. Aucun gain mesuré ici.

[Diagnostics du parallèle](../receipts/audit_independant_20261002/parallel_diagnostics_review_18/README.md) :
un premier échec W48/K5 supprime actuellement son W8. Garder un W8 indépendant
et un W1 apparié si le budget le permet ; l'issue W48 ne certifie pas celle W8.
Les nouveaux murs prefix/replay/tri/scan/assemblage et somme/max des tâches
permettent d'étudier la partie séquentielle et le déséquilibre. Publier quelques
ordinaux lourds avec les compteurs déjà possédés, sans rescanner le Cloud.
Sommes de tâches = fenêtres murales, pas CPU ; pic Buffer global =/= pic par phase.

Ajouter au lecteur la garde `task_sum <= min(W,J) * phase_wall` : le témoin
W8/J256, mur 100 ms/max 100 ms/somme 900 ms satisfait les contrôles temporels
actuels mais dépasse la borne de 800 ms. Modèle scalaire de ces relations seulement,
pas dump géométrique complet ni mauvais compteur natif constaté. Préserver le
pic public ; un restart_peak par phase ferait perdre la mesure de toute l'API.

**Levier mathématique concret dans la forêt WIP :** classify construit puis
jette toutes les traces avant leur reconstruction dans cell. Classifier sans
Buffer, au premier témoin strict, puis garder le rejeu exhaustif. Sur l'octaèdre
avec centre, ordre 3 : 31 tests/31 MEB(A) au lieu de 60, une allocation de 624 octets
au lieu de deux successives. Les deux buffers actuels ne coexistent pas.
[Preuve et fixture](../receipts/audit_independant_20261002/dated_descent_forest_review_18/README.md) :
12 naissances à 2, fusion à douze enfants à 8/3, continuation à 4. Ni gain temporel ni nouvelle
borne globale acquis. Ce conseil réduit du travail réellement répété, sans
restaurer le quotient local quadratique de la v10.

## Dernière qualification et interruption : deux statuts distincts

[Recoupe region1](../receipts/audit_independant_20261002/center_domain_qualification_review_17/README.md) :
source 7f, paquet exact aux 2 617 blobs Git, archive de 122 fichiers/manifeste de 121,
**1 398/1 398** portes, complément ASan18 **73/73** séparé. GCC Release18,
ASan24, TSan21, profils21/24 et poison passent ; Clang absent. 154 mutations
recoupées, dont 149 par code, trois par ligne et deux refus de compilation core.
Cette qualification porte sur les primitives de centres et le propriétaire FULL,
avec les briques antérieures ; elle ne porte pas sur la forêt ni le nouveau port 9c.

Banc interrompu à 820,003 s/code 124 : **14 succès K5, 15 délais persistés,
2 omissions K10 causales et 5 cases sans résultat persistant**. Leur lancement
est inconnu ; ne pas les déclarer non jouées. Groupe fermé avec résidu tué : 1,
arrêt ciblé G4 certifié ; isolation des chronos distincte. Le lecteur garde
correctement le banc en échec, y compris après son renforcement reconnu.

**P2 de traçabilité traité dans la source 2e/a7 :** les collecteurs persistent
l'intention, argv, profil et hashes XYZ/IDs avant subprocess.run ; la sonde
factice sans processus le confirme. PID indisponible, et intention ne prouve pas
spawn. Les cinq lancements inconnus de region1 restent inconnus. Les délais
natifs, le décodage Python et le collecteur sont toujours payés séparément.

## Qualification MEB acquise, échecs conservés

[Recoupe MEB3](../receipts/audit_independant_20261002/meb_requalification_review_15/README.md) :
source 257, 1 266/1 266 +ASan18 55/55, 141 mutants et 18/18 essais conformes.
Les 48 MEB+census artificiels LiDAR prennent 1,377–1,777 ms, sans mesurer une
vraie descente. Le wrapper répète les MEB : 340 704 présentations réelles,
populations coexistantes payées. Scan témoin partage num::side ; Fraction juge
indépendamment les petites fixtures. Processus/décodage distincts, aucun FULL.

[MEB1 échoué](../receipts/audit_independant_20261002/meb_qualification_review_13/README.md)
reste clos : seuls IO échouaient, ancre temporaire du sérialiseur corrigée à 257,
aucun défaut MEB établi. MEB2 refusait la capacité sans worker ; ses clôtures
restent distinctes. Archives, premières interruptions et échecs conservés.

## Numérique et frontières : lecture favorable, gardes ciblées

[Bornes par présentation](../receipts/audit_independant_20261002/native_arity_bounds_review_6/README.md) :
q1/q2/q4 natifs jusqu'à B24 ; q4 <72M⁵ pour tous les intermédiaires, M=2^B.
Q3 reste large en B21/24 : une annulation finale ne borne pas son premier produit.
Le tag porte l'arité construite, jamais qmin global. Level reste exact non réduit.
Le candidat q4 différé est qualifié à ffc : positivité→owner→I/U→S*→admission→Level.
Sur LiDAR, environ 99,7 % des niveaux candidats évités, **pas** autant du temps ;
les deux passes géométriques et pics mémoire restent identiques.

[MEB≤12](../receipts/audit_independant_20261002/meb_math_port_review_12/README.md) :
au plus 793 présentations ; support positif **et inclusion de toute F** certifient
la MEB par M1. Un parcours arité puis tuple peut s'arrêter au premier accepté et
conserver le canonique local. La baseline exhaustive est qualifiée ;
**arrêt au premier support strict contenant porté à 2e**, encore non qualifié.
Q4Candidate→positivité→inclusion→materialize reste une piste distincte, sans gain acquis. Un préfixe q3 obtus peut porter un q4 positif.
La limite12 porte sur F, jamais sur la coquille. La forêt par traces critiques
reconstruit les boules depuis S*≤4 et évite MEB13 : la fixture K12/treize sites
est contrôlée dans la revue18. Cela ne qualifie pas encore FULL K12 natif.
Le support local n'est pas S* global.

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
code élargit déjà correctement. **Garde region_cubic_width ajoutée au WIP** :
14 contrôles, les six permutations et les deux boîtes. Tests/mutants 7f passent
dans region1 ; cette nouvelle garde reste non qualifiée. Dépasser i64 ne prouve
pas seul une mort géométrique de mutant : viser aussi budget/sanitizer.

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
coexistants, refus pondéré et lectures concurrentes. **Ces portes à 7f passent sur
G4 ; reçus recoupés, aucune exécution native par cet audit.** Le port 9c reste distinct.

Les spans de stockage suivent un move ; une référence à l'objet index source
reste liée à l'objet vidé. Fixer durée de vie **et déplacement** du domaine jusqu'au
join ou à la consommation des résultats empruntés. L'admission commune des résultats de requêtes FULL concurrentes reste à
qualifier ; la concurrence qualifiée du census utilisait des budgets privés. La cohérence des jobs ne découle pas du seul partage immuable.

## Cellules/localisation publiées à 9c : gardes et fixture nouvelle

[Lecture initiale16](../receipts/audit_independant_20261002/full_locator_contract_review_16/README.md) et
[delta publié9c](../receipts/audit_independant_20261002/cells_locate_contract_review_17/README.md), sans natif : le miss local reprend le census global
au seuil k puis S* ; l'absence positive n'est un invariant que si p+q<=K+1.
Le triangle avec p2/q3/K3 reste légitimement absent malgré census complet.
**Sur hit catalogue, complete peut avoir p>=k** : X={0,2,4,6},K3,k2,F={0,6}
rend la population complète connue p2. Ne déduire ni p<k ni fenêtre d'événement du
seul kind/hit. **Le développeur a ajouté cette garde dans tests et documentation à 9c** ;
sa qualification native reste pendante. Lire p pour la descente.

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
Une nouvelle fixture protège canonical.cpp : X Morton=(5,5,0),(2,1,5),(10,5,5),
(2,9,5),(5,9,8) ; F=(0,1,2,4),K4. MEB locale q4,c=(5,5,5),beta25 ; globalement
p0,U5,S*=(1,2,3),qmin3 **sans premier U**. Attendu : miss local, census complet : 20
octets, hit global q3. Le code parcourt correctement tous les triplets ; **garde ajoutée au WIP**
(global_identity, clé(1,2,3,None), payload 20, arité locale 4/globale 3).
Le lemme d'ancrage ne vaut que sous qmin4. La revue18 distingue ces portes
préparées de la qualification native encore pendante du Pool/raccord/forêt.

## Catalogue parallèle à 9c : deux conseils avant qualification

[Lecture et comparaison critique R2](../receipts/audit_independant_20261002/catalogue_parallel_contract_review_17/README.md),
[code rapproché de 9c](../receipts/audit_independant_20261002/PUBLICATION_BINDINGS_17.json) :
frontière possédée de boîtes de centres, listes recouvrantes ; suffixes sans refaire
les préfixes, count/fill par ordinal, offsets u64 et rebasing population_begin,
tri exact global et refus transactionnels sont favorables à la lecture.
Si J<W, scratch par ordinal ; sinon par worker. Conserver ce choix : un worker
quelconque peut recevoir l'unique job. Tous les workspaces sont préadmis avant workers.

**P1 admission conservatrice :** la surcharge Pool, W1 compris, préadmet une
frontière de profondeur 8 à **1064n octets supplémentaires** : 31,92/53,2 Go pour
30/50 M sites. Ce n'est pas un pic atteint. Sous 8 Gio avec Cloud unitaire vivant,
cette première porte impose n<=7 866 240, avant sorties/scratch/autres réservations.
Choisir d suivant l'admission conjointe frontière+DFS+workspaces, puis terminer
les suffixes exhaustifs ; ou count/replay pour admettre les capacités réelles en
payant ce préambule. Réduire d peut augmenter les suffixes : mesurer les coexistences.
Ne pas restaurer le facteur 64W, les copies parentales et brouillons non comptés v10.

**P2 travail évitable G1 :** x dans la **fermeture** de Q ne peut être dominé
strictement partout sur Q, car au centre x sa distance vaut 0. Retenir directement
ces sites, même x=hi ; garder le réservoir pour ceux extérieurs. À la racine,
39 885/K5 paie 1 196 550 tests impossibles sur deux passes. Fixture au contact hi
et sorties canoniques inchangées à exiger ; compteurs changeraient légitimement.
Aucun gain temporel acquis. Ce coût est O(nK), pas quadratique.

Le Pool conserve CAS saturant et nettoyage R2, remplace cutoff première exception,
TLS/repli implicite par toutes tranches, merge et garde membre. Ne mélanger aucun
protocole de génération v10 avec sa barrière nouvelle de tous les W−1 acquittements.
Callbacks/domaine restent vivants jusqu'au retour ; résultats publiés au succès.
La revue scalaire/Python ne qualifie ni pthread, catalogue parallèle ni FULL.

## Temps catalogue, mémoire et contrats LiDAR

Dernier banc source 7f : treize succès communs gardent les hashes canoniques et
sémantiques enregistrés de ffc ; nouveau succès uniforme32k/B18 en 24,125 s,
sans ancienne sortie comparable. API K5 u21 :08/100 **17,374 s**,08/0 **21,734 s** ;
u24 : 17,509/21,996 s. Une répétition, une séquence, CPU mono, API deux passes/tri/
sorties ; aucun temps FULL. Décodage Python en plus : 8,408–9,960 s pour ces quatre
LiDAR ; Buffer n'est pas RSS. Aucun gain stable établi par ces comparaisons.

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
