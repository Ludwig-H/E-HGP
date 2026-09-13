# Sonde P0 : un rectangle séparé, pas une tour HGP

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
