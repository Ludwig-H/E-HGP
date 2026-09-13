# Les colonnes exactes peuvent additionner leurs témoins

13 septembre 2026, contrelecture de `axis_q2` en cours de construction.
Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Le [reçu](AXIS_Q2_IDENTITY_CHECKS.json) épingle le snapshot
examiné ; aucun fichier du développeur n'est modifié.

## Verdict sur le filtre actuel

Avis favorable borné sur ses demi-espaces q2, seuils stricts et
représentation du résidu. Pour z dans une colonne exacte d'a suivant
l'axe d, les autres composantes de z−a sont nulles, donc :

$$H(a,b,z)=(z_d-a_d)(b_d-z_d).$$

Le h-ième successeur fournit h témoins lorsque b_d est **strictement**
au-delà de sa coordonnée ; l'égalité reste candidate. Le raisonnement est
symétrique pour les prédécesseurs. Leur nombre requis est Kmax moins le
cœur réellement certifié, dont les IDs sont extérieurs à A et B.

L'index B partitionne ses IDs dans une permutation. Une requête émet un
nœud entièrement contenu, ou descend dans ses deux enfants disjoints :
les fragments émis sont donc disjoints et couvrent exactement les sites
de B contenus dans la boîte fermée de l'ancre. Les cas d'acceptation et
de rejet de tout B suivent le même contrat, avec ou sans index construit.

Le commentaire du snapshot initial `axis_q2.cpp` (`2cea8416…`) justifiait
le refus d'additionner les axes par un recouvrement possible des
populations. **Pour ces colonnes exactes et une ancre fixée, leurs témoins
non-self sont au contraire disjoints.** Ce commentaire est depuis corrigé
dans le snapshot `db1b2343…` ; cette seule correction textuelle ne porte
pas le filtre additif. Le filtre conservant le maximum reste sûr.

La même preuve et un modèle d'interrogation additive de l'index sont
publiés par l'autre auditeur au
[§8 de sa note sur les sous-rectangles](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md),
commit `90d22425`. Notre apport complémentaire est le juge du C++ réel
ci-dessous : 131 plans et quatre mutants exécutés sur les octets épinglés.

## Addition sûre et fixture minimale exécutée

Si z appartient aux colonnes suivant deux axes différents passant par a,
alors z−a est parallèle aux deux axes, donc z=a. Cette identité ne peut
jamais être un témoin strict. Les deux rayons d'une même colonne sont
également disjoints. Avec c_d(a,b) le nombre de témoins de la colonne d,
on obtient donc le minorant supplémentaire :

$$p(a,b)\geq h_{\text{cœur}}+\sum_{d=1}^{3}c_d(a,b).$$

Les comptes peuvent être saturés au besoin global h :
$\min(h,\sum_d\min(h,c_d))=\min(h,\sum_dc_d)$.
Il faut toujours compter des IDs distincts, exclure a et conserver
l'exclusion parentale du cœur. Des colonnes approximatives, des comptes
non identifiés ou un mélange avec d'autres populations n'héritent pas
de cette preuve.

Fixture u16, séparée à s12 : A={(0,0,0),(0,1,0),(0,0,1)},
B={(100,2,2)}, Kmax=2 et cœur nul. Pour a=(0,0,0), les deux autres
sites d'A donnent chacun H=1, sur deux axes différents. Le filtre actuel
garde cette paire car aucun axe ne possède deux témoins ; leur somme
certifie exactement deux intérieurs et permettrait de la rejeter.
L'exécution conserve actuellement trois paires au total ; la proposition
additive en conserverait deux. Ce nouveau filtre additif n'est pas porté.

La preuve de disjonction vaut aussi pour des colonnes suivant des
directions exactes non parallèles. Une direction et son opposée doivent
représenter la même ligne ; des versions proportionnelles ne peuvent pas
être comptées comme deux populations. Ce point complète la
[piste des directions obliques](P0_AXES_ET_ROTATIONS.md).

## Gain calculable sur les grilles complètes

Pour deux nappes alignées portant la même grille complète, les témoins
axiaux appartenant à A donnent, pour les rangs (i,j) et (u,v) :

$$c_1+c_2=(|u-i|-1)_++(|v-j|-1)_+,\qquad x_+=\max(0,x).$$

Sans effet de bord, le résidu actuel a (2h+1)² positions par ancre. Le
résidu additif en a **2h²+6h+1** : 261 au lieu de 441 pour h10. En effet,
sur chaque axe le coût zéro possède trois positions, et chaque coût
strictement positif en possède deux. Additionner les produits dont la
somme des coûts est inférieure à h donne la formule.

Sur une grille R×S complète, définir f_R(0)=3R−2 et
f_R(k)=2 max(0,R−k−1) pour k≥1. La masse additive exacte est
Σ_{k+l<h} f_R(k)f_S(l). Les dimensions ci-dessous correspondent aux
grilles de la comparaison des rotations :

| n | Grille | Résidu axial actuel h10 | Résidu additif calculé |
| ---: | --- | ---: | ---: |
| 8 000 | 50×80 | 1 475 800 | 918 160 |
| 16 000 | 100×80 | 3 124 300 | 1 912 660 |
| 32 000 | 125×128 | 6 483 670 | 3 928 390 |

Ces dernières valeurs sont des calculs de formule, pas un port exécuté
ni un census des grands cas. La preuve ne se transfère pas aux lignes
tronquées. Elle n'utilise que les témoins d'A ; les anciens certificats
des nappes utilisant aussi B sont une autre comparaison.

Une réalisation devra payer la sélection des produits de classes de
crédits et leurs requêtes spatiales. Retirer des candidates ne garantit
pas de réduire le coût de construction ou le nombre de fragments.
L'exactitude et la complétude du census aval restent inchangées.

## Qualification indépendante

Le [juge C++](axis_q2_identity_probe.cpp) utilise directement H sur tous
les sites, sans appeler les prédicats, tris de colonnes ou index du
producteur pour construire son oracle. Il compare le maximum axial du
contrat actuel, les IDs développés et le census, puis vérifie séparément
la somme des populations identifiées.

Il passe 131 plans : 24 143 paires, 874 128 tests du census, 64
comparaisons sous permutations et déplacement des plages A/B ; 60 plans
construisent l'index et 36 ont leur seuil déjà satisfait par le cœur.
Les coordonnées irrégulières, les trois axes, leurs réflexions u16,
des propositions de cœur refusées et des points extérieurs non proposés
sont exercés. Les 2 898 tests de témoins à H=0 restent stricts.

Le snapshot passe C++20 strict en O2 et UBSan. Quatre copies C++ mutées
sont rejetées : frontière de queue fermée, enfant d'index répété,
colonne fausse et h-ième successeur décalé. Ces modifications sont
temporaires. Le reçu embarque le juge et l'ancien `axis_q2.cpp` ; ses six
autres sources proviennent du champ `snapshot_utf8` du
[reçu des rotations](AXIS_ROTATION_CHECKS.json), avec leurs SHA vérifiés
individuellement. Une reconstruction depuis ces seuls reçus reproduit
les 131 plans. Cette qualification reste attachée au snapshot initial,
indépendamment des changements ultérieurs du worktree.
Aucun résultat de tour FULL ou de temps n'est acquis.
GCP non utilisé.
