# Contexte commun : de la hiérarchie de points v11 à un clustering plat, contre HDBSCAN

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

## Règles de travail (impératives)

- On travaille **dans la v11** de MorseHGP3D (`morsehgp3D_v11/`). Arbre de lecture :
  `/workspaces/E-HGP/build/v11-claude-20261003` (commit ab1a739d1, extraction partielle : les dossiers `receipts/`
  ne sont pas sur le disque ; les lire par `git -C /workspaces/E-HGP/build/v11-claude-20261003 show origin/main:CHEMIN`).
- **Aucune commande git qui écrit** (ni add, commit, checkout, stash, reset, push), **aucune commande GCP**, **aucune
  construction native** (ni cmake, ni make, ni compilateur), aucun accès réseau.
- Écrire **uniquement** dans votre dossier `/workspaces/E-HGP/build/v11-points-select/<votre_nom>/`.
- Rapports en français. Python 3 local autorisé (numpy, scikit-learn), sur de petits nuages seulement.
- Toute affirmation mathématique : preuve, ou contre-exemple exact, ou statut « conjecture » déclaré. Toute
  affirmation chiffrée : script et sortie gardés dans votre dossier.

## Consignes de l'utilisateur (citées)

- 4 oct. : « On travaille dans la v11 de Morse HGP 3D » ; « Le choix de la v10 pour cette question n'était pas
  forcément pertinent » ; « N'hésite pas à lancer des workflows pour les choix délicats ».
- 3 oct. : « comment passer de la tour full à une hiérarchie laminaire sur les points ? Il faut à la fois une solution
  satisfaisante mathématiquement, mais aussi que cela marche sur au moins un des exemples de Zoltan/ où la hiérarchie
  HDBSCAN échoue. Compare aussi à HDBSCAN sur des exemples synthétiques » ; « privilégie l'aspect mathématique » ;
  « sois critique vis à vis de la thèse comme vis à vis de l'auditeur » ; « Q2 ou Q3 ne sont que de peu d'importance
  par rapport au modèle mathématique ».
- 1er oct. : « Priorité aux tests synthétiques et au bon découpage (précision rappel) par rapport à la ground truth » ;
  « Il faut faire des batteries de tests pour voir ce qui marche le mieux (IoU de préférence) pour les benchmarks
  synthétiques et aussi les exemples du dossier Zoltan/. Il est impératif d'être meilleur que HDBSCAN » ; comparer à la
  vérité terrain et au MAP (classes du maximum a posteriori aux vrais paramètres du générateur) ; « beaucoup de tests
  synthétiques (avec nombre de points, difficulté, nombre de clusters différents) ». Ordre : d'abord la tour, puis la
  hiérarchie, **puis seulement** z et la sélection (c'est maintenant le tour de la sélection) ; toujours publier z = 1.
- 28 sept. : « HDBSCAN ne peut pas battre la tour » : si HDBSCAN gagne, c'est l'algorithme tiré de la tour qui est à
  revoir, depuis le modèle mathématique ; la thèse et HGP-old ne sont pas des oracles (« Il n'y a pas d'oracle : fais
  preuve d'esprit critique ! ») ; HDBSCAN = `sklearn.cluster.HDBSCAN` tel quel, sans réimplémentation de l'adversaire ;
  comparer à `min_samples` = K.
- Règles du dépôt : tailles d'intérêt n = 8 000, 16 000, 32 000 (les petits nuages sont des oracles de correction et
  n'établissent jamais une pente) ; aucune vérification exhaustive à l'échelle (invariants globaux, juges
  d'échantillon, mutants) ; ne jamais binariser les multifusions ; toute contradiction mathématique devient une
  fixture permanente.

## L'objet v11 : H^r_{k+1} (note `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md`, à lire en entier)

À ordre k fixé, l'arbre FULL_k (composantes de L_k(r²), fusions N-aires à niveaux exacts) porte, pour chaque site i,
un **propriétaire** o_i (nœud FULL qui le couvre) et une **date d'entrée** e_i (rayon, valeur exacte
√t + √m − √q) : ancrage persistant P_1 de la v10, en rayon, sur la couverture qualifiée Π_{k+1} (une composante ne reçoit
des points que si son amas discret compte au moins k + 1 sites ; à k = 1 : liaison simple). Après son entrée, le site
suit les ancêtres de o_i. Au rayon r, les blocs sont les sites entrés, groupés par l'ancêtre vivant de leur
propriétaire. Ultramétrique u(i, j) = rayon de rencontre des lignées, diagonale e_i.

Prouvé : laminaire ; aucune réunion avant la fusion FULL ; chaque bloc dans l'amas discret de son nœud ; équivariante ;
indépendante de mcs ; dates et hauteurs stables en 3ε sous déplacement apparié (pas sous insertion) ; deux triangles
équilatéraux (thèse § 6.1) rendus ABC | DEF. Prix : Q2, Q3, Q4, Q-Π2 perdus ; respect du cœur perdu à K = 2 ; aucune
structure isolée de k sites n'est un bloc ; entrées entre α_{k+1} et α_{k+1} + d_k/2. Factorisation prouvée (synthèse du
workflow précédent, § 5.1) : projeter sans mcs puis condenser à mcs donne, à tout rayon, les blocs d'au moins mcs
points de la pendaison « absorbante » (date max(e_x, a_x), a_x = premier rayon où la lignée couvre mcs sites).
Obstruction de Palm (auditeur, conditionnelle) : à k = 2, la fraction asymptotique récupérée avant fusion parasite est
strictement inférieure à celle de FULL.

## La question

Quelle **sortie plate** tirer de H^r_{k+1} : condensation à mcs (masses, **critère d'existence** d'un cluster : point
ouvert P4 du juge, cellule Q-Π2 « un point de bord ne fait pas exister un cluster »), **sélection** (excès de masse et
paramétrisation λ = r^(−z), feuilles, coût sur mesure du § 5.2 de la thèse, ε), **complétion** des points restés seuls
(par lignée, par vote, aucune) ; et comment la **juger équitablement** contre HDBSCAN (mêmes mcs, `min_samples` = k,
même machine, sélection appliquée aux deux hiérarchies, métriques, prédictions écrites d'avance).

## Mesures v11 disponibles (niveau B, meilleur bloc par objet, pas une partition)

Reçu `morsehgp3D_v11/receipts/developpement_20261003/points_g4/` ; § 7 de la note. Synthétique (128 scènes, 8 familles,
n = 2 000 et 8 000) : à n = 8 000, `margin_r` − HDBSCAN = +0,008 / +0,026 / +0,050 / +0,078 à k = 2 / 3 / 5 / 10 ; la
tour sans marge (`first`, `cover`) un peu au-dessus ; `core` en dessous de HDBSCAN. LiDAR (séquence 08, 32 k à 126 k
sites) : démos de `Zoltan/` rattrapées (vélos 55 et 56, voitures 342 et 352), une perte (démo 02, vélo 59) ; voisines
+19/−8 à +35/−1. Prédiction du juge (E1) : au niveau C, à mcs = k sur LiDAR, la qualification réduit la
sur-segmentation de `cover` ; c'est là qu'elle peut battre HDBSCAN.

## Historique v10 : DONNÉES, PAS AUTORITÉ (« le choix de la v10 n'était pas forcément pertinent »)

À critiquer, pas à reprendre : tête « entrée par couverture + EOM à z = 3 (λ = r^(−3), densité K-NN) » ; vote sur la
tour condensée `VC[coeur,W1]` (existence par taille de cœur |C ∩ X| ≥ mcs) ; « existence mûre » (maturité interpolée).
Mesures v10 (sous la hiérarchie `cover`, pas H^r_{k+1}) :
- synthétique, chaque côté réglé sur une grille de 120 configurations : tour 0,758–0,761 contre HDBSCAN 0,703–0,716 en
  mIoU ; mais sans tour, l'exposant z sur l'arbre de HDBSCAN donne déjà +0,039 ;
- 5 trames Zoltan : la tour **perd** (−0,018 à K = 5) ; à mcs = K : 5 732 clusters par trame contre 867 (« chaque boule
  de K points fonde une feuille de masse K qui passe le seuil ») ; à mcs = K, K = 5, synthétique : 0,388 contre 0,556 ;
- existence par taille de cœur : bat HDBSCAN à petit mcs (+0,110 à mcs = K, n = 2 000) mais échoue les deux triangles ;
- `filaments` et `anisotropic` veulent les feuilles, `shells` veut l'EOM ; aucune règle fixe ne prend les deux ;
- masses fractionnaires du § 9.1 : chaque triangle de T0 pèse 8/3 < 3, il n'est plus un cluster à mcs 3 ;
- scikit-learn HDBSCAN rend des étiquettes différentes selon la machine (ex aequo, `np.argsort` instable) : comparer
  toujours sur la même machine.
Sources v10 si utile : `morsehgp3D_v10/docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`, `docs/conception/CLUSTER_v2.md`,
`docs/conception/EVAL_v2.md`, `src/head/head.cpp`, `bench/synthetic/{metrics,methods}.py`.

## À lire

- `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md` (§ 5 sortie plate, § 7 mesures, § 8 ouverts) ;
- synthèse du juge précédent : `git show origin/main:morsehgp3D_v11/receipts/developpement_20261003/points_math/SYNTHESE.md`
  (§ 5 sortie plate, § 8 expériences décisives) ;
- réponses de l'auditeur : `morsehgp3D_v11/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` ;
- thèse, extraits texte : `contexte/these_ch4_4_et_ch5.txt` (déf. 19 excès de masse, § 5.1 l'arbre comme espace de
  résolution, § 5.2 « hacker » HDBSCAN), `contexte/these_9_1.txt` (partition stricte, masses, vote) ; texte complet
  `contexte/these_complete.txt` (parties I–II, à titre indicatif, sans autorité) ;
- `Zoltan/FoundationModel/` : `README.md`, `SPECIFICATION.md`, `MESURE.md` (ce que l'aval attend d'une segmentation) ;
- banc v11 : `morsehgp3D_v11/bench/points_radius.py` (règle), `points_hierarchy.py` (évaluateur, arbre de HDBSCAN par
  `HDBSCAN(...)._single_linkage_tree_`), `points_campaign.py`, `points_summary.py` ;
- oracle exact des petits nuages : `contexte/oracle_hierarchy.py` (H^r_{k+1} depuis l'étage A, n ≤ 12–14 ; voir sa
  docstring) ; `morsehgp3D_v11/reference/hgp11_ref/` (étage constructif : quelques dizaines de sites).

## Budget G4 (pour les plans de mesure)

Une session gardée offre environ 2 300 s de travail. Repères de la session F : 5 démos, 4 ordres, 5 règles et HDBSCAN :
440 s ; 72 trames voisines : 845 s ; 128 scènes synthétiques (2 000 et 8 000 sites) : 206 s, 24 tâches parallèles.

## Ajout du 4 octobre, 07 h 55 UTC : audit 3f285274b sur la sélection plate (à lire par vérificateurs et juge)

L'auditeur a relu ce chantier privé (copies datées de vos scripts) et publié quatre reçus, à lire par
`git -C /workspaces/E-HGP/build/v11-claude-20261003 show origin/main:morsehgp3D_v11/receipts/<nom>/README.md` :
`flat_selection_math_20261004`, `flat_selection_math_r2_20261004`, `flat_selection_contract_20261004`,
`flat_selection_evidence_20261004` ; conseil courant dans `morsehgp3D_v11/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`.
Points saillants (à vérifier, pas à croire) :
- masses = points ENGAGÉS (un par site entré), dates d'entrée e_i et plateaux atomiques intégrés à la condensation ;
  S(C) = somme sur i de (λ_sortie(i, C) − λ_naissance(C)) avec les dates de sortie du cluster CONDENSÉ (une petite
  branche fait sortir ses points plus tôt que e_i^(−z)) ; une seule grande branche prolonge, deux ou plus séparent ;
  égalité de score certifiée : garder le parent ; racine permise ou non, z, e = 0 et cohortes inactives font partie
  du contrat de comparaison ;
- témoin exact de neuf sites collinéaires A = {0,2,4}, B = {7,9,11}, D = {17,19,21} (k = 2, m = 3) : λ = 1/r et
  λ = 1/r³ choisissent des partitions différentes ; une translation de ±1 fait basculer l'EOM autour d'une égalité :
  la stabilité 3ε de la hiérarchie ne rend pas la sélection stable ;
- six points (vérité A de taille 4, B de taille 2) : le meilleur bloc par objet promet 5/6, aucune antichaîne ne
  dépasse 1/2 : le niveau B n'est pas atteignable par une partition ;
- EOM en flottant non exact (témoin : écart de 2^-70 inversé en binary64) : scores certifiés ou refus explicite ;
  un choix « forcé » au budget doit être compté, et une porte exacte exige zéro choix forcé ;
- équité : sklearn tel quel (adversaire principal, version de la VM, 1.7.2 épinglée sur G4 ; 1.9.1 en local) et,
  séparément, un bras de sélection commune N-aire appliqué aux deux hiérarchies ; une transcription de sklearn ne
  remplace jamais sa sortie officielle ; sur 2 400 configurations à ε = 0, la tête N-aire privée diffère de sklearn sur
  520 (plateaux) ;
- `mesure/metriques.py` : le « certificat dual d'optimalité » annoncé n'en est pas un (borne par maxima de lignes) :
  retirer la revendication ou fournir un vrai dual / solveur exact.
