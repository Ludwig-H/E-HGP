# Coordination Morse HGP 3D v8

## 13 septembre 2026 — ROOT : changement de cap demandé par l'utilisateur

La demande courante est un audit général v7 puis une refonte v8 ; publication
sur main uniquement. L'[audit constructeur](../morsehgp3D_v8/docs/AUDIT_V7_SYNTHESE.md)
et ses quatre volets sont ouverts. Les fichiers indépendants v7, le journal
v7 déjà modifié et le travail local fused_history restent intacts et non
inclus dans cette publication. Aucun moteur v8 ni usage GCP.

Les trois contrelectures internes ne se substituent pas à l'auditeur
indépendant. Merci de signaler une contradiction dans les fondements,
le rôle de la géométrie ou les périmètres de mesures, et de contre-auditer
en priorité ces propositions avant leur implémentation :

1. La construction Morton réelle possède-t-elle la borne de décomposition
   nécessaire, ou faut-il une construction fair-split qualifiée ? La
   couverture comptable actuelle ne ferme pas cette question de coût.
2. Propager des identifiants de témoins universels parent→enfants, avec
   exclusion explicite des sites déjà crédités et masques par voie,
   permet-il un réemploi sûr de h sans double comptage h_a/h_b ?
3. Quelles classes de crédits et bornes négatives évitent le travail
   quadratique des histogrammes de gros facteurs, indépendamment de
   la seule taille de la liste de rectangles ?
4. Pour le graphe daté sur les naissances, quelles obligations minimales
   suffisent à transporter la contraction parallèle vers les plateaux
   HGP, contributions et verticales, sans refaire le calendrier ?
5. L'éventuelle représentation implicite de sorties quadratiques doit
   rester un contrat explicite distinct : quelles requêtes et quels
   coûts d'expansion seraient réellement conservés ?

Aucune réponse indépendante nouvelle n'est encore enregistrée ici.
Les avis indépendants v7 restent ceux de leurs fichiers publiés.

## Fenêtre de publication ROOT

Index constaté vide sur main dc57ffd5. Réservation ciblée pour AGENTS.md,
ce nouveau journal, morsehgp3D_v8/ et tools/check_docs.py (inclusion explicite
du corpus constructeur v8 avec test de découverte). Aucun fichier v6,
v7, auditeur indépendant, registre formel ou script GCP ne sera préparé.
Vérifications sur export neuf de l'index avant commit/push. Pas de branche.
La réservation est limitée à cette publication et expire une fois son
commit publié sur main ; les contrôles de préparation sont consignés dans
le reçu PUBLICATION_CHECKS de la v8.

## 13 septembre 2026 — P0 confirmé : supprimer les histogrammes quadratiques

ROOT : l'utilisateur demande explicitement de placer le changement radical
évitant O(|A|²+|B|²) au premier rang, sans imposer la piste du petit
ensemble de témoins. Le [plan](../morsehgp3D_v8/docs/PLAN_DE_REFONTE.md)
est réordonné ; API minimale et juges servent cette comparaison.

Question prioritaire à l'auditeur : existe-t-il une meilleure structure
que les petits ensembles certifiés — parcours conjoints, requêtes
géométriques groupées, sélection directe ou combinaison — pour réduire
le travail total sur gros facteurs ? Demander une preuve de rejet et de
complétude, puis compter préparation, raffinements, résidus et coût aval.
Ne pas assimiler candidates restantes et sortie FULL. Aucun nouvel avis
indépendant n'est anticipé ; aucun code moteur ni benchmark ni GCP.

Index constaté vide sur main2b658cbe. Réservation ciblée : AGENTS.md,
ce journal, README/PASSATION v8, audits/ETAT_COURANT.md et WSPD_Q2_Q3_Q4.md,
docs/PLAN_DE_REFONTE.md, AUDIT_V7_SYNTHESE.md, ALGORITHME_EXPLIQUE.md,
FAUSSES_PISTES.md. Aucun fichier v6/v7, reçu clos, registre ou outil modifié.
Contrôles documentaires avant commit/push sur main ; réservation close
à la publication de ce changement documentaire.

Contrôles de cette publication depuis un export neuf de l'index :
`python3 -B tools/check_docs.py` PASS, 522 Markdown ;
`python3 -B tools/check_implementation_status.py` PASS, 20 phases ;
`python3 -B morsehgp3D_v8/tests/docs_scope_gate.py --selftest` et son
mode `-O` PASS, 27 contrôles chacun. Le journal est également validé
explicitement. Contrelecture de P0 et du premier chantier sans correction
requise. Ces tests restent documentaires ; aucune qualification moteur.

## 13 septembre 2026 — consignes des autres verrous pour le développeur

ROOT : à la demande de l'utilisateur, la note
[VERROUS_ARCHITECTURE](../morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md)
consigne B1 recherches répétées, B2 triangles×voisinages, B3 MEB/descentes,
B4 histoire/export et B5 résidence/transport. Sources historiques et
critères de validation sont séparés des propositions. P0 reste premier.
La contrelecture constructeur ne se substitue pas à un nouvel avis de
l'auditeur indépendant ; merci de signaler toute borne ou critère manquant.

Index constaté vide sur main f375d2c6. Réservation ciblée pour ce journal,
README/PASSATION v8, audits/ETAT_COURANT.md, docs/PLAN_DE_REFONTE.md et
docs/VERROUS_ARCHITECTURE.md. Aucun moteur, reçu clos, fichier v6/v7,
registre formel ou script GCP modifié. Contrôles sur export neuf de l'index
avant commit/push sur main ; réservation close à cette publication.

Contrelecture constructeur close : autoriser une hausse des rectangles
ou candidates si le gain net est démontré ; distinguer histoire séquentielle
et consultations parallèles de l'export. Contrôles depuis l'export neuf :
check_docs PASS523 Markdown, check_implementation_status PASS20 phases,
docs_scope_gate normal/−O PASS27 contrôles chacun. Aucun test moteur,
benchmark ou usage GCP ; les contrôles de cette passe sont documentaires.

## 13 septembre 2026 — démarrage de l'implémentation P0 mono

ROOT : feu vert utilisateur pour coder, G4 SPOT gardée si nécessaire.
Cadre actif implementation_v8_p0 / cpu_reference / quantized_u16_input_only,
hors registre, public_status=not_claimed. Pas de GCP à ce stade.
Les consignes P0 et VERROUS_ARCHITECTURE sont lues ; les cinq verrous
restent ouverts. Le premier module compare un petit ensemble de témoins
certifiés et un parcours conjoint ancres×blocs de témoins, sans imposer
les histogrammes exhaustifs. Les classes de crédits représentent le
résidu sans développer ses paires pour simplement les compter.

À l'auditeur à venir : merci de contrelire le contrat de cette brique,
la disjonction des crédits, les bornes de blocs strictes et le coût du
résidu. Les oracles restent séparés des chemins produits. Cette brique
ne sera pas annoncée comme une tour FULL ni une WSPD complète. Aucun
fichier d'auditeur indépendant ou de v6/v7 ne sera modifié/inclus.
Index libre à l'ouverture ; aucune réservation de commit pour l'instant.

ROOT — première implémentation disponible pour contrelecture :
`morsehgp3D_v8/src/pipeline/local_credits.cpp`, ses headers et
`docs/P0_CREDITS_LOCAUX.md`. Deux méthodes, pool directionnel et parcours
conjoint exhaustif saturé ; les mises à jour collectives sont différées
dans l'arbre d'ancres, sans balayage caché de U à chaque crédit de Z.
Merci de vérifier notamment cette propagation, la disjonction des tâches,
les résidus et les familles qui font dégénérer le parcours. Le retour
complémentaire sur les quantificateurs est pris en compte. Les rails et
les tubes sont des bras importants pour la suite, pas une validité
universelle prêtée au pool global. Aucun avis indépendant sur le parcours
n'est encore présenté comme acquis ; l'index reste libre à ce stade.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : première contre-fixture P0

Un second auditeur intervient dans
[`morsehgp3D_v8_complementaire/`](morsehgp3D_v8_complementaire/ETAT_COURANT.md).
Apport vérifié : pour deux facteurs collinéaires séparés de 64 sites,
un mauvais réemploi de la borne « aucun témoin universel commun » comme
« aucun crédit par ancre » laisse 4 096 paires au lieu de 55 à besoin 10.
Le modèle exact et son contrôle positif directionnel sont disponibles ;
ce n'est pas un défaut imputé au moteur en cours de construction.

Première lecture du nouveau `classify_witness_block` : le développeur
emploie bien un b0 fixe et un maximum sur toutes les ancres et témoins,
ce qui évite ce piège de quantificateurs. Avis favorable sur ce choix ;
la confrontation C++ et l'examen du parcours restent en cours. Autre piste
à comparer : arrêter un raffinement facultatif en conservant tous les
indécis dans le résidu borne l'amont sans tronquer la sortie.

Périmètre propre : ce nouveau sous-dossier seulement ; aucune réorganisation
des six rapports constructeur récents ni des reçus épinglés v7. Ce message
est ajouté au journal partagé, sans préparer les modifications antérieures
du développeur. Aucune réservation de l'index à ce stade. GCP non utilisé.

Complément concret pour le développeur et l'auditeur tubes/rangs :
la [famille de rails](morsehgp3D_v8_complementaire/P0_RAILS.md) donne une
comparaison discriminante. À q4/h8, 2 718 sites u16 séparés à s12 : tout
pool global fixe de huit témoins par facteur laisse au moins 1 436 463
paires, contre 2 916 pour les crédits locaux exacts. La preuve couvre
tous les choix de ces pools. Une partition en rails et des témoins dirigés
par rail retrouvent les comptes saturés ; les tubes à Q_C=0 aussi selon
leur contrat. Merci de l'ajouter aux comparaisons Pool/DualBlocks/tubes,
avec budget réellement utilisé : le constat ne vise pas les pools enrichis
par d'autres recherches. Modèles bornés, aucun claim de tour ou de temps.

**Raccord C++ maintenant testé** : le `.cpp` apparu pendant l'audit prend
h+1 propositions, donc neuf par facteur au besoin huit. Sur les mêmes
rails n2718, il conserve effectivement **1 846 881 paires avec Pool** et
**2 916 avec DualBlocks**, à s8/10/12. DualBlocks retrouve tous les crédits
saturés attendus ; ses 7 648 tâches et 1 224 couples de feuilles montrent
qu'il ne développe pas ici le carré local. Neuf cas C++20 strict/UBSan
passent, avec expansion physique, identités et non-vacuité des blocs
positifs/négatifs. Avis favorable sur ce raccord pour cette famille.
Le [reproducteur](morsehgp3D_v8_complementaire/local_credits_probe.cpp)
peut être repris comme fixture par le développeur ; reçus en préparation.
Ni le temps de tour ni les coûts q3/q4/census ne sont couverts.

Reçus clos : [état et commandes](morsehgp3D_v8_complementaire/ETAT_COURANT.md).
Quatre mutants C++ réellement compilés sont réfutés : frontière fermée,
maximum remplacé par minimum, colonne répétée à l'expansion et double
crédit de feuille. Les copies mutées sont temporaires ; sources intactes.

Fenêtre de publication AUDITEUR_COMPLEMENTAIRE : index constaté vide sur
main dc246d6b. Réservation limitée à `audits/morsehgp3D_v8_complementaire/`
et à cette seule section du journal. L'ouverture d'implémentation du
développeur, déjà modifiée avant mon arrivée, reste hors de cette préparation,
comme tous ses fichiers v8/v7/v6 et ceux de l'autre auditeur. Vérification
des blobs préparés, liens et empreintes avant commit/push sur main ; cette
réservation expire à la publication de ce commit. GCP non utilisé.

Contrôles de cette publication : `check_docs.py` PASS, 527 Markdown ;
`check_implementation_status.py` PASS, 20 phases ; validation explicite
des trois Markdown indépendants/journal PASS. Les hashes des quatre sources
C++ du module correspondent encore aux reçus à cette vérification.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : deuxième passe P0

**P1 reçu de comparaison :** le runner `bench/run_p0_matrix.py` accepte
le JSON sans vérifier que n/stratégie/lane/famille/Kmax/s correspondent à
la commande. Un rejeu artificiel d'une vraie ligne Pool/q2 donne huit
lignes contradictoires sur neuf mais une campagne `completed`, code 0.
Le contrôle positif avec une sonde réelle donne neuf lignes cohérentes.
Ce contretest vise la validation du reçu ; il n'allègue aucun reçu réel
corrompu ni défaut du moteur. Merci de vérifier tuple, schéma, statut,
périmètre et identités des compteurs avant d'incrémenter les succès.

**P2 diagnostic :** une sortie JSON tronquée quitte le runner avant de
consigner l'essai, sans `COMPLETION.json`. Conserver stdout brut et fermer
la campagne en `invalid` rend cet échec auditable. Le binaire est seulement
hashé au départ : figer celui du build identifié et revérifier son hash à
la fermeture évite également un mélange de versions durant la campagne.
Runner examiné `d2f2514b…`; reproduction et reçus complémentaires en
préparation. Aucune réservation d'index à ce stade. GCP non utilisé.

**Limite du certificat, même avec histogrammes parfaits :** sur deux
[rangées transverses](morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md),
les crédits universels sont tous nuls, mais p(i,j)=2 max(0,|i−j|−1).
À h10/n512, les trois méthodes émettent 65 536 paires et seulement 2 786
passent le census q2 indépendant. Leurs préparations sont courtes ; le
coût est dans le résidu. Les 24 essais C++ strict/UBSan et la frontière
mutée sont clos. Ce cas justifie une comparaison de certificats sur
sous-rectangles ou de profondeur par blocs, en conservant la propriété
WSPD et les exclusions. Le census scalaire employé est un juge borné,
pas le chemin produit proposé ; aucune sortie FULL n'est revendiquée.

**Piste favorable d'ordonnancement DualBlocks :** une tige diagonale q4
avec huit bons témoins en bout laisse 36 candidates avec Pool et Dual,
mais à n2064 Dual visite 188 910 tâches avant de trouver les bons témoins.
Une copie temporaire changeant seulement l'ordre de visite de leurs
enfants tombe à 82 tâches, avec les mêmes crédits saturés et candidates.
L'arbre reste construit et payé. La qualification du correctif est en
cours ; le critère x testé sur cette famille motive un ordre par projection
vers B, sans prétendre donner un ordre optimal universel. Ne pas initialiser
le parcours par les comptes du pool sans exclure ensuite leurs IDs.

**Tubes : qualification complémentaire close** dans
[TUBES_CHECKS](morsehgp3D_v8_complementaire/TUBES_CHECKS.json) : 3 051 plans,
65 772 crédits confrontés aux coins par un oracle indépendant, 360 933
paires vérifiées. Directions négatives, frontière D=10R, s1 avec repli,
cœurs/voies inactives et produits dépassant i64 sont exercés. UBSan et
mutants Δ≥0/100→25 passent les attentes. Avis favorable sur cette géométrie
avec propriétaires factory non mutés ; le défaut de copie/affectation
signalé par l'autre auditeur reste sous son suivi, sans qualification
d'immuabilité transférée par nos contrôles.

La [comparaison d'ordre](morsehgp3D_v8_complementaire/P0_ORDRE_TEMOINS.md)
est close : 15 cas par ordre, oracle multiprécision exhaustif, comptes
littéraux identiques et cinq cas UBSan. Les 47 248 visites de construction
restent payées à n2064. Sur grille n256/q2, moins de tâches mais couples
de feuilles 1→22 : cette hausse est conservée, aucun gain monotone allégué.
Les [reproducteurs et statuts actifs](morsehgp3D_v8_complementaire/ETAT_COURANT.md)
remplacent les exposés anciens dans notre entrée courante ; preuves et
reçus initiaux restent inchangés.

Contrôle des campagnes arrivées pendant cette passe : les **729 tuples
commande/résultat réels concordent**, et `check_p0_campaign.py` est passé en
normal/`-O` avant leur archivage (quatre campagnes, 513 configurations).
Le développeur les a ensuite déplacées dans `first_pass_pre_owner_fix` ;
aucune mesure n'est transférée aux corrections en cours. Ce constat est
[épinglé séparément](morsehgp3D_v8_complementaire/CAMPAIGN_INITIAL_CHECKS.json).
Le défaut d'ingestion P1 ne signifie donc pas que ces captures contiennent
des lignes contradictoires. Les conclusions mesurées et la recommandation
de renforcer le runner restent distinctes.

À la demande de ROOT de publier avant sa réservation : fenêtre
AUDITEUR_COMPLEMENTAIRE limitée à notre sous-dossier et à cette section
« deuxième passe P0 ». Index constaté vide sur main `7f4d2ac0`. Les messages
ROOT, ses sources/campagnes et les fichiers de l'autre auditeur restent
hors de notre préparation. Notre rejeu des rails passe aussi avec le header
corrigé `f6c89476…`, reçu distinct ; la contre-qualification du runner
durci annoncé reste à faire lorsque ses octets sont disponibles.
Cette fenêtre expire au commit publié sur main. GCP non utilisé.

Contrôles depuis un export neuf de l'index : documentation canonique PASS
(523 Markdown), registre PASS (20 phases), validation explicite de nos cinq
notes et du journal PASS. Les brouillons constructeur, dont les liens en
cours d'ajustement après archivage, ne font pas partie de cet export.

## ROOT — intégration de la proposition tubes/rangs

Les deux avis indépendants sont lus. La première brique Pool/DualBlocks
a passé cinq CTests locaux avant cette extension. La troisième stratégie
`Tubes` est maintenant codée dans `src/pipeline/tube_credits.hpp` et
raccordée au sélecteur ; son oracle C++ est en préparation. Le tri reste
payé par voie dans cette première API. Le facteur 100 de la condition
d²≥100 diam² a été rétabli lors de la contrelecture d'une consigne : 25
aurait été insuffisant. Une fixture q4 concrète est transmise au juge.

Le contrat `docs/P0_CREDITS_LOCAUX.md` intègre vos remarques : pas de
double addition du cœur sans IDs à un census extérieur ; validation et
propriété du nuage à mutualiser avant la WSPD. Les rails n2718 entrent
dans la sonde constructeur pour une comparaison chronométrée des trois
méthodes. Les premières mesures n8k/16k/32k suivront les gates ; résidu
et préparation seront publiés séparément. Aucun contrat FULL annoncé.
Merci de signaler tout contre-exemple utile à ces changements. L'index
reste libre après vos publications, GCP non utilisé.

## ROOT — corrections avant publication de la première brique

Défaut de propriétaire confirmé : les quatre opérations implicites sont
désormais supprimées dans PreparedRectangle. Les traits et les deux
contrôles géométriques de la contre-fixture intègrent p0_gate. Le runner
est également durci contre le rejeu de JSON, les sorties tronquées et les
changements de binaire ; le lecteur vérifie désormais chaque tuple de
commande. Les 729 premières captures et leurs tests sont conservés intacts
dans `receipts/p0_local_credits_20260913/first_pass_pre_owner_fix/`, sans
les appeler qualification de la version corrigée. Builds neufs et nouvelles
captures suivront les gates. Merci pour ces deux corrections substantielles.

L'ordre de visite des témoins Dual et le résidu transverse sont des pistes
suivantes à comparer ; ils ne seront pas intégrés sans nouveau différentiel.
Pas de quota de troncature introduit. L'index reste libre à ce stade ;
merci de publier vos sections du journal séparément avant notre réservation.

## ROOT — alias du tampon pris en compte, publication différée

La nouvelle contre-fixture P0_INPUT_ALIAS_CHECKS est lue et confirmée par
la contrelecture interne. Le déplacement du vecteur n'est pas une frontière
d'immutabilité. La factory va copier les coordonnées dans son stockage
privé avant validation ; un test permanent gardera la source vivante et
la modifiera après préparation, sans écrire dans un ancien tampon détruit.
La copie sera comptée dans les nouvelles mesures, puis mutualisée au
propriétaire global du nuage lorsque celui-ci sera implémenté.

Les huit gates GCC et ASan/UBSan ont passé sur r2 et ses 729 captures
concordent, mais elles ne ferment pas ce défaut d'alias. Elles seront
conservées distinctement ; r3 refera tests et campagnes après correction.
Index toujours libre pour votre publication annoncée. GCP non utilisé.

ROOT : correctif d'alias maintenant gelé dans local_credits.hpp/.cpp et
p0_gate.cpp. La signature prend const RectangleInput&, copie points et
propositions, puis valide le stockage privé. Même un appel avec std::move
ne consomme plus le tampon source. La fixture couvre les trois stratégies,
la modification de la source, les trois pertes du contre-modèle et les
quatre paires du nouveau plan transverse. Requalification r3 en cours.
Votre commit e9e97e64 est vu ; vos preuves restent sous votre autorité.

## ROOT — fenêtre de publication P0

Index constaté vide sur main e9e97e64. Réservation ROOT limitée aux sources,
tests, sonde, reçus et documentation constructeur v8, aux entrées AGENTS.md
et README.md racine et à mes sections de ce journal. Aucun fichier des
auditeurs indépendants ni des worktrees v6/v7 n'entre dans ma préparation.
Merci de ne pas modifier l'index pendant cette publication. La fenêtre
expire au commit/push de cette première brique, sans branche nouvelle.

R3 passe huit CTests GCC Release et huit Clang ASan/UBSan : 148 587
contrôles dans p0_gate, 108 730 dans tube_gate, 41 scénarios de reçus
en normal/−O. Les quatre nouvelles campagnes totalisent 729 mesures,
513 configurations ; leurs lecteurs normal/−O passent, copie privée
incluse. Les deux passes antérieures restent distinctes et conservées.
P0 n'est pas close : nappes à résidu quadratique, WSPD/aval/FULL absents.
Les propositions de sous-rectangles et groupes sont référencées dans le
plan de reprise, sans promotion de votre prototype en résultat produit.
GCP non utilisé ; aucun contrat de tour 50k ou massif acquis.

Contrôle de l'export neuf de l'index b5145378 : recompilation GCC et huit
CTests PASS, documentation PASS528, registre PASS20, lecteur de campagnes
−O PASS729/513. Le code et les tests restent inchangés après cet export ;
seuls le XML et les comptes rendus de publication sont ajoutés. Le cache
statistique de l'index a été rafraîchi pour cet export, sans changement
de son arbre Git. Aucun fichier indépendant préparé, aucune branche créée.

## ROOT — reprise après 3589a2c9 : préparation partagée et résidu

La première brique est publiée sur main ; sa fenêtre d'index est close.
Nouvelle demande « Continue » : même cadre implementation_v8_p0,
cpu_reference/u16, hors registre, not_claimed. Index libre à l'ouverture.
Le partage du tri/cellules Tubes entre q2/q3/q4 est en cours, avec comparaison
aux trois appels séparés sur le même propriétaire. Les captures précédentes
restent épinglées ; leurs résultats ne seront pas transférés aux nouvelles sources.

Question aux auditeurs : le prototype de queues résout q2 sur les rangées,
mais la famille u16 séparée ne passe plus n16k/32k. Nous étudions un
raffinement couvrant des nappes 2D sans rescans de queues, ni quotas, ni
addition de témoins recouvrants. Un arbre de boîtes et des propositions
locales identifiées pourraient servir à restreindre le facteur opposé.
Merci de signaler les contre-exemples et certificats plus simples ; le
travail de raffinement et les paires conservées seront comptés séparément.
Les groupes collectifs q3/q4 restent une piste distincte à qualifier.
GCP non utilisé à ce stade.

ROOT : vos deux nouvelles notes sont lues. Le parcours axial q2 en cours
utilise des colonnes exactes d'A : chaque demi-espace rejeté possède à lui
seul besoin témoins, avec frontières conservées ; un index B émet des
plages, pas des paires. Ce certificat diffère de vos queues utilisant A/B
et peut laisser davantage de candidates. La borne grille complète ne se
transfère pas à chaque ancre d'une nappe tronquée : une fixture permanente
conserve le cas 540>441. Les deux familles seront mesurées.

Le défaut bad_alloc d'affectation CreditPlan est pris en compte : une copie
temporaire complète puis échange sans exception remplace l'affectation
membre par membre, y compris pour le nouveau CreditBatch. Une gate injecte
les échecs d'allocation avant chaque vecteur et vérifie la cible conservée.
Sources/sondes/tests restent en développement, pas encore des captures
qualifiées ; aucun transfert des mesures r3. Index libre pour vos travaux.

ROOT — vos nouvelles contrelectures sont prises en compte : la restriction
sheet du lecteur ne concerne désormais que les voies actives, avec mutant
permanent. La disjonction des trois colonnes exactes est correcte ; le
premier filtre conservateur ne l'exploite pas. Le commentaire erroné sur
leur recouvrement est corrigé, et une fixture distingue maximum d'axes et
somme de deux vrais témoins. L'addition dans les requêtes d'index et les
allocations inutiles évitables sont les deux améliorations suivantes,
distinctes des mesures gelées de cette tranche. Merci pour ces pistes.
Les sondes partagent leurs recettes/émission, et les captures appariées
mesurent deux ordres sur le même propriétaire. Qualification complète
et matrices mono en cours ; pas d'exécution GPU. Index toujours libre
jusqu'à la réservation de livraison explicitement consignée plus bas.

ROOT — cinq matrices closes, 594 mesures/504 configurations par ordre,
sources et binaires inchangés. Les deux lecteurs normal/−O passent avec
provenances déclarées homogènes. Les P1/P2 des reçus appariés sont corrigés
(build/machine, marqueur axial), avec mutants permanents ; nouvelle suite
21 CTests Release PASS, Clang ASan/UBSan en clôture. Les captures antérieures
aux derniers contrôles restent à part, sans transfert de statut.
Le contrat et le README des nouveaux reçus distinguent le gain favorable
des nappes, le coût J/D, les grilles où Pool est meilleur et votre
contre-épreuve de rotation. Aucun bénéfice générique ni FULL revendiqué.
À l'auditeur complémentaire : vos nouveaux fichiers sont encore hors
index ; je les cite et vous laisse les publier avant de réserver la
livraison constructeur. Je ne prépare ni vos fichiers ni votre section
de journal en cours. GCP non utilisé.

ROOT — réservation de livraison après 95548c52, index constaté vide.
Les deux suites finales de 21 CTests passent, ainsi que les lecteurs des
594 mesures en normal/−O. Je réserve l'index pour mes fichiers v8,
AGENTS.md et mes seules sections de ce journal. Les fichiers des deux
auditeurs sont désormais publiés par leurs auteurs ; aucun delta v6/v7
n'entre dans mon commit. Fenêtre close après commit/push sur main.
Un export neuf de l'index vérifiera la livraison avant publication.
GCP non utilisé, aucune session à fermer.

ROOT — export neuf de l'index c7f32684 : documentation PASS532, registre
PASS20, compilation GCC stricte puis 21/21 CTests PASS. Relecture des
594 mesures normal/−O PASS ; les 31 pins produit/juges/outils concordent
avec l'index et l'export. Un index privé évite d'altérer le cache de
l'index partagé pendant les tests. Les entrées restent identiques après
export ; seuls ce compte rendu, le XML d'index et PUBLICATION_CHECKS
sont ajoutés avant commit. Aucun changement de source ni test après ce
contrôle. Les deltas utilisateur v6/v7 restent hors index. La fenêtre
de livraison se clôt par le commit/push qui porte cette section.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : nappes 2D et contre-qualification R3

Réponse concrète à la question ROOT : les queues q2 s'étendent aux vraies
nappes 2D tronquées de la sonde. Notre [prototype indépendant](morsehgp3D_v8_complementaire/P0_NAPPES_2D.md)
utilise les certificats de boîtes de `3589a2c9`, des caches de queues O(m)
et des IDs distincts. À n8k/16k/32k, h10, s12, il conserve respectivement
445 940 / 917 580 / 1 865 300 candidates, contre 16 / 64 / 256 millions.
Les 42 cas par mode normal/−O passent avec UBSan ; le mutant « rangs sans
certificat » échoue. Coût des propositions et descripteurs compté ; aucun
census des grandes entrées, chronométrage ou résultat FULL revendiqué.
Cela donne un proposeur à comparer au raffinement général par boîtes.

Le [rejeu R3 du runner](morsehgp3D_v8_complementaire/CAMPAIGN_RECEIPT_R3_CHECKS.json)
ferme nos deux défauts initiaux : JSON contradictoire et tronqué rejetés,
sortie brute conservée, changement de binaire détecté jusqu'en clôture.
Un P2 reste dans le lecteur : n8/sheet/dual/q3/Kmax1 est une voie inactive
valide (seuil nul, zéro candidate), mais l'invariant inconditionnel `sheet`
la refuse. Restreindre « toutes les paires restent » aux voies actives ;
les 729 mesures publiées de R3 ne sont pas mises en cause.

Autre cas borné, [affectation de CreditPlan](morsehgp3D_v8_complementaire/P0_PLAN_ASSIGNMENT.md) :
si une réallocation lève `bad_alloc` et que l'appelant réutilise la cible,
l'affectation implicite peut avoir changé son propriétaire avant ses
vecteurs. Le juge reproduit une émission hors du facteur B. Interdire
cette affectation si elle est inutile, ou lui donner la garantie forte
par copie puis échange ; ce dernier correctif temporaire passe le même
échec injecté. Les correctifs R3 de PreparedRectangle restent reconnus.
Nous ne modifions pas les sources du constructeur. Index encore libre.

Complément : les [quatre captures R3 réelles](morsehgp3D_v8_complementaire/CAMPAIGN_R3_MEASURES_CHECKS.json)
passent le lecteur normal/−O et notre contrôle indépendant de 729 tuples,
513 configurations, sur les sources exactes de `3589a2c9`. Le refus des
sources maintenant modifiées est correct ; aucune mesure n'est relancée
ni transférée au nouveau partage entre voies.

À l'autre auditeur et au constructeur : la preuve de
[groupes recouvrants](morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md)
complète votre certificat collectif. Des poids β_G dont la charge par ID
reste ≤1 donnent un crédit ceil(Σβ_G). Le choix β=1/degré maximal ne
demande aucun solveur : cinq triplets certifiés sur cinq IDs donnent deux
crédits, contre un avec des groupes disjoints ; la borne est atteinte sur
une vraie boule q4 positive. Le juge rationnel inclut frontières et capacités.
Deux petites preuves bornent aussi la taille des certificats : trois IDs
pour toutes les sphères d'une paire, quatre pour les moments sur un bloc,
sans borne de recherche ni héritage de la porte arithmétique des poids.

Réservation d'index AUDITEUR_COMPLEMENTAIRE sur main `3589a2c9` : uniquement
`audits/morsehgp3D_v8_complementaire/` et cette section du journal. Index
constaté vide avant réservation ; toutes les modifications ROOT et celles
de l'autre auditeur restent hors de notre préparation. La fenêtre expire
au commit/push de cette passe. GCP non utilisé.

Export neuf de l'index `a38f7e91` : documentation PASS528, registre PASS20,
validation explicite de nos huit Markdown et du journal PASS9. Les gates
ci-dessus sont exécutées dans leurs snapshots propres ; ce contrôle
d'export n'annonce pas un nouveau build du développeur. Aucun fichier
extérieur à notre dossier et notre section du journal n'est préparé.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : partage, axes et provenance appariée

Le [rejeu du partage et des exceptions](morsehgp3D_v8_complementaire/BATCH_EXCEPTION_REVIEW.json)
est favorable sur le snapshot hpp `7d96b385`, cpp `24dcbc98`, tubes
`8485c7a5` : 20 pannes d'allocation Plan/Batch, cible conservée ;
195 lots / 585 voies, puis 3 051 voies du juge Tubes comparées aux appels
isolés, préparation comptée une fois. Le défaut d'affectation précédent
est clos sur ces octets. Le reçu embarque le snapshot pour le rejouer
indépendamment des sources en cours ; aucune qualification R3 transférée.

Pour comparer le nouveau filtre axial, notre
[fixture isométrique exacte](morsehgp3D_v8_complementaire/P0_AXES_ET_ROTATIONS.md)
conserve les distances et profondeurs q2, reste u16 et passe s8/10/12.
À n32k/h10 : 6 483 670 candidates alignées, puis 256 millions après
rotation ; le retour exact de repère retrouve 6 483 670. Le repli conserve
bien un descripteur par ancre, sans arbre B : aucun carré caché dans cette
construction, mais le résidu est entier. 27 configurations × trois plans
par mode normal/−O, neuf petites avec census ; aucun census aux grandes
tailles ni temps revendiqué. La note propose le certificat de colonnes
de direction déclarée, sans prétendre résoudre leur recherche générale.

Point constructif supplémentaire sur votre commentaire initial d'addition :
pour une ancre fixée, deux colonnes exactes de directions non parallèles
se rencontrent seulement en cette ancre, exclue des témoins. Leurs comptes
peuvent donc s'additionner, avec contrôle du cœur disjoint. La somme
interdite sans preuve reste la règle générale ; ici le générateur possède
cette preuve particulière.
La [note et le juge](morsehgp3D_v8_complementaire/P0_SOMME_TEMOINS_AXIAUX.md)
sont maintenant disponibles : 131 plans, 24 143 paires, quatre mutants
C++ réfutés. Sur la grille complète 125×128, h10, la formule additive
A seule donne 3 928 390 candidates au lieu de 6 483 670 ; c'est une
proposition calculée, sans port produit ni census des grandes entrées.

La publication `90d22425` de l'autre auditeur est vue : sa preuve et son
modèle de requêtes additives concordent avec notre juge C++ indépendant.
Le constructeur a corrigé le commentaire sur le recouvrement ; c'est le
seul delta entre nos deux snapshots axiaux, confirmé par restitution du
hash. L'addition reste à porter et à mesurer. Vos fichiers restent sous
votre autorité ; nos fixtures ne leur substituent aucune qualification.

Deux points à corriger dans les
[reçus appariés](morsehgp3D_v8_complementaire/P0_RECUS_APPARIES.md) : le
lecteur peut mélanger deux vrais builds Release/Debug dans une médiane
annoncée comme répétition de la même configuration. Refuser ce mélange
ou partitionner les résumés par identité de build et machine. Aucun
mélange réel du constructeur n'est allégué. `validate_axis` doit aussi
contrôler `checksum_kind` : un marqueur de convention erroné est admis.
Les deux défauts persistent sur les nouvelles révisions relues et sont
épinglés séparément du premier snapshot.

L'ancien P2 du lecteur single sur la nappe inactive est en revanche
clos sur `0ebf0c55…` : une nouvelle capture réelle passe normal/−O.
Index encore libre ; sources constructeur et autre auditeur préservées.

Clôture avant publication : les deux corrections de protocole suivantes
sont aussi contre-vérifiées. Le validateur `212efc6e…` accepte la vraie
ligne et rejette son marqueur de checksum erroné ; le lecteur `f4148e22…`
accepte la campagne homogène, rejette l'union Release/Debug et conserve
l'identité de build dans les résumés. Normal/−O passent sur les captures
déjà conservées, sans nouvelle mesure. Les trois constats de reçus de
cette passe sont clos sur ces pins ; le [reçu](morsehgp3D_v8_complementaire/PAIRED_PROVENANCE_CHECKS.json)
garde l'historique et ses addenda. Les pistes P0 demeurent ouvertes.

Réservation d'index AUDITEUR_COMPLEMENTAIRE sur main `90d22425`, index
constaté vide : notre dossier uniquement et cette section du journal.
Les changements du constructeur et de l'autre auditeur restent hors
de notre préparation. Fenêtre close au commit/push de cette passe.
GCP non utilisé.

Export neuf des entrées d'index SHA256 `ea7ab1aa…` : documentation
PASS528, registre PASS20, validation explicite de nos onze Markdown
et de ce journal PASS12. Les entrées sont identiques avant/après export.
Les preuves C++ et lecteurs sont exécutés dans leurs snapshots décrits
plus haut ; aucun nouveau build constructeur n'est déduit de cet export.

## CONSTRUCTEUR — troisième tranche P0, addition et intersection

13 septembre 2026, reprise après `8e406f9b`, sur main. Cadre inchangé :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
implementation_v8_p0 / not_claimed`. Index libre pendant l'implémentation.
Les notes et propositions additives des deux auditeurs ont été lues.

Port en cours dans le même moteur axial : compter ensemble les colonnes
exactes disjointes hors de l'ancre, puis intersecter le résidu avec un
plan local q2 facultatif. Il ne s'agit jamais d'additionner les témoins
axiaux et Pool/Dual/Tubes, qui peuvent être identiques. Une fixture
minimale bloque précisément ce double compte. La restriction doit
posséder le même propriétaire immutable et sera copiée dans le plan,
pour résister à une réaffectation ultérieure du plan local source.

Question de contrelecture : pour un nœud de B, nous rejetons si la borne
inférieure additive atteint le besoin OU si crédit(a)+minimum des
crédits(b) l'atteint. Nous acceptons si les deux bornes supérieures restent
strictement en dessous ; sinon nous partageons. Le coût des copies,
bornes, visites et descripteurs restera visible, ainsi que celui du plan
local dans le bras intersection. Pas de promotion sur les seules nappes
alignées ; la rotation et les grilles où Pool est meilleur restent des
contre-épreuves. Vos propositions de census q2 sont vues et restent le
raccord suivant, pas une capacité déjà implémentée. GCP non utilisé.

Mise à jour constructeur : le port additif, l'intersection intégrée et
la suppression des allocations aussitôt remplacées sont codés. Le test
local de crédits puis le test de boîte axiale précèdent les recherches
de rang coûteuses. Les 26 CTests Release passent ; sanitizer et captures
finales suivent, toujours sans GCP. Vos deux propositions de composition
et de census ont été lues ; le coût de 295,5 millions de visites aval
dans le prototype publié est un témoin utile, pas un résultat de nos
nouvelles sources. Les campagnes mono seront exécutées séquentiellement
après les gates, sans autre benchmark lancé par le constructeur. Merci
de garder vos éventuelles contre-mesures lourdes hors de cette fenêtre.

Fenêtre de mesures close : 26 CTests passent en Release et Clang
ASan/UBSan ; 648 mesures valides dans `receipts/additive_q2_20260913/`,
576 configurations par ordre, sources/binaire inchangés. Lecteurs normal/−O
PASS. La nappe complète garde 3 928 390 candidates mais son filtre coûte
environ 238 ms contre 57 ms pour Independent. L'intersection Pool sur
grille garde 114 716 candidates en 34–35 ms ; Pool seul reste plus rapide
à préparer (2,4–2,6 ms, 378 840 candidates). Ces comparaisons sont toutes
dans le rapport, sans choisir seulement la référence favorable.

Réservation d'index CONSTRUCTEUR après `28bcd9fb`, index constaté vide :
fichiers propres v8, AGENTS et notre section du journal uniquement.
Les audits indépendants et changements v6/v7 restent hors préparation.
Export neuf et tests avant commit/push sur main ; la fenêtre sera close
après cette publication. Aucun usage GCP.

Export d'index `bf14c38a…` clos : 26 CTests PASS, mêmes sorties que le
Release principal, 36 pins source identiques aux blobs préparés,
lecteurs normal/−O PASS648, documents PASS534 et registre PASS20.
Le reçu PUBLICATION_CHECKS conserve les commandes et le pin du XML.
La relecture `4f8017f7` est vue : aucun défaut sur les sources épinglées ;
la remontée des extrema de crédits et le raccourci de restriction vide
restent deux optimisations suivantes, sans modifier la capture gelée.
Fenêtre d'index close dès le commit/push de ces fichiers constructeur.


## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : intersection et coût du census

Réponse à la troisième tranche : la règle de bornes proposée est correcte.
Sur un nœud B, rejet si L_ax≥h OU c_A+min(c_B)≥h ; conservation de tout
le nœud si U_ax<h ET c_A+max(c_B)<h ; sinon subdivision. Les extrema
peuvent provenir de sites B différents : cela affaiblit la décision sans
l'invalider. C'est une intersection de deux résidus, sans addition de
leurs témoins. Le même propriétaire et la copie de la restriction sont
les bons contrats. Cette contrelecture ne vaut pas encore qualification
de l'implémentation concurrente ni de ses coûts.

Pour comparer votre parcours intégré à une autre construction, notre
[prototype de rangs partagés](morsehgp3D_v8_complementaire/P0_INTERSECTION_RESIDUS.md)
intersecte exactement les deux résidus déjà construits en O(h|B|+D),
sans développer les paires, avec au plus D fragments de sortie. Les
sous-listes gardent l'ordre axial mais contiennent les IDs originaux.
48 plans / 172 800 paires confrontées ; 18 intersections réduisent
strictement les deux résidus, deux mutants C++ réfutés. Cette alternative
paie les deux plans, les rangs et les IDs ; elle ne présume pas être
plus rapide que votre arbre intégré. Les essais épinglent 8e406f9b.

Le [consommateur d'audit q2](morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md)
paie maintenant le travail aval sur les candidates du filtre publié.
Index indépendant sur tous les sites, compte depuis zéro, arrêt à Kmax.
À n32k/Kmax10 sur la nappe 125×128 : 6 483 670 candidates effectivement
consommées, 455 418 paires sous le seuil, 295 540 004 visites de nœuds.
Ce coût aval justifie de comparer le partage des recherches par blocs,
proposé dans la section 9 de l'autre auditeur, au parcours par paire.
La construction de l'index paie seulement 990 464 visites de points ;
aucun scan des n sites par requête ni tableau A×B n'est dissimulé.
Six grandes exécutions terminées ; 16 petites fixtures O2/UBSan jugées
par 3 076 448 tests ponctuels, trois mutations C++ réfutées. Les temps
bruts sont explicitement non qualifiés ; aucune tour ni gain global P0.

Les [fixtures de raccord](morsehgp3D_v8_complementaire/P0_CENSUS_Q2_ET_COQUILLE.md)
complètent la proposition de census : deux diamètres peuvent représenter
la même boule demi-entière avec quatre sites de coquille. Compter les
intérieurs stricts ne collecte pas cette coquille et ne canonise pas
les boules. Les 192 requêtes du juge passent normal/−O, avec quatre
contre-modèles réfutés. La publication 9633d8ef de l'autre auditeur est
lue et concorde avec ces obligations ; ses propres preuves restent
sous son autorité.

La [relecture des 594 mesures](morsehgp3D_v8_complementaire/SHARED_AXIS_MEASURES_CHECKS.json)
de 8e406f9b est favorable : 504 configurations, cinq matrices, sources,
provenance, comptes et tableaux du README concordants, lecteurs normal/−O.
Aucun benchmark relancé pour ce contrôle. Les sources publiées de partage
et d'axes correspondent aux snapshots déjà audités ; leurs corrections
restent closes. Le [delta d'allocation](morsehgp3D_v8_complementaire/ALLOCATION_DELTA_CHECKS.json)
cpp 522009ad… passe aussi les gates batch et affectation sous GCC strict
-O1/UBSan : 20 pannes mémoire, cible conservée. La tentative -O2/UBSan
refusée par GCC est conservée dans le reçu ; elle ne dépend pas du delta
et n'est pas annoncée comme un succès. Aucun nouveau axe en cours n'est
qualifié par ce contrôle ciblé.

L'état actif de notre dossier est recentré sur ces suites ; les preuves
closes restent dans leurs reçus. GCP non utilisé.

La contrelecture du cpp axial 2922f425… et du hpp 99beeebe… confirme
les branches ponctuelles : égalités exclues, bornes identiques aux feuilles,
crédits B lus par ID original et restriction copiée après validation du
propriétaire et de q2. Les sources ont ensuite évolué ; cet avis de lecture
ne qualifie pas automatiquement la dernière révision.

Réservation d'index AUDITEUR_COMPLEMENTAIRE après 9633d8ef, index constaté
vide : notre dossier et cette section uniquement. Les modifications du
constructeur, de l'autre auditeur et des lignées v6/v7 restent exclues.
La fenêtre expire au commit/push de cette passe.

Export neuf des entrées d'index SHA256 98741f28… : documentation PASS532,
registre PASS20, validation explicite de nos quatorze Markdown et du
journal PASS15 ; entrées stables avant/après export. Ces contrôles
s'ajoutent aux gates exécutées dans leurs snapshots décrits plus haut,
sans nouveau build constructeur ni reprise de ses fichiers en cours.

## 13 septembre 2026 — CONSTRUCTEUR : comptage q2 partagé, quatrième tranche

Reprise après `f5430f57`, index vide. Vos deux notes de census et les
fixtures de coquille ont été lues. Implémentation en cours : index immutable
sur tous les sites du propriétaire, comparaison paire par paire / groupes
de B à ancre A fixe, consommation des plages du plan axial sans catalogue
A×B. Le compte part de zéro, sature à Kmax, et une subdivision de requête
hérite seulement de la frontière de témoins non consommée. Les liens de
continuation et leur mémoire maximale seront comptés ; pas de copie de
longues listes ni de préchargement du cœur.

Les paires sous le seuil feront une seconde collecte indexée des intérieurs
stricts et de toute la coquille. Ce passage, les IDs et le callback de sortie
sont inclus dans la mesure. Chaque incidence conserve son support et la
clé entière `(a+b, |a-b|²)` ; la déduplication globale des boules et la tour
restent distinctes. Les deux diamètres d'une même boule ne seront donc pas
confondus avec deux boules déjà canonisées.

Question de contrelecture : nous partageons un compteur uniforme et une
frontière persistante, classons les boîtes Z par vos extrema L/M4, puis
raffinons B ou Z selon leur étendue géométrique. À B singleton, même test
de distance à centre doublé que la référence par paire. Voyez-vous une
obligation manquante pour ce raccord limité aux supports q2 ? Vos fichiers
en cours, dont `p0_q2_census_bounds_probe.py`, restent hors de nos écritures
et de notre index. Pas encore de fenêtre de benchmark lourd. GCP non utilisé.

Vos nouvelles notes `P0_CENSUS_PARTAGE_ET_SEUILS.md` et
`P0_PROPRIETE_ET_MINIMUM_Q2.md` viennent d'être lues. L'API en chantier
n'a pas de cache entre seuils ni de reprise d'un compte saturé : chaque
appel traite le Kmax fixe du propriétaire et repart de zéro. Le futur
partage par clé/nuage devra garder les surplus de blocs consommés si des
seuils supérieurs arrivent ensuite ; cette obligation est retenue.
La propriété du minimum transversal explique aussi pourquoi le raccourci
« restriction q2 vide à besoin positif » n'a pas de cas actif actuellement.
Nous ne l'implémentons pas comme une économie nouvelle. Ces notes ne sont
pas encore citées comme des résultats publiés ni comme une qualification
de notre nouveau moteur.

La section 9.1 et le nouveau dialogue de l'auditeur viennent d'être lus
intégralement. Nous intégrons le curseur DFS avec échappement avant gel :
notre parcours Z est déjà fixe, donc il remplace directement l'arena de
liens initialement codée. Le compte et le curseur sont transmis aux enfants
B ; la collecte reste différée. La version à listes ne fera pas l'objet
d'un prototype concurrent conservé ni d'une prétendue mesure publiée.
L'index compte la préparation des échappements et les gates bloquent les
sauts de frère ou retours vers un préfixe consommé. Merci pour cette
simplification directement exploitable vers de petits états parallèles.

Le port à curseur est gelé. Votre lecture `256957a5` est vue : le cpp
reste `3c513cc4…`, et le hpp ne change ensuite que pour expliciter les
emprunts pendant tout l'appel, la propagation des exceptions et le flux
partiel non annulé. Le gate indépendant passe 277 cas/557 exécutions,
avec IDs et coquilles exacts ; les deux suites finales Release et Clang
ASan/UBSan tournent actuellement. Fenêtre de mesures mono ensuite :
merci de ne pas lancer de contre-benchmark lourd pendant cette capture.
Nous annoncerons sa fermeture avant toute réservation d'index. Pas de GCP.

Les 31 CTests sont PASS en Release et Clang ASan/UBSan, y compris les
lecteurs normal/−O. La campagne mono est maintenant ouverte : sources et
binaire gelés, exécutions séquentielles n8k/16k/32k, K5/10, s8/10/12,
avec coût complet du composant q2 et deux ordres. Pas de compilation ni
de gate concurrente lancée par le constructeur pendant les chronométrages.

Fenêtre de mesures close : quatre campagnes, 204 lignes appariées / 164
configurations, lecteurs normal/−O PASS. n8k/16k/32k et s8/10/12 sont
couverts ; 24 lignes supplémentaires portent sur le composant à50k.
Le partage perd après intersection malgré moins de visites, mais gagne
sur les grilles avec les deux filtres moins sélectifs. Les temps complets
à32k/K10 sont consignés avec trois répétitions par ordre. À50k/K10 après
intersection, individuel : environ 176–186 ms sur grilles, 4,05–4,09 s
sur nappes. Aucune tour ni gain générique annoncé. Les sources restent
gelées ; aucun autre benchmark lourd n'est prévu avant cette publication.

Réservation d'index CONSTRUCTEUR après `256957a5`, sous réserve du contrôle
de vacuité : nos sources, tests, documents et reçus v8, AGENTS et notre
section du journal uniquement. Les modifications v6/v7 et les audits
indépendants restent exclus. Export propre et contrôles avant commit/push
sur main ; fenêtre close à cette publication. GCP non utilisé.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : stabilité publiée et raccord du census

Réponse à votre quatrième tranche : le raccord décrit est cohérent.
Pour tout groupe de requêtes non saturé, le compteur doit être exact
sur les blocs Z entièrement consommés et identique pour chaque paire.
La frontière restante partitionne les IDs non consommés. Raffiner B
transmet ce compteur et cette même frontière aux deux enfants ; raffiner
Z remplace un seul bloc par ses enfants disjoints. Ne redémarrer aucun
enfant B à la racine Z, ni oublier une continuation après un crédit.
Le choix par étendue est une politique de travail, pas une hypothèse
d'exactitude ; une feuille ne doit jamais rester indécidable.

Votre saturation à Kmax peut terminer le groupe pour ce seuil. Elle
ne promet pas un état reprenable à seuil supérieur : si un nœud intérieur
contient deux IDs et que le compte est tronqué à 1, retirer tout le nœud
puis reprendre perdrait le surplus. Notre [fixture de reprise](morsehgp3D_v8_complementaire/P0_CENSUS_PARTAGE_ET_SEUILS.md)
précise deux contrats possibles : garder le vrai cardinal consommé ou
recalculer au nouveau seuil. Cela n'impose pas de cache multi-seuil à
votre première version. La seconde collecte annoncée doit comparer
son nombre d'intérieurs au compte exact sous le seuil et conserver les
égalités pour la coquille ; ses coûts font bien partie de votre mesure.

La clé entière de boule et les supports séparés correspondent au
périmètre déclaré. Pour une éventuelle canonisation ultérieure, le cache
dépend aussi du nuage immuable et de l'espace d'IDs. Une même boule a
au plus floor(|coquille|/2) diamètres parmi les sites : le partenaire de
x est uniquement S−x. Un lookup d'antipodes évite le produit de toutes
les paires de coquille, sans supprimer le coût de découverte des clés.
Le [juge de réutilisation](morsehgp3D_v8_complementaire/Q2_SHARED_CENSUS_CHECKS.json)
passe 42 demandes normal/−O et réfute cinq contre-modèles ; il ne qualifie
pas votre futur index ni la tour FULL.

Les [648 nouvelles mesures](morsehgp3D_v8_complementaire/ADDITIVE_MEASURES_CHECKS.json)
de f5430f57 sont contre-vérifiées : 576 configurations, références,
provenance, trois matrices, pins et comptes concordants. Votre rapport
conserve correctement le ralentissement de la sélection sur nappe et
le coût Pool inclus dans l'intersection. Lecteurs normal/−O favorables,
aucune anomalie significative ; aucun benchmark relancé pour cette lecture.
GCP non utilisé.

Sur les sources publiées, notre [porte de propriété axiale](morsehgp3D_v8_complementaire/P0_PROPRIETE_ET_MINIMUM_Q2.md)
est favorable : 50 pannes injectées dans les nouvelles constructions,
quatre déplacements sans allocation et 96 plans dont keeps/fragments
concordent sur 345 600 paires. Aucun défaut trouvé. Une preuve réduit
également la liste des optimisations utiles : choisir la paire de distance
minimale entre A et B interdit tout témoin diamétral strict interne.
Ses deux crédits locaux valent donc zéro. Pour h>0, elle reste dans les
plans q2 actuels ; restriction vide équivaut à h=0, déjà court-circuité.
L'économie supplémentaire proposée par l'autre auditeur sur le seul
résidu vide est donc redondante aujourd'hui. Sa remontée des extrema B
reste une proposition distincte. Des témoins extérieurs peuvent bien
éliminer cette paire au census : la preuve ne garantit aucune sortie FULL.

Notre [parcours de frontière partagé](morsehgp3D_v8_complementaire/P0_FRONTIERE_PARTAGEE_Q2.md)
a maintenant été exécuté sur les résidus f5430f57. À n32k sur la nappe
additive : 189 649 460 visites par paire, 147 292 056 classifications
partagées, mais 13 830 224 visites de couverture B et 145 720 627 écritures
de cellules de continuation. Le pic de l'arène est de 201 cellules ;
ce ne sont pas autant d'allocations du tas. Le nouveau curseur en préordre
proposé dans la section 9.1 de l'autre auditeur est pertinent pour éviter
ces écritures sous ordre Z fixé. Notre prototype conserve les listes
comme référence ; il ne mesure pas la variante à curseur.

Sur grille/intersection Pool, à n32k : 6 311 590 visites par paire contre
5 717 707 classifications partagées. Les six grandes lignes concordent
en histogrammes et digests ; 57 petites fixtures passent le census
indépendant (8 278 451 tests), trois mutations de transmission réfutées,
normal/−O concordants. Aucun temps, collecte de coquille ou callback
n'est mesuré dans ce prototype : vos sorties restent à payer dans le
consommateur en chantier. Les classifications de boîtes coûtent davantage
qu'un test ponctuel ; leur baisse seule ne prouve pas une accélération.

Lecture API favorable sur hpp 7643b97b… / document b2c0d407… : vues
empruntées uniquement pendant le callback synchrone, clé correctement
dimensionnée, coquille comprenant les supports, pas de compte saturé
présenté comme exact. Le seuil est unique par propriétaire : le piège
de reprise ci-dessus ne s'applique donc pas à cette première API.
Une précision contractuelle utile reste le flux en cas d'exception du
callback : documenter la propagation et les émissions déjà livrées,
et ne pas les présenter comme une collecte achevée. Aucun défaut concret
de cette future implémentation n'est allégué par cette lecture d'API.

La publication f47559b1 de l'autre auditeur est vue : sa continuation
par curseur et ses tests de transmission complètent notre comparaison
sur les résidus C++ publiés. Aucun de ses fichiers n'est repris ici.

Réservation d'index AUDITEUR_COMPLEMENTAIRE après f47559b1, index constaté
vide : notre dossier et cette section uniquement. Les changements du
constructeur et des lignées v6/v7 restent exclus. La fenêtre expire au
commit/push de cette passe. GCP non utilisé.

Export neuf des entrées d'index SHA256 b64d4221… : documents PASS534,
registre PASS20, validation explicite de nos dix-sept Markdown et du
journal PASS18 ; entrées identiques avant/après export. Les modèles et
gates sont exécutés dans leurs snapshots propres décrits ci-dessus ;
aucun nouveau build du consommateur constructeur n'est annoncé.

## 13 septembre 2026 — AUDITEUR_COMPLEMENTAIRE : census réel, fermetures et coquilles

Avis borné favorable sur le census cpp 3c513cc4… / hpp 2116ac4b… ;
**les deux P2 sont clos sur 8141982a…, puis confirmés sur le lecteur
publié 892bd3ae…**.
Notre [état courant](morsehgp3D_v8_complementaire/ETAT_COURANT.md)
retire les demandes résolues et conserve les limites de P0 et FULL.
Les messages provisoires de cette section sont condensés ci-dessous ;
les reçus gardent les témoins et toutes les étapes de contrelecture.

La [porte géométrique indépendante du C++](morsehgp3D_v8_complementaire/P0_CENSUS_CPP_Q2.md)
passe 466 fixtures, 932 exécutions, 8 099 paires et 184 448 tests H directs :
supports, clés et IDs exactement conformes dans les deux modes. Les bornes
sont confrontées à 16 625 évaluations rationnelles ; 257 partages après
crédit sont exercés. Sept vraies mutations du C++ copié sont compilées et
réfutées. GCC strict O2 et O1/UBSan concordent ; aucun oracle constructeur
utilisé pour les réponses attendues. Le reçu conserve ces octets autonomes.
Les contrôles Python normal/−O du runner portent sur CLI/SHA ; ils ne
représentent pas deux campagnes entières de compilations.

La [porte d'exceptions](morsehgp3D_v8_complementaire/P0_CENSUS_Q2_ET_EXCEPTIONS.md)
confirme trente pannes d'allocation, dont huit après émissions, et 24 reprises
complètes avec les mêmes payloads et compteurs. Une exception au troisième
callback conserve trois supports chez le client, se propage, puis la reprise
repart correctement de zéro après abandon du flux partiel. Deux mutants
(exception avalée, retour vide avant validation) sont réfutés, normal/−O.
La clarification des emprunts et exceptions est ainsi contre-vérifiée ;
l'invalidation des arguments empruntés pendant l'appel reste hors contrat.

Notre [P2 de réception](morsehgp3D_v8_complementaire/P0_CENSUS_RECEPTION_ET_FERMETURE.md)
provenait de deux captures authentiques : n8 réussie, puis échec initial
avec binaire absent dans un dossier frère, COMPLETION failed sans MANIFEST.
Le lecteur 311fce7f… oubliait cet échec et rendait passed/campaigns=1.
Le correctif 8141982a… accepte la même capture n8 et rejette son même parent
code 1 pour « incomplete q2 campaign receipt », normal/−O. Le refus vient
du triplet incomplet, pas d'un hash périmé. Notre P2 est clos.

L'autre auditeur conserve le suivi du P2 distinct de construction B dans
son [dialogue](../morsehgp3D_v8/audits/DIALOGUE_COURANT.md), également clos.
Notre corroboration accepte une vraie ligne n8, puis refuse la suppression
de chacun des deux compteurs et des deux ensemble, normal/−O.
Les 408 bras authentiques avaient déjà les bons comptes ; aucune anomalie
de mesure réelle ni défaut géométrique n'est allégué par ces deux P2.

Le lecteur étant l'une des 18 sources épinglées, ses correctifs conservent
les pins historiques : l'archive est identique au runner de capture
311fce7f…, le nouveau contrôle annonce séparément son SHA 8141982a….
Les 204 mesures restent admises normal/−O. Aucun chronométrage du moteur
inchangé n'a dû être refait pour ces corrections de validation.
Le [reçu à addenda](morsehgp3D_v8_complementaire/Q2_CENSUS_RECEIPT_CHECKS.json)
garde le témoin initial, les étapes 180 puis 204 mesures et la fermeture.

La contrelecture des quatre campagnes retrouve 204 mesures, 164 configurations
et 408 bras : matrices, commandes, sorties brutes, provenance, pins,
supports, candidates, compteurs et tableaux n32k/50k concordent. Les plages
de médianes sont séparées par ordre. Moins de visites ne rend pas le partage
plus rapide après intersection sur les grilles et nappes testées ; il gagne
sur les grilles avec les préfiltres moins sélectifs. Pool seul reste absent
de la comparaison. Vos conclusions bornent correctement ces constats.
Les XML conservés annoncent chacun 31 tests sans échec en Release et
ASan/UBSan ; ils sont lus, pas rejoués. Les essais 50k ne qualifient aucune tour.

Le périmètre des temps est correctement payé à la lecture : le callback
parcourt tous les IDs, la collecte et les buffers sont inclus, les coûts
communs et leur destruction sont ajoutés par bras. L'inspection comparative
est séparée. Ce flux de supports ne promet pas encore le catalogue de boules.

Pour ce prochain catalogue, la [fixture de coquille répétée](morsehgp3D_v8_complementaire/P0_COQUILLES_REPETEES_Q2.md)
traverse réellement s8/10/12 : 398 sites sur une sphère, cinq ancres près
d'un pôle et cinq antipodes. Aux seuils 1/5/10, les 25 candidates donnent
exactement cinq supports, une seule clé et 398 sites de coquille. Le flux
actuel émet 1 990 IDs de coquille par appel et collecte par support,
conformément à son contrat. Le juge normalise une boule + cinq incidences,
puis reconstruit exactement le flux ; supprimer aussi les incidences est
réfuté. Normaliser après callback ne rembourse pas les collectes déjà payées.
Ce plateau non régulier prépare une contre-épreuve du catalogue annoncé,
sans exiger de remplacer le flux avant son gel ni déduire un résultat FULL.

Le contrôle final confirme sur 892bd3ae… les positifs n8/204 mesures et
les refus ciblés du frère incomplet, des comptes B supprimés et du nouveau
mutant de comptage vacant, normal/−O. Le reçu de qualification final
0992d953… passe la lecture : 46 pins, caches et XML cohérents, deux suites
de 31 tests sans échec ; l'arrêt intermédiaire code 130 après 16 tests
est conservé séparément. Les douze fichiers de capture historiques restent
inchangés. Les versions précédentes de notre reçu ne sont pas réécrites.

Réservation d'index AUDITEUR_COMPLEMENTAIRE après la publication
constructeur f4815cd4, index constaté vide : notre dossier et cette seule
section du journal. Les deux sections constructeur sont déjà dans HEAD ;
les fichiers de l'autre auditeur et des lignées v6/v7 restent exclus.
Les sources census publiées sont identiques à nos snapshots exécutés.
Cette fenêtre expire à notre commit/push.

Export neuf des entrées d'index 8539a881… : documents PASS536,
registre PASS20, nos 21 Markdown et le journal PASS22 ; entrées identiques
avant/après export et contrôles. Ce bilan est ensuite ajouté au journal
et validé explicitement. Les seize fichiers préparés appartiennent à notre
audit ; aucun fichier moteur, constructeur ou de l'autre auditeur n'est
repris dans ce commit. GCP non utilisé.

## 13 septembre 2026 — CONSTRUCTEUR : lecteur renforcé, publication coordonnée

Les deux P2 de lecture sont corrigés dans le runner `8141982a…` : comptes
B Shared imposés à `2*|B|-1` nœuds et `|B|` visites, et découverte de tout
dossier portant MANIFEST, COMPLETION ou MEASURES, sans ignorer une fermeture
initiale échouée. Les portes normal/−O ajoutent les mutants ciblés et le
vrai cas succès + frère sans binaire. Les 204 captures originales passent
le nouveau lecteur ; aucun brut, manifeste ou pin historique n'est réécrit.
Le seul ancien runner accepté est `311fce7f…`, authentifié par son snapshot
`CAPTURE_RUNNER.json` ; chaque autre source reste comparée au disque courant.
Le rapport distingue explicitement le hash de capture de celui du lecteur.

Moteur et probe inchangés ; pas de nouvelle campagne de performances.
Les suites complètes Release et ASan/UBSan sont relancées après ces seuls
correctifs de protocole. L'audit C++ indépendant et les contre-épreuves
d'exception/coquille ont été lus ; la répétition des payloads d'une même
boule reste une cible déclarée du futur catalogue, pas une omission du
flux de supports actuellement testé.

**Index partagé libéré pour la publication d'audit complémentaire déjà
préparée**, constaté vide après `2e75b2f3`. Merci de publier seulement vos
fichiers et vos sections du journal, en préservant les sections constructeur
non publiées. Je reprendrai l'index après votre commit, pour l'export neuf
du moteur, ses juges et ses reçus. Cette attente n'ouvre aucune nouvelle
campagne lourde. GCP non utilisé.

Complément de gel constructeur : le lecteur final est `892bd3ae…`.
La revue interne a ajouté un plancher de non-vacuité : un résidu non vide
ne peut déclarer zéro racine/tâche/visite de comptage tout en gardant index
et payload. Ce contre-modèle est maintenant rejeté par capture et lecture,
normal/−O ; les gates passent avec 15 mutants runner et 17 lecteurs.
La suite ASan intermédiaire a été arrêtée volontairement code 130 après
16 tests réussis pour geler ce dernier correctif, puis les deux suites
complètes ont été relancées. Il n'y a ni défaut géométrique allégué ni
nouveau chronométrage de composant. Les preuves historiques de vos deux
P2 sur `8141982a…` restent valides et distinctes de ce dernier ajout.

L'export neuf a été préparé sur **index privé**, sans réserver l'index
partagé pendant votre publication : entrées `463ff167…`, compilation GCC
stricte puis 31 CTests PASS (79,55 s), lecteurs des 204 mesures normal/−O
PASS, documentation PASS536 et registre PASS20. Les sources exécutables
sont gelées. Seuls le reçu de qualification final et nos sections du
journal seront ajoutés/rafraîchis après la fin de la suite ASan ; ces
pièces documentaires ne changent pas le moteur ni ses juges.

Suites finales closes : Release 31/31 (76,32 s), ASan/UBSan 31/31
(288,12 s). L'export retrouve aussi les 31 sorties de gates et le SHA
du binaire de capture, `243387ac…`. Le reçu de qualification est rafraîchi
avec le lecteur final et conserve la trace de l'arrêt intermédiaire.

**Réservation constructeur reprise pour la publication immédiate**, index
partagé toujours vide et HEAD encore `2e75b2f3` au contrôle : seuls nos
fichiers et nos deux sections du journal seront préparés. Votre nouvelle
section et vos fichiers restent en worktree, non inclus ; vous conservez
le créneau suivant notre commit/push. Aucun benchmark ne tourne. Cette
fenêtre expire à la publication constructeur. GCP non utilisé.

## 13 septembre 2026 — CONSTRUCTEUR : bornes préparées et cible massive

Reprise après `f4815cd4`, cadre inchangé `exploration_v8_hors_registre /
cpu_reference / quantized_u16_input_only / implementation_v8_p0 /
not_claimed`. Votre publication en préparation garde son index ; aucune
réservation constructeur pendant le code. L'utilisateur réaffirme que les
contrats 50k sont sur G4 et demande le contrôle continu de la complexité.

Tranche bornée : préparer C=a+e et D=(e−a)² une fois par tâche Shared,
puis réemployer les deux carrés aux bornes de 2Z pour min et max. Chaque
constante tient en u32, avec promotion i64 avant le calcul des bornes :
objectif de 48 octets copiables sans pointeur, pas de table A×B ni liste
de continuation. Le parcours, ses décisions, les compteurs et la collecte
doivent rester identiques au moteur publié, testé comme différentiel.

Questions de contrelecture : voyez-vous un défaut dans cette factorisation
ou une contrainte supplémentaire pour transporter ces constantes avec le
curseur Z ? Votre nouveau raccord Pool seul (§9.2) est lu comme étape
suivante ; le partage CloudOwner/RectangleView est également nécessaire
avant d'appliquer le census à une vraie WSPD sans recopier n sites par
rectangle. Ni cette économie constante ni la parallélisation ne ferment
une borne globale sur les visites ou la sortie. Pas de benchmark lourd
encore ouvert, pas de GCP à ce stade.

Reprise du 14 septembre : les suites sont closes, 32+2 CTests Release
et 32+2 Clang ASan/UBSan PASS. Sources préparées gelées (header 7bb46b4b,
census b1ca5edd). **Fenêtre de mesures mono ouverte** : comparaison avec
f4815cd4 sur 8k/16k/32k, familles grid/sheet_full/skew, Kmax10, s8/10/12,
deux ordres des bras ; répétitions complémentaires à 32k sur grid/sheet_full.
Pas de compilation ni campagne lourde concurrente côté constructeur.
Le benchmark paie préparation, index, préfiltre, census et collecte de ce
composant, sans prétendre mesurer la tour. Aucun index partagé réservé.
GCP non utilisé.

**Fenêtre de mesures close**, 124 lignes = 62 par révision, travail et
digests identiques, lecteur comparatif PASS. À 32k/K10/s8, trois mesures
par ordre : Shared baisse de 4,0–5,7 % sur grid et 4,2–9,5 % sur sheet_full,
temps total du composant. Pairwise inchangé varie aussi : interprétation
exploratoire, pas d'intervalle de confiance ni de domination universelle.
Le nombre de visites reste identique ; la classe de complexité ne change
pas. Prochaine étape : export neuf et qualification de publication ; les
sources restent gelées. Vos nouveaux audits restent hors de notre index.

L'export privé reconstruit le binaire de capture à l'identique (b76591c0),
sans inclure vos fichiers en préparation. La dernière suite complète est
en cours ; les suites Release/ASan 32+2 restent closes. **Réservation
constructeur de l'index pour la publication de cette tranche**, index
partagé vide au contrôle. Préparation limitée à nos sources/juges/docs,
nos nouveaux reçus et cette section du journal ; aucun fichier d'audit
indépendant ni section AUDITEUR ne sera inclus. Réservation levée après
commit/push et contrôle. Aucun benchmark lourd ni GCP en cours.

Qualification de l'export close : 34/34 CTests PASS (87,88 s), toutes les
sorties de gates identiques aux suites Release/ASan, 50 pins de sources
et juges identiques entre worktree et export ; lecteurs 62+62 normal/−O
PASS, documentation PASS538, registre PASS20. Seuls le reçu de qualification
et les textes de clôture sont rafraîchis après les tests. Publication
immédiate de notre liste explicite et de cette section seulement ; le
créneau suivant revient aux auditeurs, sans inclusion de leurs brouillons.

P2 du dialogue reçu avant commit : le profil comparé omettait lien/IPO.
Correction bornée du lecteur : familles de flags de lien et options IPO,
y compris suffixes par configuration, maintenant comparées ; quatre
mutants dédiés ajoutés. Aucune source moteur/probe/capture ni mesure ne
change. Les valeurs réelles sont compatibles. Requalification des deux
gates Python, normal/−O sur les deux builds, puis export complet actualisé ;
les premières validations et leurs pins restent conservés séparément.
Notre réservation est prolongée jusqu'à cette fermeture, sans nouveau
benchmark ni besoin GCP. Merci pour ce contrepoint précis.

Correction finale close : lecteur 1d0bd7c0, gate f88e406c, 20 mutants
normal/−O PASS sur les deux builds ; export actualisé 34/34 PASS et mêmes
sorties que les suites composées 32+2. Les 124 mesures passent encore les
deux lecteurs. Les sources moteur restent inchangées ; la première
qualification est archivée explicitement, sans réécrire les captures.

## 14 septembre 2026 — CONSTRUCTEUR : nuage et index partagés

Reprise après 3c29ea1e et la contrelecture 1bf806f0, cadre inchangé
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
implementation_v8_p0 / not_claimed`. Le nouvel auditeur est bien pris en
compte. Lecture de son front pur dans `wspd_regime_20260914` : le nombre
de rectangles et la somme des tailles de facteurs justifient une API sans
copie/validation/index global par rectangle. Ces mesures v4 exploratoires
ne deviennent pas des qualifications v8 ; le ledger de masse ne remplace
pas une preuve de couverture sans doublons.

Tranche proposée : PreparedCloud immuable avec copie privée/unicité une
fois ; index de boîtes de plages originales en O(n) puis requêtes O(log n),
pour ne pas simplement déplacer le scan global vers les boîtes. RectangleSpec
référence les facteurs sans points ; séparation/cœur/seuil restent propres
au rectangle. L'index census Z appartient au nuage et prend son seuil dans
le plan. Restrictions axiales toujours liées au même rectangle exact.
Les IDs/permutations B restent ceux du plan ; aucune continuation publique
ou ordonnanceur n'est ajouté dans cette tranche.

Les fixtures des trois identités seront portées. Le raccord garde pour
l'instant les plages de l'ordre original : une vraie WSPD doit fournir
sa permutation et ses nœuds certifiés, pas recopier les coordonnées pour
rendre ses facteurs contigus. Questions au nouvel auditeur : au vu du coût
du front pur, examiner le rejet certifié par K témoins sur des produits
ancêtres avant d'achever leur séparation WSPD ; cela pourrait éviter de
matérialiser d'abord des millions de rectangles ensuite rejetés. La sûreté
des certificats et la couverture résiduelle doivent rester explicites.
Aucune réservation d'index, aucune mesure lourde ni GCP encore lancé.

Point constructeur après lecture de 77bcd0b8 et 7a56d852 : raccord implémenté,
37 tests Release/Clang ASan/UBSan PASS (35 puis 2 tests de reçus). Les
restrictions gardent l'identité de rectangle ; le seuil vient du plan ;
les boîtes indexées restent un pont de compatibilité, hors du futur chemin
WSPD par nœuds certifiés. Le contre-exemple d'ordre B de §9.4 est inscrit
dans le contrat. Le front futur doit fixer sa convention de séparation ;
le s des fixtures ne sera pas réinterprété en s v4. Les invariants de
rétention par boule et de plus longue arête q3/q4 sont ajoutés au plan.

La campagne 8k/16k/32k en cours partage le nuage/Z mais confirme un terme
Ω(R|B|) encore présent dans Pool/Axis, exactement visible dans les copies
de restriction |A|+R|B|. Elle comprend R fixe puis R croissant avec n :
aucune revendication sous-quadratique globale ne sera tirée du seul premier
régime. Merci de prioriser le front fusionné, le chemin petits facteurs et
le partage des préparations des facteurs, plutôt qu'une nouvelle variante
micro-locale de q2. GCP non utilisé ; index toujours non réservé.

Clôture constructeur : 66 essais, quatre campagnes closes et lecteurs
normal/−O PASS. Le partage divise les préparations globales par R à sorties
et travail local identiques. Le témoin de croissance R32/64/128 donne
132000/520000/2064000 copies locales sur les grilles, donc ×3,94 puis ×3,97.
La nappe32k/R32 garde 35,6 millions de candidates et coûte environ 16 s ;
le découpage affaiblit les crédits et n'est pas une accélération globale.
Les chronomètres bruités, y compris un doublement à ×5,62, sont conservés.
Les deux builds sont épinglés. Les 37 tests par build et l'échec initial
LSan/ptrace suivi du rejeu autorisé hors sandbox sont dans QUALIFICATION.

Réservation d'index constructeur pour cette publication, index constaté
vide : AGENTS.md, ses seuls fichiers code/tests/bench/docs/entrées v8,
receipts/cloud_reuse_20260914 et la présente section CONSTRUCTEUR uniquement.
Les modifications v6/v7, les fichiers des auditeurs et la section précédente
AUDITEUR_COMPLEMENTAIRE restent exclus. Fenêtre close après commit/push main.

## 14 septembre 2026 — CONSTRUCTEUR : premier front réel v8

Reprise après 85015a8c, cadre `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`. Périmètre
en cours : front sur les nœuds du même index spatial global, ordre explicite,
boîtes certifiées déjà disponibles. Aucun PreparedRectangle, Pool ou Axis
alloué par produit ; pas de copie de facteur ni de catalogue de rectangles.
Parcours Pure et MidpointSamples : descente unique vers le milieu, puis au
plus K rangs proposés et certificats positifs Hmin/Xi, masques indépendants
q2/q3/q4 hérités lors des subdivisions. Les comptes partiels ne sont pas
hérités, aucun crédit n'est additionné deux fois. Les échecs de proposition
conservent le produit ; aucune limite de visite ou de sortie.

Convention explicitement conservée pour ce premier pilote :
`box_gap_diameter_v1`, gap≥s·max(diag), pas le s v4. Comparer s8/10/12
sur le vrai front et rapporter rectangles, sommes de facteurs, masses par
classe, coût du proposeur et résidus par voie. L'arbre à bisection au milieu
vient de la v8 ; ce n'est pas le trie Morton du reçu indépendant et aucune
borne O(s³n) ne lui est attribuée. La couverture unique sera testée par
énumération bornée, au-delà du seul ledger de masse.

Contrepoint théorique à conserver : sur A_i=(1000,i,0), B_j=(60000,j,0),
si 3(m−1)²<59000², aucun site n'est W3/W4 pour une paire transversale.
Même tous les témoins ponctuels laisseraient donc m² paires candidates
q3/q4 ; ne pas prétendre résoudre ce cas par un autre K ou s. Les groupes
collectifs déjà proposés par l'auditeur et l'aval canonique restent requis.
Questions ouvertes : efficacité d'un bloc Z certifié pendant la descente
(max avec les échantillons, pas somme emboîtée), puis transmission compacte
et sûre des certificats partiels. Ne pas retarder le raccord au census.
Builds neufs `v8_front_20260914` et `v8_front_sanitize_20260914`, aucune
réservation d'index à ce stade. GCP non utilisé.

Lecture des publications indépendantes 7009ec8b et 795a29dd effectuée.
Le partage de jobs d'un parent (§9.5) est distingué de la reconstruction
des plans ; il est intégré au plan de raccord. Les frontières strictes
W3/W4 figurent dans le nouveau gate du front (727 parcours et sept
contre-modèles), mais cela ne ferme pas automatiquement le mutant de
`classify_witness_block` signalé par B : le front évalue H/Xi directement.
Les autres lacunes de tests et l'intermittence SIGINT sont conservées
comme questions ouvertes, sans relance jusqu'au vert d'un test en échec.
Le test préalable de lentille proposé par B est pertinent mais non
implémenté dans cette capture ; comparer son coût au raccord census.
Les comptes partiels ne sont pas transmis, seulement les rejets certifiés
par voie. La réserve de A sur I1 dans la note B est respectée : un rejet
de lane ne signifie jamais inertie globale d'une boule.

Clôture constructeur : 40 CTests Release et Clang ASan/UBSan PASS,
72 mesures closes (54 uniforme/terrain/huit amas, 18 rangées), n8k/16k/32k,
Kmax10, s8/10/12. Sources/binaires/caches/XML épinglés ; lecteurs normal/−O
identiques. Les nouveaux builds sont maintenant réservés aux preuves.
Résultat négatif : uniforme32k/s8, rectangles 56,8→20,9 M mais total
4,87→37,4 s ; 954 M pas d'index. Sur huit amas, q2 résiduel fait
29,7→116,8→460,1 M, presque quadratique. Sur rangées, front32k/s8
environ 204 ms mais résidu q3/q4 ≥256 M. Le temps du front compact ne
qualifie pas l'aval. Aucun claim de tour ni de sous-quadraticité globale.

Demande aux auditeurs : priorité au raccord direct du census sur les
nœuds B du front, sans plan reconstruit, puis à la réduction collective
des produits inter-amas et q3/q4. Le plan documente les fixtures crédit
hérité {0,5,10,11} et coquille du cube. Pour le test préalable proposé
par B, distinguer lentille continue non vide et présence d'un site utile :
deux feuilles sœurs aux abscisses 0 et 100 ont une lentille ouverte non
vide, malgré l'absence de site extérieur dans ce nuage. La phrase sur
les frères radix ne doit pas devenir un rejet général sans hypothèse.

Réservation d'index constructeur, index constaté vide : AGENTS.md,
ses seuls fichiers code/tests/bench/docs/entrées v8, le reçu
`receipts/wspd_front_20260914` et cette seule section CONSTRUCTEUR.
Les modifications v6/v7, le budget de B, les fichiers complémentaires
et leur section précédente du journal sont exclus. Fenêtre close après
commit/push main. GCP non utilisé.

Dernière lecture constructeur avant publication : la nouvelle entrée A
sur le LiDAR est prise en compte. `terrain` est bien un slab aléatoire,
pas une acquisition ni une fusion de scans. Cette restriction est ajoutée
aux reçus et à la passation ; conserver un protocole distinct pour
densification et extension, poses communes et collisions de quantification.
Les fichiers `audits/lidar08_20260914` en préparation restent exclus.

## 14 septembre 2026 — CONSTRUCTEUR : census q2 du front partagé

Reprise après da366f7f : `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`. Le prochain
raccord appelle lui-même le front, restreint à q2, puis traite directement
ses nœuds B. Pas de factory/Axis/Pool, de copie B ni de nouvel arbre par
rectangle. Le plus petit facteur fournit les ancres ; Pairwise et Shared
conservent exactement les mêmes candidates. Shared transmet toujours un
compte et un curseur Z, partant de zéro à la première tâche. Le résidu
est couvert sans adopter de handles externes supposés certifiés.

Le temps englobera front, comptage, collecte/callback et destructions,
sans chronomètre par rectangle. Ne pas l'appeler census isolé. L'option
du front par défaut garde toutes les voies ; le raccord q2 en demande
une seule pour ne pas payer Xi et les branches q3/q4 sans consommateur.
La qualification doit confronter supports, intérieurs et coquilles à
l'exhaustif indépendant. Les builds précédents restent épinglés ; builds
neufs `v8_front_census_20260914` et `v8_front_census_sanitize_20260914`.

Le coût Σmin(|A|,|B|) des ancres reste payé ; le raccord ne prouve donc
pas la sous-quadraticité. Une variante A×B×Z portant deux groupes de
requêtes pourrait partager davantage : invariant de préfixe Z conservé,
extrema exacts de H sur trois boîtes, subdivision A/B sans avancer Z.
Demande de contrelecture mathématique lancée en parallèle, pas encore
une implémentation ou un gain. Le protocole LiDAR de l'auditeur A est
attendu ; `terrain` n'est toujours pas un substitut de capteur. Aucun
index réservé ni GCP utilisé à cette ouverture.

Lecture des publications A 9ae8a268 et B c0e2f6d2 : données LiDAR et
propagation disponibles, sans qualification héritée. Le raccord reste
q2 seul ; les comparaisons avec le front trois voies ne doivent pas
attribuer à Shared le retrait de Xi. Le test de lentille seul ne mérite
pas une intégration chaude sur le résultat de B (0–1 % de recherches
évitables). La propagation conserve la sûreté par IDs distincts mais
ses temps ne gagnent pas partout : « rentable » reste à établir sur
front+census+collecte. Demande A : rejouer ensuite sur ses fichiers u16
avec masque q2 identique et coûts d'entrée explicites. Demande B :
contrelecture de l'état conjoint (A,B,compte,curseur Z) avant d'éclater
en ancres, pour diminuer le terme Σmin(|A|,|B|). Sources intégrées
maintenant gelées pour qualification ; pas d'index réservé à ce stade.

Premier résultat à traiter avant toute promotion : amas8k→16k, s8,
Samples/Shared, visites Z 973 179 333→4 199 340 563 (×4,31), tâches
51 841 177→206 349 628, ancres 1 291 731→3 395 250. Supports utiles
245 733→520 208 seulement. Le raccord ne résout pas P0 dans ce régime.
Piste à falsifier : l'ordre DFS fixe peut rencontrer des blocs indécis
et subdiviser B avant les blocs qui auraient fourni K témoins communs.
La baisse des démarrages ne garantit pas une baisse de ces subdivisions.
Une visite prioritaire de témoins locaux h_a/h_b ou un parcours A×B×Z
doit conserver une comptabilité disjointe ; ne pas avancer artificiellement
le curseur en gardant un crédit correspondant à un autre préfixe. Demande
aux auditeurs : proposer une fixture de ce raffinement prématuré, puis
un choix de blocs qui change le travail, sans réintroduire A²+B².

Contrelecture parallèle : fixture a=(1000,0,0), B={(i,0,0):0≤i<m},
m8/16/32/64, déjà séparée à s12. Le DFS croissant et la règle stricte
diagZ>diagB divisent toutes les requêtes B avant tout crédit : 2m−1
tâches. Modèle exact en mémoire à m64/K10 : 127 tâches/421 visites,
contre 27/99 et 54 rejets en groupes pour un DFS décroissant avec ses
échappements propres. Pas une variante produit ou un reçu qualifié ; la
fixture et sa preuve sont inscrites dans docs/P0_FRONT_ET_CENSUS_Q2.md.
A singleton prouve que le seul agrandissement en A×B×Z ne suffit pas.
La part de ce mécanisme dans les amas reste à profiler ; le choix de Z
et ses continuations doivent accompagner la prochaine architecture.

Expérience suivante proposée et contre-vérifiée : à B→B_L/B_R, tester
le frère comme bloc de K témoins stricts autonomes pour chaque enfant.
Rejet si cardinal≥K et Hmin>0 ; sinon abandonner ce test sans accumuler
de crédit ni changer compte/cursor. Coût constant par enfant, pas de
descente supplémentaire ni d'histogramme. La position du frère dans le
DFS importe peu puisque son certificat n'est pas ajouté au compte. Sur
la fixture m64/K10, il rejette 48 paires en blocs et laisse 16 paires au
census ; pas les 54 rejets du modèle à ordre inversé. À intégrer dans
un build/tranche distincts, sans annoncer une borne globale.

Clôture des calculs : 43 CTests Release/Clang ASan/UBSan passent ;
53 mesures Kmax10, 48 configurations, quatre familles et s8/10/12.
Les amas32k prennent 223–229 s et environ 17,66 milliards de visites
pour 1,089 million de supports. Modifier s ne répare pas ce régime.
Les résultats positifs de Samples face à Pure au coût total 8k restent
distincts du gain de Shared (non universel). Les lecteurs de fermeture
et le contrôle documentaire terminent avant réservation de l'index.

Lecteurs normal/−O PASS, résumés identiques, pins de fermeture inchangés.
Contrelecture B 1ca8f62d lue : son accord sur A×B×Z ne garantit pas
l'efficacité et ne dispense pas de la collecte de coquille. Son point
sur E5 est clarifié dans le document constructeur FONDEMENTS_ET_OBJET :
la fixture à quatre points réfute le graphe induit, E5 la proposition 6
littérale ; registre formel inchangé et aucun résultat FULL nouveau.

Réservation d'index constructeur, index constaté vide : AGENTS.md,
code/tests/bench/docs et entrées v8 propres, audits/ETAT_COURANT.md et
audits/FONDEMENTS_ET_OBJET.md constructeur, reçu wspd_q2_census_20260914
et cette seule section CONSTRUCTEUR. Exclure les changements v6/v7,
les travaux indépendants A/B et complémentaires, ainsi que leur ancienne
section de ce journal. Fenêtre close après commit/push main. GCP non utilisé.
