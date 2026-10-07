# Audit géant des transpositions vers la v11 — contexte commun

4 octobre 2026, vers 11 h 45 UTC. Workflow lancé par Claude à la demande de l'utilisateur.

## La demande (mots de l'utilisateur)

« À présent, lance-toi dans un audit géant de tout ce qui a été fait dans les versions précédentes et qui pourrait
être transposé utilement dans la v11. Ne prends en compte que les idées vraiment intéressantes (s'il y en a).
Notamment : la v10 était plus rapide que la v11 actuelle. » Puis : « Il serait pertinent de lancer des workflows pour
cet audit géant. » Puis : « Rappel que l'objectif du contrat est toujours 100 ms. »

## Le contrat et l'écart

- **Contrat v11** : la tour HGP FULL sur les trames SemanticKITTI **sans sol** (30 000 à 60 000 sites), grille 1 mm,
  moteur entier exact, en **100 ms** sur G4 (VM `g4-standard-48` : EPYC 9B45, 24 cœurs × 2 SMT, 48 fils) à **K = 5**,
  et si possible K = 10. Le jalon intermédiaire de 200 ms n'est pas le but : le but est 100 ms.
- **v11 aujourd'hui** (moteur `c40f40798`, [DEVELOPPEMENT.md] et reçu `qualification_performance_20261003`) : à W48,
  mode 16379, médianes FULL **489 / 345 / 432 ms** sur les trames 08/000000, 08/000100, 08/000200 à K = 5 ; génération
  du catalogue 131–176 ms, résolution régulière 74–131 ms ; « le domaine seul dépasse 200 ms » ; passe unique du
  catalogue ~4,8 s à W1 et 150–190 ms à W48 (plafond SMT) ; forêt W48 08/000000 ramenée de 244 à ~150 ms ; aucune
  des 81 prises sous 200 ms.
- **v10** : moteur exact à **0,20–0,25 s à K = 5** et ~1 s à K = 10 sur G4 48 fils. Audit précédent (2 octobre,
  `build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md`, `L06_CODE_TOUR.md`) : générateur v10 = 48–58 candidats testés
  par boule, 135–270 cycles par pièce, feuille scalaire, build sans `-march`, plancher de travail 126 ms à K = 5 ;
  tour v10 = plancher séquentiel 33–47 ms (Kruskal par lots + verticales) ; leviers fondés ×1,9 (générateur) et
  ×1,6–3,2 (noyau sans lots). La v11 a été refaite « plus propre » à partir du raccord R2 de la v10
  (`build/v10-integration-r2/src`, commit 865f5e6), et elle est plus lente.
- Hors moteur : hiérarchie de points $H^{r}_{k+1}$ et sortie plate (E1) en Python seulement (`bench/points_radius.py`,
  `bench/points_flat.py`) ; leur port natif est ouvert.

## Où lire

| Source | Chemin (lecture seule) |
| --- | --- |
| v11 à jour (`origin/main`) | `/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/` (extraction partielle : un fichier absent se lit par `git -C /workspaces/E-HGP/build/v11-claude-20261003 show origin/main:<chemin>`) ; docs : `docs/INDEX.md`, `ARCHITECTURE.md`, `CONCEPTION_MOTEUR.md`, `DEVELOPPEMENT.md`, `PERFORMANCE_FULL.md`, `CATALOGUE*.md`, `FULL_*.md`, `MATHEMATIQUES.md` ; audits : `audits/` et les reçus `receipts/` |
| v2 à v10 | `/workspaces/E-HGP/morsehgp3D_v2` … `morsehgp3D_v10` (extraction principale, lecture seule) ; en cas de doute, `git show origin/main:<chemin>` depuis le worktree ci-dessus |
| raccord R2 de la v10 (source de la v11) | `/workspaces/E-HGP/build/v10-integration-r2/` |
| audit v10 du 2 octobre | `/workspaces/E-HGP/build/v11-persist/audit_v10/` (L01–L10, preuves) et `morsehgp3D_v11/docs/AUDIT_V10_SYNTHESE.md` |
| conception v11 | `/workspaces/E-HGP/build/v11-persist/conception/` (CONCEPTION_GENERATEUR, CONCEPTION_TOUR, PISTES_DE_RUPTURE) |
| bibliothèque produit | `/workspaces/E-HGP/morsehgp3d/` |
| E-HGP, Zoltan | `/workspaces/E-HGP/E-HGP/`, `/workspaces/E-HGP/Zoltan/` |
| pistes fermées | `docs/archive/abandoned/README.md`, `morsehgp3D_v3/audits/PISTES_FERMEES.md` et équivalents |
| registre des preuves | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` |

## Règles pour chaque agent

- Écrire en français. Écrire **seulement** dans `/workspaces/E-HGP/build/v11-persist/audit_transpositions/<ton dossier>/`.
- **Aucune** commande git qui écrit (add, commit, checkout, reset, stash…), **aucune** commande GCP, **aucune**
  construction ni exécution native (cmake, make, g++, binaires) : consigne de l'utilisateur, les tests lourds passent
  sur G4. Lecture du code, des documents et des reçus ; petits calculs Python sur des JSON existants permis.
- Chaque affirmation cite sa source : chemin, et ligne, commit ou reçu quand c'est possible. Distinguer **mesuré**
  (reçu G4 ou local nommé), **estimé** et **conjecturé**. Aucun chiffre sans origine.
- Ne retenir que les idées **vraiment intéressantes** pour la v11 : un gain mesurable vers 100 ms, une garantie
  mathématique, une simplification forte, ou un outil de test qui manque. Mieux vaut trois idées solides que vingt
  faibles ; « aucune » est une réponse acceptable.
- Pour chaque idée : est-elle **déjà dans la v11** (où) ? Respecte-t-elle la doctrine v11 (moteur entier exact,
  flottant seulement comme filtre certifié à repli exact, aucune matérialisation de la mosaïque de Delaunay d'ordre
  supérieur ni de catalogue global ∝ C(n,k), sorties identiques octet pour octet, portes et mutants) ? Est-ce une
  **piste fermée** (ne se rouvre qu'avec un nouveau théorème de complétude et une fixture) ?
- La machine locale est partagée (8 cœurs) : lectures et `grep` ciblés, pas de calcul lourd.
- Reprise : si ton fichier de sortie existe déjà et se termine par la ligne `FIN`, relis-le et rends son contenu
  sans refaire le travail.
