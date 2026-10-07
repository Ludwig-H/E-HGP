# Registre des constats de la v12

Une ligne par constat ([README](README.md) § 1). Les constats reportés de la v11 (README § 2) restent à inscrire par
le développeur. Bloc `CST-0101` à `CST-0199` : contre-lecture des lemmes T
([`AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md`](AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md)). Pin des lignes de ce bloc :
`13c52bc60`, `CONCEPTION_TOUR.md` blob `8e7432dcc680`.

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0101` | `LEM-T1` : la décision de `resolve1` omet le test $S \subseteq F$ ; « $S = S^{*}(b)$ et $F \subseteq P_b$ » ne suffit pas | 2026-10-07 | auditeur (Claude) | exactitude | majeure (latente, avant code) | § 3.2, § 3.4 | carré, $F$ un côté, proposition $S^{*}(b)$ ; mutant « test retiré » | ouvert | — |
| `CST-0102` | `LEM-T4` : à énoncer comme le lemme P.3, avec ses trois ponts ; le théorème J n'en est pas une prémisse | 2026-10-07 | auditeur (Claude) | document | mineure | § 4.2, A.4, PO-T4 | — | ouvert | — |
| `CST-0103` | `REG:243` (composantes fortement connexes, spécification l. 729) : à clore comme caduque, remplacée par le lemme P, `LEM-T4` et `LEM-T7`, pas comme prouvée | 2026-10-07 | auditeur (Claude) | document | mineure | registre l. 243 ; spécification blob `46e3b18fa5e8` | — | ouvert | — |
| `CST-0104` | `LEM-T3` : graver `WIT-D2` et `WIT-MEMO` dans sa porte ; aucune garde $\beta(F_0) \leq \ell(r_b-1)$ | 2026-10-07 | auditeur (Claude) | exactitude | mineure | A.3 | `WIT-D2`, `WIT-MEMO` | ouvert | — |
| `CST-0105` | `LEM-T5` (iii) : hypothèse $\mathrm{rang}(\ell) \leq r$ absente ; listes par survivant dans l'ordre de traitement | 2026-10-07 | auditeur (Claude) | document | mineure | A.5 | appel hors domaine refusé | ouvert | — |
| `CST-0106` | `LEM-T7` : la famille contient des séparables non maximaux ; (iv) majore l'apport à la couverture | 2026-10-07 | auditeur (Claude) | document | mineure | § 2.2, § 6.4, A.7 | cercle $x^{2}+y^{2}=25$ à quatre sites | ouvert | — |
| `CST-0107` | `LEM-T4` : clé (rang, plus petite naissance) à lire dans la numérotation canonique de la v12, pas dans l'ordre du catalogue | 2026-10-07 | auditeur (Claude) | document | mineure | § 2.1, § 4.2 ; contrat v12 § 1 | empreinte appariée | ouvert | — |
