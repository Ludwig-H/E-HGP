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
