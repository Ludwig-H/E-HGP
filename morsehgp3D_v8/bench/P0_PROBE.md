# Sonde P0 : un rectangle séparé, pas une tour HGP

Complément du 14 septembre, raccord WSPD q2 :
`mhgp8_wspd_q2_census_probe n famille Kmax s seed pure|samples pairwise|shared [none|sibling [global|complement [anchors|joint|joint-a [pool_min_factor]]]]`.
Les arguments facultatifs choisissent le certificat frère, l'ordre
des témoins, le partage du travail entre ancres, puis le seuil de taille
du filtre Pool terminal. Les schémas v1 à v4 restent disponibles ; le
dernier argument numérique produit v5, avec vingt-trois compteurs Pool
et deux temps imbriqués. Le détail des CLI,
reçus et coûts figure dans la section finale de cette note. Voir les
contrats du [certificat frère](../docs/P0_CERTIFICAT_FRERE_Q2.md) et de
l'[ordre des témoins](../docs/P0_ORDRE_TEMOINS_Q2.md), puis le contrat du
[census conjoint](../docs/P0_CENSUS_CONJOINT_Q2.md) et le
[raccord Pool terminal](../docs/P0_POOL_TERMINAL_RACCORD.md).

13 septembre 2026. Cadre `exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=implementation_v8_p0`, `public_status=not_claimed`.

Cette sonde compare `pool`, `dual` et `tubes` pour produire des crédits locaux et
des descripteurs de paires résiduelles, sur **un seul rectangle**. Elle
ne construit pas de WSPD, n'énumère pas ces paires et n'exécute ni census,
ni q3/q4 complet, ni tour FULL. Les voies 2/3/4 nomment ici les prédicats
du préfiltre, pas les générateurs complets. Aucun temps de cette sonde
ne valide le contrat 50k ou ne clôt P0.

## Invocation

```text
mhgp8_p0_probe n pool|dual|tubes 2|3|4 grid|sheet|skew|tube|rails kmax s
```

Une invocation effectue une mesure mono-thread, sans échauffement caché.
Les sources et commandes du build, la machine et les répétitions doivent
être épinglées par le runner ou le reçu de campagne. Cette note ne rapporte
aucune mesure exécutée. Succès : code 0 et un objet JSON sur une ligne.
Argument, géométrie ou exécution refusés : code 2, diagnostic sur stderr,
aucun résultat JSON de succès. Les gates doivent précéder la campagne.

Le [runner](run_p0_matrix.py) utilise des reçus de validation v2 : vérifier
chaque tuple commande/JSON, schéma, statut, périmètre, types, compteurs,
identité de fixture et partition des temps avant de compter un succès.
Il conserve stdout/stderr bruts et en base64, vérifie le binaire avant
et après chaque essai, puis les sources et le binaire à la fermeture.
Une campagne ferme en completed, invalid, failed ou interrupted ; un
JSON tronqué ne fait pas disparaître l'essai. Les annulations terminent
l'enfant ciblé, puis le tuent s'il ignore TERM ; ce n'est pas une durée
limite imposée aux mesures en cours normales. Les 41 scénarios de la
[gate de reçus](../tests/campaign_gate.py) passent en normal et sous −O.

## Familles déterministes, version 1

Tous les points sont distincts, d'identité égale à leur position d'entrée.
A précède B. Aucun site extérieur n'est proposé comme témoin universel :
la sonde isole le repli local. Il n'y a ni aléatoire ni graine implicite.
Le hash FNV-1a encode chaque coordonnée x, y, z en u16 little-endian dans
l'ordre original ; c'est un repère reproductible, pas un certificat.

| Famille | Construction | Domaine de la recette |
| --- | --- | --- |
| `grid` | Deux grilles 3D, A de taille n/2 arrondie vers le bas ; B reçoit le reste | Au plus 32³ sites par facteur |
| `skew` | Mêmes grilles, B de taille max(1, n/16 arrondi vers le bas), A reçoit le reste | Au plus 32³ sites par facteur |
| `sheet` | Deux grilles dans les plans x=1000 et x=60000, facteurs équilibrés | Au plus 256² sites par facteur |
| `tube` | Deux segments de sites entiers consécutifs, à partir de x=1000 et x=60000 ; y=z=1000 | Au plus 5536 sites par facteur, puis séparation effectivement vérifiée |
| `rails` | Contre-fixture de l'auditeur : neuf rails de 151 sites par facteur, écart transverse 600, translation 64 800 | Instance fixe n=2 718, coordonnées et géométrie inchangées |

Pour les cubes, le côté est le plus petit entier dont le cube contient
le facteur ; pour les feuilles, celui dont le carré le contient. Le
dernier plan ou la dernière ligne peuvent donc être incomplets. Les
coordonnées variables commencent à 1000, sauf x du facteur B à 60000.
Les parcours de grille sont x puis y puis z ; les feuilles sont y puis z.
Ces grilles comportent des plateaux : elles ne prétendent pas satisfaire
le régime régulier d'une future tour FULL.

`grid`, `skew` et `sheet` acceptent n=8 000/16 000/32 000 pour s=8/10/12.
`tube` est un contrepoint 1D borné : n=8 000 convient encore à ces trois
valeurs de s, mais n=16 000/32 000 est refusé par son domaine u16. Aucun
rabattement, ajout d'épaisseur, mélange de points ou abandon de la
séparation ne remplace silencieusement une fixture impossible.
Ces capacités sont les domaines explicites de recettes fixes ; ce ne
sont pas des quotas de troncature de l'algorithme.

La famille `rails` et la stratégie `tubes` sont deux choses différentes.
L'instance de rails est reprise explicitement de la
[preuve complémentaire](../../audits/morsehgp3D_v8_complementaire/P0_RAILS.md),
sans en hériter un temps ou une qualification moteur. À q4/Kmax10, la
formule attend 2 916 paires avec les crédits locaux complets. Le pool
global directionnel peut être très défavorable ; ce cas doit être conservé.

## Sens précis de s et des chronomètres

La préparation exige `box_gap >= s * max(box_diameter)`. Ici s est
**une précondition vérifiée**, pas un paramètre qui produit de nouveaux
rectangles. Pour une même famille et une même taille, s=8/10/12 donne les
mêmes coordonnées, lorsque la séparation passe. Comparer ces invocations
ne compare donc ni la taille ni le travail d'une WSPD Callahan–Kosaraju.
Cette comparaison globale reste à implémenter et à mesurer séparément.

Le JSON sépare `generation_ms` (allocation, génération et hash),
`prepare_ms` (validation et préparation du propriétaire immuable), puis
`plan_ms` (crédits et descripteurs). `total_component_ms` couvre ces trois
phases, pas le parsing, le rapport JSON ou l'aval absent. Les compteurs
entiers de préparation et du plan sont publiés séparément, avec leurs
sous-compteurs de prédicats ; leurs unités ne doivent pas être additionnées
comme si elles représentaient toutes la même opération.

`candidate_pairs` est calculé sans développer les produits décrits par
`candidate_descriptors`. Il reste un **volume de travail potentiel aval**,
pas un nombre de boules utiles ni de nœuds FULL. Les feuilles parallèles
peuvent laisser un résidu quadratique malgré un préfiltre rapide. Il faut
conserver et expliquer ce résultat négatif, puis payer le travail aval
avant de conclure à une meilleure architecture.

## Sondes appariées : partage et filtre axial

La deuxième tranche ajoute deux points d'entrée, décrits dans le
[contrat de partage et de filtrage](../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md) :

```text
mhgp8_batch_probe n pool|dual|tubes grid|sheet|skew|tube|rails kmax s baseline-first|batch-first
mhgp8_axis_probe n pool|dual|tubes grid|sheet|sheet_full|skew|tube|rails kmax s baseline-first|axis-first
```

Le batch partage la préparation de trois voies géométriques avec une
comparaison physique aux trois appels séparés. L'axe compare deux
préfiltrages q2 dont les résidus diffèrent ; il ne revendique pas une
égalité des plans. Chaque paire de mesures utilise le même propriétaire,
avec deux ordres d'exécution pour rendre visible l'effet de cet ordre.
L'inspection des plans est hors des temps de chaque bras et mesurée à part.
Les sorties restent compactes ; leur développement et le census sont absents.

`sheet_full` est une recette **v2 des sondes axe et additive**. Pour n pair,
m=n/2, sa largeur est le plus grand diviseur de m inférieur ou égal à
sa racine entière, et sa hauteur est m/largeur. Les deux plans x=1000 et
x=60000 portent la même grille y/z commençant à 1000. Les coordonnées
et la séparation sont vérifiées ; aucune rangée n'est tronquée. Les
recettes v1 précédentes et leurs empreintes ne changent pas.

`run_p0_matrix.py --probe-kind batch|axis` emploie les deux ordres par
défaut (`--orders` peut les expliciter). Les listes tailles/familles/K/s/
stratégies/répétitions définissent le produit cartésien enregistré.
`check_paired_campaign.py` relit les sous-dossiers de campagnes et leurs
sources épinglées ; `--summary` donne les médianes **par ordre**. Les
valeurs s restent des préconditions d'un rectangle fixe, pas une WSPD.
Les résumés exigent un build homogène par type de sonde et des métadonnées
machine communes ; la seule ligne volatile `cpu MHz` est exclue de cette
comparaison, pas de la capture brute. Cette provenance déclarée ne prétend
pas identifier de manière unique une machine physique. Les identifiants
de provenance figurent dans le résumé, au lieu de mélanger les builds.
Les anciens reçus r3 se rejouent sur leur commit `3589a2c9`, pas en
leur attribuant les nouvelles sources. Les lecteurs ne qualifient jamais
la géométrie ou la complétude de la tour.

## Comparaison additive et intersection

La troisième sonde conserve `Independent` comme référence et mesure
`Additive`, seul ou intersecté avec un plan local q2 :

```text
mhgp8_additive_probe n pool|dual|tubes grid|sheet|sheet_full|skew|tube|rails kmax s independent-first|variant-first additive|intersection
```

`variant_ms` inclut `local_plan_ms` puis `selection_ms`. Sans intersection,
le coût local vaut zéro et ses champs de résultat sont explicitement nuls.
Avec intersection, la copie des crédits et l'index de sélection sont
compris dans `selection_ms`. L'inspection des descripteurs reste séparée ;
elle ne développe pas les paires. La génération et le propriétaire sont
comptés une fois par total normalisé, sur le même propriétaire pour les bras.

Le runner existant accepte `--probe-kind additive` et `--variants additive
intersection` (ces deux variantes par défaut). Les deux nouveaux ordres
sont employés par défaut. Les variantes figurent dans le produit cartésien
du manifeste et dans les clés des résumés : leurs temps ne sont jamais
mélangés. La référence Independent doit rester identique entre variantes
et stratégies ; l'Additive sans restriction ne dépend pas de la stratégie
locale inutilisée. Les lecteurs vérifient aussi ces invariants croisés.

Les compteurs distinguent copies de crédits, scans de l'index, rejets
locaux, rejets par boîte axiale, bornes additives, recherches de rang et
fusion des plages. Les identités entre compteurs interdisent de masquer
un travail effectué derrière des zéros arbitraires. Elles ne constituent
ni un oracle géométrique ni une mesure de l'aval absent. Voir le
[contrat additif](../docs/P0_ADDITION_ET_INTERSECTION.md).

## Census q2 avec émission des intérieurs et coquilles

La quatrième sonde consomme vraiment le résidu et compare les recherches
individuelles au parcours partagé à curseur, sur le même propriétaire,
index global et préfiltre. Voir le [contrat](../docs/P0_CENSUS_Q2_PARTAGE.md).

```text
mhgp8_q2_census_probe n grid|sheet|sheet_full|skew|tube|rails kmax s independent|additive|intersection_pool pairwise-first|shared-first
```

La recette `sheet_full` v2 et les autres recettes v1 sont inchangées.
Le consommateur reçoit et calcule un digest de chaque support conservé,
de sa clé exacte et de tous les IDs intérieurs/coquille. Ces IDs sont
matérialisés pendant le callback, puis les buffers sont réutilisés.
Il n'y a ni catalogue global dédupliqué ni nœud FULL dans la sortie.
L'égalité des digests appariés ne remplace pas l'oracle physique des gates.

Le temps de chaque bras inclut génération/hash, copie et validation du
propriétaire, construction de l'index global, préfiltre, comptage, collecte,
digest et destructions. L'intersection paie aussi Pool. `query_index_ms`
mesure l'arbre de requêtes B construit seulement par Shared ; `payload_ms`
mesure collecte et callback ; `count_ms` est le reste du temps englobant,
y compris instrumentation et destruction des temporaires. L'inspection
diagnostique qui confronte les deux bras est publiée séparément. Parsing,
sérialisation JSON et capture externe ne font pas partie de ce total
de composant. Les deux ordres évitent d'attribuer au seul second bras
les effets d'un index déjà parcouru.

Le [runner spécialisé](run_q2_census_matrix.py) conserve les mêmes conventions
de sources/binaire épinglés, sorties brutes, erreurs et fermeture des matrices :

```bash
python3 -B morsehgp3D_v8/bench/run_q2_census_matrix.py run --probe build/v8_new/mhgp8_q2_census_probe --output morsehgp3D_v8/receipts/q2_new --sizes 8000 16000 32000 --families grid sheet_full --kmax 5 10 --s 8 10 12 --prefilters intersection_pool
python3 -B morsehgp3D_v8/bench/run_q2_census_matrix.py check morsehgp3D_v8/receipts/q2_new --summary
```

`--repeats` vaut 1 par défaut ; les deux ordres sont inclus. Employer un
répertoire de sortie neuf. Le lecteur contrôle aussi que tous les préfiltres
conservent les mêmes supports finaux à famille/n/K fixés, même si leurs résidus
diffèrent. Les comparaisons de K différents restent séparées. Le rejeu de
captures historiques demande leurs sources épinglées ; aucun schéma antérieur
n'est silencieusement requalifié en census exécuté.

Exception de filiation explicite pour les captures q2 du 13 septembre :
le runner original `311fce7f…` est conservé comme snapshot authentifié dans
leur reçu. Le lecteur corrigé impose les comptes de construction B et
refuse aussi les frères échoués avant création de MANIFEST. Il relit les
bruts sans les modifier, distingue les hashes capture/lecteur et exige
l'égalité de toutes les autres sources. Voir le
[détail des corrections et des mutants](../receipts/q2_census_20260913/README.md).

## Comparer deux révisions du census

Le comparateur `compare_q2_revisions.py` relit une référence explicitement
épinglée à `f4815cd42d572db6aef27ec73d100f52303fff26` et la version courante.
Il ne capture rien et ne remplace aucun hash dans les bruts historiques.
Chaque côté doit passer le lecteur complet avec son propre périmètre de
sources ; même l'exception historique de runner ci-dessus est désactivée
pour cette comparaison. La baseline se capture avec son runner et son
binaire épinglés, la candidate avec ses propres sources et son binaire.

```bash
python3 -B morsehgp3D_v8/bench/compare_q2_revisions.py morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/baseline morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/candidate --summary
python3 -B -O morsehgp3D_v8/bench/compare_q2_revisions.py morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/baseline morsehgp3D_v8/receipts/q2_prepared_bounds_20260914/candidate
```

Les matrices, nombres de répétitions, identités d'entrée, tous les
compteurs discrets et les digests de sortie doivent coïncider. Machine,
compilateur et options C++, de lien et IPO (y compris par configuration)
doivent être compatibles ; le comparateur vérifie
la provenance déclarée et les pins de fermeture, pas une reconstruction
du binaire. Les médianes restent séparées par ordre des bras. Un ratio
candidate/référence inférieur à 1 signifie plus rapide, un dénominateur
nul donne `null`. Les doublements n→2n conservent famille, K, s et filtre.
Ni intervalle de confiance ni borne de pire cas n'est déduit des médianes.

Les gates normal/−O emploient de petites captures effectives et des
mutations temporaires explicitement synthétiques, notamment pour vérifier
les médianes de trois répétitions ; ce ne sont pas des mesures de vitesse.
L'option de gate `--baseline-probe` permet aussi le différentiel réel avec
l'ancien binaire. Les futures modifications du moteur demanderont une
nouvelle filiation explicite, pas la réécriture de ces captures.

## Partage du nuage et de l'index entre rectangles

La sixième sonde partitionne A en R bandes contiguës non vides et conserve
B entier. Elle couvre une fois A×B, sans générer de WSPD. Voir le
[contrat de propriété et de coûts](../docs/P0_NUAGE_ET_INDEX_PARTAGES.md).

```text
mhgp8_cloud_reuse_probe n grid|sheet_full|skew kmax s rectangles fresh-first|shared-first
```

Le bras `fresh` prépare R fois le nuage et Z ; `shared` les prépare une
fois. Les deux paient le même chemin Pool ∩ Additive, le census individuel,
la collecte et le digest des supports. Un contexte est actif à la fois ;
les destructions sont payées. Le temps de composant exclut la comparaison
des bras et le rapport JSON, pas les copies ni l'émission du payload.
Les capacités nuage/index sont distinctes du pic mémoire total non mesuré.

```bash
python3 -B morsehgp3D_v8/bench/run_cloud_reuse_matrix.py run --probe build/v8_new/mhgp8_cloud_reuse_probe --output morsehgp3D_v8/receipts/cloud_new/main_matrix --sizes 8000 16000 32000 --families grid sheet_full skew --kmax 10 --s 8 10 12 --rectangles 32
python3 -B morsehgp3D_v8/bench/run_cloud_reuse_matrix.py check morsehgp3D_v8/receipts/cloud_new --summary
```

Les deux ordres sont capturés, puis résumés séparément. Le lecteur refuse
les campagnes incomplètes, les changements de sources/bruts/binaire,
les provenances hétérogènes et les sorties incohérentes entre partitions.
Il vérifie notamment `restriction_credit_copies = |A|+R|B|` : ce travail
local persiste malgré le partage global. Ajouter des campagnes séparées
avec R proportionnel à n, par exemple (8k,32), (16k,64), (32k,128), pour
ne pas dissimuler cette croissance. Aucun ratio local ne qualifie une tour
ou une complexité générale ; s reste une précondition des rectangles.

## Premier front WSPD réel, sans census

La septième sonde emploie l'index spatial v8 partagé et la convention
`box_gap_diameter_v1`, différente du s v4. `pure` couvre toutes les paires ;
`samples` propose au plus Kmax sites par produit et certifie séparément
les rejets q2/q3/q4 avant séparation. Aucun plan local n'est préparé.

```text
mhgp8_wspd_front_probe n uniform|terrain|clusters|rows Kmax s seed pure|samples
```

Les recettes uniformes, terrain mince et huit amas utilisent SplitMix64
et une unicité contrôlée ; génération et comparaisons sont comptées.
`rows` contient deux rangées parallèles, n pair jusqu'à 131072 (limite
physique des coordonnées), et ignore explicitement seed. Ce domaine de
recette n'étend pas la condition mathématique de résidu q3/q4 démontrée
pour les tailles 8k/16k/32k. Une entrée invalide retourne 2, une exception
d'exécution 1, le succès une seule ligne JSON et 0.

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_front_matrix.py run --probe build/v8_new/mhgp8_wspd_front_probe --output morsehgp3D_v8/receipts/front_new/main_matrix --sizes 8000 16000 32000 --families uniform terrain clusters --kmax 10 --s 8 10 12 --seeds 3 --modes pure samples --repeats 1
python3 -B morsehgp3D_v8/bench/run_wspd_front_matrix.py check morsehgp3D_v8/receipts/front_new --summary
```

Le lecteur compare les entrées et masses initiales entre modes, pas leurs
descripteurs résiduels. Répéter exactement un tuple impose le même travail
discret et le même digest. Le temps inclut génération, préparation unique,
parcours, checksum synchrone par descripteur, contrôles et destructions ;
`front_and_callback_ms` n'est pas le front seul. Les capacités retenues
ne sont ni le pic RSS ni une mesure GPU. Les produits restent compacts :
aucun census, support, catalogue ou parent FULL n'est construit. Lire le
[contrat et la preuve de couverture](../docs/P0_FRONT_REEL.md).

## Raccord WSPD et census q2 de tout le nuage

```text
mhgp8_wspd_q2_census_probe n uniform|terrain|clusters|rows Kmax s seed pure|samples pairwise|shared [none|sibling [global|complement [anchors|joint|joint-a [pool_min_factor]]]]
```

Même recette d'entrée que le front, mais **q2 seul**, suivi de son census,
de la collecte complète et du hash canonique des supports avec populations.
Les deux modes census voient les mêmes candidates pour un front donné.
Les combinaisons permises et les s doivent donner les mêmes supports à
entrée/Kmax identiques. Aucun catalogue dédupliqué ni FULL n'est construit.

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py run --probe build/v8_new/mhgp8_wspd_q2_census_probe --output morsehgp3D_v8/receipts/front_q2_new/main_matrix --sizes 8000 16000 32000 --families uniform terrain clusters rows --kmax 10 --s 8 10 12 --seeds 3 --modes samples --census-modes shared
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_new --summary
```

Le runner conserve aussi les échecs et vérifie la fermeture des sources
et du binaire. Répéter un tuple impose compteurs et digest identiques,
mais pas le temps. `input_descriptors` compte les rectangles ;
`anchor_queries` compte la somme des petits facteurs ; aucun arbre B ni
scan de couverture local n'est payé. Le total inclut le coût du callback
(copies, tris, validation, hash) ; les sous-chronos sont imbriqués.
Lire le [contrat complet](../docs/P0_FRONT_ET_CENSUS_Q2.md).

### Schémas v1 à v5 : des options explicites

Le suffixe du schéma suit les arguments effectivement fournis,
pas seulement leur effet : `none global` produit v3 et
`none global anchors` produit v4 et `none global anchors 0` produit v5,
même si Pool reste désactivé. Les préfixes
sont `mhgp8_wspd_q2_census_probe_` pour la sonde et
`mhgp8_wspd_q2_campaign_` pour la campagne.

| Arguments facultatifs de la sonde | Options facultatives du runner | Version et champs ajoutés |
| --- | --- | --- |
| Aucun | Aucune | v1, ni certificat frère ni ordre explicite |
| `none` ou `sibling` | `--sibling-modes none sibling` | v2, `sibling_mode` et `sibling_work` |
| `none\|sibling global\|complement` | `--sibling-modes none sibling --witness-orders global complement` | v3, champs v2 plus `witness_order` et `order_work` |
| `none\|sibling global\|complement anchors\|joint\|joint-a` | Options v3 puis `--anchor-modes anchors joint joint-a` | v4, champs v3 plus `anchor_mode` et `joint_work` |
| Arguments v4 puis entier `pool_min_factor` | Options v4 puis `--pool-min-factors 0 64` | v5, champs v4 plus `pool_min_factor` et `pool_work` |

Chaque liste du runner peut ne contenir qu'une des valeurs permises.
`--witness-orders` exige `--sibling-modes` explicite, même pour `none`.
`--anchor-modes` exige `--witness-orders` explicite, même pour `global`.
Sans `--anchor-modes`, aucun champ joint n'est ajouté aux anciens schémas.
`--pool-min-factors` exige `--anchor-modes` explicite ; omettre cette
option conserve les schémas antérieurs, sans champs Pool supplémentaires.
`sibling`, `complement`, `joint` et `joint-a` exigent
`--census-modes shared` ;
une matrice comportant une combinaison interdite est refusée, jamais
filtrée silencieusement. La sonde refuse aussi une valeur inconnue ou
un argument supplémentaire (code 2, sans JSON de succès). Une exception
d'exécution donne le code 1 ; un succès donne 0 et une ligne JSON.

`global` conserve le parcours DFS de référence. Pour une ancre
individuelle, `complement` visite d'abord les témoins hors du facteur B
initial, sans l'ancre a, puis B.
L'ancre contribue zéro au compte d'intérieur ; elle reste dans la
collecte de la coquille. Le B initial et l'ordre ainsi défini restent
fixes pendant les subdivisions de la requête. Ce choix ne change ni
l'index ni le front ; il peut changer la quantité de travail du census.

### Compter aussi les déplacements structurels

`order_work` contient quatre entiers non négatifs. Ils sont tous nuls
avec `global`, y compris lorsque celui-ci est explicitement demandé.
Pour `complement`, le lecteur vérifie les bornes suivantes,
avec T = `census_work.query_tasks`.

| Compteur | Travail compté | Borne vérifiée |
| --- | --- | --- |
| `structural_splits` | Descentes nécessaires pour isoler B initial ou l'ancre, avant les tests géométriques | Au plus 96T |
| `deferred_skips` | Sauts de B initial pendant la première phase, sans consommer ses sites | Au plus T |
| `anchor_skips` | Reconnaissances de l'ancre comme contribution intérieure nulle | Au plus T |
| `phase_switches` | Passages du complément vers B initial | Au plus T |

`count_node_visits` et `witness_splits` gardent leur sens géométrique ;
les descentes structurelles ne sont pas cachées dans ces champs.
`cursor_advances` inclut aussi les mouvements structurels, sauts et
changements de phase. `sibling_work` conserve séparément ses six comptes
de propositions et certifications. Ces compteurs ont des unités et des
recouvrements différents : une baisse des seules visites géométriques
ne suffit pas à conclure à une baisse du travail total. Les bornes par
tâche ci-dessus ne bornent pas T et ne prouvent pas une complexité
globale sous-quadratique.

Les chronomètres ne changent pas : `pipeline_total_ms` englobe le front,
le census et la destruction des buffers privés ; il est décomposé en
`front_and_count_ms` et `payload_ms`. Ce dernier comprend collecte et
callback. `query_index_ms` reste nul. Le `total_ms` extérieur paie aussi
génération, préparation du nuage, index global, validation et destructions.
Il n'y a ni chronomètre de front isolé dans ce raccord ni temps census
isolé à reconstruire par soustractions de mesures par rectangle.

### v4 : partager les ancres avant le relais individuel

`anchors` conserve le chemin historique : une requête initiale par ancre
du petit facteur A. Tous les champs de `joint_work` sont alors nuls.
`joint` commence par un produit A×B et teste les blocs de témoins Z pour
toutes ses paires à la fois. Si le test reste indécis, il peut subdiviser
A ou B. `joint-a` utilise les mêmes bornes, mais ne subdivise que A tant
que celui-ci contient plusieurs sites ; B reste entier à cette étape.

Lorsqu'A devient un singleton, les deux modes conjoints transmettent
le compte acquis, le curseur non consommé et la phase au parcours
individuel existant. Ils ne recommencent ni le comptage ni l'index.
Avec `complement`, le B différé reste le B du rectangle initial.
Le groupe A n'est jamais retiré du comptage : une autre ancre peut être
un témoin intérieur. Seule l'ancre devenue individuelle peut ensuite
être reconnue comme contribution nulle. Collecte des intérieurs,
coquilles et callback restent payés pour tous les supports acceptés.

Les vingt champs de `joint_work` décrivent uniquement l'étape conjointe,
y compris son action terminale de relais :

| Champs | Sens |
| --- | --- |
| `root_products`, `tasks`, `max_depth` | Rectangles initiaux, tâches conjointes et profondeur observée de leurs subdivisions |
| `splits_a`, `splits_b`, `splits_after_credit` | Subdivisions des facteurs, dont celles après acquisition d'un compte positif |
| `bound_tests`, `witness_splits` | Bornes géométriques sur A×B×Z et descentes géométriques dans Z |
| `cursor_advances`, `structural_splits`, `deferred_skips`, `phase_switches` | Mouvements de continuation, dont ceux imposés par le report de B |
| `consumed_witness_sites`, `credit_events`, `credited_pair_mass` | Sites résolus, blocs crédités et somme des masses de paires bénéficiant de ces crédits |
| `singleton_handoffs`, `handoffs_after_credit`, `handoff_pair_mass` | Relais individuels, relais après crédit et masse des paires confiées à ces relais |
| `rejected_pairs`, `accepted_pairs` | Paires décidées avant relais individuel |

`credited_pair_mass` compte une masse à chaque événement de crédit :
une même paire peut contribuer plusieurs fois. Ce n'est pas une
population dédupliquée ni une masse à ajouter aux paires terminales.
Les visites de comptage `census_work.count_node_visits` et leurs
sous-tests, `census_work.witness_splits` et les quatre champs
`order_work` comptent seulement le parcours individuel après relais.
`sibling_work` compte les certificats testés lors des subdivisions B de
ce parcours, jamais les subdivisions conjointes. Les comptes globaux
d'acceptation, de rejet et de payload incluent en revanche les deux
étapes ; `count_root_starts` compte les rectangles initiaux.

Le lecteur vérifie ces identités en mode conjoint, avec S égal à
`joint_work.splits_a + joint_work.splits_b` :

- `joint_work.root_products = input_rectangles = census_work.count_root_starts` ;
- `joint_work.tasks = joint_work.root_products + 2S` ;
- `census_work.query_tasks = joint_work.singleton_handoffs + 2*census_work.query_splits` ;
- les masses jointes `accepted_pairs + rejected_pairs + handoff_pair_mass` donnent exactement `candidate_pairs` ;
- `joint_work.cursor_advances + S = joint_work.bound_tests + joint_work.structural_splits + joint_work.deferred_skips + joint_work.phase_switches`.

Avec `global`, les trois derniers compteurs structurels de cette
identité sont nuls. `joint-a` impose en outre `splits_b = 0` et
`singleton_handoffs <= anchor_queries`. En mode conjoint, `anchor_queries`
reste la somme descriptive des petits facteurs, pas le nombre de
recherches réellement lancées. `joint` peut confier plusieurs groupes
B différents à une même ancre : son nombre de relais n'a donc pas cette
borne. Les anciens contrôles de racines par ancre restent inchangés
pour v1/v2/v3 et v4 `anchors` ; ils ne sont pas appliqués aveuglément
aux nouveaux comptes conjoints.

La validation n'impose aucun plafond arbitraire à `bound_tests` ou
`max_depth`. Comparer les deux étapes, leurs subdivisions, leurs relais
et le payload reste nécessaire ; un compteur individuel plus faible
ne prouve pas un gain total. Aucun chronomètre n'isole artificiellement
le temps conjoint du temps après relais : les temps englobants décrits
plus haut restent la référence.

### v5 : Pool sur les rectangles terminaux sélectionnés

`pool_min_factor=0` désactive Pool. Un entier positif sélectionne les
rectangles dont le plus grand facteur atteint cette taille ; 64 est
un seuil de comparaison, pas un optimum ni un quota de troncature.
Tous les rectangles non sélectionnés gardent le census, l'ordre et le
mode d'ancres demandés. Les combinaisons d'options interdites restent
refusées même si le seuil sélectionne tous les rectangles.

Le plan Pool est préparé une seule fois par rectangle sélectionné,
sur le propriétaire et l'index globaux. Il regroupe les crédits en au
plus K bandes disjointes de paires. Seules les survivantes de ces bandes
sont développées, puis comptées individuellement contre tout le nuage,
avec compte initial nul : les crédits ne sont jamais préchargés dans
le census. Aucun arbre de requêtes local n'est construit.

Si le plan conserve toutes les paires, il ne sert pas à les développer
individuellement : le rectangle reprend son parcours d'origine. Ce
retour, nommé `passthrough`, évite notamment de remplacer un parcours
partagé efficace des rangées par une expansion de tout leur produit.
Le plan inutile reste payé et compté ; il n'est pas effacé des mesures.

`pool_work` distingue les populations, le coût du plan et le parcours
effectivement choisi :

| Champs | Sens |
| --- | --- |
| `selected_rectangles`, `selected_pairs`, `factor_sites`, `original_selected_anchors` | Plans préparés, masse initiale sélectionnée, somme F des tailles des facteurs et ancres initiales concernées |
| `residual_pairs`, `filtered_pairs` | Masse conservée par ces plans, passthrough compris, et masse éliminée par certificat |
| `passthrough_rectangles`, `passthrough_pairs`, `passthrough_anchors` | Rectangles sans aucune élimination et populations rendues au parcours d'origine |
| `selection_tests`, `pool_selected`, `pool_insertions`, `pool_shifted_entries` | Comparaisons de scores et opérations de sélection des propositions Pool ; `selection_tests` ne compte pas la politique de taille |
| `witness_attempts`, `universal_queries`, `q2_axis_terms` | Propositions essayées et travail de certification géométrique |
| `factor_read_visits`, `grouping_visits`, `prefix_class_visits` | Relectures des facteurs, regroupement et construction des préfixes |
| `bands`, `selected_anchors`, `pair_roots` | Bandes et ancres effectivement développées hors passthrough, puis recherches individuelles lancées |
| `plan_peak_bytes` | Maximum des octets de plan déclarés, pas un pic RSS/VRAM de tout le programme |

Le lecteur impose `factor_read_visits = grouping_visits = 2F`,
`prefix_class_visits = (K+1)*selected_rectangles` et au plus K bandes
par rectangle. Pool désactivé, ou aucun rectangle sélectionné, implique
zéro pour les vingt-trois compteurs et les deux temps Pool.

Les masses se conservent selon
`front_residual = candidate_pairs + pool_work.filtered_pairs`,
`selected_pairs = residual_pairs + filtered_pairs` et
`pair_roots = residual_pairs - passthrough_pairs`.
`accepted_pairs + rejected_pairs = candidate_pairs` concerne ensuite
le census, pas les rejets déjà certifiés par Pool.

En Shared avec ancres individuelles, les racines comptées valent
`anchor_queries - original_selected_anchors + passthrough_anchors + pair_roots`.
En mode conjoint, `joint_work.root_products` vaut
`input_rectangles - selected_rectangles + passthrough_rectangles` ;
les racines génériques ajoutent `pair_roots` à ce nombre. La masse
décidée ou relayée par l'étape conjointe exclut seulement les recherches
Pool réellement développées, donc vaut `candidate_pairs - pair_roots`.
Les tâches génériques valent alors
`singleton_handoffs + pair_roots + 2*census_work.query_splits`.
En Pairwise, les racines restent exactement `candidate_pairs`.
Ces adaptations sont réservées à v5 ; les identités v1 à v4 restent
inchangées pour leurs propres reçus.

`pool_work.preparation_ms` mesure le plan ; `selected_total_ms` englobe
le traitement sélectionné, y compris census, callback, destruction du
plan et éventuel passthrough. Ces temps sont inclus dans le temps global
et se recouvrent avec `payload_ms` : **ne pas les additionner**.
Le lecteur ne les traite pas comme du travail entier reproductible.
Il conserve les vingt-trois compteurs sous `pool_work` dans le résumé
et publie leurs deux médianes temporelles sous `median_pool_timings`.
Une baisse de résidu ne prouve encore ni un coût total sous-quadratique
ni une qualification de tour FULL ; F, front, petits rectangles et
sorties restent à mesurer ensemble.

### Exemples v3/v4/v5 et lecture comparative

Exemple de petite capture, à exécuter dans un répertoire neuf après
construction et validation du binaire ; ces commandes ne constituent
pas un résultat de mesure :

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py run --probe build/v8_new/mhgp8_wspd_q2_census_probe --output morsehgp3D_v8/receipts/front_q2_order_new/main_matrix --sizes 64 --families uniform rows --kmax 5 10 --s 8 10 12 --seeds 3 --modes samples --census-modes shared --sibling-modes none sibling --witness-orders global complement --repeats 1
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_order_new --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_order_new --summary
```

Pour comparer les trois modes d'ancres en v4, conserver les mêmes
entrées, K, s, certificats et ordres :

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py run --probe build/v8_new/mhgp8_wspd_q2_census_probe --output morsehgp3D_v8/receipts/front_q2_joint_new/main_matrix --sizes 64 --families uniform rows --kmax 5 10 --s 8 10 12 --seeds 3 --modes samples --census-modes shared --sibling-modes none sibling --witness-orders global complement --anchor-modes anchors joint joint-a --repeats 1
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_joint_new --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_joint_new --summary
```

Exemple v5, avec Pool désactivé puis seuil 64 et le même parcours de
repli pour les petits rectangles ou les plans sans élimination :

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py run --probe build/v8_new/mhgp8_wspd_q2_census_probe --output morsehgp3D_v8/receipts/front_q2_pool_new/main_matrix --sizes 128 --families clusters rows --kmax 5 10 --s 8 10 12 --seeds 3 --modes samples --census-modes shared --sibling-modes sibling --witness-orders complement --anchor-modes anchors --pool-min-factors 0 64 --repeats 1
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_pool_new --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_pool_new --summary
```

Les campagnes de croissance utilisent séparément les tailles
`8000 16000 32000` et les quatre familles, sans confondre cette petite
capture de contrôle avec leur qualification. Le lecteur peut réunir
des captures v1/v2/v3/v4/v5 de provenance compatible et de sources épinglées
conformes ; cela ne réattribue pas les sources courantes aux anciens
reçus. Les clés de résumé incluent chaque option effectivement présente.
À entrée/Kmax/front/s identiques, changer le certificat frère, l'ordre
ou le mode d'ancres doit préserver exactement le front et, à seuil Pool identique, les
candidates. Changer le seuil Pool peut modifier les candidates du census,
jamais le front ni le digest canonique des supports. Celui-ci
doit aussi rester identique entre les ordres, modes et s.
Les compteurs de parcours peuvent différer ; les répétitions d'un même
tuple complet doivent retrouver le même travail discret. La validation
v5 réemploie les contrôles antérieurs avec les seules adaptations
explicites de masses, racines, tâches et visites nécessaires aux modes
conjoints et au filtrage Pool.

Le périmètre reste `q2_all_cloud_supports_not_full`, mono-thread CPU,
`public_status=not_claimed`. Ni les schémas, ni les digests, ni les bornes
de compteurs ne qualifient une tour FULL, le contrat 50k en une seconde,
le GPU ou plusieurs dizaines de millions de points. Aucun chiffre de
performance de cette tranche n'est ajouté ici pendant sa qualification.
