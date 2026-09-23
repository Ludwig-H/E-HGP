# D5 : fermer la preuve de naissance et certifier la composante des facettes

23 septembre 2026. Contrelecture de la [proposition D5](c_alternatives_20260923/propositions/D5.md)
et de sa [synthèse](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md),
à code produit inchangé. D5 est prometteuse pour réduire la tour FULL,
mais reste un sidecar : aucun encodeur/expanseur compact ni gain G4 D5
n'est qualifié. Les lemmes A et B avec la règle 0 ne sont pas réfutés ici.

## La borne basse manquante dans le lemme C

La preuve publiée dit qu'une boule B qui **naît** à l'ordre K est déjà
un bloc de K−1. Il faut éliminer explicitement le cas
`K = p_B + q_min(B) − 1`, borne basse de sa fenêtre : B n'est alors pas
programmée à K−1. Dans ce cas, une naissance est impossible. Pour une
boule régulière, `visit_block_at` émet ses `q_min` facettes strictes.
Pour une coquille étendue, `ShellTable::rank(K)` prend
`t = K−p_B = q_min−1` : aucune sélection de t sites de coquille ne
contient un support minimal, donc au moins une composante stricte et sa
facette sont émises. `prepare_block` a ainsi une racine parentale ; ce
bloc ne peut être une naissance. Une naissance vérifie donc
`K > p_B + q_min−1`, ce qui place B dans le programme K−1. La suite
de la preuve de C peut alors utiliser l'ancre inférieure. Cette ligne
doit précéder le statut `proved_here`, en gardant le refus produit
`full_ball_vertical_birth_anchor` comme garde.

## E1 doit prouver le lien facette–cible

La porte E1 proposée teste séparément `MEB(F)<λ`, l'existence de la
cible au catalogue, sa fenêtre, son niveau `<λ` et la prise de racine
à `λ⁻`. Ces conditions n'impliquent pas que **F et la cible sont dans la
même composante**. Contre-fixture exacte : quatre sites collinéaires
`0, 1, 10, 11`, ordre K2. Le bloc de la boule diamètre `(0,10)` a
`λ=25` en rayon carré et une facette `F={0,1}`. La boule diamètre
`(10,11)` est cataloguée, admise à K2 et de niveau `1/4<25`. Elle
passe les contrôles structurels si un résolveur la renvoie pour F,
alors que leurs composantes restent distinctes juste avant le rayon 5.
En effet, pour tout `r<5` assez proche de 5, les intervalles de centres
des paires `(0,1)`, `(1,10)` et `(10,11)` sont séparés ; les autres
paires reliant ces trois composantes n'apparaissent qu'à `r≥5`.
Ce mutant n'invalide ni le témoin central du lemme B ni le contrôle
global `same_payload` ; il montre que la porte **par facette** annoncée
ne juge pas la propriété qu'elle prétend isoler.

Porte constructive : garder la sortie FULL explicite et la validation
actuelle pendant le premier port de la règle 0 et du saut ; comparer
pour **chaque facette** sa racine pré-lot à celle de la route produit,
ou certifier chaque saut par le centre commun de B. Ajouter la fixture
ci-dessus comme mutant ciblé. Porter ensuite séparément le compact :
chaque exception doit conserver nœud cible, boule, masque de coquille,
drapeau intérieur et position du premier bloc du groupe ; l'expansion
doit être comparée champ par champ à `same_payload` sur `lat5_3` K8,
`lat5_1` K9 et `fx_ico12` sous ASan avant un reçu LiDAR/G4.
