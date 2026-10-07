# Registre des constats de la v12

Une ligne par constat ([README](README.md) § 1). Les constats reportés de la v11 (README § 2) restent à inscrire par
le développeur. Bloc `CST-0101` à `CST-0199` : contre-lecture des lemmes T
([`AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md`](AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md), `CST-0101` à
`CST-0107`, pin `13c52bc60`, `CONCEPTION_TOUR.md` blob `8e7432dcc680`) et relecture du contrat numérique
([`AUDIT_CONTRAT_NUMERIQUE_20261007.md`](AUDIT_CONTRAT_NUMERIQUE_20261007.md), `CST-0108` à `CST-0113`, pin
`e264de6f2`, `CONTRAT_NUMERIQUE.md` blob `9b6a17bd61d1`).

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0101` | `LEM-T1` : la décision de `resolve1` omet le test $S \subseteq F$ ; « $S = S^{*}(b)$ et $F \subseteq P_b$ » ne suffit pas | 2026-10-07 | auditeur (Claude) | exactitude | majeure (latente, avant code) | § 3.2, § 3.4 | carré, $F$ un côté, proposition $S^{*}(b)$ ; mutant « test retiré » | ouvert | — |
| `CST-0102` | `LEM-T4` : à énoncer comme le lemme P.3, avec ses trois ponts ; le théorème J n'en est pas une prémisse | 2026-10-07 | auditeur (Claude) | document | mineure | § 4.2, A.4, PO-T4 | — | ouvert | — |
| `CST-0103` | `REG:243` (composantes fortement connexes, spécification l. 729) : à clore comme caduque, remplacée par le lemme P, `LEM-T4` et `LEM-T7`, pas comme prouvée | 2026-10-07 | auditeur (Claude) | document | mineure | registre l. 243 ; spécification blob `46e3b18fa5e8` | — | ouvert | — |
| `CST-0104` | `LEM-T3` : graver `WIT-D2` et `WIT-MEMO` dans sa porte ; aucune garde $\beta(F_0) \leq \ell(r_b-1)$ | 2026-10-07 | auditeur (Claude) | exactitude | mineure | A.3 | `WIT-D2`, `WIT-MEMO` | ouvert | — |
| `CST-0105` | `LEM-T5` (iii) : hypothèse $\mathrm{rang}(\ell) \leq r$ absente ; listes par survivant dans l'ordre de traitement | 2026-10-07 | auditeur (Claude) | document | mineure | A.5 | appel hors domaine refusé | ouvert | — |
| `CST-0106` | `LEM-T7` : la famille contient des séparables non maximaux ; (iv) majore l'apport à la couverture | 2026-10-07 | auditeur (Claude) | document | mineure | § 2.2, § 6.4, A.7 | cercle $x^{2}+y^{2}=25$ à quatre sites | ouvert | — |
| `CST-0107` | `LEM-T4` : clé (rang, plus petite naissance) à lire dans la numérotation canonique de la v12, pas dans l'ordre du catalogue | 2026-10-07 | auditeur (Claude) | document | mineure | § 2.1, § 4.2 ; contrat v12 § 1 | empreinte appariée | ouvert | — |
| `CST-0108` | `NUM-GARDE` ne vaut que pour une boule certifiée : une proposition flottante non certifiée (centre circonscrit d'un triangle obtus) ne doit jamais être confrontée à un site ou à une boîte extérieurs | 2026-10-07 | auditeur (Claude) | numérique | majeure (latente, avant code) | contrat § 2 | triangle presque aligné ; mutant « recensement avant certificat » | ouvert | — |
| `CST-0109` | recensement de la v11 (`LatticeSphere`, chemin chaud) : centre absolu $a_j D+N_j$ ($5B+6$ bits, $B \leq 24$) et coin lointain des boîtes hors garde ; à reformuler en local | 2026-10-07 | auditeur (Claude) | numérique | majeure (bloque u32) | `src/index/census.cpp`, `src/num/predicates.cpp` | boîte à cheval sur le pavé, à $B=32$ | ouvert | — |
| `CST-0110` | requêtes à centre entier (`core`, $k$ plus proches) : distances carrées de $2B+2$ bits ; `u64` ne suffit plus à $B=32$ ; absent du contrat | 2026-10-07 | auditeur (Claude) | numérique | majeure (bloque u32) | contrat § 2 ; conception de la tour § 3.6 | deux sites aux coins opposés de $[0,2^{32})^{3}$ | ouvert | — |
| `CST-0111` | prédicats sur les sites de coquille (`LEM-T7`, support canonique) et côté d'un site gardé : budgets du repère $s+2$, absents de la table du § 3 | 2026-10-07 | auditeur (Claude) | numérique | mineure | contrat § 3 | témoins de palier à $s=14$/$15$ et $17$/$18$ | ouvert | — |
| `CST-0112` | `NUM-COUVERTURE` : le filtrage G1 d'un enfant lit la liste du parent ; il s'évalue dans le repère du parent | 2026-10-07 | auditeur (Claude) | document | mineure | contrat § 2 ; `src/catalogue/boxes.cpp` | témoin hors de la liste de l'enfant | ouvert | — |
| `CST-0113` | la clé de Morton décide $S^{*}$ (coquilles à plusieurs supports minimaux), donc l'ordre des `BallIdx`, des lignes `supports` et `cover_v10`, et l'ordre des sites que le lecteur strict exige et hache ; une clé sur (coordonnées − minimum) change ces ordres ; la porte d'invariance par translation doit comparer à translation près | 2026-10-07 | auditeur (Claude) | exactitude | majeure | contrat § 4 et § 7 ; `bench/full_semantic.py` l. 110 à 113 | carré à deux diagonales, translaté | ouvert | — |
