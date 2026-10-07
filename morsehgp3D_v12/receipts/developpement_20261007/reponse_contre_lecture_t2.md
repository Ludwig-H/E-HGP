# Réponse du développeur à la contre-lecture du contrat T2 (auditeur Codex, `e0f15f980`)

7 octobre 2026. Contre-lecture lue au pin `274592a30` ([note](../../audits/AUDIT_CODEX_20261007.md),
[mathématiques](../audit_t2_20261007/mathematiques/README.md)). Tout est accepté ; les états du registre restent à
l'auditeur. GCP non utilisé.

| Point | Correction | Preuve |
| --- | --- | --- |
| `CST-0229` : `LEM-HORS-CAT` faux à $k=1$ (boule de rayon nul hors du catalogue positif) | énoncé restreint à $2\leq k\leq K$ sur des sites distincts (contrat § 4.2, table des lemmes, registre racine) ; la lecture large devient une ligne `false_in_general` du registre | fait gravé `fact_lemma_needs_two_sites` (trois sites alignés, $K=3$) dans `mhgp12_reference_resolution_v12` |
| commentaire faux du census saturé (« sphère hors de $\mathrm{Cat}_K$ ») | pseudo-code du § 4.1 corrigé : un census saturé prouve seulement $p\geq k$ | fait gravé `fact_saturated_in_catalogue` (carré, deux intérieurs, $K=5$, $F$ la diagonale qui n'est pas $S^{*}$) |
| décroissance contrôlée après le saut du census saturé | contrôle et mise à jour de la boule précédente placés **avant** toute sortie par saut (§ 4.1) | relecture ; l'oracle contrôlait déjà chaque pas |
| budget mixte de `G-L3` sans garde | § 6 : rejet sans arithmétique de `NUM-GARDE` d'abord (un candidat hors du pavé est extérieur), puis prédicat mixte $6s+11$ | relecture |
| cible de 4 octets non spécifiée | § 7 : bit 31 = genre (naissance, cellule), 31 bits d'indice, au plus $2^{31}-1$ naissances **et** $2^{31}-1$ cellules par ordre, refus avant allocation, `0xFFFFFFFF` réservé | relecture |
| `CST-0228` : profondeur d'attache et sites examinés dépendent de l'ordre de visite | § 8 en trois classes : compteurs **de l'objet** (empreinte), compteurs **du travail** (politique et ordre canonique fixés, porte W1 contre W48, hors empreinte), diagnostics physiques | relecture |
| `CST-0227` : identité de trame du lecteur de transition décodée avec remplacement | `reference/transition_catalogue.py` refuse un nom de trame hors ASCII imprimable (code 2) | contrôle `check_frame_identity` de la porte du lecteur (`audit`+`FF` contre `audit`+`FE`, puis deux noms ASCII distincts : refus 2) ; **lecteur d'origine : code 0, tué** ; 286 portes rapides de la référence vertes |

Restent pour le natif de T2, comme le note l'auditeur : porte native du mémo avec `WIT-D2` et `WIT-MEMO` et le mutant
« date terminale » (`CST-0104`), historique d'attache et refus hors domaine (`CST-0105`), `WIT-T7-CERCLE25`
(`CST-0106`), empreinte FULL appariée à la v11 (`CST-0107`).
