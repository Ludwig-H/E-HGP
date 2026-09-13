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
