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
