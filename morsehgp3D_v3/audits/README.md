# Audits de MorseHGP3D v3

Ce dossier conserve les audits indépendants de la v3. Chaque finding doit être rattaché à un commit ou à des empreintes SHA-256; un fichier modifié pendant sa lecture est réaudité sur un nouveau snapshot avant verdict.

- [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md) : audit historique de la première voie proposée.
- [`AUDIT_PROPOSITION_2.md`](AUDIT_PROPOSITION_2.md) : second audit actualisé et journal continu de la proposition, de l'oracle, des prototypes, des reçus et des portes.
- [`AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md`](AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md) : audit dynamique du juge M1, réfutation du certificat M2.1 commité et suivi de son retrait dans le delta live.
- [`AUDIT_CONTINU_CORRECTIONS.md`](AUDIT_CONTINU_CORRECTIONS.md) : journal vivant des snapshots, reproductions, corrections en cours et portes restantes; c'est le point d'entrée à lire pendant le développement concurrent.
- [`AUDIT_EDGE_SHALLOW_AD9DEF2.md`](AUDIT_EDGE_SHALLOW_AD9DEF2.md) : audit dynamique historique du premier dictionnaire `edge_shallow`, de ses fixtures et de son coût.
- [`REPONSE_README_50K_K10.md`](REPONSE_README_50K_K10.md) : réponse quantitative à la question du README sur 50 000 points, $K=10$ et la seconde, avec conditions GO/NO-GO et audit des largeurs des arités trois et quatre.
- [`REPONSE_README_PREFIXE_SHALLOW.md`](REPONSE_README_PREFIXE_SHALLOW.md) : réponses Q0--Q3 au README : correction du certificat de localité, contre-exemple minimal au peeling par couches, constructeur shallow crédible, tri `i128` et impossibilité d'un rejet complet brut en `O(1)`.

Ces audits motivent les corrections; l'autorité mathématique reste la spécification et le registre des preuves.
