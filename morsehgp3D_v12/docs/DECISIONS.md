# Décisions de la v12

7 octobre 2026. Source : audit géant de la v11 (`../../morsehgp3D_v11/docs/AUDIT_GEANT_V11.md`, § 5.5 et § 9.2) et
passation (`../../morsehgp3D_v11/PASSATION.md`, § 6.1). **Ces décisions appartiennent à l'utilisateur.** Les
recommandations sont celles de l'audit ; une ligne passe à « décidée » seulement sur réponse explicite, datée et citée.

## 1. Décisions en attente (avant tout code)

| # | Question | Options | Recommandation | Conséquence | État |
| --- | --- | --- | --- | --- | --- |
| D1 | Régime du contrat de temps | processus neuf (à froid) ; Session résidente qui reçoit des trames successives (à chaud) | **Session résidente**, temps à chaud contractuel ; temps à froid publié à côté | le contexte CUDA (75–150 ms), les arènes et les caches ne se paient qu'une fois ; question posée depuis le 7 août | en attente |
| D2 | Latence ou cadence | latence par trame ; cadence de 10 trames/s avec recouvrement de deux trames | **latence par trame** ; cadence comme repli déclaré | avec un catalogue sur le GPU et une tour sur le CPU, la cadence est bornée par le plus lent des deux étages, pas par leur somme | en attente |
| D3 | Périmètre des 100 ms | FULL K1..5 en mémoire ; plus l'écriture ; plus `points` / `plat` | **FULL K1..5 en mémoire, verticales comprises** ; écriture et vues mesurées à part | l'écriture de `full` coûte aujourd'hui 2,16 s à K5 ; `plat` et `points` 0,4 et 0,75 s hors tour (local) | en attente |
| D4 | K = 10 | contrat ; objectif | **objectif**, cible 0,3 à 0,5 s | aucune estimation ne place la tour K10 sous 100 ms en CPU | en attente |
| D5 | GPU dans le chemin contractuel | oui ; non | **oui, pour le catalogue** | un catalogue CPU seul ne descend pas sous 140–170 ms à K5 | en attente |
| D6 | Quantification | u21 seul ; u21 + u24 ; trois profils | **u21 seul dans le produit**, u24 en matrice, u18 abandonné | trois profils ont multiplié les défauts de qualification pour 5 % de vitesse | en attente |
| D7 | Données du contrat | trois trames de la séquence 08 ; plusieurs séquences | **plusieurs séquences (00–10)** ; redéfinir la plage de tailles ; dire si le contrat vise le maximum ou la médiane | sur 132 trames sans sol de la séquence 08, 89 dépassent 60 000 sites (médiane 68 049, maximum 126 267) | en attente |
| D8 | Multiplicités (retours fusionnés au millimètre) | refus ; modèle par copies | **refus explicite** avec compteur publié de bout en bout, aucun dédoublonnage silencieux | aucun doublon à 1 mm sur les trames ; le modèle par copies est implanté dans l'oracle mais pas prouvé pour le moteur | en attente |
| D9 | Seuil de condensation | absolu (thèse, p. 97) ; relatif au parent | **absolu par défaut**, relatif comme bras déclaré ; amender `CLAUDE.md` (l. 134) | le relatif rejette structurellement un objet contre plus massif que lui (vélo contre mur : 9 objets sur 19) | en attente |
| D10 | Masse du § 9.1 | comptage entier (critère A) ; masse fractionnaire $m_\tau$ | à trancher, avec la famille de faces (Gabriel, $W_K$) | avec $m_\tau$, chaque triangle de la fixture du § 6.1 pèse moins de 3 et cesse d'être un amas à mcs 3 | en attente |
| D11 | Application prioritaire | segmentation en ligne (latence) ; modèle de fondation Zoltan (débit, stockage, exports) | à dire | fixe l'ordre des vues : `plat` d'abord, ou `coverage_v1` et `weighted_gabriel_v1` et un `full` compact | en attente |
| D12 | Polyèdres d'ordre k | dans la v12 ; en aval | **en aval**, à la demande, hors des 100 ms | environ 1 000 faces par site à K5 ; touche l'invariant « pas de mosaïque d'ordre supérieur » | en attente |
| D13 | Bibliothèque produit `morsehgp3d/` | archiver ; définir un `CertifiedTowerInput` v2 comme vue du registre | à dire | aucune version n'en est productrice depuis le 9 août ; la CI la qualifie encore | en attente |
| D14 | Hygiène du dépôt | purger ou non le scan KITTI brut versionné dans un reçu de la v8 ; l'identité du compte GCP dans 528 fichiers de reçus | à dire | réécrire l'historique est irréversible ; la v12 ne doit plus écrire ni l'un ni l'autre | en attente |
| D15 | Ouverture formelle | appliquer [`OUVERTURE_PROPOSEE.md`](OUVERTURE_PROPOSEE.md) à `AGENTS.md` et `CLAUDE.md` | oui, après D1–D7 | `CLAUDE.md` annonce encore la v11 en u18, sans section v10 | en attente |

## 2. Décisions déjà prises, toujours en vigueur

| Date | Décision | Source |
| --- | --- | --- |
| 7 oct. 2026 | v12 « aussi propre, simple et efficace que possible » ; feu vert GCP G4 ; dossier v12 à créer | demande de l'utilisateur |
| 6 oct. 2026 | « Il vaut mieux toujours tester sur données LiDAR réelles » : toute décision de vitesse se prend d'abord sur des trames réelles ; le synthétique complète | consigne de l'utilisateur |
| 6 oct. 2026 | u21 reste le profil par défaut | décision de l'utilisateur |
| 2 oct. 2026 | même objet, mêmes contrats (100 ms sans sol, K5, si possible K10), rigueur, comparaison à HDBSCAN sur synthétique et réel | ouverture de la v11 |
| 2 oct. 2026 | pousser sur `main`, sans branche ; un worktree par acteur ; vérifier l'index avant tout `git add` | `AGENTS.md` |
| 2 oct. 2026 | tests lourds sur G4, sessions gardées, une seule VM, arrêt certifié | `AGENTS.md` |
| 1er oct. 2026 | ordre : la tour contient-elle l'objet, puis la hiérarchie, puis seulement la pondération $z$ | consigne de l'utilisateur |
| 28 sept. 2026 | HDBSCAN = `sklearn.cluster.HDBSCAN` tel quel, jamais réimplémenté ; comparer à $K$ = `min_samples` | consigne de l'utilisateur |
| 28 sept. 2026 | la thèse est une source critiquable, pas une autorité ; `HGP-old/` n'est jamais un oracle | consignes de l'utilisateur |
| 22 sept. 2026 | moteur entier exact sur grille de 1 mm ; float32 arrêté ; multi-millions de points : objectif secondaire | `AGENTS.md` |
| 21 sept. 2026 | LiDAR sans sol prioritaire ; trame entière, jamais sous-échantillonnée ; les étiquettes ne bloquent pas les chronos | `AGENTS.md` |
| 16 août 2026 | tailles d'intérêt 8 000 / 16 000 / 32 000 ; jamais de vérification exhaustive (les oracles bornés T2 exceptés) | `CLAUDE.md`, `docs/TEST_PLAN_MORSEHGP3D.md` |
| permanent | aucune mosaïque de Delaunay d'ordre supérieur ni catalogue en $\binom{n}{k}$ dans le chemin produit ; toute contradiction devient une fixture et une ligne du registre des preuves ; aucun banc ne promeut un statut public | `CLAUDE.md` |
| 14 sept. 2026 | jamais de séparation WSPD sous 8 (sans objet tant qu'aucune WSPD ne revient) | `AGENTS.md` |

## 3. Comment une décision est consignée

Une ligne du § 1 passe à « décidée » avec : la date et l'heure lues par `date -u`, la citation exacte de l'utilisateur,
et le commit qui la consigne. Une décision qui change un contrat met à jour, dans le même commit, `README.md`, le
document concerné de `docs/` et, après l'ouverture, `AGENTS.md`.
