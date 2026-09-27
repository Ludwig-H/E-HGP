# Passation v9

## Référence pondérée implémentée, attaches silencieuses corrigées — 27 septembre

[État et fichiers actifs](experiments/weighted_clustering_20260927/ETAT_COURANT.md).
Nouvel export du catalogue complet, masses de facettes avant réduction,
attaches à la composante FULL fermée au rayon MEB, arbre augmenté, EOM
pondéré puis vote. Géométrie rationnelle ; décisions statistiques binary64.
Le premier essai a réintroduit le fold Gabriel réfuté par E5 :26racines
au lieu d'une sur la première gaussienne. Zéro score produit par cet essai,
sources et reçu failed conservés. Le nouveau chemin ignore ce graphe et
réutilise seulement son calcul de masses/votes.
Qualification corrigée Release et GCC ASan/UBSan :46commandes/capture,
33fixtures,464facettes,1 166coupes exactes, lecteurs normal/−O passés.
Ancres sans contribution, plateaux, K10 et C vide à K=n contrôlés.
LSan non qualifié ; échecs Clang-link/LSan historiques distingués.
Pilote13scènes n1200/K5/10/m20/50/z1/2 en cours, comparateurs figés.
Aucun nouveau score global, aucune croissance LiDAR/G4 déduite.
Le [routage ponctuel emboîté](experiments/weighted_clustering_20260927/PLAN_PARTITIONS_POINTS.md)
est encore un plan séparé : le vote plat n'en est pas une implémentation.

## Correction méthodologique après relecture intégrale I–II — 27 septembre

[Audit thèse/HGP-old](audits/RELECTURE_THESE_ET_HGP_OLD_20260927.md).
Les expériences ci-dessous évaluent `first_coverage`, **pas** le §9.1 :
elles projettent les points avant condensation, avec masses unitaires.
Ancien clusterer : S_facettes/T_points, masses fractionnaires, condensation,
EOM, puis vote. `expZ` modifie aussi les masses et votes historiques.
Le seuil pondéré n'est pas une borne sur la cardinalité finale après vote.
Retirer `C∩X` comme correction fidèle : définition 8 impose la couverture
dilatée. La variante `whole_tree=True` suggère un routage descendant, mais
son attribution globale entre racines reste à qualifier. FULL topologique
seul ne donne pas les scores d'incidences ; fixer le catalogue contributif.
Trois points exacts suffisent à montrer l'effacement des branches par LCA ;
rejeu rationnel v7 des masses normal/−O passé, anciennes archives intactes.
Priorité à une référence pondérée fidèle, puis à l'emboîtement ; ni nouveau
grand chantier ni GCP. Tous les scores/capsules antérieurs sont conservés.

## Gaussiennes 3D et condensation explicite — 27 septembre

[Résultats](audits/b_gaussian_point_clustering_20260927/RESULTATS.md),
[API de condensation](audits/b_gaussian_point_clustering_20260927/CONDENSATION.md).
48 scènes n1200, G2/4/8/16, δ8/4/2, isotropes/allongées/déséquilibrées ;
96 exports natifs,384 fitsHDB,1 920 scores, zéroéchec. K5/10, tailles10/20/50/100,
expZ1/2 ; référenceK5/20/1 figée. `min_cluster_size` expose maintenant parents,
masses, dates et n sorties de points, sans recalcul géométrique ni changementK.
Condensation structurelle C≤max(1,2floor(n/m)−1), stockageO(n+C) ;
ne pas transférer cette borne au générateur. Neufcommandes de gates passent.
Sphériques δ4 : ARI0,5633 HGP/0,2228 HDBcommun, singleton-bruit0,5929/0,2735.
À δ2, échec des deux ; allongéδ4, F1classes0,194/0,441 malgré arbre HGPprometteur.
À G16, m100 interdit de récupérer exactement les classes75 ; garder ces cas.
ExpZ2effetvariable ; aucune supériorité générale. HDBstandard conservé :
exæquos atomisés changent217/384partitions, parfois sensiblement ; reproduction
sansatomisation384/384. PrototypePythonbinary64, moteur et anciennes sources
inchangés, CPUlocal uniquement, zéroGCP, aucune preuve nouvelle de croissance/G4.

## Clustering de points à ordre K fixé — 27 septembre

[Résultats et suite mathématique](audits/b_point_hierarchy_k_20260927/RESULTATS.md),
[protocole](audits/b_point_hierarchy_k_20260927/README.md).
Nouvelle demande utilisateur : partitions emboîtées depuis **T_K seulement**,
comparées à HDBSCAN avec EOM commun et expZ1 puis2. Prototype export natif CPU,
première couverture exacte + LCA daté, variante vote d'entrées exploratoire ;
aucune masse facette/coface inventée, aucune verticale utilisée.
12 scènes entières, 36 exports natifs, 72 fits HDBSCAN, 504 lignes, zéro échec.
Neuf commandes de gates normal/−O passent ; contrelecture des504scores et hashes.
PrincipalK5/taille20/z1, macro10scènes3D : ARI0,9168 HGP/0,9060 HDB commun,
pureté dendrogramme0,9977/0,9609 ; **évaluation3D ARI0,8647/0,9165**, donc
pas de supériorité générale. Atom est séparable dans l'arbre mais EOM
sur-segmente ; Spiral est déjà moins bien séparé dans l'arbre. ExpZ2 n'est
pas un remède. Vote environ4,83×plus coûteux en prototype, sans gain convaincant.
Garder première couverture comme baseline expérimentale. Le supplément C∩X
alors proposé, non implémenté, est désormais retiré comme restitution fidèle
de la thèse par la relecture ci-dessus. Ne pas changer la garde stricte du résolveur
parents à l'aveugle. Moteur, registre et contratsG4 inchangés ; zéroGCP.
Sources et captures épinglées, données privées, scripts/reçus/tableaux publiés.

## FULL profilé sur G4, avec réserves explicites — 27 septembre

[Rapport pédagogique](docs/PROFIL_FULL_NSYS_20260927.md),
[reçu R2](receipts/full_nsys_20260927/r2/README.md).
Reconstruction du moteur inchangé puis trois préflights et quatre passages
sans/quatre avec Nsight, mêmes objets contrôlés. Trame ng00 entière K1..5
à1mm ; baseline chaude médiane912,219ms, un seul processus. Trace84noyaux,
207,895ms/pass ; copies5,184ms/pass. Tour CPU277–303ms : ni les copies ni
un noyau isolé ne permettent de promettre100ms. Pas de nouveau port moteur.
Deux avertissements : piloteCUDA13.0 non supporté par Nsight2025.3.1
(collecteCUPTI12.9), ordonnancement CPU absent. Ne pas certifier occupation,
exhaustivité ou attribution des trous CPU. Même génération G4 arrêtée,
220,439s d'allocation ; binaire et rapports récupérés en privé.
Suite conditionnelle : attribution CPU par phases avec profiler compatible,
pas nouvelle refonte spéculative. Anciennes captures ci-dessous historiques.

## Priorité au profilage FULL, pas à une nouvelle refonte — 27 septembre

[Premier essai de profilage G4](receipts/full_nsys_20260927/r1/README.md)
clos en échec avant calcul : contrôle de disponibilité du binaire FULL
historique dans `/tmp` refusé. Aucun téléchargement/lancement Nsight ni
nouvelle mesure GPU.
Même génération arrêtée après157,524s, pas de relance automatique.
Protocole local relu/testé et publié en `acb51d62e` ; moteur inchangé.
Un futur essai devra disposer d'un binaire épinglé durable ou reconstruit
et qualifié explicitement. Pas de grande refonte sans profil exploitable.

[Décision et protocole minimal](docs/PROFILAGE_AVANT_REFONTE_20260927.md).
Avant R2, aucune trace Nsight attestée dans les campagnes examinées. Profiler le vrai
`mhgp9_tower_probe` avant un nouveau chantier important ; chronos sous
profiler distincts du contrat, K déjà partiellement simultanés.

[Mesures A closes](audits/b_full_a_real_20260927/RESULTATS.md) : qualification
69 commandes, Release/sanitizers ; ng00 entière et uniforme8k/16k/32k,
120 comparaisons exactes, lecteurs normal/−O contre-relus. Min-label/event
sur ng00 : somme des médianes K 3 148,510/4 099,105 ms, capacités maximales
210,372/290,674 Mo. Ce ne sont ni des murs FULL ni un gain sur A natif.
Croissance uniforme des volumes proche du doublement ; temps M ×2,558/2,713.
Une trame LiDAR seule ne démontre pas sa croissance.

[Session G4 S2 R1](receipts/q34_survivors_g4_20260927/r1/README.md) close
en échec, source `5571957ca` : `prepin` attend à tort un `.rsp` obligatoire,
alors que le CMake distant ne l'émet pas. Build et portes CUDA non exécutés.
Reçu conservé comme failed, VM même génération arrêtée, allocation155,926s.
Pas de nouveau chrono ni de relance automatique. Protocole et sources figés ;
un correctif futur devra autoriser les options directes tout en fermant tous
les fichiers indirects effectivement référencés, dans une capture distincte.

## Comparateur S2 compilé avant G4 — 27 septembre

[Harnais apparié](audits/b_q34_survivors_compare_20260927/README.md) clos :
13 commandes, six TU, 1 088 dépendances pré-épinglées, deux binaires CUDA
compilés sans exécution GPU. Porte portable52lots/208opérateurs et trois
refus CLI ; lecteurs normal/−O contre-relus. ABBA/BAAB distinguent premier
contexte et premier appel de chaque implémentation, sorties détruites entre
passages. Anciennes sources et moteur inchangés ; pas encore de chrono G4
de ce comparateur ni de gain à cette porte locale. Le protocole cloud a
depuis été publié ; son premier essai échoué est décrit ci-dessus.

## Tri des survivants avant téléchargement — 27 septembre

[Bilan](docs/TRI_RESIDENT_DES_SURVIVANTS_20260927.md).
Prototype de sortie q34 uniquement : accumuler S sur le device, radix u64
avec payload attaché, refus des doublons, télécharger 12 octets par sortie.
Même filtrage et mêmes compteurs physiques ; pas de tableau global P/E.
R1 ferme 26 commandes Release/Clang ASan/UBSan/LSan/compilation CUDA,
85 lots et 1 360 appels appariés portables, deux mutants compilés détectés.
Lecteurs LIVE normal/−O passent. Aucune exécution GPU de cette variante,
aucun nouveau FULL/G4/croissance : prochaine porte device puis appariement
sur trame entière. Clés hautes testées en portable, pas encore device.
Sources/builds/reçu figés, moteur inchangé, GCP non utilisé dans ce lot.

Porte dédiée haut-u64 compilée dans une nouvelle capture de dix commandes,
24 cas hôte et huit falsifications du lecteur ; aucune exécution GPU.
Elle ferme les fichiers de paramètres indirects, absents des pins CUDA de
R1. L'[addendum](audits/b_q34_resident_survivors_review_20260927/RESPONSE_FILES.md)
limite précisément l'ancienne preuve CUDA, sans annuler les tests portables.
Le prochain comparatif G4 devra fermer ses propres options avant/après.

## Manifeste FULL et constructeur événementiel C++ — 27 septembre

[Bilan pédagogique](docs/CONSTRUCTION_EVENEMENTIELLE_FULL_20260927.md).
Le manifeste reprend toutes les entrées A natives, y compris muets et
contributions datées ; R2 close17commandes/1140dépendances,44captures,
376rejeux,20corruptions de lecteur. R1nonqualifiée conservée : inventaire
préalable d'includes incomplet, harnais corrigé sans changer le C++.

Le nouveau constructeur C++ de A utilise MSF, requêtes historiques et
doublement des renvois ; aucune copie du solveur géométrique. R1close
17commandes/1141dépendances,376comparaisons natives,64rejeux abstraits et
deux frontièresK1 ; Release/ASan/UBSan/LSan, lecteurs normal/−O et
25corruptions passent. Trois branches mutantes détectées,32parents réels
dans le corpus géométrique. Encore séquentiel, pas TSan/GPU ni chronoLiDAR.
La borne quasi linéaire-logarithmique porte sur V/E du manifeste, pas sur
le nombre de points du générateur. Suite : vrais catalogues et profils
V/E/G/parents/contributions, puis parallélisme interne ; B/C restent natifs.
Moteur inchangé et GCP non utilisé dans ce lot. Sources/builds/reçus figés.

Variante min-label désormais qualifiée séparément : R1 ferme 17 commandes,
1 142 dépendances, 376 rejeux natifs, 74 abstraits/quatre frontières et
1 886 024 requêtes de coupe indépendantes sur 227 graphes. Elle supprime
CSR/DFS, table des maxima et redirections, sans perdre les continuations.
Historiques : 3 368 pour 3 784 groupes ; capacités maximales observées
11 884 octets contre 13 004 dans R1 événementiel sur le même petit corpus,
pas pic RSS ou gain LiDAR. Table `up` O(V log V), DSU et tris sériels
restent ; mesurer les vrais V/E avant port parallèle. Proposition d'index
linéaire séparée, pas encore implémentée. Lecteurs normal/−O et leurs
29 corruptions passent aussi en contrelecture indépendante.

## Raccord résident qualifié en portable et sur G4 — 27 septembre

[Bilan et prochaine mesure](docs/RACCORD_RESIDENT_Q34_20260927.md).
Le prototype filtre les rectangles avant l'arène, garde index/K et filiation,
compacte les descriptions vivantes, puis consomme E par vagues sans tableau
global P/E. La sortie ordonnée reste exactement native.50commandes closes,
85cas×3configurations par build, Release/ClangASan/UBSan/LSan, trois mutants
et quatre fautes portables ; lectures normal/−O passent. Puis vrai gate
CUDA :85cas,255passages portables/194CUDA. Ng00 entière K5/s8 retrouve
exactement P/E/S natifs ; adaptateur froid W4/W48 627,503/504,328ms,
arène202,425/83,113ms. Front W1 conservé : front+adaptateur2,739/2,616s.
Une mesure par largeur, pas de temps chaud ni de comparaison GPU/GPU
qualifiée. Arrêt même génération certifié après191,901s d'allocation.
Reçu LIVE normal/−O et contrelecture indépendante passent.
Ne pas promouvoir le moteur sur un gain contre l'ancien prototype CPU9,9s :
battre le vrai filtre GPU et la chaîne complète reste à établir.
Pas de nouveau FULL ni de croissance mesurée du raccord. Les deux prochains
tickets S résident et manifeste FULL sont liés au bilan. Sources, builds
et reçus r1 désormais figés ; manifeste FULL qualifié dans la tranche ci-dessus.

## Coûts hors S2 et premier raccord FULL — 27 septembre

[Ledger q34](audits/b_q34_outer_ledger_20260927/README.md) et
[plan d'implémentation FULL](audits/b_full_construction_parallel_20260927/README.md).
Deux premiers passages historiques rejugés, pas une nouvelle campagne :
q34 total487–491ms, S2≈101ms, S3≈119ms, attente S4≈80ms ;83–84ms
restent non attribuées et ne sont pas un gain promis de validation.
Ne pas additionner S4 et son attente, ni les phases q2 recouvertes.

La prochaine couture S2 reçoit des décisions de rectangles scellées et
prépare seulement les descriptions vivantes ; voir le
[contrat](audits/b_q34_filtered_contract_review_20260927/README.md).
P logique et E physique restent distincts. Aucun masque extérieur déclaré
fiable, aucun recalcul géométrique CPU des rectangles déjà décidés sur GPU.
Pour FULL, exporter les blocs actifs, cibles, contributions et ancres
réelles puis comparer les mêmes drafts, avant le port du graphe temporel.
Les seuls drafts finaux perdent des événements silencieux nécessaires.
Moteur inchangé, aucune nouvelle dépense GCP pour ces audits.

## Vagues réelles et chemin critique — 27 septembre, nouvelle qualification

[Bilan actuel](docs/VAGUES_REELLES_ET_CHEMIN_CRITIQUE_20260927.md).
S2 mono sur trame sans sol entière : 47,691→31,048 s, mêmes survivants,
préparation comprise ; pas un mur FULL. Uniforme8k/16k/32k régresse de
7–8 %, quoique les travaux principaux restent sous le quadruplement.
Six coupes capteur closes : postes dominants sous β=2, exception de
préparation β=2,230 conservée ; ni preuve générale ni plusieurs scènes.
`Prepared` LiDAR conserve187,227Mo, pas seulement29,328Mo d'arène.

Priorité de port : ne pas refaire le filtre rectangle GPU en CPU,
compacter les descriptions utiles et conserver les ordinals originaux.
Le consommateur CUDA isolé est publié en `33c1d28d7`, porte portable
31 commandes et protocole gardé hors cloud qualifiés. La capture device
suivante passe : vrais tests CUDA et trame entière exacte,35ms de vagues,
mais préparationCPU9,898s ; aucun gain net du raccord. Allocation188,416s,
VM même génération relueTERMINATED. Voir le bilan pour les périmètres et
le reste non attribué du runner. Moteur inchangé.

Correction d'interprétation FULL : les K sont déjà encodés simultanément.
Sur deux premiers passages G4 historiques, fenêtre37–48ms et construction
de tour hors encodage252–256ms. La somme des K ne se soustrait pas au mur.
Il faut aussi traiter S3/S4, census et construction des événements FULL.

## Parents parallèles et vagues q34 — 27 septembre, suite qualifiée

[Bilan et décisions de port](docs/PARENTS_PARALLELES_ET_VAGUES_Q34_20260927.md).
Le first-parent est réellement parallèle : 372 entrées/2 232 comparaisons
par build, Release/sanitizers et TSan passent. Sur vrais drafts LiDAR00,
natifs/W1/W4 163,973/168,825/120,793 ms en sommes médianes K ; observation
locale favorable, variable, pas un mur FULL/G4. Uniforme8k/16k/32k testé.
Le lemme des blocs réguliers explique l'absence de continuations ; ABCZ
prouve que les supprimer ailleurs perd une contribution malgré une
topologie inchangée. Conserver détection et repli général.

Consommateur q34 par vagues qualifié : 175 lots/1 050 consommations,
ordre et masques S identiques au natif, aucune allocation globale P/E.
Fallbacks entiers, Q non limitant la recherche, tri S toujours payé.
15 commandes Release/sanitizers closes ; défauts S vide/pic mémoire
corrigés avant gel. Préparation CPU et port GPU restent ouverts.
Ne pas activer CPU Pool + GPU S2 comme un gain présumé ; S3/S4 et FULL
reçoivent le même S. Aucun nouveau GCP ni contrat 100 ms, moteur inchangé.

## Arène collective et FULL linéaire — 27 septembre, nouvelle clôture

[Résultats et décisions](docs/ARENE_COLLECTIVE_ET_FULL_LINEAIRE_20260927.md).
Les facteurs q34 sont préparés une seule fois dans six buffers collectifs.
LiDAR00 : capacité finale 29,328 Mo, pic des tableaux d'un constructeur
41,661 Mo, W1/W4 524,64/276,66 ms locaux partagés ; mêmes crédits et E.
Dix mesures 8k/16k/32k et une trame, 23 commandes closes, Release/sanitizers.
Petits lots plus lents en W4 ; résidu amas quasi quadratique inchangé.
R1 ptrace conservé ; défaut de couverture du refus ordinal traité par un
complément séparé, sans modifier les sources gelées.

Le FULL sans continuation passe 19 294 comparaisons par build et quatre
mutants. Voie linéaire, 16A octets temporaires ; sinon repli général.
Sur vrais drafts LiDAR00, sommes médianes K natif/prototype 168,12/171,08 ms :
ne pas l'activer comme gain CPU. Il prépare une réduction minimum et un
scatter parallèles, mais aucun thread/GPU de cet encodeur n'est qualifié.
Reçus et revues indépendantes liés au bilan. Moteur inchangé, GCP non utilisé.

## Lignes ordonnées et vraie charge FULL — 27 septembre, suite suivante

[Synthèse et décisions](docs/ORDRE_ET_VRAIS_DRAFTS_20260927.md).
Les lignes B partagées par classe A évitent structurellement le tri final
de S, sans scanner chaque paire du produit original : W≤K(K−1)F_B,
T≤E. 25 commandes closes, 15 mesures, mêmes masses Pool. Sur LiDAR00
K5 : W4,080M/T1,969M pour E9,123M, mais ancien Pool toujours payé.
Les amas conservent le résidu quasi quadratique. Pas encore S2/GPU/FULL.

Quatre vrais drafts FULL capturés sans changer le moteur : ng00 entier
et uniforme8k/16k/32k. L'encodeur général donne les mêmes tableaux, mais
coûte environ deux fois le natif en sommes de temps CPU par ordre.
Le port scalaire est donc à différer. Aucun de ces drafts n'a de continuation :
une voie first-parent linéaire est mathématiquement contre-jugée, à qualifier
séparément. Le général reste nécessaire lorsqu'une continuation existe.
Captures et échecs de compilation conservés, GCP non utilisé.

## Bandes directes et encodeur FULL reconstruits — 27 septembre, suite

Lire [la nouvelle tranche](docs/PROTOTYPES_DIRECTS_ET_FULL_20260927.md)
avant les paragraphes historiques. Trois dossiers de prototypes qualifiés :
`b_q34_direct_bands_20260927`, `b_q34_batch_seam_20260927` et
`b_full_batch_encoder_20260927`, avec reçus et contrelecture FULL séparée.
Le brouillon FULL perdu est désormais reconstruit et testé, sans hériter
des anciens binaires. 6 838 entrées et deux calendriers par build,
mêmes tableaux explicites et premier motif de refus, trois mutants.

Le plan direct ne paie plus les anciennes cellules. Trois trames sans sol
K5 : préparations CPU ancien/nouveau ≈975→638, 643→432, 1 122→740 ms ;
pas un chrono moteur ni une attribution causale aux seules bandes.
La porte de raccord conserve exactement l'ordre et les masques natifs sur
576 configurations. Au port, restaurer l'ordre avant S3 et séparer P logique
d'E effectivement testé. Allocations par rectangle et arène GPU restent
à traiter. L'encodeur est scalaire, sans verticales ni génération du draft.

Moteur inchangé dans ce lot, GCP non utilisé. Dernier chrono G4 FULL reste
environ 923 ms K5 sur 00 sans sol ; 100 ms et sous-quadratique global
non acquis. Ne pas écraser les nouvelles captures ni leurs builds clos.

## Reprise et publication DEV — 27 septembre

Lire [le bilan actuel](docs/REPRISE_DEV_20260927.md) avant les tranches
ci-dessous. Bandes q34 reconstruites/qualifiées après perte du worktree
temporaire : 23 commandes closes, dix mesures, normal/−O et mutants.
Sur LiDAR00, −85,9 % de capacité des descripteurs mais +49,4 ms CPU
de préparation dans le prototype différentiel ; pas de port aveugle.
L'encodeur FULL non publié n'est plus accessible et reste à reconstruire.

G4 core ON/OFF close : 18 passages FULL, trois digests identiques,
923,417 contre 975,936 ms à chaud, une seule trame K5/s8/W48 sans sol.
Conserver core ON. VM arrêtée après 248,353 s d'allocation. Le défaut
public d'alias de banque FULL est reproduit, sans impact interne G4
démontré. Moteur et protocole inchangés ; tous les nouveaux travaux et
preuves sont dans les dossiers datés du 27, builds précédents préservés.

## Plans q34 éprouvés — 26 septembre, après la campagne q3

Le [prototype par facteurs](audits/b_q34_factor_plan_20260926/README.md)
est implémenté séparément du moteur, avec [42 commandes/24 mesures closes](receipts/q34_factor_plan_20260926/README.md).
Il applique le Pool aux vrais nœuds du même index après le filtre rectangle,
puis garde des classes conjointes q3/q4 avant toute expansion en paires.
Les trois trames sans sol K5 perdent 52,7–61,5 % de leurs paires ; 00
passe de 23,687 à 9,123 millions. Mais le plan mono local coûte 676 ms et
ses capacités cumulées 125 Mo : **ne pas le brancher tel quel sur G4**.
Sur amas 8k/16k/32k, E=2,09/7,79/30,70 M, dernier ratio ×3,942 ; le
problème de croissance n'est pas fermé. Les six coupes LiDAR ont des
pentes de résidu 1,004–1,820, mais un compteur de préparation atteint 2,230.

Les [propositions de port](audits/b_q34_factor_plan_20260926/NEXT.md)
précisent arène collective/bandes sans doublons, masques exacts et tubes
adaptatifs à éprouver ; ce ne sont pas des noyaux GPU qualifiés. Ce plan
ne peut économiser que S2 et son transport, pas supprimer le travail des
lanes survivantes ni FULL. Garder en parallèle les priorités front GPU
compact et [A→images→écriture explicite FULL](audits/FULL_PARTAGE_INTER_ORDRES_20260926.md).
Un encodeur parallèle sur le draft actuel est isolable avant le nouveau
graphe événementiel ; le contrôle des parents vivants doit être conservé.

Le [contre-audit q3](audits/AUDIT_CROISE_PORT_Q3_20260926.md) ne démontre
aucun nouveau défaut fonctionnel. Deux limites : capacité GPU payload
sous pression, ledgers détaillés seulement du premier passage chaud.
Moteur inchangé, GCP non utilisé dans cette nouvelle tranche. Les builds
r2 du prototype et les captures sont désormais épinglés ; ne pas les
reconstruire pour continuer. Le prochain build doit être neuf.

## Reprise active du 26 septembre

À la demande de l'utilisateur, l'auditeur B reprend le développement.
Voir [la tranche q3 → catalogue](docs/REPRISE_DEVELOPPEUR_IDS_Q3_20260926.md)
et [l'interface de mesure v30](docs/DEVELOPPEMENT_IDS_Q3_20260926.md).
La sonde multitrames v29 du précédent développeur est reprise explicitement,
sans modifier son worktree. Le transport exact des IDs q3 est maintenant
implémenté (`9751bae69`, protocole corrigé `f9f273bb0`) et testé : six
portes Release, deux ASan/UBSan/LSan, trois non-régressions et protocole
31/31 normal/−O. La [capture locale](receipts/q3_payload_local_20260926/README.md)
ferme dix paires FULL ON/OFF et montre le verrou quasi quadratique des
amas ; ne pas confondre baisse du census et croissance du générateur.

La [campagne G4 close](receipts/g4_q3_payload_20260926/README.md) compare
26 cas, dont 18 GPU et huit témoins moteur, sans divergence des objets.
K5/s8 sans sol ON : 918–927 ms sur 00, 749 ms sur 01 et 969 ms sur 02,
trois trames de la seule séquence 08. Sur 00, le gain médian ON/OFF de
5,1 ms n'est pas robuste : une des deux paires régresse de 1,2 ms.
Le levier reste **désactivé par défaut**. K10 sans sol vaut 2,951 s,
brut K5 1,943 s, brut K10 5,938 s ; aucun contrat 100 ms ni résultat
multi-séquence acquis. Lecture, segmentation et condensés sont hors
chrono de chaîne, mur externe publié séparément.

La première capture avait échoué au rapatriement SSH ; une récupération
refusée avant démarrage puis une récupération allouée infructueuse sont
[conservées séparément](receipts/g4_q3_payload_failed_capture_20260926/README.md),
sans chiffres FULL qualifiés. Une seule campagne identique a été relancée.
Trois allocations SPOT, toutes arrêtées et relues `TERMINATED`, totalisent
737,423 s (12 min 17 s) ; ce n'est pas une facture. Aucune VM laissée active.

Priorité de développement : plans q34 avant expansion (sans tableau de
masse P caché), front GPU compact, puis FULL événementiel sur tous les K.
Le port q4 des IDs est spécifié mais secondaire tant que son gain net est
inconnu. Voir le plan de reprise ; ne pas continuer les micro-variantes q2.
Les paragraphes suivants sont datés et historiques.

22 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `quantized_u18_input_only`, `not_claimed`. Aucun
statut formel modifié ; `docs/implementation_status.toml` n'est pas touché par
cette exploration hors registre.

## État courant (22 septembre, soir) : premier moteur v9

La tranche verticale de V9-1 existe et passe ses portes :

- `src/tower/` : la tour FULL de la v7 portée au domaine 18 bits ;
  `src/gen/` : le générateur exact de la v8 ; `src/chain/` : la chaîne
  générateur → catalogue recoupé → tour. Détail et empreintes :
  [provenance](docs/PROVENANCE.md).
- 20 CTests verts (`ctest --test-dir build/v9 -L gate`), dont le juge T2 de la
  v7 appliqué à la chaîne réelle (catalogue égal à l'inventaire rationnel
  exhaustif, tour égale au modèle Γ, K = 1..10, s = 8/10/12, un et quatre fils).
- Premier essai à l'échelle, **exploratoire et sans reçu** (hôte chargé, charge
  32 au départ) : trame 08/000000 sans sol à 1 mm, 39 885 sites, K = 5, huit
  fils : tour complète en 131 s de mur et 834 CPU·s, RSS 1,06 Go. Catalogue
  de 1 306 696 boules, coquilles d'au plus 5 sites (227 coquilles étendues,
  aucune au-delà de 12), aucune divergence entre les deux implémentations.
  Le générateur q3/q4 prend 117 s (89 %), la tour 11 s en un fil, q2 1,7 s.
  Le poste dominant est donc l'amont q3/q4, pas l'aval comme en v7 uniforme.

Base de temps à reçu ([`first_tower_20260922`](receipts/first_tower_20260922/README.md),
huit fils, hôte local) : tour complète en 143 / 132 / 264 s à K5 et
523 / 381 / 802 s à K10 sur les trames 000000 / 000100 / 000200 ; q3/q4 fait
73 à 93 % du mur, la tour en un fil 13 à 25 % à K10. Porte arithmétique
18 bits de la tour (`mhgp9_tower_arith_u18`) verte : elle montre que l'ancien
test de plateau i128 de la v7 rendait de vraies réponses fausses à 18 bits.
Protocole G4 v9 écrit (`gcp-migration/tower_*_v9.py`, selftests hors ligne
verts), pour une première session à 48 fils.

Première session G4 ([`g4_tower_r1_20260922`](receipts/g4_tower_r1_20260922/README.md),
48 fils, `TERMINATED` certifié) : tour complète en 18,8 / 15,1 / 29,3 s à K5
et 111,7 / 82,3 / 125,4 s à K10 ; condensés identiques au local. Le générateur
q3/q4 passe à l'échelle (×11,6 de 8 fils locaux à 48 fils G4) ; à K10 la tour
en un fil domine (56–76 s), 34 s en voie statique à 48 fils.

Noyau MEB « première paire maximale » (port du prototype qualifié de la v7) :
condensés identiques, tests de puissance −56 % à K5 et −64 % à K10, tour K10
locale 130 → 109 s. La sonde publie maintenant le registre du générateur
(`ledger`).

Deuxième session G4 ([`g4_tower_r2_20260923`](receipts/g4_tower_r2_20260923/README.md),
paquet `0b29b6c3`, `TERMINATED` certifié) : **refusée** par le validateur du
worker (`probe_failed`, champs MEB non entiers), donc sans qualification. Les
sorties brutes, toutes `complete_relative`, donnent 10,5–20,2 s à K5 et
37,1–64,4 s à K10 (48 fils, tour statique), condensés identiques à R1 ; la
tour K10 plafonne à 23 s de 24 à 48 fils. Correctif : schéma de sonde v4,
voies géométriques épinglées par cas, arrêt au premier défaut de protocole,
porte `mhgp9_probe_worker_contract` qui fait juger la vraie sonde.

Census q3 sur feuille exacte de l'atlas (levier de l'auditeur A, défaut de la
chaîne) et portes du générateur portées de la v8 (27 portes, mutants compilés) :
voir la [provenance](docs/PROVENANCE.md). Condensé identique sur 000100 K5 en
local ; bornes de census q3 −54 % mais 610 M tests ponctuels de frontière
(contre-audit B) : gain net non qualifié, parcours de frontière par boîtes à
faire.

Arêtes sans sortie ([reçu](receipts/q34_dead_edges_20260923/README.md)) :
86 % des cycles q3/q4 instrumentés (97 % hors filtre de paires) vont aux arêtes qui n'émettent rien. Le **certificat de voie
morte** (`lanes/q34_dead_lanes`, défaut de la chaîne, épinglé par la sonde v5
et le plan G4 v3) couvre le disque des centres possibles de chaque voie par
des cellules portant T intérieurs uniformes ; il divise le CPU q3/q4 par 2,9 à
K5 et K10 sur 08/000000 (prototype), condensés identiques sur les six cas.
Protocole v5 durci : schémas exacts de la sonde, preflight natif sur la vraie
sonde avant tout cas, résumés recalculés à la réception, campagne sans tour
complète refusée, paquet recertifié contre les objets Git de son commit.

Session G4 R3 ([`g4_tower_r3_20260923`](receipts/g4_tower_r3_20260923/README.md),
paquet `b4e480fc`, **`completed`**, `TERMINATED` certifié) : ablation appariée
du certificat, quatorze cas complets, preflight natif accepté. q3/q4 ÷2,0 à
÷3,0 ; chaîne complète à 48 fils **6,2 / 8,9 / 10,9 s à K5** et
**26,0 / 35,6 / 37,8 s à K10** (000100 / 000000 / 000200), condensés
inchangés. À K10 la tour domine (17,5 à 22,8 s) et plafonne dès 24 fils.

Depuis R3 : préparation parallèle de la voie statique de la tour (validation,
collecte, tris, forêts ; lots et banque restent séquentiels), preuve des deux
voies mortes en une récursion, et **cache des nœuds témoins** du filtre de
paires (70 % des paires rejetées sans recherche à K5 sur 08/000000).
Protocole v6 : tous les leviers de la chaîne sont épinglés par cas
(`levers`, `--lever=NOM=0|1`).

Session G4 R4 préemptée par GCE (aucune mesure, [reçu](receipts/g4_tower_r4_preempted_20260923/README.md)) ;
reprise R4b ([`g4_tower_r4b_20260923`](receipts/g4_tower_r4b_20260923/README.md), paquet `a1d7a9bc`,
**`completed`**) : cache −5 à −19 % de CPU, tour K10 −35 % contre R3 ; chaîne
complète **5,4 / 7,4 / 9,2 s à K5** et **19,4 / 26,7 / 29,5 s à K10** à 48 fils,
condensés inchangés.

Tour à ordres K construits en parallèle (voie statique à plusieurs fils) :
lots de chaque ordre en parallèle, identifiants de populations attribués dans
l'ordre séquentiel puis lignes construites en parallèle, images verticales de
l'ordre K depuis l'histoire achevée de K−1, banque par déplacement et
validation parallèle. Lots 12 → 2,1 s en W8 local sur 000100 K10 ; tour
26 → 18 s ; condensés inchangés sur 000000, 000100, 000200 à K10.

Session G4 R5 ([`g4_tower_r5_20260923`](receipts/g4_tower_r5_20260923/README.md),
paquet `aae9da0e`, **`completed`**, `TERMINATED` certifié) : tour K10
14,9 → 5,4 s contre R4b ; chaîne complète à 48 fils **4,3 / 6,0 / 7,4 s à K5**
et **12,1 / 17,4 / 19,6 s à K10**, condensés inchangés, deux répétitions. q3/q4
redevient le premier poste (65 à 70 % à K5). Filtre par ligne a × B
(proposition B) prototypé : perte nette, écarté (coordination, 23 septembre
03 h 45).

Échecs de la tour : priorité au plus petit K sur les phases A et C (comme la
boucle séquentielle) et bilan de travail fusionné une fois même après échec
(porte `mhgp9_chain_order_failure_priority`, deux mutants compilés). Porte
produit du propriétaire du certificat (`mhgp9_gen_q34_dead_lanes_owner` : ABA,
`load()` interrompu).

**Noyau diamétral** du certificat de voie morte (levier `q34_dead_core`,
défaut) : le certificat est d'abord tenté sur la boule diamétrale fermée de
l'arête, sous-ensemble du cover, et le cover n'est construit que pour les
voies restées ouvertes. Harnais local W8 sur 08/000000 : CPU q3/q4 −18 % à K5,
−16,5 % à K10, flux identique ; sonde W8 K5 : condensé inchangé. Protocole G4
v7 (sonde) / v5 (plan) : cinq leviers, douze compteurs du noyau et leurs
identités exactes.

Session G4 R6 ([`g4_tower_r6_20260923`](receipts/g4_tower_r6_20260923/README.md),
paquet `78ce9fd4`, **`completed`**, `TERMINATED` certifié) : ablation
appariée du noyau, 24 cas, objets égaux ON/OFF ; CPU −10 à −20 %, mur −2 à
−8 %, formes chargées −77 à −82 % ; meilleurs totaux **4,15 / 5,76 / 6,87 s à
K5** et **11,73 / 16,67 / 18,06 s à K10**. Défaut ON gardé.

Queue de chaîne : le condensé FNV de vérification (≈1,1 s local à K10) est
sorti du chrono de chaîne et publié à part (`times_ms.digest`, sonde v9) ; la
fusion des présentations est un tri d'échantillonnage parallèle par plages de
clés (1,04 → 0,47 s en W8 local à K10) ; la tour certifie par un balayage
un catalogue déjà strictement trié (`presorted_catalogues`) et ne le retrie
pas.

Tour à K10 (catalogue 08/000000, W8 local) : la phase de cibles statiques
prend 70 % de la tour, dont la résolution des facettes (MEB exact puis
recherche d'intrus). **MEB proposé** (Welzl en double vérifié exactement,
canonisé au bord, repli exact) : même résultat que l'énumération, cycles MEB
÷2,2, tour −24 % local, condensé inchangé (sonde v10, levier
`tower_meb_proposal`). Index exact par hachage des clés du catalogue à la
place des recherches dichotomiques : tour −8 % local.

Session G4 R7b ([`g4_tower_r7b_20260923`](receipts/g4_tower_r7b_20260923/README.md),
paquet `8e8b83a3`, **`completed`**, `TERMINATED` certifié ; R7 avait été
refusée par rupture de stock GCE, [reçu](receipts/g4_tower_r7_stockout_20260923/README.md)) :
ablation appariée du MEB proposé, objets égaux, tour K10 −5 à −8 %. Chaîne
hors condensé (publié à part) : **3,67 / 5,31 / 6,40 s à K5** et
**9,58 / 13,91 / 15,28 s à K10** ; tour K10 3,2–4,0 s ; fusion 0,04–0,12 s.
Depuis : Welzl à déplacement en tête (proposition ÷2,7), séparateurs
pseudo-aléatoires du tri parallèle, index de clés libéré avant la banque.

Croissance LiDAR locale ([`lidar_scaling_local_20260923`](receipts/lidar_scaling_local_20260923/README.md),
sonde v12 `4530644b`, runner v2, trois trames sans sol, K5 et K10, emboîtés
8k/16k/32k + entier + morceaux, 60 cas conformes, revalidés par le lecteur durci : [addendum](receipts/lidar_scaling_local_20260923_revalidation/README.md)) : temps de chaîne ×1,7 à
×2,8 par doublement et boules sous-linéaires, mais `core_sites` (sites
énumérés dans les cœurs diamétraux) atteint p = 2,5 à 3,05 sur un doublement
de s00 et de s02 aux deux K. Le cœur est la première cible d'échelle.
Le crédit par nœuds du certificat, décisions identiques, est mesuré puis
fermé : CPU +27 à +32 % ([reçu](receipts/dead_node_credit_negative_20260923/README.md)).

Sonde **v13** (`c768e06a`) :
- **Invariant d'Euler** du catalogue (auditeur C, preuve par le nerf de B),
  imposé par la chaîne : `chain_catalogue_euler_violated`.
- Portes : `chain_euler`, porte T2, porte `scale8000` de C avec juge
  d'échantillon.
- Occupation par ouvrier de q34, chronos par phase de la tour ; lecteur G4
  revu par une revue multi-agents (un faux refus corrigé, `515b3666`).

Session G4 R8 ([`g4_tower_r8_20260923`](receipts/g4_tower_r8_20260923/README.md),
paquet `515b3666`, **`completed`**, `TERMINATED` certifié) :
- Euler « holds » sur les trois trames réelles.
- Chaîne K5 3,67 / 5,46 / 6,39 s, K10 9,40 / 13,69 / 15,19 s.
- **q34 affamé à W48** : 35 à 49 % du temps des fils à K5 passe à attendre
  la file de tâches (jobs du front trop gros).
- Tour K10 : phase 0 1,1–1,5 s, lots séquentiels 0,8–1,2 s.
- s = 8 reste le meilleur choix, devant s = 10 et 12.

Session G4 R9 ([`g4_tower_r9_20260923`](receipts/g4_tower_r9_20260923/README.md),
paquet `fe1142b5`, **`completed`**, `TERMINATED` certifié) : jobs du front
préparés par masse et 4× plus fins (sonde v14). **Chaîne K5 2,80 / 3,80 /
3,99 s**, **K10 8,60 / 11,75 / 11,89 s**, condensés égaux ON/OFF. Sonde
**v15** : phase A de la tour recouvrant la phase 0 (`tower_overlap_static`).

Session G4 R10 ([`g4_tower_r10_20260923`](receipts/g4_tower_r10_20260923/README.md),
paquet `33d51efd`, **`completed`**, `TERMINATED` certifié) : recouvrement de
la tour, avec la tour K10 −0,46 à −0,76 s. **Chaîne K5 2,74 / 3,64 / 3,94 s**,
**K10 8,07 / 11,13 / 11,36 s**. Validation du catalogue : passe 2 et
programmes parallélisés.

Session G4 R11 ([`g4_tower_r11_20260923`](receipts/g4_tower_r11_20260923/README.md),
paquet `f685461a`, **`completed`**, `TERMINATED` certifié) : jobs du front q2
par masse (sonde v16), q2 divisé par 3 à 4,5. **Chaîne K5 2,54 / 3,21 /
3,52 s**, **K10 7,68 / 10,36 / 10,53 s**. Juge d'échantillon des clés jamais
émises (`chain_absent_keys`) : 77 000 boules admissibles, toutes présentes, à 2k et
8k. Mesures sans suite immédiate :
- l'index des selles seul ([négatif](receipts/saddle_index_negative_20260923/README.md)) ;
- le pouvoir de preuve des voisins proches pour le certificat, 96 % des
  fermetures du cœur à K5 ([mesure](receipts/knn_core_probe_20260923/README.md)).
- le certificat sur les voisins proches **globaux** (liste kNN de 16 par
  site) : exact mais neutre, CPU q34 −2 % à K5 et +3,7 % à K10
  ([négatif](receipts/near_sites_negative_20260923/README.md)).

Profil q3/q4 par échantillonnage (`SIGPROF`) : le filtrage (front,
rectangles, paires) fait environ 40 % du CPU, le cœur et le certificat 18 %,
la génération q3/q4 25 %, sans poste au-delà de 16 %
([reçu](receipts/q34_micro_levers_20260923/README.md)). Aucun micro-levier
retenu : pas +1 des compteurs non contrôlé (−4 %, mais contrat public de
dépassement changé), compteurs locaux du DFS (−0,9 %), raffinement des
rectangles et cache par `b` (pertes).

Voie GPU S1 (23 septembre après-midi) : filtre témoin exact q3/q4 porté
hôte/device (`src/gpu/`), protocole G4 dédié `gcp-migration/gpu_filter_*_v9.py`.
Session G4 S1 ([reçu](receipts/g4_gpu_s1_20260923/README.md), paquet
`6e0e43a0`, **`completed`**, `TERMINATED` certifié) : sur le sous-nuage
08/000000 **sans sol** à K5, les filtres rectangles et paires sans cache
**hors construction du front WSPD** (3,13 M rectangles, 23,7 M paires)
tiennent en
**63,8 ms** sur la RTX PRO 6000, contre 1,18 s au CPU à 48 fils avec cache
(×18 à ×24 sur les six cas). Masques et totaux de visites identiques. Seuil
S1 (0,1 s) franchi. Suite GPU : raccorder le passage à la chaîne (front CPU,
filtre GPU, survivants au CPU), puis porter le cœur et le certificat de voie
morte. La tentative 1 a échoué à la configuration (CMake 3.22.1 sur la VM,
[reçu](receipts/g4_gpu_s1_attempt1_20260923/README.md)).

Voie GPU S2 (23 septembre, fin d'après-midi) : filtre témoin q3/q4 par lots
dans la chaîne (`run_wspd_q34_batched`, leviers `q34_batch_filter` et
`q34_gpu_filter`, sonde v17, protocole de la tour v17 avec build CUDA).
Frontière de confiance à trois couches :
- contrôle structurel des survivants ;
- différentiel moteur/lots (porte `chain_batch_filter`, jumeau moteur du
  préflight et des cas GPU) ;
- juges indépendants de C.

Session G4 R12 ([reçu](receipts/g4_tower_r12_20260923/README.md), paquet
`2059189d`, **`completed`**, `TERMINATED` certifié), toutes les tours
égales à leur jumeau moteur :
- **K5 2,01 / 2,47 / 2,69 s** et **K10 6,63 / 8,32 / 8,70 s**, soit −15 à
  −25 % contre le chemin moteur ;
- appel du filtre 0,2–0,3 s, dont 47–119 ms de passe GPU, le reste côté
  hôte (contexte, copies) ;
- survivants 0,8–1,2 s à K5, tour 0,6–0,8 s.

Suites : contexte GPU et index préparés pendant q2 ; cœur et certificat sur
GPU ; tour D5.

Voie GPU S3 (23 septembre, soir) : certificats de voie morte par lots. Le
[reçu de ventilation](receipts/q34_survivor_phases_20260923/README.md)
situe la phase des survivants à 08/000000, en ticks TSC écoulés sur hôte
chargé (parts indicatives, pas des cycles CPU ;
[addendum](receipts/q34_survivor_phases_20260923/ADDENDUM_20260923.md)) :
- cœur et couverture, avec leurs certificats : environ 35 % à K5, 28 % à K10 ;
- atlas, q3 et q4 des arêtes restées vivantes : environ 65 % et 72 %.

Calculer les formes du cœur à la première consultation rapporterait de
l'ordre de 1 % (projection, non bornée) ; seule cette variante est
écartée, et sur CPU. Même en supprimant idéalement tous les survivants,
R12 garderait 1,18 à 1,51 s à K5 : S3 n'est pas le dernier levier.

Livré (sans GCP) :
- préchauffage du contexte CUDA et de l'index plat pendant q2 ;
- libellé GPU honnête (`GPU_executed` exige une tour LiDAR achevée sur
  l'appareil) ;
- phase de certificats par lots (leviers `q34_batch_certificates`,
  `q34_gpu_certificates`), avec son port portable exact, un warp par arête,
  et la mise en attente sur le CPU d'une arête trop grosse ;
- juge de l'appel des certificats, arête par arête contre la référence CPU,
  exécuté par les deux préflights G4 ;
- condensé canonique (FNV-64) du catalogue, hors chronomètre ; ce n'est
  pas une égalité littérale du catalogue ;
- sonde et protocole G4 v18, plan R13 à 18 cas (voir `docs/PROVENANCE.md`).

Local : même tour, même catalogue et même travail des certificats que le
moteur sur la trame entière 08/000000/K5.

Session G4 R13 ([reçu](receipts/g4_tower_r13_20260923/README.md), paquet
`46c50432`, **`completed`**, `TERMINATED` certifié) :
- préflights GPU jugés arête par arête contre la référence CPU : ardoise par
  défaut, puis ardoise de 64 sites (3 404 mises en attente, le compte local) ;
- les 18 cas reproduisent les six condensés épinglés par C ;
- **K5 1,74 / 2,20 / 2,35 s**, **K10 5,91 / 7,83 / 7,97 s** (−11 à −13 %
  contre R12 à K5) ;
- préchauffage : appel du filtre de 205–315 ms à 81–154 ms ;
- S3 : 211 ms d'appel (189 d'appareil) pour 351 ms retirés aux survivants à
  000000/K5. Le noyau (un warp par arête, 250 registres, 8 warps par SM) a
  encore de la marge.

Reste à 000000/K5 (2,20 s) : survivants 0,73 s (atlas, q3 et q4 des arêtes
vivantes, et 0,7 M covers reconstruits), tour 0,75 s, certificats 0,21 s,
q2 et recensement 0,20 s. Suite :
- réduire le travail des survivants : atlas, q3 et q4 sur GPU (le shadow
  à huit cellules avant cœur ferme moins de 0,14 % des formes, écarté) ;
- tour : le chemin critique à K5 est la phase A mono-fil de l'ordre K5.
  Le saut D5 seul ne gagnerait que 5 à 10 ms à K5 (100 à 125 ms à K10,
  conception multi-agents du 23 septembre) ;
- noyau S3 : compteurs par arête en u32, 128 registres, 16 warps par SM
  (`0b41e4c86`, à mesurer sur G4).

Phase A allégée (`aa29245f`,
[reçu](receipts/tower_phaseA_lean_local_20260923/README.md)) :
- rangs de plateau exacts au lieu de produits U320 par facette ;
- lots singletons sans allocation pour les blocs inertes.

Localement, la phase A de l'ordre le plus élevé baisse d'environ 30 % à K5
et de 35 à 40 % à K10 (tour K10 −10 à −16 %).

Brouillon plat (`f93dc165`,
[reçu](receipts/tower_flat_draft_local_20260923/README.md)) : le chemin
statique range ses lots dans des tableaux plats, lus par les populations,
les images et l'encodage (constructeur générique). En local :
- phase A de l'ordre K5 encore −10 à −13 % (environ 790 → 455 ms depuis la
  base) ;
- tour K10 −15 à −23 % ;
- RSS −5 à −11 %.

Sonde et protocole G4 v19 : noyau et transferts d'appareil publiés
séparément (B). Plan R14 : les jumeaux GPU/moteur de R13, puis des paires S2
seul / S2 + S3 répétées et entrelacées (C, R-27). R14 mesure sur G4 la tour
allégée et le noyau S3 à 16 warps par SM.

Session G4 R14 ([reçu](receipts/g4_tower_r14_20260923/README.md), paquet
`b68b6761`, **`completed`**, `TERMINATED` certifié) :
- **K5 1,56 / 1,98 / 2,05 s**, **K10 5,49 / 7,00 / 7,05 s** ;
- tour à 000000 : 0,745 → 0,587 s à K5, 3,04 → 2,51 s à K10 ;
- noyau S3 : 114 ms à K5 (appel 189 → 116 ms sur l'appareil) ;
- paires répétées et entrelacées : S3 retire 0,22 s à K5 et 0,79 s à K10 ;
- épingles de C et juges des préflights conformes.

Reste à 000000/K5 (1,98 s) : survivants 0,73 s, tour 0,59 s, certificats
0,14 s, q2, recensement, front et filtre environ 0,39 s.

**S4** ([conception](docs/s4_conception_20260923/README.md)) : S4.0 (session
d'appareil résidente), puis S4a (voie q3 sans atlas sur le cover, une voie
par graine, après quatre restructurations CPU munies de portes), puis S4b (q4
par la fenêtre exacte, derrière une porte de coût). Projection : K5 vers
1,55–1,65 s à 08/000000.

**S4a livré en local** (voie q3 sans atlas, [provenance](docs/PROVENANCE.md),
sonde et protocole v20). Les quatre restructurations Ra1–Ra4 du plan sont
réunies dans un seul en-tête portable (`src/gpu/lanes.hpp`) :
- graines tirées du cover ;
- recensement exact par graine, réparti par site ;
- puissance non réduite, clé primitive seulement à l'acceptation ;
- exécuteur hôte par lots.

La porte d'objet juge chaque arête contre la voie q3 du moteur. L'ordre de
balayage par anneaux autour du milieu divise le travail des recensements par
36 à 08/000000/K5 (5,29 G → 146 M tests de points). En local (W8, CPU), le
levier reproduit les condensés épinglés à K5 et K10, chaque arête étant
jugée. Le noyau GPU compile (120 registres, sans débordement).

Session G4 R15 ([reçu](receipts/g4_tower_r15_20260923/README.md), paquet
`8b47a75a`, **`completed`**, `TERMINATED` certifié) :
- **K5 1,45 / 1,81 / 1,86 s**, **K10 5,24 / 6,72 / 6,69 s** ;
- paires répétées et entrelacées à 000000 : S4a retire 0,16–0,18 s à K5
  et 0,37–0,41 s à K10 ;
- noyau q3 : 28 ms à K5, 92–98 ms à K10, caché derrière les voies q4 du CPU
  (attente nulle) ; transferts des enregistrements du même ordre ;
- préflights jugés, ardoises réduites (11 976 voies q3 en traîne) et six
  épingles conformes.

Reste à 000000/K5 (1,81 s) : atlas et q4 CPU 0,54 s, tour 0,56 s,
certificats 0,14 s, q2, front, filtre et recensement environ 0,40 s.
**S4b livré en local** (voie q4 sans atlas par seaux de lentilles,
[conception](docs/s4b_conception_20260924/README.md),
[provenance](docs/PROVENANCE.md), sonde et protocole v21). Sur 08/000000,
la voie q4 de l'hôte est égale à la voie Local28 du moteur, arête par arête,
sur toutes les arêtes demandées : 576 456 à K5 et 1 357 994 à K10, soit
158 496 et 1 732 548 tétraèdres. La chaîne avec `q34_batch_q4` reproduit
les condensés épinglés, ainsi que le condensé des présentations du bras
moteur. Le noyau compile sur sm_120.

Session G4 R16 ([reçu](receipts/g4_tower_r16_20260924/README.md), paquet
`931a0862`, **`completed`**, `TERMINATED` certifié) :
- **K5 1,21 / 1,53 / 1,65 s**, **K10 3,92 / 5,16 / 5,01 s** ;
- paires répétées et entrelacées à 000000 : S4b retire 0,26–0,28 s à K5
  et 1,57–1,59 s à K10 ; CPU de la chaîne −53 % à K5 et −56 % à K10 ;
- 12 comparaisons égales, dont le condensé des présentations : même
  multiensemble (clé, arité, support) sur l'appareil et dans le moteur ;
- préflights jugés, ardoises réduites (11 993 voies en traîne) et six
  épingles conformes.

Le noyau des voies (190 ms à K5, 586 ms à K10, une arête par warp) entre
dans le chemin critique : les ouvriers l'attendent. Reste à 000000/K5
(1,53 s) : tour 0,57 s, appel des voies 0,28 s, certificats 0,14 s, q2 et
recensement 0,20 s, front, filtre, fusion et index environ 0,34 s.

Suite, pour le contrat de 1 s à K5 :
- la tour (phase A mono-fil de l'ordre le plus haut, phase 0) ;
- le noyau des voies : tâches (arête, graines) et raffinement des seaux,
  recouvrement avec les certificats ;
- ensuite, les certificats et le front/filtre.

**Conception tour + voies**
([synthèse](docs/tour_voies_conception_20260924/README.md)). Deux panneaux de
trois conceptions, avec un juge chacun, ont produit des plans par étapes :
- la tour vise ≤ 200 ms à K5 ;
- l'appel des voies vise ≤ 80 ms à K5.

Point corrigé : à K5, l'ordre critique de la fenêtre de la tour est K3, pas
K5. La phase 0 séquentielle pèse autant que la phase A.

L'étape 1 des deux pistes est réalisée en local, avec les mêmes condensés,
le même `tower_work` et toutes les arêtes égales au moteur à K5 et K10 :
- **tour** : arène de requêtes sans mise à zéro, images par rangs de
  plateau, sous-chronos E0 ;
- **voies** : buffers résidents réservés pendant q2, plus d'initialisation
  par valeur, T1 par candidats indépendants à sortie anticipée
  (`group_steps` −70 %), sous-chronos ;
- **protocole** : sonde et protocole v22.

Session G4 R17 ([reçu](receipts/g4_tower_r17_20260924/README.md), paquet
`b557fb75`, **`completed`**, `TERMINATED` certifié) :
- **K5 1,14 / 1,40 / 1,52 s**, **K10 3,53 / 4,62 / 4,57 s** ;
- à 000000 : −0,13 s à K5 et −0,50 à −0,54 s à K10 par rapport à R16 ;
- tour à K5 : 566 → 501 ms ; appel des voies : 275 → 199 ms, dont un
  noyau de 147 ms et 13 ms d'hôte ;
- 12 comparaisons égales, condensé des présentations compris.

Les sous-chronos E0 placent le chemin critique de la tour à K5 dans la
**phase A de l'ordre 5** (227 ms, un fil). La validation vaut 91 ms (tri
des niveaux 31 ms) et la queue 98 ms. À K10, la phase 0 séquentielle
domine (1,30 s).

Étape 2 livrée et mesurée : session G4 R18
([reçu](receipts/g4_tower_r18_20260924/README.md), paquet `446b45f7`,
**`completed`**, `TERMINATED` certifié). Une première tentative avait été
refusée avant démarrage, faute d'authentification GCP. Contenu :
- tour E3 (phase A maigre) et E2 (sections série de la validation) ;
- voies en tâches (arête, plage de graines), en étapes P, T et C sur
  l'appareil ; sonde v23.

Résultats :
- **K5 1,05 / 1,25 / 1,36 s**, **K10 3,26 / 4,27 / 4,18 s** ; 12
  comparaisons égales ;
- tour à K5 : 501 → 417 ms ;
- appel des voies à K5 : 199 → 147 ms (noyau 93 ms : P 12, T 49, C 32).

À K5, la fenêtre de la tour est bornée par la phase 0 séquentielle des
ordres (207 ms) suivie de la phase A des ordres inférieurs ; la queue vaut
environ 90 ms.

Session G4 R19 ([reçu](receipts/g4_tower_r19_20260924/README.md), paquet
`e9f0a104`, **`completed`**, `TERMINATED` certifié). Contenu : q2 pendant les
appels de l'appareil (levier `q2_during_device`, sonde v24) et certificats
S3 par blocs.
- **K5 1,03 / 1,18 / 1,31 s**, **K10 3,21 / 4,01 / 4,05 s** ; 12
  comparaisons égales.
- Le recouvrement de q2 retire 60 à 97 ms à K5 et 130 à 234 ms à K10
  (paires entrelacées).
- La préparation de l'appareil (136 à 224 ms) n'est plus cachée que par le
  front : le filtre l'attend jusqu'à 102 ms. C'est le prochain poste.
- Noyau des certificats : 117 → 90 ms à K5.

Session G4 R20 ([reçu](receipts/g4_tower_r20_20260924/README.md), paquet
`48791e72`, **`completed`**, `TERMINATED` certifié). Contenu : étape 3 des
voies (C sans boucle sur les tâches, L11, L10, L15 sous le levier
`q34_lanes_fused`), préparation de l'appareil en deux étapes, sonde v25.
- **K5 1,01 / 1,11 / 1,26 s**, **K10 3,15 / 3,87 / 3,86 s** ; 12
  comparaisons égales.
- Appel des voies à 000000 : 145 → 112 ms à K5 (noyau 57 ms : P 15, T 40,
  C 1) et 580 → 454 ms à K10.
- **L15 mesuré plus lent** : T +4 % à K5 et +3,5 % à K10 en paires
  entrelacées. Le levier reste désactivé par défaut.
- L'attente de la préparation (11 à 82 ms à K5) vient de la création du
  contexte CUDA, payée une fois par processus : les deux étapes ne la
  cachent pas.
- Le transfert des enregistrements des voies (128 o, mémoire pageable)
  coûte 30 ms à K5 et 151 ms à K10.

Session G4 R21 ([reçu](receipts/g4_tower_r21_20260925/README.md), paquet
`d054c1c5`, **`completed`**, `TERMINATED` certifié). Contenu : tour intégrée
(queue en pipeline E4, regroupement haché de la phase 0, pool persistant E2,
options nommées), session d'appareil ouverte par le processus (v26), sonde
v27, trames brutes avec sol.
- **K5 0,97 / 0,81 / 1,03 s** sans sol (08/000000, 000100, 000200) : sous la
  seconde sur deux trames sur trois ; **K10 3,17 / 2,45 / 3,14 s**.
- Trames brutes avec sol, 123 à 126 k sites : K5 1,91 à 2,18 s, K10 5,71 à
  6,49 s, épingles de C reproduites, reports sans refus de capacité.
- Tour à 08/000000/K5 : 300 ms, contre 424 ms pour le témoin apparié
  (phase 0 de 209 à 83 ms) ; à K10, 1,27 s contre 1,89 à 1,94 s.
- 34 cas complets, 22 comparaisons égales, douze épingles reproduites.

Session G4 R22 ([reçu](receipts/g4_tower_r22_20260926/README.md), paquet
`43c5ad25`, **`completed`**, `TERMINATED` certifié). Contenu : catalogue scellé
(R-29 de C), recensement des clés q2 côté q2, bassin hôte épinglé des voies,
sonde v28 ; constats 2, 3 et 8 à 12 de C corrigés.
- **K5 0,93 / 0,76 / 0,98 s** sans sol (08/000000, 000100, 000200) : **sous
  la seconde sur les trois trames** ; **K10 2,96 / 2,27 / 2,89 s**.
- Trames brutes avec sol : K5 1,81 à 2,03 s, K10 5,31 à 6,01 s.
- Les trois leviers retirent 62 à 82 ms à K5 et 224 à 266 ms à K10 (paires
  avec `gpu_r21`) ; copie des enregistrements des voies divisée par 14.
- 36 cas complets, 24 comparaisons égales, douze épingles reproduites.

Suite, après le seuil d'une seconde à K5 sans sol (trames avec sol : 1,81 à
2,03 s à K5 ; 100 ms restent l'objectif) :
- tour : A(Kmax) (148 ms à K5), l'allègement d'abord puis la
  parallélisation après preuve ; phase 0 sur l'appareil (E6) ;
- glu hôte de q3/q4 (environ 300 ms au brut K5) et tâche traînarde du front
  (C) ;
- K10 ; TSan sur la porte des huit combinaisons et sur `--unwind`.
- (fait en R22 : positivité côté chaîne et catalogue scellé, recensement
  q2 précoce, bassin épinglé) ;
- encodage scindé (E5) ; front, filtre, certificats ; les trames avec sol
  demandent la tour sur l'appareil (C).

Suite : tour maigre (D5 de l'auditeur C :
index des selles, saut au centre, images de naissance directes), puis
réfutation des ancres longues avant expansion (66–70 % du CPU q3/q4 selon C).

Suite : coût q3/q4 (ventilation K10 W8 locale en Gcycles : atlas 229, q4 235,
paires 187, rectangles 123, noyau 120, q3 120, preuve sur cover 86) ; seuil
K−2 des arêtes q4 seules ; équilibrage de la tour K10 ; voie GPU (V9-4).

## État du dépôt au moment de l'ouverture (historique)

- `origin/main` avant l'ouverture : **12294241** (dernier commit d'audit v8 du
  22 septembre). La v8 est gelée à ce commit pour ses sources publiées ; la v7
  n'a pas changé depuis `dc57ffd5`.
- Le commit d'ouverture v9 a été préparé dans un worktree séparé, sans toucher
  l'index du worktree partagé `/workspaces/E-HGP`.
- **Worktree partagé** : `HEAD` local à `a74e90f2`, en retard des 13 commits
  d'audit et de ce commit d'ouverture. Son index contient une tranche **non
  commise** de 89 fichiers, qui mêle le travail du développeur v8 (portes
  jumelles 18 bits et campagne d'identité u16, 06:29–06:53 UTC) et celui d'un
  « constructeur » (reprise u18 et atlas saturant, 09:58–10:56 UTC). L'arbre
  porte aussi des modifications non indexées (`AGENTS.md`, passation, journal
  et coordination v8, fichiers v6/v7) et non suivies (captures
  `u18_resume_20260922`, brouillon float32 global, notes de l'auditeur
  complémentaire du 13 septembre, `claude-install.sh`). Rien de cela n'est
  dans la v9. La tranche doit être commise en v8 (après correction de son
  lecteur de captures, qui refuse des CTests pourtant verts) ou abandonnée par
  écrit ; c'est une décision pour l'utilisateur. Toute personne qui travaille
  dans ce worktree doit d'abord vérifier `git diff --cached --quiet`.

## Ordre de lecture

1. [README](README.md), puis la [synthèse de l'audit](docs/AUDIT_V8_SYNTHESE.md).
2. [Plan](docs/PLAN_V9.md), [héritage](docs/HERITAGE_V7_V8.md),
   [fausses pistes](docs/FAUSSES_PISTES.md).
3. Selon la tâche, les [rapports détaillés](docs/audit_v8/README.md).
4. Pour l'objet : `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`,
   `morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md`, le registre
   `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, et les parties I–II du
   manuscrit (`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`).
5. Pour le générateur : `morsehgp3D_v8/docs/ALGORITHME_EXPLIQUE.md`,
   `morsehgp3D_v8/docs/Q34_GLOBAL_ET_LIDAR_20260921.md`,
   `morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md`,
   `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` (le code fait foi
   sur les bornes : la note publiée contient des bornes fausses, relevées par
   l'audit, sans effet sur le code).

## Première tranche proposée

Phase V9-0 du [plan](docs/PLAN_V9.md) : manifeste CMake, CI, portes à code
exact, mutants `--inject`, type de point certifié, table de bornes, format de
reçu, données hors Git. Puis V9-1 : la tranche verticale mono de bout en bout
(générateur porté, catalogue canonique, tour FULL, lanceur chronométré) sur les
trois trames sans sol 1 mm. Le premier chiffre utile de la v9 est le temps de
la **tour complète** sur ces trames, pas celui d'un composant.

## Décisions et questions ouvertes

L'utilisateur veut un livrable qui fonctionne et laisse les choix de détail
au développeur (22 septembre). La [synthèse](docs/AUDIT_V8_SYNTHESE.md) § 9
fixe donc des hypothèses de travail révocables : contrat jugé d'abord sur les
trames sans sol, chronomètre du nuage préparé en mémoire à la tour complète en
mémoire (lecture, grille et masque mesurés à part) ; nœuds explicites
convertibles en `CertifiedTowerInput` ; sites distincts. Les oracles de
correction bornés servent de portes ; aucune étiquette SemanticKITTI.
L'échelle multi-millions vient après les contrats LiDAR. Restent à
l'utilisateur : les données KITTI et le profil OS Login versionnés dans un
dépôt public, et le sort du travail non commis d'autres acteurs. En
attendant, la v9 ne versionne aucun octet KITTI.

Rectificatif au 23 septembre : « jugé d'abord sur les trames sans sol »
désigne le **premier jalon de développement**, non une substitution au
contrat principal sur trame brute entière de plusieurs séquences. La
grille 1 mm est devenue prioritaire sur choix explicite de l'utilisateur ;
float32 reste un objectif secondaire distinct. Le reçu GPU S1 ci-dessus
mesure un filtre isolé sur sans-sol, jamais cette tour contractuelle.

## Entrées disponibles

Les trois trames sans sol à 1 mm (08/000000, 000100, 000200 ; 39 885 / 35 551 /
45 845 sites) existent aujourd'hui dans
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`
(sha256 dans les manifestes voisins). Elles sont versionnées en v8, ce que la
v9 ne reproduit pas : régénérer ces entrées depuis les scans bruts par un
fetcher et les préparateurs, dans un `data/` ignoré par Git, et ne versionner
que leurs manifestes.
