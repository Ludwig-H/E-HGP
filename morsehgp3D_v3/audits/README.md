# Audits de MorseHGP3D v3

Ce dossier conserve les audits indépendants de la v3. Chaque finding doit être rattaché à un commit ou à des empreintes SHA-256; un fichier modifié pendant sa lecture est réaudité sur un nouveau snapshot avant verdict.

- [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md) : audit historique de la première voie proposée.
- [`AUDIT_PROPOSITION_2.md`](AUDIT_PROPOSITION_2.md) : second audit actualisé et journal continu de la proposition, de l'oracle, des prototypes, des reçus et des portes.
- [`AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md`](AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md) : audit dynamique du juge M1, réfutation du certificat M2.1 commité et suivi de son retrait dans le delta live.
- [`AUDIT_CONTINU_CORRECTIONS.md`](AUDIT_CONTINU_CORRECTIONS.md) : journal vivant des snapshots, reproductions, corrections en cours et portes restantes; c'est le point d'entrée à lire pendant le développement concurrent.
- [`AUDIT_EDGE_SHALLOW_AD9DEF2.md`](AUDIT_EDGE_SHALLOW_AD9DEF2.md) : audit dynamique historique du premier dictionnaire `edge_shallow`, de ses fixtures et de son coût.
- [`AUDIT_LIVE_1216D16.md`](AUDIT_LIVE_1216D16.md) : audit reproductible du prototype et du juge aux commits `1216d16` puis `389a742`, avec résultats Release/sanitizers, reçus hostiles, domaine `RelevantGP`, compteurs et portes 50 k.
- [`AUDIT_QUESTIONS_README_389A742.md`](AUDIT_QUESTIONS_README_389A742.md) : réponses au README courant, dont la fermeture conditionnelle de PEL-1 sous `RelevantGP`, l'audit Q0--Q3 et les deux fixtures Q1 invalides publiées.
- [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) : contre-exemple u16 exact au clipping depuis une ancre non diamétrale, faux `dictionary_refuted`, fixture `RelevantGP` et validation sous `/tmp` de la précondition carrier--carrier manquante.
- [`AUDIT_ORDER_K_BFS_A8111F0.md`](AUDIT_ORDER_K_BFS_A8111F0.md) : audit du parcours de l'arrangement relevé, correction du signe `InSphere`, contre-exemple coplanaire bien centré persistant, arités manquantes, complétude ouverte et coût réel en `8(n-4)V`.
- [`AUDIT_ORACLE_389A742.md`](AUDIT_ORACLE_389A742.md) : audit hostile distinct du juge et de ses CTests au commit `389a742`, avec faux rejets `RelevantGP`, faux reçus, mutations de champs publics, fixtures Q1/constante et bornes CLI.
- [`REPONSE_README_50K_K10.md`](REPONSE_README_50K_K10.md) : réponse quantitative à la question du README sur 50 000 points, $K=10$ et la seconde, avec conditions GO/NO-GO et audit des largeurs des arités trois et quatre.
- [`REPONSE_README_PREFIXE_SHALLOW.md`](REPONSE_README_PREFIXE_SHALLOW.md) : réponses Q0--Q3 au README : correction du certificat de localité, contre-exemple u16 bien centré et RelevantGP au peeling par couches, constructeur shallow crédible, tri `i128` et impossibilité d'un rejet complet brut en `O(1)`.

Ces audits motivent les corrections; l'autorité mathématique reste la spécification et le registre des preuves.
