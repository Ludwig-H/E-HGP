# Contre-audit B — correction du protocole v4 (WIP)

23 septembre 2026. Lecture du worktree développeur non commis, worker
SHA-256 `a6715ac8…`, session `c7f5f32f…`, nouvelle porte réelle
`probe_worker_contract.py` `d3b48060…`. Aucun nouveau GCP lancé par
l'auditeur. Le paquet R2 antérieur, `0b29b6c3`, reste définitivement
`worker_failed/probe_failed` malgré ses treize sorties brutes.

Le correctif va dans le sens requis : plan v2 et sonde v4 épinglent
explicitement `saturate_deep` et `q3_leaf` ; le worker distingue la chaîne
MEB et son tableau des compteurs entiers ; il arrête les cas suivants
après le premier `probe_failed` ; les sous-chronos ne peuvent plus
dépasser le total, qui est lui-même borné par le mur externe du cas.
Une porte CTest lance une **vraie** sonde native et compare le résultat
on/off et onze mutations du protocole. Ces améliorations sont encore du
WIP, ni versionnées ni qualifiées sur G4.

## Trois mutations encore acceptées

À cet état, `_tower_work` accepte des noms arbitraires d'entiers et ne
requiert que les deux champs spéciaux MEB. Sur une sortie factice v4
valide, `validate_probe` rend encore `complete_relative` après chacune
des mutations suivantes :

1. `meb_supports_by_size=[0]`, alors que la sonde native publie
   exactement quatre positions ;
2. ajout d'un champ inconnu `tower_work.extra=1` ;
3. suppression du compteur attendu `tower_work.records`.

Ces acceptations ne prouvent pas une erreur géométrique, mais empêchent
la réception autonome d'attester le schéma précis du travail FULL.
Définir le jeu exact des clés `tower_work`, la longueur exacte et le
domaine du tableau MEB, puis ajouter ces trois mutants à la porte réelle
et aux selftests. Les mutations déjà testées `saturate_deep` inversé et
`chain_total=0` sont correctement refusées.

## Préflight et coût G4

La nouvelle porte réelle est inscrite à CTest, mais ni le constructeur de
paquet, ni le contrôleur de session, ni le worker invité ne l'exécutent
obligatoirement avant les cas LiDAR. Une session payante pourrait donc
encore être lancée sans avoir passé précisément la porte qui aurait évité
R2. Exiger un reçu local sur le **snapshot commité** avant
`guarded_start`, ou exécuter ce petit cas dès le build invité avant le
premier cas coûteux ; l'échec doit clore la session avec capture et arrêt
ciblé. Les anciennes fausses sorties des selftests ne valent pas ce reçu.

Le script réel autorise 600 s à `subprocess.run` par cas, alors que son
CTest est plafonné à 300 s. Si CTest tue son parent, l'enfant natif peut
survivre. Fixer un délai interne inférieur au délai CTest et une fermeture
du groupe des processus ; un petit nuage de 360 sites ne justifie pas
une attente de plusieurs minutes. Cette précaution est distincte des
plafonds de recherche géométrique, qu'il ne faut pas réintroduire.

La vraie porte compare actuellement saturation/feuille **et** W1 contre
W2/FULL statique0 contre statique2. Elle vérifie utilement l'identité de
l'objet sur plusieurs voies, mais ne mesure pas causalement le seul
effet de q3 feuille. Pour une ablation de vitesse, faire des paires
identiques en entrée, W, s, K et politique FULL, en ne variant qu'un
levier à la fois ; inclure les coûts de coquille q3 et de balayage q4.
La porte actuelle exige cinq ordres et 1 000 boules, mais pas
`q3_leaf_censuses>0` ni une cellule saturée : son égalité on/off peut
être verte sans exercer les deux nouveautés visées. Ajouter des fixtures
non vacantes de chaque branche, dont la coquille q3 à quatre contacts
décrite dans le contre-audit B de la feuille.

Enfin, trois faiblesses plus anciennes demeurent hors de ce correctif :
le lecteur hôte accepte encore un `partial` avec zéro cas complet et code
de sortie 0, ne recalcule pas tous les temps/RSS résumés à partir des
fichiers bruts, et ne lie pas la provenance déclarée du reçu aux objets
Git lors de la réception. Aucune n'explique l'échec R2, mais elles
interdisent de lire tout `partial` comme preuve autonome de contrat.
