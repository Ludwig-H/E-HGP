# Reçu local de MES-M3 et MES-M4 (7 octobre 2026)

Microbancs de la tour (tranche T0), joués sur le codespace (8 cœurs chargés, un processus par mesure) contre la v11
gelée (`ac081a06f`, `libmhgp11.a` u21, SHA-256 `050532a95c322cc2…`) : neuf cas, ng00, ng01 et ng02 à K5 et K10, et
uniformes de 8 000, 16 000 et 32 000 sites à K5. Code : [`../../microbancs/mes_m3_m4_tour/`](../../microbancs/mes_m3_m4_tour/README.md) ;
rapport de l'agent : [`RAPPORT.md`](RAPPORT.md) ; tableaux : [`out/tableaux.md`](out/tableaux.md) ; données brutes du
pilote : `out/rapport_mes_m3_m4.json` et les journaux des portes.

- **MES-M3** (plus petite boule proposée par Welzl en flottant, certifiée par `LEM-T1` avec ses deux inclusions,
  sinon certificat exact du support proposé et canonisation, sinon repli) : identité de la sphère, du support canonique
  et de l'identification sur 29 878 990 parties ; certification sans arithmétique de 75 à 80 % des parties sur les
  trames (85,7 % sur uniformes) ; résolution à K10 réduite de 52, 49 et 43 % (ng00, ng01, ng02) contre la réplique
  v11, **localement**. La règle d'adoption (au moins 40 %) se juge sur G4.
- **MES-M4** (noyau union-find sans lots, contraction `LEM-T4`, numérotation canonique) : forêts identiques à la v11
  sur tous les ordres des neuf cas ; `LEM-T6` sans écart sur 17 497 207 naissances ; noyau à un fil de 9,7 à 13,1 ms à
  l'ordre 5 et de 25,8 à 34,0 ms à l'ordre 10, **localement**. Les seuils (10 ms, 35 ms, contraction 3 ms) se jugent
  sur G4.
- **Constats** : `CST-0101` (témoin du carré et mutant tués dans la porte de MES-M3) ; `CST-0212` (codage gardé,
  domaine écrit : au plus $2^{31}-1$ naissances par ordre, refus avant allocation, porte à la borne).
- **Données** : les vidages (7,8 Go, dérivés de SemanticKITTI) restent hors dépôt ; ce reçu ne contient que des
  comptes, des temps locaux et des empreintes.

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. GCP non utilisé.
