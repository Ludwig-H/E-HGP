# Audits v10 — état courant

Mise à jour : 1er octobre 2026, 18 h50 UTC. Sauvegarde avant coupure du
codespace. Priorité utilisateur :
vérifier la présence de la GT dans FULL puis dans une hiérarchie
laminaire de points ; sélection et z sont REMIS À PLUS TARD.

**Dernière reprise indépendante A→B terminée normal/−O.** Sur256 scènes
natives medium/hard à8 groupes, K5 : A contient316/2048 groupes EXACTS,
1578 maxima dépassent0,8 ; cover conserve312 groupes exacts et1565
maxima au-dessus de0,8. IoU moyens0,842310→0,835397, perte0,006913.
31 scènes ont leurs8 groupes exacts dans A,32 dans cover. Huit égalités
exactes sont perdues, quatre gagnées ;355 groupes sont purifiés par cover.
Ce sont des maxima par groupe, PAS une coupe à rayon commun ni une
preuve de complétude du catalogue. «1−IoU moyen» n'est pas la proportion
de GT absentes. La moyenne quasi identique core/cdelay[1] masque308/2048
maxima différents : leurs blocs ne sont pas identiques.
[Résultats, témoins et instructions de reprise](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#sauvegarde-du-1er-octobre-à18-h50-utc).

**Référence modale : corriger la définition et le coût avant extension.**
Le nouveau helper calcule des bassins d'EM modal, pas nécessairement
ceux du gradient euclidien ; un exemple anisotrope exact les distingue.
Une convergence peut aussi aboutir à un point selle. Les filaments
finis ont un maximum isolé, quoique très plat. Enfin, group_modes
matérialise n(n−1)/2 paires lorsque les sites convergent vers un mode :
quadratique dans LA RÉFÉRENCE, pas un nouveau défaut du moteur FULL.
Preuve locale rejouée normal/−O ; aucune qualification GPU/100ms.

**Sauvegarde et prochain travail.** L'archive privée compacte associée
à la note détaillée conserve les scripts, les essais échoués et les
petits témoins hors /tmp volatile. Le MAP courant reste contournable
sur ses cinq gardes ; le nouveau plan1920 unités n'est pas une
qualification. Deux défauts supplémentaires de table_completion_locale
sont reproduits : hash CSV ignoré et doublon de clé écrasé.
Prochaine étape : fermer les gardes MAP puis mesurer ses témoins A/B
sur quelques scènes iid, avec présence exacte ET compatibilité,
avant la campagne étendue. Ne pas régler z maintenant.
G4 vc1 a désormais un reçu local : commande CPU code0,1409,827s,
closure=stopped. Ni l'inventaire complet ni ses résultats nouveaux
ne sont encore rejugés ici ; pas une mesure GPU ou100ms.

**Q7/Q8 : deux réponses vérifiées pour le développeur.** Condenser
FULL puis affecter chaque point une fois garantit des partitions
emboîtées pour un mcs fixé, mais pas leur stabilité géométrique.
Cinq points K2/core-mcs2 donnent un saut de réunion5,5→10 sous une
perturbation arbitrairement petite, sans changement de cofaces.
Vraie AST et Γ exhaustive :2150 gardes,30 profils, normal/−O.
Sur la même famille, le délai à marge d'ER0h supprime ce saut :
la réunion tend vers10 des deux côtés.66 profils,2316 gardes,
chemins fermé/crédits/formule indépendante et les CINQ points contrôlés.
Ce n'est pas une preuve générale de continuité ; ER0h n'a pas le
même mcs/cœur ni les mêmes dates précoces que la première tête.
[Contre-exemple et portée du résultat positif](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#q7--condenser-puis-voter--laminarité-oui-continuité-non).

**Lemma M est correct ; une compression est possible, pas acquise.**
Les crédits se transportent proportionnellement à S(enfant), en
divisant par S(parent)−ω(parent).96 forêts,504 masses,892 contrôles,
deux erreurs arithmétiques rejetées causalement. Les chaînes à
enfant unique transmettent toute la masse et peuvent être télescopées,
à condition de conserver leurs poids au prochain embranchement.
Le prototype scanne encore points×ancêtres : la formule seule ne
prouve ni croissance sous-quadratique ni gain100ms.
[Preuve, tests et piste de port](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#q8--lemma-m-confirmé-et-effet-réel-du-délai-à-marge).

**Complétion corrigée effectivement présente.** Renommage des groupes,
k voisins déjà étiquetés, rayon par groupe et dette au plafond corrigés.
La variante engagée une fois est laminaire. Sur46 archives DEV àK5,
cover passe de0,932611 à0,935627 de mIoU oracle avecρ1 ; sans limite,
elle tombe à0,051645. Pas un levier majeur démontré ni un test scellé.
Réserves nouvelles : «jamais retardé» ne contrôle pas les dates des
branches incomparables ; unreached omet les refus par densité ; domaine
u18 à valider AVANT les produits.81 petits contrôles normal/−O, sans
erreur native ou LiDAR déduite. Les anciens contre-tests restent épinglés
à l'ancien code et ne réfutent pas les corrections actuelles.
[Nouveau code, scores et gardes ciblées](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#complétion-corrigée--progrès-réel-et-limite-de-dates).

**Nouveau MAP : modèle amélioré, certification à renforcer.** Le
module map_ref est maintenant présent avec les vraies densités des
composantes. Cinq contre-contrôles courants normal/−O : compenser
T×2 par Z÷2 conserve le brut mais contourne la certification des
paramètres et change une décision ; les IDs sub, les coercitions,
les sites postcert et le hash MAP du lecteur restent à protéger.
Objets corrompus en RAM, pas erreurs réelles de la campagne déduites.
Le bruit à boîte dépendante du tirage donne une référence plug-in,
pas automatiquement le posterior conditionnel exact du générateur.
[Témoins et corrections limitées](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#nouveau-map--bonnes-densités-certification-encore-contournable).

**A contient beaucoup de bonnes GT, pas toutes.** Recoupe rationnelle
ROOT sur199 scènes natives medium/hard à8 groupes : àK5,1413/1592
maxima dépassent0,5 et1240 dépassent0,8 ; seuls244 valent exactement1.
La moyenne0,846120 ne prouve pas «la GT est dans FULL». Examiner les
groupes restants et leurs témoins avant de régler la sélection.
[Comptes et prochaine vérification A→B](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#présence-dans-a--publier-les-groupes-manquants-pas-seulement-la-moyenne).

**G4 clôturée et R2 : états désormais recoupés.** tvpc1 produit536 unités,
tvpc2 les262 restantes, sans chevauchement ;798 unités ciblées,256
exclusions easy/extreme distinctes. ROOT vérifie1656 puis834 hashes,
aucune mutation cloud. Ancien payload CPU, pas qualification GPU/100ms.
R2 : les six portes GCC/Clang/ASan/TSan sont maintenant terminales,
mais Pool reste code1 avec MR1 SURVIT ; pas de qualification globale
déduite des CTests verts ni des preuves anciennes copiées.
[Reçus et réserve Pool](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#sessions-g4-tvpc1tvpc2-et-intégration-r2--état-terminal-courant).

## Observations antérieures ancrées à leur lecture

Les paragraphes suivants conservent les constats des versions et reçus
précédents. Leur statut RUNNING/partiel ou leur contre-test d'ancien
helper n'est pas le statut courant lorsqu'il est remplacé ci-dessus.

**Comparaison MAP demandée, neuf scènes effectivement calculées.**
ROOT génère neuf mixtures gaussiennes iid de2048 points,2/3/8/20
communautés, puis exécute18 diagnostics natifs àK5/K10 sur grille1mm.
Paramètres vrais connus, aucun ajustement de mixture, aucun réglage z.
Aux séparations faciles, FULL retrouve les classes MAP avec IoU moyen
0,986–0,990 et cover0,989–0,997. Avec deux gaussiennes séparées de3σ,
MAP classe93,7% des labels générateurs correctement mais FULL n'a
que0,522/0,500 d'IoU moyen face aux classes MAP, àK5/K10 : l'écart
apparaît déjà dans A, pas uniquement dans la sélection différée.
Une composante de niveau de densité n'est cependant PAS une classe
MAP : même deux bosses distinctes peuvent fusionner avant d'avoir
absorbé leurs queues. Cette distinction est démontrée sur la mixture
de population ; elle ne prouve pas la cause de chaque perte native.
Lecteur indépendant relu puis rejoué ROOT normal/−O :9 entrées
reconstruites,18 appels,18 432 retours conservés, zéro fusion.
Reçus privés LIVE, ordre K seul, pas qualification GPU/FULL1..K,
archive autonome, croissance LiDAR ni victoire générale.
[Tableau MAP et conséquences pour la projection](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#comparaison-map-et-full-sur-neuf-mixtures-gaussiennes).
**Complétion : un potentiel plat n'est pas une hiérarchie.** La nouvelle
sonde choisit une antichaîne AVEC les GT, puis propage les labels par
k-NN. Deux contre-exemples de la vraie AST, rejoués ROOT normal/−O :
un départage dépend du numéro arbitraire des groupes ; recalculer la
complétion après fusion fait croiser deux blocs. Ce n'est pas un
défaut démontré de FULL natif. Corriger le départage local puis, pour
une hiérarchie, engager chaque point une fois et suivre les ancêtres ;
publier séparément les sorties hors couverture FULL et la perte A→B.
[Témoins et correctifs ciblés](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#complétion-des-queues--ne-pas-confondre-un-découpage-et-une-hiérarchie).
**Le MAP du nouveau helper n'est pas toujours celui du générateur.**
La famille bridge a des segments gaussiens flous, pas le bruit uniforme
utilisé par bayes_ref. Son garde de rejeu contrôle les tailles seulement ;
trois covariances volontairement corrompues restent acceptées malgré
des décisions différentes. Aucune erreur de covariance réelle déduite.
Ces réserves ne changent PAS les neuf mixtures iid du diagnostic ROOT,
avec paramètres vrais et lecteur indépendant distinct.
[Modèle du helper à expliciter](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#map-du-helper--modèle-exact-et-preuve-de-rejeu).
**Nouveaux lots G4 maintenant terminaux et partiels.** tvppy1 :1587/2336
unités présentes,749 restantes ; neuf erreurs de conversion décimale
de Fraction en métadonnée, onze unités avec délais de règles. Zéro
échec worker ne les qualifie pas. tvppy2 : délai externe1480s/code124,
1200 unités archivées sur3072 prévues, pas de FIN ni done_units.
Parmi elles,18 délais et huit unités avec tables headers seuls :
oracle_ok=1 ne signifie pas projection produite.
ROOT vérifie tous les4809 puis3647 hashes ; fermeture des générations
certifiée par les reçus du développeur. Aucun stop/recover ROOT.
CPU46 processus sur G4, pas GPU ni contrat100ms. Les statuts courants
remplacent les observations RUNNING antérieures, sans les effacer.
[Inventaire, causes et limites](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#sessions-g4-tvppy1-et-tvppy2--résultats-partiels-et-causes).
**Intégration R2 privée :** nouvelles portes GCC82/82, ASan80/80 et
TSan80/80 terminales ; G4sim44/44 reste CPU. Tête :52 rejets, quatre
équivalents, une limite, baseline après identique. ClangASan encore
74/80 à la recoupe14 h57 ; pas qualification globale transférée.
[Journaux et portée](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#intégration-r2--nouvelles-portes-terminales-réserves-distinctes).
**Gabriel strict et normalisation sans expansion.** La définition28
autorise les points extérieurs sur la coquille. Le filtre de population
fermée du prototype est donc plus restrictif que l'Algorithme1.
Carré, cube K5 et cube avec intérieur : référence Fraction,2810 gardes,
normal/−O identiques ; la vraie AST TT manque les cofaces du carré.
Pour TOUTES les facettes, T_x=KΣ_{σ contenant x}ψ(ρ_σ) exactement :
normalisation possible par classes de boules, sans développer chaque
facette. Cela ne borne pas le comptage des coquilles. Une facette
requise a aussi sa MEB absente du catalogue saturant Kmax10 : son
attache doit être résolue séparément, pas héritée aveuglément de la coface.
[Formules, cas géométriques et portée](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#gabriel-strict-coquilles-et-masses-sans-expansion).
**Nouvelle G4 tvpab1 récupérée, partielle par ordonnanceur.** La commande
CPU réussit :741/1054 unités,313 restantes, zéro échec d'ouvrier.
Toutes les512 petites unités sont faites ;313 unités8k n'ont pas été
soumises après l'échéance des tâches longues. ROOT vérifie les2332
hashes et l'inventaire exact de l'archive ; l'autre auditeur contrôle
les741 unités et465204 lignes A/B, sans anomalie réelle observée.
Le worker global rend1 car le contrôle Python par défaut échoue ;
la commande utilise son environnement embarqué et rend0. Ne pas
confondre ces deux statuts ni déclarer la batterie complète.
Vingt snapshots ont été évincés par le plafond d'archive, les tables
restent présentes. Ancienne génération arrêtée puis VM réutilisée
pour une nouvelle génération : aucune reprise ou mutation ROOT.
CPU sur hôte G4, pas qualification GPU/100ms ni croissance LiDAR.
[Inventaire et provenance G4](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#nouvelle-session-g4-tvpab1-et-inventaire-récupéré).
**Témoins G4 réels reconstruits, pas seulement des moyennes.** Sur
une scène2k hierarchical/20 parents/60 sous-groupes, ROOT relit et
rejoue normal/−O un lecteur indépendant :36 arbres,2880 lignes B,
8640 IDs de témoins et640 comparaisons Bnat↔B, aucun écart.
Les20 parents sont exactement présents dans A. ÀK5, sous-groupes :
FULL0,997548, cover0,997043, core0,934641, HDBSCAN0,964688.
Les meilleurs blocs B sont tous compatibles en antichaîne, membres
reconstruits ; les meilleurs sous-blocs n'ont PAS de coupe commune,
sans exclure d'autres choix compatibles à rayon commun. Entrée u32
et GT base recoupées par hash ; brut float64 non reproduit bit à bit,
sous-labels dérivés du générateur épinglé mais non hachés dans le reçu.
Une scène DEV, pas classement final ni règle utilisant les GT.
[Témoins et limites](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#témoins-g4-reconstruits-et-compatibilité-réelle).
**Q4 précise un verrou de fidélité à la thèse.** L'Algorithme1 prend
TOUTES les facettes des cofaces Gabriel, pas seulement les facettes
elles-mêmes Gabriel. Le prototype choisit le second univers. Une
référence exacte privée, relue et rejouée ROOT normal/−O, confirme
des masses différentes malgré la même conservation totale, sur
quatre points collinéaires ET non collinéaires. Ne pas identifier
équivalence topologique et conservation des poids frontière.
[Référence Q4 et univers à expliciter](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#q4-référence-exacte-et-univers-des-facettes).
Le banc PR DEV est TERMINÉ :96 scènes,23 040 lignes, zéro refus.
Le diagnostic dominant est une fragmentation excessive, sensible à
l'exposant et à min_cluster_size. La tête propriétaire unique puis
EOM n'est PAS celle de la thèse : celle-ci répartit une unité par
point entre ses facettes AVANT condensation/EOM, puis vote.
[Résultats et prochaine campagne à trois étages](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#campagne-à-trois-étages-et-portée-du-plafond-full).
Même géométrie et même convention K confirmées. La copie TVP corrige
maintenant Decimal et le rescan des comptes ; le paquet iou1 garde
ses anciennes sources. Filtre d'ancêtres et préenregistrement restent
distincts. Aucun ancien score réel n'est déclaré changé par ces corrections.
z2 figure dans les nouvelles grilles ; ajouter plusieurs nombres de
communautés, encore fixés à8 dans les grandes scènes DEV.
R2 f42669a : GCC75/75, ASan74/74 et TSan74/74 terminaux ; les44 tests
«G4sim» sont CPU, pas GPU ; clang complet reste74/75. Le nouveau
ECLI, Pool et bancs sont terminaux. Le reçu privé fad6f68 contient
140 pièces dont les six portes Pool complémentaires ; ROOT vérifie
ses140 hashes. Cela ne clôt pas les réserves causales détaillées.
B21 est suspendu
par le développeur à08 h19 pour libérer la machine ; ses campagnes
interrompues ne sont pas closes. Aucun nouveau contrat FULL/G4,
performance100ms ou croissance LiDAR acquis.
Les sources publiées du moteur restent inchangées
dans cette tranche ; le développeur travaille désormais dans une copie
isolée d'intégration, distincte du worktree partagé. Les parties I
et II de la thèse ont été relues intégralement dans la tranche précédente.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

**Session G4 iou1 reprise par le développeur, sans résultat récupéré.** ROOT observe
directement à09 h51 la VM TERMINATED, même génération09 h03 min45,325 s,
lastStopTimestamp09 h50 min54,384 s. Aucune commande mutante ROOT.
Le développeur publie maintenant la reprise gardée10 h07,
already_terminated/arrêt certifié et clé retirée. ROOT recoupe les
trois hashes publiés et celui du reçu complet hors dépôt : aucun
résultat rapatrié, aucune mesure tirée de cette session. Les512 scènes
DEV et10 unités Zoltan sont PRÉVUES, pas déclarées calculées.46 processus
CPU, pas de chrono GPU/FULL1..K100ms. Le paquet conserve le défaut
Decimal des additions EOM. Les cinq scènes LiDAR sont complètes sur
grille1mm, quatre sans sol et une avec sol, toutes séquence08.
Sur décision explicite de l'utilisateur, ROOT ne récupère ni n'arrête
la session. La cause est maintenant déclarée par le développeur,
distincte de l'observation cloud indépendante. L'arrêt ne prouve
pas la réussite de la batterie.
[Preuves, coûts et réserves](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#session-g4-iou1-et-nouvelle-campagne-à-ne-pas-confondre).
La nouvelle campagne trois étages a désormais deux petits tests
locaux terminaux :2k bridge et8k hierarchical. Chacun384 lignes A/B
et3192 configurations C, zéro refus/doublon observé. Sur bridge/K5,
potentiel FULL0,912, cover0,903, core0,748 ; mIoU plat EOM/z1/mcs20
sans remplissage : cover0,889 contre HDBSCAN0,809. Un seul cas DEV,
pas confirmation générale. Sur hierarchical, les huit GT ont
rappel1 et IoU0,980–0,996 dans FULL et cover. Ce sont des maxima
indépendants : tester aussi leur compatibilité, pas seulement la moyenne.
[Qualification et correctifs du collecteur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#diagnostic-tvp-premières-mesures-et-corrections-causales).
Une nouvelle capture bridge2k termine à12 h08 min57 s, avec le
comptage corrigé. ROOT recoupe ses entiers : àK5, cover conserve
un rappel moyen0,942 contre0,770 pour core. Ses huit meilleurs blocs
ont au moins196 vrais positifs et au plus16 faux positifs ; sous
l'invariant laminaire vérifié, ils sont donc DISJOINTS. Ce certificat
prouve une antichaîne, pas encore une coupe à rayon commun ni une
égalité exacte aux huit GT. Les IDs manquent aux CSV de cette capture ;
le code TVP courant les ajoute désormais, sans réécrire l'ancien reçu.
[Preuve et sortie à conserver](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#présence-des-gt-et-compatibilité-sans-sélection).
Les nouvelles Q4/Q5/Q6 du développeur sont répondues : masses §9.1
distinctes de la projection actuelle ; petite référence Q5 recoupée
ROOT741 cas/3344 antichaînes ; packing exact A seulement sur petits
diagnostics, pas extension du DP aux couvertures recouvrantes.
Le rescan initial56/240/992 est supprimé : vraie AST corrigée,
7/15/31 éléments inspectés, normal/−O. Petit oracle A/B5670/5670
terminal, inventaire désormais contrôlé. Le collecteur3f0d1c22 refuse
maintenant groupes répétés, mauvais unit et inventaire vidé par délai ;
ce dernier rend PARTIEL_PAR_DELAIS/code3. Des métadonnées et valeurs
impossibles restent acceptées ; l'autre auditeur les retrouve encore
dans af97b8c0 par la vraie AST, sans qualifier une campagne nouvelle.
La jointure A→B entre parties est corrigée et recoupée ; ne pas qualifier
le nouveau chemin ab depuis l'ancienne capture abc.
La réduction TVP5fbeac7d retire les événements vides et continuations
sans entrée propre : mêmes blocs, au plus2N événements conservés pour
N≥1 attaches, preuve et contrôle abstrait4319 arbres/25200 coupes.
Cela ne borne pas les incidences/votes de la thèse ni tout FULL.
Le raccord des niveaux exacts de l'export
de points reste une réserve distincte du meilleur score par GT.
Le lemme des amas discrets via boules fortes est recoupé indépendamment
sur373 couvertures exactes ; cela ne qualifie pas le catalogue natif.

**Qualité mesurée et piste prioritaire.** Sur les64 scènes DEV medium/hard,
K5/mcs20/EOM sans remplissage : F1 objets cover/z6=0,227, cover/z1=0,600,
core/z6=0,604, HDBSCAN=0,628. À mcs=√n, cover/z6=0,703 contre0,652 ;
les gaussiennes sphériques donnent1,000 contre0,841. Ce sont des
moyennes DEV, pas une victoire holdout ni une domination générale.
Décision utilisateur : arrêter la préparation des anciennes batteries
et réorganiser FULL discret→projections→condensation/sélection.
L'oracle indépendant par vrai groupe n'est pas une partition réalisable.
Le meilleur IoU d'une couverture FULL n'est pas non plus un plafond
automatique après projection : celle-ci peut enlever une contamination
frontière. Publier cette purification et les pertes de rappel séparément.
Reprendre la conservation des masses frontière avant
sélection, pas seulement renommer un vote final. Laminarité exige
ensuite un engagement ponctuel unique suivi de ses ancêtres ; un
argmax recalculé indépendamment à chaque coupe ne suffit pas.

**Deux optimisations exactes à porter :** [préfixes des poids temps-points et saut admissible](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#couper-les-remontées-hors-bande)
évitent respectivement le rescan de tout l'amas par vote et les remontées
avant de connaître la bande. Contrôles ROOT des vraies AST :1680 poids
et7920 comparaisons A/votes/mode, normal/−O sans écart. Les deux médianes
d'ER s'appliquent aussi aux poids temps-points positifs figés.
Le poids temps-points admet désormais une préparation SANS D :
B_v IDs distincts à naissance et seulement les premières entrées
propres tardives.90 décompositions/765 poids exacts concordent ;
deux recherches de préfixe suffisent, stockage des masses O(N+T).
Les graines complètes, le calcul de B_v et les votes restent payés.
Il reste à porter ces briques, pas à développer inc puis le compresser.
Aucun gain natif, croissance LiDAR ou résultat100ms nouveau.

**Saut exact des continuations désormais démontré localement :** une
[requête par niveau](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#sauter-aussi-les-continuations-dans-la-majorité-et-le-cône)
trouve la majorité du segment ; seuls deux niveaux entourant
W²/(16κ²A) sont utiles pour son cône intérieur.14 petits contrôles de
majorité et56 comparaisons composées exactes normal/−O passent, avec
pré-histoire réelle et calcul radical indépendant. Le propriétaire à H
est en coupe fermée, l'atome médian Euler en coupe gauche : ne pas les
confondre. Jonctions rivales, première activation, majorité antérieure
et owner final restent séparés. Pas de port ni preuve globale LiDAR/G4.
Le raccord sur branches abstraites est maintenant recoupé :14 profils,
127 votes uniquement dans l'oracle, deux vraies AST concordantes avec
la sélection comprimée, normal/−O sans écart. Les LCA et égalités sont
contre-jugés ; produire le squelette natif complet sans D reste ouvert.
Neuf cas racine supplémentaires passent, avec cap fini ; deux
découpages gardent les votes identiques. Une lignée avec atome Euler
interposé ou jonction rivale intérieure doit être scindée, pas traitée
aveuglément comme une continuation stricte.

**Robustesse ER : le seuil n'est pas le seul problème.** Le
[témoin Thalès contre-recalculé](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#la-naturalité-héritée-peut-sauter-au-plateau)
fait disparaître une branche de vie nulle, loin deρ=Λ, mais change
l'ancre et toute la masse héritée. Une rampe enρ seule, ou un facteur
vanissant commun avant renormalisation, conserve ce saut. Réparer
l'influence héritée et le raccord du repli avant de présenter ER comme
robuste ; ce n'est pas une réfutation du théorème à strate fixée.

**Réponses et aide au port :** les comptages LCA généraux ont maintenant
un [reçu statique clos](../receipts/audit_continu_20260929/distinct_lca_counts_20261001/README.txt),
20 profils/180 cardinalités, replays normal/−O identiques, cinq contrôles
arithmétiques causaux. Les profils avant élagage exigent encore des
corrections tardives ; ne pas y appliquer la formule simplifiée de
l'antichaîne. Un petit diagnostic natif de tête confirme séparément
la bonne sortie d'un point entrant tardivement ; son mutant change
la stabilité et le bruit et est refusé. Une nouvelle capture est
[close et contre-recompilée](../receipts/audit_continu_20260929/head_direct_exit_20261001/README.txt) ;
le premier diagnostic OPEN n'est pas réétiqueté.
[Détails et scopes](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#tête-nouvelle-et-dent-sur-lentrée-tardive).
Le tableau ECLI est informatif, pas une porte :11 contrôles de sa vraie
AST confirment code0 malgré journal vide, doublons ou référence divergente.
Les63 observations réelles restent valides dans leur portée recoupée.
Le commit privé f42669a ne publie pas ses sources sur main : head.cpp
privé371d1444 diffère encore de la source publiée f583da40 à la recoupe.

**Nouveaux lots globaux séparés :** extraction f42669a, GCC75/75,
ASan74/74, TSan74/74 et cibles CPU «G4sim»44/44 terminaux ; ce dernier
n'est pas une exécution G4. Le sous-lot tête suivant a aussi son témoin
final positif et ses binaires identiques :52 rejets documentés, dont
trois plafonds de coût, cinq équivalents annoncés, un signal Os sans
diagnostic et une LIMITE RO3 sans rejet. Le nouveau ECLI termine
avec60 mutants déclarés tués et trois équivalents, témoin final0.
Pool principal conserve six MOTIF_ABSENT ; un complément ciblé les
exerce6/6 par thread_creation_refusal1, avec témoin final13 portes0.
Les bancs terminent91/95 rejets et quatre équivalents en Python3.12,
21/23 et deux équivalents dans le sous-lot3.10. Causes et incidents
restent séparés ; le reçu collecté n'est pas une qualification G4.
B21 est désormais suspendu. À08 h23, la recapture ciblée HEAD0f4115
a8/8 mutants dans chaque build, six rejets et deux survivants ; pas
de témoin final sanitizer ni inventaire/table. AA1 sanitizer s'arrête
à36/87 ; le nouveau lot complet Release0f à17/88, code143. Ne pas
fusionner ces inventaires ou transformer l'interruption en clôture.
Le juge à l'échelle ne couvre que des
boules/supports locaux et attaches core échantillonnés : pas tout FULL.
Sa vraie AST accepte des IDs intérieurs répétés et la présence fondée
sur le seul hash ; corrections simples recommandées, aucun défaut
géométrique natif observé déduit. Le banc PR relancé à07 h21 avec
la source175245 termine à08 h23 min47 s :96/96 scènes calculées,
zéro échec worker. Conserver séparément l'ancienne campagnefc52.
[Détail des résultats et réserves](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#tête-nouvelle-et-dent-sur-lentrée-tardive).

**Corrections PR avant le test final :** [contre-tests et proposition de port](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#comparaison-pr-et-corrections-avant-le-test-final).
Les puissances utilisent Decimal80, mais les additions EOM le contexte
réel28 ; deux cas sur un petit arbre changent de choix par rapport à Fraction
et Decimal80. Les quasi-égalités sont comptées, pas départagées comme
le dit la documentation. Encadrer toutes les opérations dans le même
contexte et distinguer décision approchée et décision certifiée.
Le filtre Python des ancêtres fait9/35/135 remontées sur des peignes
à4/8/16 feuilles ; une passe7/15/31 nœuds garde les mêmes choix.
Le nouveau verdict «un gain, zéro perte» est un amendement explicite
du générateur, mais reste incompatible avec le draft courant à
majorité stricte. Réconcilier puis geler avant TEST, sans transférer
les pins du draft à un autre lecteur. Le même G dédupliqué sert aux
deux méthodes ; publier pas, fusions et coûts de préparation séparés.

**Précaution numérique concrète pour le port des masses :** une durée
positive d'un vrai triangle aigu u18,1/4478554084, devient zéro si l'on
soustrait les deux approximations de niveaux. La différence rationnelle
locale reste positive : diagnostic natif Release/UBSan, code0 et sorties
identiques, sources stables. [Calculer les largeurs avant les sommes](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#préserver-les-durées-positives-dans-le-port-exact).
Seize supports u8 donnent séparément une somme à dénominateur149bits ;
cela ne réalise PAS leur FULL commun. Garder les durées/formes compactes,
intervalles certifiés puis repli exact, pas un budget fixe hérité des
prédicats. Huit comparaisons exactes du cône passent normal/−O, sans
construire systématiquement e*. Aucun coût LiDAR/G4 nouveau acquis.

**Deux distinctions à conserver :** les43 lignes ER de base utilisateur et64
lignes catalogue observées concordent avec leurs reçus, mais le collecteur
peut aussi rendre code0 pour zéro ligne, un reçu absent ou divergent.
Ce n'est pas un défaut des résultats observés ; ce code ne constitue
pas une porte. Les nouveaux modes points/pref ont33 divergences affichées
dues à l'inversion total/réussites ;10 lignes points ne sont pas appariées.
La clé du mode doit être explicite, ses paramètres publiés et le schéma
des compteurs présent avant comparaison. La nouvelle question tétraèdres/mcs5 est un choix de
modèle : admissibilité par couverture ne signifie pas cluster obligatoire.
[Retour au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#er--résultats-concordants-mais-code0-nest-pas-une-porte).

**Statuts du catalogue contre-relus :** les165 fenêtres contestées ont
toutes un amas admissible contenant un point NON libre et une date dans
la fenêtre. Leur absence de cluster n'est donc pas imposée par FULL
et condensation seuls. Cela ne réfute pas leur cible statistique
« bruit ». Le [témoin pont_court et les conditions](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#statuts-forcés-et-choix-statistique)
distinguent cette non-universalité de la sélection effective.
Pour les tétraèdres, date d'admissibilité, entrée de T3a et première
date du bloc attribué sont trois objets différents à publier.

**Nouveau verrou de robustesse :** la [preuve CR noyau](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#le-masque-cr-noyau-introduit-un-saut-géométrique)
montre une hauteur de réunion discontinue sous c4ab. Les gardes de
noyau, admissibilité et balayage passent ; le masque de porteurs
réintroduit une histoire rivale entière au plateau. Ne pas le porter
comme règle robuste. Le contrôle MMtA complet ne présente pas ce saut
sur cette famille, mais retirer le masque ne répare pas automatiquement CR.
Le [paquet clos](../receipts/audit_continu_20260929/mmta_cr_core_comparability_20261001/README.txt)
garde961 contrôles et les jumeaux1mm ; le groupe conservé à mcs3 change
aussi à coupe fixe. Le nouveau mémo admet déjà la discontinuité ; il ne
s'agit pas d'une contradiction ignorée par le développeur.

**Relance sur les questions :** la [réponse actuelle](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#réponses-actuelles-au-développeur)
répond explicitement Q1/Q2/Q3, l'audit conditionnel du transport S et
la correction angulaire encore nécessaire à N. Elle distingue les préférences encore
destinées à l'utilisateur, dont la nouvelle question ER « chaînes ».
Le développeur a maintenant [consigné les réponses Q1bis et Q-Π2](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#condensation-et-changement-de-propriétaire-sont-deux-décisions) :
triangles aussi à mcs2, aucun cluster en Q-Π2 ; départager les cellules
ouvertes sur LiDAR Zoltan, pas par de nouvelles questions synthétiques.
La cible v2 conserve pourtant les paires à mcs2. Contre-calcul ROOT
Γ2/vraies AST ER et pref :240 comparaisons exactes sans écart entre
implémentations, mais20 coupes mcs2 donnent encore AB|CD|EF aux deux
jeux de paramètres. Le bilan «toutes les cellules utilisateur» ne
couvre pas ce choix consigné ; ne pas modifier les anciens reçus v2.
Compresser sans perdre de masse ne transforme pas l'attente en
adhésion immédiate : ce dernier choix change la règle statistique.
La réduction du modèle MMtA à MMt pour mcs≤K ne suffit pas à prouver
l'identité des deux implémentations pour toutκ : l'ancien MMt omet
l'unanimité continue. Borner cette identité à κ≥√(1+η), ou réparer
le terme. Les défauts actuels sont dans ce domaine.

**Port conseillé pour la projection :** conserver l'antichaîne complète
des feuilles couvrantes, les vrais LCA et un breakpoint d'admissibilité H
par chaîne, pas tous les ancêtres. Les durées se télescopent ; les rivales
se transmettent sur l'arbre virtuel par minima préfixe/suffixe. Garder
les feuilles hors bande pour Ah/Rt, les fusions après E2, l'unanimité
continue et le propriétaire réel en coupe fermée. La nouvelle sonde
`compression.py` ne compte qu'un squelette : H manque à sa taille.
[Argument et précautions](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#la-compression-utile-est-celle-des-lignées).
C'est une proposition exacte à porter et requalifier, pas un gain natif
mesuré ou une borne sous-quadratique globale.

**Le carré des ancêtres est désormais réalisé à K5/K10 :** le
[peigne exact](../receipts/audit_continu_20260929/ancestor_closure_comb_20261001/source/README.txt)
possède une forêt et des graines fortes linéaires, mais sa couverture
développée D vaut K(n−K+1)+(n−K)(n+K+1)/2. Cinq petits cas, trois
Γ exhaustives, signatures analytiques indépendantes ; lecteurs et
replays normal/−O concordent octet pour octet. L'arbre virtuel par
point reste O(K) sur cette famille. Filtrer seulement la bande peut
encore laisser ce carré à K10. Ce n'est pas une borne sur LiDAR, ni
un résultat natif/G4. Le chrono actuel `cout.py` refait la préparation
des porteurs pour compter un squelette : il ne mesure pas le port
comprimé. [Détails et pins](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#la-fermeture-des-ancêtres-est-un-coût-évitable).

**Autre optimisation sans changer ER-n :** sous persistance, l'amas
de l'enfant avant sa mort est inclus dans celui du parent à sa naissance.
L'égalité des amas se juge donc par deux cardinalités de préfixe,
sans rescan du nuage ni construction de deux ensembles par vote.
100 recoupes exactes et68 mutants refusés en RAM ouverte normal/−O ;
pas une qualification géométrique/native. Ce correctif laisse le
critère discret ER-n inchangé et ne répare pas sa robustesse.
[Conditions strictes et coût](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#un-rescan-er-peut-devenir-deux-comptages-de-préfixe).

**Priorité de port désormais constructive :** dédupliquer les entrées
par point/nœud/minimum d'activation, retirer les ancêtres redondants,
puis compter les points distincts par corrections aux LCA et un
préfixe Euler signé. Naissance=avant-mort−entrées propres tardives.
Cela évite D et donne exactement l'admissibilité par l'offset mcs−B
ainsi que sigma, avec top2 d'IDs DISTINCTS et égalités incluses.
180 comptes,630 admissibilités,126 sigma et9 refus vides recoupés
normal/−O ; contrôles causaux du mauvais offset, des doublons et du
descendant non immédiat. [Preuve, coût et parallélisation](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#comptages-et-échelles-sans-fermeture-dense).
LCA par sauts binaires et tris restent payés ; ni O(N+T) global,
port natif ni performance LiDAR/G4 acquis. Le développeur calcule déjà
les full-counts par LCA : ce complément poursuit cette architecture.
Les nouveaux D synthétiques K5 font×4,36/×3,81 entre8k/16k/32k ;
c'est le coût de la fermeture développée à éviter, pas une mesure LiDAR
ou de ER complet. Leur bande alpha² n'est pas une borne des votes ER.


**Lemme utile sur les rivales MMtA :** le complément du poids de
préhistoire peut s'écrire comme maximum de fonctions clip/min
sans porte h<b.3171 gardes AST sur120 évaluations abstraites,
recoupées normal/−O ; ce n'est pas encore un transport global lorsque
la topologie change. [Expression exacte et portée](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#le-seuil-des-rivales-admet-une-expression-sans-porte-dure).

**Préparation ER à corriger sans changer le modèle :** la
[preuve étoile](../receipts/audit_continu_20260929/er_entry_star_20261001/README.txt)
donne n(n+1)/2 scans d'enfants pour D=2n, dans la vraie AST du
constructeur. Retirer les parents des nœuds couverts donne les mêmes
entrées en2n retraits.20 cas exacts,20 mutants sémantiques refusés,
lecteurs/replays normal/−O recoupés. K1/préparation seulement, ni
règle ER à ancre positive ni K5/LiDAR ; les grandes tailles sont
analytiques, pas exécutées. Ce correctif ne supprime pas le coût
de la fermeture dense des ancêtres dans les autres étapes.

**Progrès mathématique pour les masses tardives :** sous FULL exact
et couverture fermée, les porteurs massifs se réunissent avant
α+√E2 et date_finale≥√Ahat. Pour κ≥√(1+η), une masse ε qui seule
retarde la fusion ne fait dépasser le plancher que d'au plus
2κε/(ηα), puisque W≥ηAx. La
[preuve](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#les-fusions-tardives-ont-une-borne-géométrique)
contrôle ce mécanisme ; le transport global des poids/topologies
et la continuité de MMtA restent ouverts. Aucun contre-exemple
nouveau au profil MMtA complet n'est acquis dans cette tranche.

**Qualification ciblée, pas performance :** le
[filtre d'orientation réel](../receipts/audit_continu_20260929/actual_orientation_filter_20261001/README.md)
a été recompilé et jugé indépendamment :192 appels, témoin GNU/UBSan
sans décision fausse, mutant à borne trop faible avec27 décisions
fausses dont5 en arrondi natif. Lecteurs normal/−O, déplacement,
faux SHA et quatre refus causaux en RAM sont recoupés. Certification
fermée seulement ; aucun contrat FULL/G4/100ms supplémentaire.

**Au développeur :** la [réponse prioritaire](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#réponses-prioritaires-au-développeur-et-alerte-sur-les-mutants)
répond aux trois questions mathématiques et détaille les deux défauts
de juge nouvellement recoupés. Pour MMtA c4ab, conserver Ax/Ahat non
filtrés et la rampe des rencontres ; la bonne borne est désormais
W≥ηAx, pasηAhat. Les contrôles03ff7 restent historiques. Les préférences
des nouveaux mémos privés sont destinées à l'utilisateur, pas des
réponses statistiques à présumer par l'auditeur. ER reste discontinu
à son seuil dur ; λ9/8 est un rapport de rayons, pas de niveaux carrés.

**Suivi des juges à ne pas confondre avec le moteur courant :** le
[complément R2/B21](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#dent-native-du-filtre-et-suivi-des-qualifications)
confirme les163 fichiers du groupe R2 clos, pas la tête en cours
d'intégration. B21 :577 contrôles exacts verts ; le signal mutant
n'est pas un rejet géométrique et la restauration des sources sans
rebuild final n'atteste pas le dernier binaire. T4 vérifie les niveaux
mais pas S* aux égalités ; HEAD trie correctement. L'absence de dent
T2 dans la recherche ne prouve pas son impossibilité. Aucun chrono G4.


**Simplification mathématique utile pour FULL :** dans la branche p≥K,
le saut peut prendre **n'importe quels K sites strictement intérieurs**,
pas nécessairement les K plus proches du centre. Leur MEB a un niveau
strictement inférieur ; une chaîne d'échanges de K-parties, contenue
dans l'ancienne boule, conserve la composante au niveau utile. Pour un
représentant de lot λ, son niveau β<λ conserve aussi la composante
pré-lot. Les feuilles terminales brutes peuvent changer, pas leur ancêtre
requis. La [preuve et les sites d'usage](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#saut-intérieur-sans-tri-des-plus-proches)
précisent cette portée : aucune extension à p<K ou à la coquille,
aucune borne sur les pas ni aucun gain mesuré. Un census limité à K
intérieurs certifiés devient donc une piste exacte ; un simple retrait
du tri laisserait le coût de collecte exhaustive intact.

**Proposition neuve pour accélérer ce saut :** un flux unique d'intérieurs
stricts, réparti en huit orthants du centre exact, s'arrête au certificat
ou à EOF après au plus8(K−1)+1 reports, soit33 àK5 et73 àK10.
Un orthant saturé fournit G avec `β(G)<2β(F)/3`. Sinon, à EOF,
tous les intérieurs sont connus et p≤8(K−1) ; p peut encore être≥K.
La [preuve et ses limites](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#choix-par-orthants-avec-contraction-certifiée)
bornent les sauts saturés par O(B) sur grille entière, pas les autres
pas ni les visites d'index. Pas de port moteur, test natif ou chrono.
Une famille de couches K3/K5 force aussi m−1 sauts de secours par
résolution ; la mémo peut les amortir. Ne pas en déduire un carré LiDAR.
Ne pas appliquer ce raccourci au census complet du catalogue ni au
K2 à ancre fixe en lui transférant sa borne d'états.

**Questions du développeur immédiatement accessibles :** la
[réponse courte Q1/Q2/Q3](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#réponse-directe-aux-questions-q1-q2-q3)
précède désormais les détails. Les seuls K voisins ne déterminent pas
les classes de bande ; les K-parties fixes restent l'univers exact ;
leurs poids souples évitent le saut de disparition du catalogue, sous
les conditions de stabilité publiées, notamment la date à marge avec
κ≥√(1+η′). MMt change de modèle de masses. Une occupation locale m borne
les supports en3D par O(m⁴), pas par K ; K3 possède déjà quadratiquement
beaucoup de classes. Les regrouper n'autorise pas à remplacer leur compte
exact par un vote. La [nouvelle réponse au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#relance-du-développeur-et-décisions-pour-le-prochain-port)
précise ce qui peut être repris dans le port. Les questions de préférence
du nouveau `juge_final/cibles/CIBLES_REVISEES.md` restent destinées à
l'utilisateur. Une condensation à mcs ne réaffecte aucun point ; Π2 est
un choix supplémentaire. Si les cibles AB|CD|EF à mcs2 et ABC|DEF à mcs3
sont toutes deux retenues, une projection indépendante de mcs devient
impossible. La taille couverte par FULL ne certifie pas celle du cluster
laminaire après attribution. Les [203 partitions des six points](../receipts/audit_continu_20260929/condensation_fixed_cut_203_20261001/README.txt)
sont conservées dans un témoin clos, lecteurs et rejeux normal/−O recoupés,
avec quatre cas compatibles ; portée strictement conditionnelle aux
deux cibles, pas un nouveau jugement HGP. Pour les oracles R2, recommandation explicite :
`leaf=max(8,K+3)` sur le chemin normal, petites feuilles dans des diagnostics
séparés avec budget. Aucun de ces conseils ne qualifie un nouveau moteur.

**Ancienne Π2c et nouvelle MMtA distinguées :** le
[contre-exemple géométrique clos](../receipts/audit_continu_20260929/pi2c_positive_anchor_jump_20261001/README.md)
réalise désormais le saut d'ancre de l'ancien noyau sur cinq sites K2/mcs3.
La réunion(x,a) passe de√12 à5 à la limite, malgré κ4 ; six cas exacts,
dix cofaces exhaustives chacun, vrai code par AST, deux mutations et
contre-rejeux normal/−O. Ce n'est pas un défaut démontré de MMtA03ff7.
Pour sa nouvelle échelle S, la [preuve constructive](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#saut-réalisé-par-l-ancienne-règle-et-propriété-utile-de-mmta)
donne √S stable par déplacement apparié et W≥ηS>0, sous les hypothèses
précisées de FULL complet/sites distincts/n≥mcs. Conserver cette définition ;
pré-histoire, rivales et propriétaires restent à éprouver. La couverture
mcs n'est toujours pas une masse dure finale de mcs points.

**Localité LiDAR mesurée :** le
[paquet compact](../receipts/audit_continu_20260929/lidar_halo_argmax_exact_20261001/README.txt)
recoupe douze cas de trame entière, K5/K10, brut/sans-sol sur trois trames
de la seule séquence08, grille1mm. 48 maxima de colonnes, 28 ancres
distinctes, plus de2,1millions de distances entières par passage.
Sans-sol/K5/η′1 : médianes archivées35/35/38 du halo majorant, mais
occupations exactement réalisées11086/5232/13923 ; celles du halo
minorant1262/734/2743 interdisent aussi de présumer K voisins suffisants.
Les autres ancres restent non certifiées, donc pas des maxima globaux
exacts ni un nombre de classes, ni une borne de croissance. Préférer
comptage implicite et tâches de taille variable sans quota de résultat.
Les74Mo du diagnostic initial restent hors de Git ; dépendances LIVE
déclarées. Aucun chrono HGP/FULL/G4.

**Raccord relu au01 h28 :** HEADR2 reste4b457bd ; `notes/oracles.md`
annonce EN COURS. Les aides bad_alloc restent à corriger. Le
[témoin MP10](../receipts/audit_continu_20260929/mp10_temp_registration_20261001/README.txt)
confirme quatre cas natifs puis une recompilation indépendante :
zéro fuite du témoin, FD/temp1/1 du mutant à allocation après création,
nettoyage ensuite0/0. Conserver l'enregistrement sans allocation ; ne
pas réattribuer la cause du code3 historique. Le nouveau rapport B21
au30c66d8 garde justement S1–S4 ouverts ; son20/20 est19/20 puis une
porte documentaire rejouée, pas un run unique. Les37 nouveaux mutants
comptent28 rejets code1, un plancher code3, sept survivants dont un
équivalent et un signal distinct. Aucun nouveau défaut géométrique de
HEAD déduit, ni clôture R2 ou qualification G4.

La contre-relecture de S reste cohérente. Le passage de la proposition N
au délai exact g=1 garde toutefois la perte angulaire déjà signalée dans
le mémo privé ; corriger par une limite à g<1, sans transformer ce détail
en réfutation de la stabilité locale. Voir la réponse Q3 détaillée.

**Réponse constructive au coût du comptage :** pour une classe de boule
déjà obtenue, les comptes de K-parties peuvent être calculés sans les
énumérer. Si sa coquille est exactement un support minimal positif de
taille 2/3/4, deux coefficients binomiaux suffisent ; pour toute coquille
de taille≤4, au plus 16 sous-ensembles. Pour une coquille dégénérée plus
grande, un découpage exact de la sphère des directions, puis un calcul
transposé partagé, donnent les comptes de **tous** ses points à la fois.
La [preuve détaillée](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#compter-une-classe-sans-développer-ses-k-parties)
sépare la construction géométrique du circuit O(u²), u étant la taille
de coquille. Un seul passage transposé suffit par K ; tous les intérieurs
partagent un même compte, pas nécessairement un même poids statistique.
Le [paquet autonome clos](../receipts/audit_continu_20260929/meb_shell_euler_transpose_20261001/README.md)
recoupe 480 cas, 41 808 occurrences de sous-ensembles contre des MEB exacts
indépendants, cinq variantes fausses et les lecteurs normal/−O.
Le constructeur jouet reste cubique ; aucune qualification moteur/G4.
Le total Σu_B², les classes absentes, le census et les propriétaires
restent à payer. Ce n'est pas une borne sous-quadratique globale.

**Nouveau contrôle causal des tests R2 :** le
[main réel du collecteur](../receipts/audit_continu_20260929/collector_mutant_causality_20261001/README.md),
avec builds/juges/écritures remplacés en RAM, compte comme « tués » un
signal−11 et un délai, et accepte une sélection de mutants vide. Huit
captures normal/−O, source complète figée, lecteurs hash-first et
contre-rejeux indépendants. Aucun moteur ou signal réel lancé ; ce
constat n'accuse pas les captures natives d'avoir utilisé ces défauts.
Réparer la classification et l'inventaire avant les reçus finaux.
Le différentiel R2 a, lui, 43 cas terminaux et vérifie désormais présence,
non-vacuité et SHA complets des dumps ; pour les lots/témoins, ajouter
l'inventaire **attendu**, pas seulement celui observé des deux côtés.

**Mesure utile avant une nouvelle optimisation FULL :** l'Atlas ne
mémoïse pas les états profonds p≥K, ni tous les p<K ; même un HIT arrive
après MEB et census. Le
[cache distinct proposé](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#cache-des-descentes-profondes-et-mesure-utile-avant-le-port)
doit garder clés exactes, K, propriétaire du nuage et naissance témoin,
puis remonter à la bonne coupe. Mesurer les répétitions sur LiDAR avant
de porter une table concurrente ; `FlatIndex` actuel ne devient pas sûr
par simple ajout d'insertions. Constats de code, aucun gain100ms mesuré.

**Raccord vivant22 h35 :** HEAD privé `6d2d3bc5` pour P4 ; P2 est encore
dans l'index/travail suivant. La nouvelle `OutputSet` réserve des
temporaires, protège les alias et ferme les flux par RAII ; les quatre
CLI contrôlent son commit. Relecture statique seulement. Un deuxième
rename en échec laisse toutefois la première destination remplacée :
pas de transaction globale ni de conservation des deux sentinelles sur
ce refus tardif. Le [suivi précis](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#raccord-courant-et-six-décisions-utiles-30-septembre-22-h-35-utc)
distingue ces corrections des anciennes pertes de fichiers. Le contenant
u32le de ce raccord utilise encore le moteur u18, pas le palier B21.

Le [reçu nouveau](../receipts/audit_continu_20260929/outputset_transaction_20260930/protocol.md)
reproduit désormais le refus au deuxième rename sur les headers complets
`763e2ee3…`, avec deux sentinelles : NEW_A/OLD_B, refus correct mais pas
restauration. Cinq cas, compilation native isolée et recompilation
indépendante, zéro fuite de temporaire/FD ; ni moteur ni CLI HGP lancés.
Archive textuelle close dix payloads, lecteurs normal/−O hash-first
recoupés, binaire omis et non relancé par eux. Son code0 constate ce
défaut ; il ne qualifie pas la transaction. L'idempotence est corrigée.

**Nouveau verrou ciblé du raccord :** quatre aides laissent échapper
`std::bad_alloc` : chemin filesystem du lecteur, chaîne du flottant,
vecteur de liste et tokens des configurations. Le
[reçu autonome](../receipts/audit_continu_20260929/input_allocation_boundaries_20260930/PROTOCOL.md)
fige les cinq headers compilés et quatre callers complets. Huit cas :
quatre témoins valides et quatre exceptions reproduites, FD/fichiers
intacts ; recompilation indépendante concordante, aucun moteur/CLI HGP
lancé. Lecteurs normal/−O hash-first, quatorze payloads textuels,
deux mutations rejetées. Ajouter les conversions `memory_budget` aussi
avant les `try` actuels ; ne pas tenir les seuls refus ordinaires pour
une qualification de toutes les allocations. Les calls CLI correspondants
sont hors protection globale au snapshot audité. Les noms de statuts
restent les valeurs par défaut du harnais quand aucun Result n'est revenu,
pas une sortie produit « ok ».

**B21 relevé du 1er octobre, 00 h 34 :** 20/20 gates au30c66d8, logs terminaux16
comparaisons tour/tête et4 catalogue,6/6 fast au30c66d8,1/1 sanitizer
au9f54c5b ; ombre réelle positive et zéro violation dans ses logs.
La campagne relancée utilise bien la porte30c66d8, pas9f : M1
`jump_reach_first_site`, les deux M2 `sitetree_band_0p02` et
`tower_meb_band_0p02` ont maintenant des juges Release en échec reçus ;
M1 a aussi un diagnostic UBSan de débordement signé. Les M2 jugent
les décisions de repli exact, pas une mauvaise géométrie démontrée.
La suite de 38 noms est maintenant terminale, code0 : 35 rejets Release1
avec ECHEC ciblé et trois équivalents annoncés, tous0/0. Inventaire exact
et résultats recoupés ; pas de signal/délai/code3 dans cette capture.
La partie9f antérieure est conservée. Pas de témoin final après restauration
des sources ; le binaire mutable reste celui du dernier mutant, pas un
build témoin restauré. Ces scopes ne sont pas une clôture finale du produit.
Les gardes de collecteurs sont encore à fermer : dump absent peut
donner une égalité de hashes vides, `--only` inconnu une campagne vide
verte, signal sanitizer un « tué » sans juge causal. Sources relues,
pas nouvelles campagnes exécutées par cet audit ; aucun chrono G4/FULL acquis.
Le mode auxiliaire `--audit` juge seulement les ordres présents : exiger
l'inventaire1..min(K,n), le mode attendu et l'activation de l'ombre.
Un compte d'ombre nul peut être légitime sur le cube200 ; ne pas poser
un plancher positif universel. Le runner conserve actuellement un stdout
filtré, sans lignes par ordre. Voir les
[consignes de clôture au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#contre-relecture-nouvelle-du-raccord-et-des-portes-b21).

**À corriger avant les comparaisons EOM :** le banc privé
`masses_selection/dev_scenes.py` réutilise `phi_date` dans une nouvelle
échelle z sans conversion. Pour une attache β4, mort β25, z2→z3,
`hard_stats` donne121/500 au lieu de117/1000. En sens inverse,
β9, mort β25, z3→z2 donne−2/675 malgré une attache valide.
Le [contre-audit clos](../receipts/audit_continu_20260929/selection_semantics_20260930/README.md)
épingle cinq sources complètes, vérifie le vrai appel par AST et recoupe
six cas exacts, quatre à exposants identiques positifs. Lecteurs autonomes
normal/−O,15 payloads, SHA externe et intégrité avant/après ; les deux
échecs de préflight du harnais restent documentés. Seules les comparaisons EOM à exposants différents
sont atteintes par ce constat, pas les diagnostics de rappel/jitter qui
réutilisent leur propre échelle. Le contrôle de condensation dur reste
aussi non équivalent à la condensation standard des départs de points.

**Réponse Q2 renforcée :** la bande α des paires d'ordre K peut être
vide, même K5. Au centre d'un tétraèdre régulier entier, α²=3 et
les quatre votes ont ℓ²=19/4 ; η1/4 donne le bord75/16<76/16.
La normalisation du prototype MMp par `min ℓ` évite le zéro mais change
l'échelle. Décider cette sémantique avant le port K5 ; la seule
localisation des centres ne suffit pas. Le
[reçu K3/K5 autonome](../receipts/audit_continu_20260929/pair_band_totality_20260930/README.md)
recoupe les quatre votes exhaustifs, la translation u18, les contrôles
K2 et aux seuils,20 bornes d'échelle, fonctions réelles sur contexte
analytique. Pas de propriétaire natif ni de défaut du K2 implémenté.

**Complexité des paires :** le
[packing global recoupé](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#paires-locales--borne-globale-utile-recherche-encore-ouverte)
borne la cardinalité par O((K−1)c³n(1+log Δ)), soit O(n(B+1)) à
K/c fixés sur une grille B bits. Ce n'est pas une borne Γ ni de recherche.
Le [contre-exemple du vrai SiteGrid](../receipts/audit_continu_20260929/pair_grid_packing_20260930/README.md)
à n exact8k/16k/32k a23994/47994/95994 votes, mais le filtrage peut
recevoir63 968 004/255 936 004/1 023 872 004 IDs de candidats denses.
Comptes déduits, neuf requêtes de vote AST recoupées, pas boucle quadratique
exécutée ni régime LiDAR/G4. Les extrêmes contrôlent le pas d'une grille
uniforme et font tomber tous les points denses dans la même cellule.
Instrumenter candidats et census ; ne pas porter cet index comme garantie
sous-quadratique. Le census matérialisant tous les intérieurs garde un
second carré possible même avec un meilleur index ; sa preuve collinéaire
et la borne positive sur les états mémoïsés sont dans la note au développeur.
Le [nouveau reçu autonome](../receipts/audit_continu_20260929/pair_census_square_20260930/README.md)
recoupe ce second carré avec un index **idéal** donnant seulement les
vrais intérieurs :20 petites configurations,368 propriétaires comparés
aux composantes L2 exactes, mémo et argmin réel par AST. Chaque collecte
et argmin paie m(m−1)/2 pour3m−2 votes ; un argmin en flux réduit
la mémoire, pas ce temps. Lecteurs normal/−O, deux mutations causales
et une du lecteur, sept fichiers clos ; grandes tailles analytiques
seulement, m distinct de n=m+1. Pas d'index ni de forêt natifs appelés.
EOM Python a aussi des remontées quadratiques évitables.
Un [générateur par cellules d'ancres et échelles](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#générateur-de-paires-avec-budget-de-recherche)
a maintenant un budget complet proposé : O(nJ+An+C), hash attendu,
ou radix/jointure déterministe avec coût de clés publié. J≤B+1 ;
C≤(K−1)AnJ borne aussi les faux candidats et les cases vides coûtent
au plus An. Mais A=1331 déjà à K2/η1/4 : constantes, k-NN initiaux,
census et propriétaires restent payés. Preuve d'architecture seulement,
pas prototype mesuré ni contrat100ms.

**Verrou confirmé pour la tête :** une bande dure reste discontinue
même sur l'univers des paires, sans disparition de vote du catalogue.
Le [témoin à cinq sites](../receipts/audit_continu_20260929/hard_band_border_20260930/README.md)
à K2/η1/4 fait sauter une hauteur de réunion de55 à105 lorsqu'un
seul site franchit infinitésimalement le bord de bande. Version
entière u18 : déplacement de deux unités, réunion56320→107520.
Fonction réelle extraite AST, deux constructions géométriques exactes indépendantes,
24 cas et1274 comparaisons normal/−O ; deux mutants causaux rejetés.
Lecteur avec SHA externe, inventaire fermé et intégrité après rejeu.
Pas de résolveur natif, MMt, EOM ou G4 dans ce lot. Garder la bande
dure comme contrôle statistique ; ne pas lui attribuer la continuité
des variantes à poids souples et marge continue.

**Voie constructive pour MMt :** le
[raccord des couvertures](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#construire-les-couvertures-sans-développer-γ)
utilise le lemme catalogue complet déjà éprouvé, puis propriétaires
vivants, déduplication par point/nœud et antichaîne des seeds. Pour les
boules fortes, D≤(K−2)M+Σ|U| ; D=KM si coquille=support minimal.
Le CSR existe déjà ; un chemin comptage/préfixes/radix peut être linéaire
dans ce payload, après sélection, index Euler et calcul des propriétaires.
Le [prototype structurel clos](../receipts/audit_continu_20260929/seed_cover_antichain_20260930/README.md)
recoupe désormais512 cas et68 699 coupes :16 839 incidences→6327 seeds,
six exports natifs clos copiés, internes seuls K3/K5, plateaux,
activation=mort et chaîne20k. Cinq mutations changent la couverture ;
une sixième, quotient par doubles, viole le propriétaire vivant.
Lecteurs autonomes normal/−O, SHA externe et intégrité avant/après.
Tri Python O(D log D), normalisation brute pouvant coûter O(DH) : pas
encore de radix industriel, port moteur ou nouveau benchmark natif.
Ne pas confondre cette réduction avec les votes des K-parties : leurs
comptes ne sont pas conservés. Les coquilles étendues et M restent payés.

**Statistiques :** erreur inconditionnelle, précision des seuls points
affectés et rappel avant fusion ne s'impliquent pas. Attendre la racine
est admissible mais a un rappel pré-fusion nul. La
[contre-relecture](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#ce-que-les-résultats-statistiques-autorisent)
précise aussi les régimes K fixé/K croissant, les sites/retours pondérés,
la portée des88 nuages et des coupes gaussiennes. Aucun score MMt/EOM
ni avantage HDBSCAN n'est acquis par ces nouveaux contrôles.

**À corriger dans les bancs désormais committés privés :** la garde des dossiers de
fusion ne protège pas les fichiers liés aux entrées. Le
[contre-exemple isolé](../receipts/audit_continu_20260929/merge_output_file_alias_20260930/README.md)
reproduit cinq alias stables : fusion code0, session source modifiée,
sortie encore acceptée par `--check-only`. Sept cas normal/−O, aucun
fichier partagé touché. Protéger les trois sorties contre toutes les
entrées et entre elles avant écriture ; ne pas se limiter au dossier.
Le [comparateur CSV privé](../receipts/audit_continu_20260929/scale_comparator_vacuity_20260930/README.md)
accepte aussi une sortie vide, tronquée ou supplémentaire : contrôler
effectifs/inventaires, pas seulement `zip`. Lecteur final sans écriture,
inventaire fermé et huit rejeux sur snapshot épinglé, normal/−O.
Les différentiels archivés observés restent non vides ; ce défaut de
lecteur ne démontre pas qu'ils soient faux. Leur portée reste le runner,
pas la condensation ni une performance FULL/G4 nouvelle. Le commit bancaire
`d2640c8f89e5078f53a4fd4b2f925c44d16bfb77`, à 19 h 26 UTC, intègre
les premières gardes mais conserve exactement les trois sources fautives
épinglées par ces contre-épreuves. Ces défauts ne sont donc pas corrigés
par ses 26/26 gates. La condensation reste également ouverte.

**Questions du développeur traitées en priorité :** la
[réponse Q1/Q2/Q3](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#réponses-aux-trois-questions-sur-les-votes-de-bande)
borne les sites utiles par B(x,2R), mais pas leur nombre par K. Un
[cap K3 rationnel](../receipts/audit_continu_20260929/band_classes_locality_20260930/README.md)
possède quadratiquement beaucoup de classes dans une petite bande :
preuve analytique, quinze classes Fraction recoupées, pas un régime LiDAR
mesuré. Les seuls K voisins ne déterminent pas les votes exacts. Le
[contrôle CDF K2/K5](../receipts/audit_continu_20260929/component_ballot_cdf_20260930/README.md)
montre aussi des admissions sans nouvelle fusion, à couverture identique.
Comptage implicite ouvert ; ne pas reconstruire toutes les parties/classes.
Les poids souples échappent au saut de contact sur des K-parties fixes,
pas sur des votes de catalogue. W≥1 pour les K-parties à K≥2/n≥K/sites
distincts ; ne pas transférer cela aux paires normalisées par α à K≥3.
La [preuve S est cohérente à la contre-relecture](../receipts/audit_continu_20260929/majority_SN_counterreview_20260930/README.md) ;
un témoin exact à quatre sites réfute l'appartenance utilisée pour la
constante précise de N, à g=1. L'erreur angulaire doit être contrôlée ;
la croissance Ω(N) n'est pas réfutée par ce cas. MMt change de modèle de masses
et mérite un petit prototype, pas un grand port déjà justifié. La
[contre-relecture MMt](../receipts/audit_continu_20260929/mmt_median_transfer_20260930/README.md)
valide une réduction exacte à la lignée médiane : au plus deux événements
par atome, cinq arbres Fraction/AST concordants, dates et propriétaires
compris. L'extraction des atomes reste payée ; aucun gain industriel acquis.
La [compression préalable](../receipts/audit_continu_20260929/mmt_cover_compression_20260930/README.md)
est désormais recoupée sur 108 arbres rationnels, 15 360 comparaisons et
quatre mutations : au plus 2S nœuds virtuels, durées télescopées sans
changer les masses par composante originale, sans remontée par seed.
Ni complétude des seeds natifs ni date QS complète rejouée dans ce lot.
Lemme T et propriétaires S_t demandent les précisions de seuil/intégration
publiées ; la majoration finie simplifiée oublie un rapport d'échelles.
99 contre 35 pour cette majoration, pas un échec de stabilité de MMt.
Le [témoin géométrique K5](../receipts/audit_continu_20260929/mmt_rational_mass_k5_20260930/README.md)
donne déjà W à numérateur réduit de 142 bits sur cinq sites u18 : i128
seul est insuffisant, même avant l'élargissement u24/u32. Aucun appel natif.
Le [petit complément de domaine](../receipts/audit_continu_20260929/mmt_final_endpoint_20260930/README.md)
confirme aussi un terme oublié lorsque κ est petit : deux sites/K2,
η2/3 et κ1/10, date vraie √(5/3)−1/10, prototype √(4/3).
Le réglage recommandé κ4 reste exact. Corriger l'endpoint continu à la
première unanimité ou restreindre explicitement le domaine des paramètres ;
ne pas admettre aveuglément tous les événements après l'unanimité.

**Rôle courant : audit, sur la nouvelle instruction utilisateur.** Aucun
changement du moteur dans cette reprise ; prototypes de preuve isolés,
revue des raccords et suivi du développeur seulement. Plus tôt le même
jour, l'utilisateur avait demandé la reprise du développement et une
précision supérieure à u18. Le
[suivi actif](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md)
décrit le correctif RankIndex intégré et le nouveau candidat exact par bande.
Les régressions natives de ce candidat restent u18 et bornées ; la tête
statistique, le profil large et G4 ne sont pas qualifiés. Les correctifs R2
des autres acteurs restent à intégrer séparément.
Deux relectures indépendantes valident la première brique u32 isolée
(distance u128, Morton96, 212 684 contrôles). Elle ne qualifie pas les
supports q3/q4, le catalogue, la tour ou le GPU à ce nouveau domaine.

**Nouveau verrou de tête, confirmé :** les départs de points directement
attachés ne déclenchent pas le contrôle de masse `min_cluster_size`.
Le [contre-exemple clos](../receipts/audit_continu_20260929/point_condensation_20260930/README.md)
reproduit un changement EOM sur le vrai C++, racine exclue, mcs5.
HDBSCAN réel et deux oracles exacts indépendants confirment la correction.
La réalisation 3D des arbres API précis reste hors preuve ; les scores
historiques A/C ne sont pas invalidés sans rejeu. Corriger et ajouter cette
porte avant d'interpréter de nouvelles expériences frontière/statistiques.
Le [complément géométrique séparé](../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md)
matérialise l'export natif historique d'un vrai nuage à six sites K3,
recoupé contre Γ3 Fraction. La tête actuelle y surestime aussi la stabilité
à mcs6, mais ne change pas EOM ni les étiquettes. Deux nouvelles invocations
de tête normal/UBSan, aucun nouvel appel générateur : ne pas confondre ce
témoin géométrique de score avec le renversement EOM des arbres API.

**Développeur actif :** la copie `build/v10-integration-r2/src` a maintenant
HEAD `7edb91123be00796563f333feaca77d70e201163` : Pool après bancs,
faits_math, SiteTree et son addendum. Le reçu bancaire précédent possède 92 pièces hachées,
toutes recoupées ; manifeste `2b48788a872a0fee6dc55bec9f5c4337179c34bbbf615e4c48e5a6c41c627354`.
Terminaux archivés : 26/26 gates, 1391,68 s ; 7/7 fast, 37,11 s, codes0.
Le bilan bancaire 91 mutants tués/4 équivalents est une union de campagnes
et requalification de deux survivants initiaux, pas un rejeu final des95.
Les premières gardes schéma, doublons et `--calls`/`--out` sont intégrées ;
les nouveaux alias de fichiers et lecteurs restent ouverts, comme indiqué
en tête. L'addendum antérieur SiteTree/tour/CMake conserve sa campagne
94/94 mutants non équivalents tués et un équivalent : autre groupe,
ne pas additionner ces comptes comme qualification d'une union R2.

Le nouveau groupe Pool/catalogue/tour possède une relecture favorable de
l'annulation saturante, de la durée de vie des callbacks et de la relance
après sortie des workers. Petite sonde pool-only : compilation stricte et
trois répétitions correctes, sans nouveau test HGP ni sanitizer. Les pins
de l'index et du worktree sont séparés : `make_pool` et sa traduction
`session_overhead` ont maintenant une relecture favorable distincte :
deux exceptions prévues converties, quatre points d'entrée avant ouverture
de sortie ; pas avant la préparation de l'index mreach. Le test RLIMIT
suppose Linux/glibc et piles8Mio, limite virtuelle1Gio, non RSS. Aucun
nouveau test natif de ces voies par cet audit.
Réserve secondaire préexistante : Pool(1) ne marque pas sa région ; aucun
sous-Pool du moteur observé. Ni union des sept groupes ni condensation
corrigée acquises. Aucun test GCP lancé par cet audit.

Nouveau terminal du développeur :39/39 gates Release, code0,936,50s,
SHA `fcab734f…`. Le passage antérieur2242,69s est conservé séparément.
Le journal mutants archivé `eb699ff…` contient47 enregistrements,
une synthèse42/42 tués,4/4 survivants attendus et témoin vert.
Le nouveau MV3 est tué3/3 grâce aux rafales ; le premier passage1,0,1
reste archivé comme interrompu. Ces journaux ont été relus, pas rejoués
par cet audit. Le développeur a maintenant fermé le reçu et amendé
son commit :156/156 pièces et inventaire recoupés normal/−O, manifeste
`e1e9bb08ddb3fb98f28ab161771bc7049d1d82935206ba5cfa9ce1beb2f77537`.
Les différentiels archivés tour10/10 et catalogue10/10 conservent les
trois SHA complets ; tête18/18 compare les SHA complets en mémoire,
mais n'en imprime encore que les préfixes et un argv `<travail>`.
Préciser cette limite de provenance ; pas de défaut de sortie déduit.
Groupe privé clos, pas union complète R2 ni contrat G4 acquis.

**Précision supérieure à u18 :** le chantier privé `build/v10-b21/src`
a avancé après notre lecture épinglée à `5dd83b5c68919d87ada067204d19ee4edb166859`.
CLI fine et raccord R2 restent non qualifiés. Les voies larges
et I192/I192 répondent aux débordements identifiés, pas un élargissement
aveugle. Notre [relecture rationnelle figée](../receipts/audit_continu_20260929/b21_bound_counterreview_20260930/README.md)
recoupe les majorants et précise une dette documentaire : seuil flottant
arrondi omis dans la preuve grossière ; borne conjointe au même centre et
borne du rayon d'une MEB ferment la preuve plus fine. Deux rejeux Python
normal/−O, aucun appel natif. Le nouveau ledger privé
`10d8f386…` explicite la borne conjointe ; préciser encore r2a≥m dans
l'affirmation d'exactitude du seuil dyadique (contre-exemple2^-100+m).
Le snapshot publié reste immutable et cette lecture de5dd83b5c reste
distincte des corrections suivantes et de R2. Ne pas transformer les journaux privés
20/20 gates et 27/27 mutations en qualification CLI, u24/u32 ou G4.
Le nouveau rapport adverse privé `notes/verif-mutants/RAPPORT.md`,
SHA `d60167da…`, rend FAIL pour les portes, pas pour une sortie HEAD
fausse. Priorités : portée du saut sur tous les intérieurs (mutant UB
sur coins graine49), marge effective aux sites d'usage, pas seulement
getters. Sondes relues, non réexécutées par cet audit. En revanche,
l'équivalence du mutant comparaison2limbs est maintenant prouvée par
la borne cubique4L⁶<2^128 ; ne pas la fonder sur un maximum numérique.
Cette petite borne ne sécurise pas les produits intermédiaires i128.

Suivi22 h45 épinglé à `7af07c53` : compare-gap/marge représentable relu,
preuve par monotonie cohérente sans troisième erreur de soustraction.
L'ancien seuil arrondi avec comparaison stricte était également sûr
par monotonie : ne pas transformer la borne grossière en ancienne
erreur géométrique. Les nouvelles portes adressent les trous de portée,
marges, axes, domaine et départage ; ajout de code de test, pas campagne
de mutants déjà rejouée/clôturée. Le différentiel des chemins vaut pour
une même politique de sélection ; deux sauts valides différents doivent
être jugés aussi par propriétaires aux coupes, pas seulement compteurs.
La [réponse actualisée](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#raccord-courant-et-six-décisions-utiles-30-septembre-22-h-35-utc)
sépare ce nouveau code des anciens reçus. Aucun nouveau chrono G4.

**Plateaux de tête :** les événements de même rang doivent être traités
ensemble, avant le test des masses. La
[référence de l'autre auditeur](../receipts/audit_independant_20260930/developer_rebound/condensation_reference/README.md)
contracte ces plateaux et représente les cohortes en O(H+n) nœuds, sans
modifier FULL ; son temps de prototype n'est pas une borne native.
Contre-relecture de son code et de ses 40 hashes, puis lecteurs normal/−O :
536 condensations exactes et 2 144 sélections concordent, sans nouvel appel
natif. Le mapping des votes de boules reste hors de cette référence.
Notre [contre-épreuve à douze points](../receipts/audit_continu_20260929/point_plateau_condensation_20260930/README.md)
compare deux API valides ayant exactement la même ultramétrique : la tête
publiée sélectionne des clusters dans un encodage, tout bruit dans l'autre,
racine exclue, mcs5, z1/z2. Normal et UBSan concordent. Pas de réalisation
3D de ces deux encodages ni de nouvel appel HDBSCAN revendiqué.
Le [plan de correction direct](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#condensation-directe-sans-expansion-de-la-tour)
évite l'arbre d'événements supplémentaire : quotient, cohortes triées,
visites uniques et propagation des labels. Borne proposée pour cette tête
seule, pas pour le nombre d'incidences ni pour la construction FULL.
Le [témoin natif u18](../receipts/audit_continu_20260929/point_exact_rank_coalescence_20260930/PROTOCOL.txt)
confirme un autre préalable : trois sites, FULL K2 exact 3/4, mais rangs
de points 3/3 en core et 2/2 en cover, même en nearest. Conversion
approchée numérateur/dénominateur impliquée ; le quotient correctement
arrondi serait ici distinct d'un ulp. Préserver les rangs exacts séparés
des doubles, pas seulement figer l'arrondi. Pas de flip EOM démontré,
pas de FULL faux, archive native avec pins critiques avant/après seulement.

**Piste q3/q4 utile :** le [crédit quantitatif par moments de groupe](../receipts/audit_continu_20260929/group_moments_20260930/README.md)
certifie plusieurs intérieurs sans témoin individuellement universel.
Deux vrais supports 3D en donnent quatre pour q3 et trois pour q4, au-delà
des seuils tight K5. 5 793 contrôles Fraction normal/−O et deux erreurs
logiques causales ; pas de port natif ni de gain LiDAR acquis. Tester un
petit nombre de groupes préparés une fois, masques q3/q4 de supports
seulement : ne pas retirer ces sites du census ni déplacer le carré.

Le [complément boîtes réelles](../receipts/audit_continu_20260929/group_moments_box_r2_20260930/README.md)
ajoute un témoin cubique, quatre crédits q4 de trois sans dominance
individuelle. Mais les petites entrées 14/16 sites s'arrêtent dès la
première feuille M16 : ce n'est pas la boîte des témoins. Si l'ancre est
dans la boîte fermée, σ≥0 et le certificat ne peut rien rejeter ; sauter
ce calcul. Le recadrage interdit aussi de supposer les feuilles toujours
presque cubiques. 503 contrôles Fraction normal/−O, aucun appel générateur.
Prochain essai : vraies listes S/candidats, ancres hors S et coût total.

**Frontière, piste plus légère :** le [contre-audit des témoins redondants](../receipts/audit_continu_20260929/antichain_counterreview_20260930/README.md)
confirme un vrai changement de partition à K2/η1/8 sur quatre points,
réunion 0/3 avancée de β25 à 200/9 ; Γ2 et univers fort complets Fraction.
Les six exports antérieurs n'avançaient aucune des 434 hauteurs contrôlées.
Surtout, l'antichaîne n'a pas besoin d'être triée ou stockée : deux extrêmes
Euler donnent exactement son LCA, un balayage des incidences puis au plus
une LCA par point. 2 892 bandes et 6 227 sélections supplémentaires,
normal/−O concordants. Réductions parallélisables ; ni port natif, gain
LiDAR/EOM/ARI, borne du nombre d'incidences ou chrono G4 acquis.

**Piste avec borne de stabilité :** le mémo privé `ancrage_marges`
propose Pκ, qui retarde l'attache selon la persistance des branches
concurrentes. À K fixé, sa preuve tient : déplacements appariés ≤ε,
dates `(1+2κ)ε`, hauteurs `(1+4κ)ε`, en rayon. Aucun résultat EOM/ARI
ou ajout/retrait de sites n'en découle. Pour κ≥2, cutoff exact β≤4α².
Notre [simplification en flux](../receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md)
supprime aussi le tri de l'antichaîne et le code-barres par point :
LCA préfixe de toutes les incidences fortes, puis maximum pénalisé,
propriétaire conservé depuis toute la première cohorte. Oracles abstraits,
pas nouveau moteur. Le D réellement parcouru et les LCA restent payés ;
aucune borne sous-quadratique du générateur ni performance acquise.
La bande non saturée reste instable même avec une marge au bord :
une branche courte parasite dans la fenêtre suffit. L'optimisation exacte
de son calcul n'est donc pas une justification de sa robustesse.

La [réponse récente du développeur](REPONSE_CLAUDE_AUDIT_GEANT_20260930.md)
rappelle une limite de ce consensus : sur les deux triangles de la thèse,
les trois couvertures initiales AC/BC/CD peuvent faire attendre C et D
jusqu'à la fusion globale. Une borne de stabilité ne prouve pas la bonne
gestion de ces frontières. La majorité de bande à dénominateur figé mérite
donc une étude bornée, avec majorité stricte, unité des témoins explicitée
et contrôle des redondances ; elle n'a pas encore de preuve de robustesse
globale ni de supériorité statistique. Pκ reste un contrôle robuste à
comparer, pas une règle produit validée. Le développeur annonce désormais
condensation et choix de la tête avant le palier u24 ; ce palier reste ouvert.

**Majorité uniforme : robustesse réfutée pour les boules fortes de bande.**
La [preuve exacte](../receipts/audit_continu_20260929/uniform_majority_contact_20260930/README.md)
donne quatre sites affine-3D, K2/η1/8 : un contact devenu intérieur retire
un vote fort et fait sauter la hauteur de réunion de β25 vers 171/4,
pour un déplacement tendant vers zéro. Γ2 complet conserve la fusion
faible ; aucune limite de bande n'est franchie. Deux contre-relectures
concordent. Onze cas Fraction et deux mutants causaux, normal/−O ;
ni erreur FULL, ni appel natif, ni résultat EOM/ARI démontré. Réduire η
ne répare pas généralement ce mécanisme : une famille permet η>0
arbitrairement petit, avec les restrictions géométriques publiées.

**Condition utile, pas nouvelle règle industrielle.** Si identités,
univers et poids des votes sont fixes et transportés par les deux maps
compatibles de Γ, les partitions de majorité s'entrelacent avec le même
ε en rayon. Les K-parties contenant chaque point donnent une référence
qui satisfait cette condition, avec une marge de bande explicite ; dans
ce témoin leur hauteur reste β25. Ce n'est pas le vote par boule forte.
Leur comptage efficace/compression sans perte reste ouvert : ne pas
énumérer les K-parties dans le produit, ni reconstruire Γ explicitement.
Pas de garantie de labels/EOM ou d'ajout/retrait de sites.

**Comptage des votes : une compression exacte, un manque de classes.**
Le [complément mathématique](../receipts/audit_continu_20260929/parts_meb_class_counts_20260930/README.md)
regroupe les K-parties par leur boule minimale exacte, sans double compte.
Coquille régulière : un binôme suffit par point intérieur/de coquille ;
coquille dégénérée : compter aussi les sous-ensembles non minimaux qui
contiennent le centre dans leur enveloppe convexe. 113 contrôles Fraction,
quatre classes, normal/−O. Mais des classes ayant p≥Kmax sont absentes
du catalogue FULL et portent néanmoins des votes de cette référence.
Perte de masse démontrée, pas défaut de connectivité ni flip de hauteur.
L'extension analytique concerne aussi K5/K10 ; détail dans le suivi.
Ni génération sous-quadratique des classes ni port/comptage GPU acquis :
ne pas élargir aveuglément le catalogue ni énumérer C(n,K) en production.

**Majorité, optimisation exacte validée abstraitement :** deux sélections
pondérées et une LCA par atome remplacent le parcours de toutes les
lignées. Choisir un médian pondéré m dans l'ordre Euler des propriétaires,
puis le quantile strict des dates `max(c_i,b(LCA(v_i,m)))` ; remonter m
à cette date. La [preuve close](../receipts/audit_continu_20260929/weighted_majority_select_20260930/README.md)
concorde avec le sweep indépendant sur 392 cas et 1 176 variantes,
normal/−O ; deux contre-relectures indépendantes, quatre erreurs de valeur
causales rejetées. Pour l'uniforme : deux médianes hautes, requêtes LCA
indépendantes et sélection segmentée possible sur GPU. Cela conserve la
règle, pas une preuve de meilleure robustesse ou qualité statistique.
D incidences, ordre exact commun, index LCA natif et coût bit des poids
rationnels restent payés ; aucun port natif ni gain LiDAR/G4 acquis.

L'[alternative quadratique Qκ](../receipts/audit_continu_20260929/quadratic_anchor_rule_20260930/README.md)
évite les sommes de racines, avec une preuve de stabilité conditionnelle
à l'entrelacement couvrant. Mais elle retarde davantage que Pκ au même
paramètre et demande un ordre rationnel plus large. C'est une ablation
bornée proposée, non une refonte ni une meilleure qualité démontrée.

**Deux défauts ciblés nouveaux :** le [juge de précision](../receipts/audit_continu_20260929/precision_reader_orientation_20260930/README.md)
accepte les orientations manquantes ou fausses, code0 normal/−O ;
renforcer inventaire et exigences. Le [helper OutputSet](../receipts/audit_continu_20260929/outputset_exception_20260930/README.md)
du clone de raccord fuit un descripteur après `bad_alloc` ou exception
du writer. Deux microcaptures natives normal/UBSan identiques, contrôle
sans exception et sentinelles privées ; RAII du FILE avant toute opération
qui peut lever. Ces captures ne prouvent pas de fichier utilisateur perdu.
La porte d'arrondi de la tour compare catalogue et `OrderForest`,
pas `point_dendrogram` ni la tête. Une division non dyadique comme β=200/9
peut publier deux doubles différents selon l'arrondi : borner « sorties
identiques » à l'objet exact réellement jugé. Aucun changement d'étiquettes
n'est démontré par ce seul 200/9 ; le chantier suivant a activé
`ball_nodes` et ajouté la porte dendrogramme distincte décrite ci-dessus.
Les sept correctifs réunis textuellement n'ont toujours pas de binaire
commun qualifié retrouvé ; le nouveau raccord faits_math/SiteTree ne ferme
pas cette union. Tests santé sur l'ancien HEAD et builds partiels ne
s'additionnent pas. Le défaut de condensation reste présent.
La [contre-porte des options citées](../receipts/audit_continu_20260929/quoted_build_flags_20260930/README.md)
montre aussi que Clang accepte réellement `"-freciprocal-math"` malgré
la garde CMake et sans macros de refus au préprocesseur. Deux configurations,
deux prétraitements ; ni objet moteur compilé ni résultat géométrique faux
démontré. Les 22/22 de B ne couvrent pas ce cas. À 17 h 23 UTC, la
nouvelle source privée `cmake/fp_flags.cmake` tokenise effectivement les
flags ; les journaux GCC et Clang terminés refusent `cite_auditeur`,
avec 285 contrôles unitaires, 24 refus et quatre témoins chacun. Progrès
observé, pas reçu clos à empreintes avant/après ni nouveau commit.
Les 22/22 mutants CMake seuls sont ensuite clos ; leur somme avec les
anciennes campagnes n'est pas une nouvelle campagne commune. L'archive d303c88 reste
la version fautive éprouvée ; ne pas attribuer ce défaut à la source
nouvelle sans rejeu.

**Juges des fixtures, preuves closes et correction privée observée.**
Le [contrôle causal portable](../receipts/audit_continu_20260929/target_reader_control_flow_20260930/README.md)
montre `valide_lib` code0 malgré `sources_stables=False`, et une variante
`target=[]` déclarée `passe=True` sans aucun jugement. Fonctions réelles
extraites par AST, tests/mathématiques stubés ; normal/−O concordants.
La validation privée close de 389 contrôles a, elle, des sources stables
et zéro échec : elle n'est pas invalidée. Refuser les hashes divergents
et les cibles vides, publier le nombre réellement jugé. La
[réponse du développeur](REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md)
annonce ces gardes, désormais visibles dans `run_target`/`valide_lib`
privés : cible non vide, inventaire, code3 en cas de hashes divergents.
Cette lecture n'est pas un nouveau reçu clos de leur intégration.
Ce ne sont ni deux défauts géométriques ni une qualification des règles.

**Banc de croissance, collision à refuser.** Le
[contre-exemple](../receipts/audit_continu_20260929/banc_output_alias_20260930/capture/README.md)
montre `--calls==--out` : code0/`ok`, CSV et JSONL corrompus par deux
descripteurs sur le même fichier. Vrai `cmd_run`, `measure` simulé,
normal/−O, contrôle distinct valide ; aucun calcul HGP ou mesure 8k.
Refuser avant troncature. La copie fautive éprouvée est désormais dans
le groupe bancs partiel sauvegardé, pas dans le clone courant réinitialisé.
Le [suivi au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md)
distingue aussi condensation terminale/progressive et hypothèses statistiques.
Une garde `realpath`/`samefile` et ses contrôles d'alias sont maintenant
visibles dans un prototype séparé de bancs, SHA
`406bba7b1ae428b922a46631aafdda0e9ef255da6fbb729bbf3a8a64aef01c15`.
Elle n'est pas encore la source du clone d'intégration courant ; ne pas
attribuer le défaut de l'archive à ce prototype corrigé.

**Massif, garde globale distincte :** `ExtCell::rep_first` indexe une seule
arène `ext_reps` pour tous les K, mais son cast et l'addition à l'accès
restent u32 (`tower.cpp:1290/1024`). Les gardes par ordre `sr[k]` et sur
le nombre de cellules ne bornent pas cette somme globale. Élargir décalage
et addition, ou refuser avant insertion/conversion. Réserve d'adressage
pour le futur massif, pas contre-exemple géométrique exécuté ; la preuve
indépendante protégeant forêt/CSR actuelle reste correcte.

**Port précis, contre-épreuves du même jour :** relever seulement le refus
u18 serait incorrect. Les corps géométriques donnent déjà un rayon q4
tronqué à u21 ; à u24, un rayon devient zéro sans diagnostic UBSan.
Les filtres de contacts doivent aussi changer, pas seulement leurs entiers.
Les [preuves séparées](../receipts/audit_continu_20260929/precision_port_20260930/README.md)
conservent les erreurs attendues, les contrôles positifs et un préflight
scalaire rejeté. Elles ne démontrent aucun défaut du profil u18 protégé.

**Réparation isolée après ces contre-épreuves :** le
[filtre relatif certifié](../receipts/audit_continu_20260929/relative_filter_20260930/README.md)
passe 3 600 requêtes Fraction normal/UBSan, dont 196 contacts et 40
translations bit à bit ; deux mutants code0 sont rejetés mathématiquement.
La contre-relecture du juge conserve R1/R2/R3 et leurs limites. Ce n'est
pas un nouveau nearest natif, un repli exact ni un port FULL large ;
parallélisation, croissance et G4 restent non qualifiés.
Le [lecteur renforcé](../receipts/audit_continu_20260929/relative_filter_reader_r2_20260930/README.md)
vérifie les hashes avant import et les ensembles d'empreintes obligatoires,
sans modifier la première clôture.

**Ordre des niveaux larges :** le
[comparateur entier isolé](../receipts/audit_continu_20260929/level_order_20260930/README.md)
passe 2 444 requêtes normal/UBSan et tue les deux mutants numériques.
Les 729 comparaisons géométriques sont 27² couples de niveaux Python,
pas un constructeur natif qualifié. Le port doit aussi reprendre les
seuils, les distances K-NN sur 66 bits et les exports de rangs exacts ;
la table double de points fusionne délibérément certains niveaux distincts.
Ni le tri natif large, FULL, croissance ou performance G4 ne sont acquis.

**Compléments indépendants publiés dans `782e0d2a0` :**
[Morton96](../receipts/audit_independant_20260930/grid32_followup/README.md)
conserve bien les coordonnées dans un contexte de grille ; une translation
commune peut inverser ses rangs sans changer les distances. Ne pas employer
ces rangs comme IDs persistants entre origines. Le
[croisement K2/K3](../receipts/audit_independant_20260930/cover_band_followup/README.md)
des partitions de points ne réfute ni FULL ni la laminarité à K fixé.
L'utilisateur demande actuellement une hiérarchie depuis le seul arbre
de niveau K : ce croisement n'est donc pas un blocage de cette cible.
Une combinaison future de plusieurs K devra annoncer une autre règle.
Ces petits contrôles ne qualifient pas la robustesse statistique, FULL
large, la croissance ou une nouvelle performance G4.
Réserve de rédaction dans la preuve 1D : la formule p=K−2, q_min=m=2
concerne K≥2 ; K1 est le singleton p=0, q_min=m=1. Le script K1 reste
correct. À K fixé, comparer l'antichaîne minimale des témoins avant LCA
pour éviter les retards dus à un ancêtre redondant ; cette variante change
le bras et n'a pas de qualité statistique démontrée.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Pκ : stabilité et flux, pas qualité frontière démontrée. Vote uniforme par boule forte : instable au contact, même loin du bord de bande. Votes d'identités/poids fixes : stabilité conditionnelle ; comptage industriel ouvert. Deux sélections restent exactes. Corriger cohortes/plateaux avant EOM ; aucun choix produit ni victoire ARI acquis. | [Contact et contrôle fixe](../receipts/audit_continu_20260929/uniform_majority_contact_20260930/README.md), [deux sélections](../receipts/audit_continu_20260929/weighted_majority_select_20260930/README.md), [suivi](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md) |
| Fixtures de projection | F1–F4 cohérentes. Notre Γ exact confirme 75 couples nuage/K ; campagnes développeur 10/10 closes, distinctes de notre test autonome. Aucun vote ni nouveau traitement frontière qualifié. | [Contre-audit des fixtures](audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | R2 : CAS saturant et série sans overflow. Deux différentiels clos 24/24, oracles 2/2 ; comparateurs limités à des préfixes SHA96/64bits. Timeout d'une sonde à barrière distinct d'un deadlock démontré. Copies non intégrées. | [Complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces | Parseur strict sur copie R2. Nouvelle CLI tête : refus numériques propagés, mais mêmes destinations étiquettes/arbre → texte écrasant les labels, code0. Quatre sondes closes, raccord avec écritures vérifiées encore en chantier. | [Collision et raccord R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md) |
| SiteTree et centres rationnels | Filtre limité à FE_TONEAREST. Raccord isolé d303c88 : 22/22, guards tour sous quatre arrondis ; objets exacts jugés, pas dendrogramme/condensation. Contre-porte 34 mutants recouvrante, ASan/TSan isolés. Pas d'union FULL qualifiée. | [Complément SiteTree R2](audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md), [suivi du raccord](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md) |
| Tête numérique | R2 protège racine zéro et M·λ_max ; notre porte native antérieure passe. Quatre nouvelles sondes confirment Outcome dans la CLI tête et un refus tardif avant écriture dans la même hiérarchie. Ancienne tête dans la copie CLI stricte ; union non qualifiée. | [Contrôle R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [défauts et borne d'origine](audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md) |
| Condensation des points | Seuil des départs différés et plateaux atomiques à réparer. Deux encodages de la même ultramétrique changent les clusters ; référence indépendante par cohortes. Une tête directe O(H+n log n) est possible sous les préalables publiés ; aucun port ni gain natif mesuré. | [API et référence](../receipts/audit_continu_20260929/point_condensation_20260930/README.md), [témoin géométrique](../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md), [plateaux](../receipts/audit_continu_20260929/point_plateau_condensation_20260930/README.md), [plan direct](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#condensation-directe-sans-expansion-de-la-tour) |
| Rejets q3/q4 par groupe | Somme affine de puissances → plusieurs intérieurs certifiés ; dominance individuelle vide sur deux fixtures 3D. Test division-free strict, contacts gardés, groupes recouvrants non additifs. Sélection/coût total/croissance encore à mesurer. | [Preuve et essai borné proposé](../receipts/audit_continu_20260929/group_moments_20260930/README.md) |
| Juges catalogue/FULL | Petits juges R2 renforcés contre-vérifiés. Nouveau lecteur structurel des grands dumps : ordre K entier manquant ou coordonnées d'attaches inconnues acceptés sur fixtures ; contrôles linéaires à ajouter. Aucun dump LiDAR réellement fautif observé. | [Compléments R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [angles morts d'origine](audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) |
| Juges des cibles de clustering | Deux défauts causaux : sources divergentes n'imposent pas l'échec ; variante vide déclarée gagnante. Contrôles AST avec stubs, pas géométrie. Les 389 contrôles privés source-stables ne sont pas réfutés. | [Preuve portable](../receipts/audit_continu_20260929/target_reader_control_flow_20260930/README.md) |
| Comparaison PR face à HDBSCAN | Même G unique et K incluant l'ancre. EOM Python : puissances80 mais calculs28 ; filtre des ancêtres quadratique évitable. z1 descriptif, z2 absent. Amendement de verdict non reporté dans le draft et contrôles d'inventaire CSV à renforcer avant TEST. Aucun score réel changé démontré. | [Contre-tests et corrections](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#comparaison-pr-et-corrections-avant-le-test-final) |
| Bancs et arrêt des calculs | Groupe partiel sauvegardé/retiré du clone, portes finales non closes. Collision CSV/JSONL reproduite dans sa copie 14f3915d, `measure` simulé, code0 malgré corruption. 120 vrais signaux POSIX antérieurs distingués des simulations. A/C non réfutés. | [Collision](../receipts/audit_continu_20260929/banc_output_alias_20260930/capture/README.md), [complément R2](audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) |
| Prototypes CPU | J3 réduit le CPU de t_boxes ×1,37–1,55, mêmes comptes ; mutant survivant équivalent par parité. 1 060 cas conclusifs et dix délais observés, TSan frontière v3b terminé. Gain p1c CPU total FULL K5 seulement 2,5 % sur le lot local ; variantes non combinées sur G4. | [Contre-audit CPU, périmètres et preuves](audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md) |
| Aval ordre/tête | Contre-audit nouvelle copie : validation parallèle CSR hors bornes sur objet public forgé, alors que série refuse. Temps mur local ordre+assemblage réduits, sans preuve GPU/FULL 100 ms. Gate nouvelle copie 9/9 réellement close, défaut CSR toujours reproductible ; refus et interruptions séparés des cas conclusifs. | [Contre-audit ordre/tête](audit_continu_20260929/performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md) |
| CUDA | Unsigned accepté ; contrôle hôte UBSan propre. Statuts et durées corrigés dans 779dd38a9, mais lecteur d'enveloppe seulement : vingt entrées et huit simulations en précisent les limites. Débits historiques signés invalides, aucun nouveau reçu GPU ni port FULL GPU qualifié. | [Sonde corrigée](audit_continu_20260929/timeout/CONTRE_AUDIT_SONDE_CORRIGEE.md), [statuts et échecs](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |
| Plusieurs dizaines de millions | RankIndex corrigé dans le produit : milieu par différence et `lo*64` élargi avant clamp, 36 047 contrôles virtuels et deux mutants tués. Cela ne qualifie pas la capacité massive, les autres conversions ou la nouvelle précision. Certificat Q×Z encore à implémenter. | [Développement et portée](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md), [audit massif indépendant](AUDIT_MASSIF_LIDAR_20260930.md) |
| Précision au-delà de u18 | Morton96/distance u128, filtre relatif et comparateur 266/200 bits éprouvés isolément. Constructeurs, seuils K-NN 66 bits, nearest, propriétaire et FULL encore à porter ; distinguer rangs exacts et table double. Préparateur décimal exact disponible ; consommateur v10 des pas, origines et IDs manquant. Aucune croissance ou performance G4 large héritée. | [Ordre exact](../receipts/audit_continu_20260929/level_order_20260930/README.md), [filtre et limites](../receipts/audit_continu_20260929/relative_filter_20260930/README.md), [ordre de développement](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md) |

La borne locale K2 demande une marge stricte autour du seuil d'ambiguïté.
Elle garantit des dates sous perturbations appariées, pas l'ARI, l'EOM ni
une généralisation K5. Différer des points jusqu'à la fusion peut perdre
le rappel frontière recherché : mesurer leur récupération avant connexion
parasite, pas seulement les hauteurs. La borne de packing des voisins ne
borne pas les paires de voisins, les q3/q4 ou les visites de l'index.

Les [trois verrous transmis au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md)
priorisent le choix de masse frontière, la validation CSR et une qualification
sur un binaire réellement intégré. La réponse `e9eab2754` retient ces choix,
mais leur raccord reste à auditer. Ajouter le diagnostic de masse fractionnaire
du chapitre 9 avant condensation. Les petits contre-exemples ne sont pas
une autorisation d'élargir les optimisations sans signal utile.

Le [nouveau complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md)
transmet la collision de sortie, le raccord des refus et les gardes de
schéma peu coûteuses. La section 10 de
la note mathématique interdit de considérer `1/β` comme solution robuste
déjà acquise. L'attache réellement unique avec marge et le diagnostic de
durée couverte restent des bras limités, pas des choix produit qualifiés.
La section 11 impose de traiter les entrées frontière internes à K3/K5
et de conserver la masse des continuations ; elle prouve à K2 au moins une
incidence de feuille par point, pas toutes ses composantes couvrantes.
Le succès SiteTree R2 est clos seulement
à son périmètre, pas comme contrat FENV de toute la tour.

Observation de conception, vers 05:24 UTC : la porte frontière en chantier
reprend nos entrées internes K3/K5 en G7 ; G8 vérifie le dédoublonnage de
deux boules couvrantes d'une même composante. Les sources ont évolué pendant
la lecture : aucune exécution ni qualification nouvelle de cette porte
par notre audit. Les bras actuels n'utilisent pas de poids de durée ; le
cas de continuation devient une garde à ajouter si cette piste est retenue.
Le contact inverseβ reste un contre-test, pas une preuve de robustesse.

Les preuves natives de couverture utilisent l'archive `6206d1d11` ; les
preuves de consommateurs utilisent la copie corrigée du pool. Elles ne
constituent pas ensemble une qualification d'un unique binaire intégré.

## Références historiques — ne pas confondre avec les travaux actifs

- [Archive de l'audit v9](../receipts/audit_v9_20260928/README.md) : origine de la refonte v10,
  avec sources et contre-vérifications. Ne qualifie pas automatiquement v10.
- [Audit hiérarchie k-NN](audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md) :
  étude antérieure de l'objet et de la tête, à lire avec les objections ci-dessus.
- [Trois pistes multi-K](tete_multik_20260929/README.md) : expériences et juge,
  pas trois implémentations recommandées ni une solution à la laminarité.
- Les essais interrompus, mutants et échecs des sous-dossiers de preuves
  sont conservés pour la traçabilité ; ils ne sont pas des qualifications vivantes.

## Rangement et coordination

Une seule vue courante : ce fichier. Une réponse cite le constat et indique
son état — ouvert, corrigé à relire, ou clos — sans reproduire son rapport.
Pas de copies de sources, builds, journaux ou reçus entre dossiers d'audits.
Le prochain état remplace cette vue ; il n'ajoute pas un nouvel index concurrent.

Le point d'entrée `audits/README.md` a été actualisé par son propriétaire ;
je ne le modifie pas. L'audit indépendant et l'archive v9 ont été publiés
dans `dc4915666`, puis la navigation dans `bd8a9286f`. Les travaux encore
locaux des autres acteurs sont préservés et exclus de notre publication.
Nos captures brutes sont
dans `receipts/audit_continu_20260929/`, avec relocalisation à hashes identiques
publiée dans `c5015a570`. Les nouveaux petits tests y restent aussi. Les
rapports utiles sont conservés dans `audits/`, sans copie de journaux ou
de builds. Les fichiers des autres intervenants restent sous leur contrôle.
GCP non utilisé dans cette tranche ; aucun contrat ou statut public promu.
