# Découpe de `tower_s` : le calendrier séquentiel est le vrai mur

11 septembre 2026, second auditeur (session e-hgp-c6), sur `49b793be`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Aucune source active modifiée : l'instrumentation vit dans un
arbre isolé obtenu par `git archive`.

Ma [note précédente](NOTE_CLAUDE_COEUR_MEB_20260911.md) signalait que `tower_s`
est chronométré en un seul bloc et que sans découpe la loi d'Amdahl est
inapplicable. J'ai posé les chronomètres. Voici le chiffre.

## 1. La découpe, à deux échelles

Trois chronomètres dans `build_full_ball_tower` : autour de
`prepare_static_order()`, autour de la boucle des lots, et du début de la
construction jusqu'à l'entrée de la boucle des ordres. Exécution avec
`--static-threads=1`, qui hisse toute la géométrie dans `prepare_static_order`
et rend les deux coûts discernables. Le chemin mono par défaut les entremêle,
et c'est pourquoi ils étaient jusqu'ici indiscernables.

| n | `tower_s` | géométrie | calendrier | prologue | épilogue |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 63,97 s | 40,15 s (62,8 %) | 10,77 s (16,8 %) | 9,84 s (15,4 %) | 3,21 s (5,0 %) |
| 16 000 | 140,45 s | 85,68 s (61,0 %) | 25,50 s (18,2 %) | 21,83 s (15,5 %) | 7,43 s (5,3 %) |

Contrôle de robustesse : une exécution antérieure à 8 000, sur machine plus
chargée, donnait `tower_s` à 83,10 s au lieu de 63,97 s, soit 23 % d'écart
absolu, mais la même fraction géométrique de 62,8 %. Les proportions sont
stables quand les temps ne le sont pas.

## 2. Le plafond d'Amdahl, et sa dégradation avec la taille

La géométrie est démontrée séparable, donc parallélisable. Le calendrier, qui
ferme les lots par niveau et fait l'union-find sur les racines pré-lot, est
séquentiel par construction. Le prologue valide le catalogue boule par boule et
paraît parallélisable ; l'épilogue construit la banque immuable.

| n | géométrie seule parallélisée | géométrie et prologue |
| ---: | ---: | ---: |
| 8 000 | 2,69x | 4,58x |
| 16 000 | 2,56x | 4,26x |

### Et le plafond empire quand on optimise le mono-thread

Mesure refaite avec les mêmes chronomètres, mais **sur la variante MEB réparée**,
à n=8000. J'avais annoncé cette dégradation avant de la mesurer ; elle est
confirmée.

| phase | moteur d'origine | après noyau MEB |
| --- | ---: | ---: |
| géométrie | 40,15 s (62,8 %) | 22,72 s (**49,5 %**) |
| calendrier | 10,77 s (16,8 %) | 10,26 s (22,3 %) |
| prologue | 9,84 s (15,4 %) | 9,70 s (21,1 %) |
| épilogue | 3,21 s (5,0 %) | 3,25 s (7,1 %) |
| `tower_s` | 63,97 s | 45,92 s |
| plafond, géométrie seule | 2,69x | **1,98x** |
| plafond, géométrie + prologue | 4,58x | **3,40x** |

Le calendrier et le prologue sont inchangés en valeur absolue, ce qui vérifie que
l'instrumentation mesure bien la bonne chose : je n'ai touché qu'au noyau
géométrique, qui passe de 40,15 à 22,72 s, soit 1,77x.

**Conséquence contre-intuitive à retenir.** Optimiser proprement le mono-thread
rend le parallélisme **moins** rentable, puisque la part parallélisable rétrécit.
Les deux leviers ne s'additionnent pas, ils se disputent le même gisement. Il ne
faut donc pas promettre le produit de leurs facteurs.

**Le plafond baisse quand n monte.** En doublant n, la géométrie croît d'un
facteur 2,134 et le calendrier d'un facteur 2,367, soit des exposants locaux de
1,094 et 1,243. Le calendrier croît donc plus vite que la part parallélisable,
et prend mécaniquement le dessus à l'échelle. C'est l'inverse de ce qu'on
attend d'une piste de parallélisation.

## 3. Conséquence chiffrée pour 50k

En appliquant les proportions à 16 000 au `tower_s` **mesuré** de 389,7 s à 50k,
K1..10, et en y composant les deux leviers disponibles. Ceci est une allocation
indicative, pas une mesure à 50k.

| étage | aujourd'hui | après noyau MEB | après parallélisme |
| --- | ---: | ---: | ---: |
| géométrie | ~238 s | ~138 s | ~7 s |
| prologue | ~60 s | ~60 s | ~3 s |
| calendrier | ~71 s | ~71 s | ~71 s |
| épilogue | ~21 s | ~21 s | ~21 s |
| total | ~390 s | ~290 s | **~102 s** |

Le gain retenu ici est celui **mesuré de bout en bout sur le flux réel**, soit
1,72x sur la phase géométrique et 1,357x sur la tour entière, à `payload_digest`
identique : voir le § 4bis de la [note MEB](NOTE_CLAUDE_COEUR_MEB_20260911.md).
Le 4,12x du banc ne se transporte pas.

Deux lectures s'imposent. D'abord, les deux leviers réunis donnent environ 3,8x,
ce qui laisse le contrat d'une seconde à **environ cent fois**. Ensuite, et c'est
le point structurel, après ces leviers **90 % du temps restant est le calendrier
et l'épilogue**, tous deux séquentiels. Le goulot se déplace complètement.

Point notable : ce total vaut 102, 98 ou 104 secondes selon que le noyau MEB
gagne 1,357x, 4,12x ou 1,33x, parce que la géométrie finit parallélisée de toute
façon. **La conclusion stratégique ne dépend donc pas du sort de mon prototype**,
et elle a survécu à sa réfutation, à sa réparation et à sa mesure réelle.

## 4. Ce que cela dit de l'ordre des travaux

Le noyau MEB d'abord, parce que son gain mesuré de 1,357x porte sur les 61 à 63 %
géométriques et ne rencontre aucun plafond d'Amdahl, et parce qu'il améliore
aussi le mono-thread. Le prologue ensuite, dont la parallélisation fait passer
le plafond de 2,56x à 4,26x à 16 000 : le laisser séquentiel, c'est abandonner
15 % gratuitement.

Puis vient le mur. Le calendrier séquentiel devient dominant, et il croît plus
vite que le reste. Le constructeur l'a déjà vu qualitativement, en écrivant
qu'accélérer les seules descentes ne supprime pas le coût des fusions CPU ; sa
[voie par lots](../docs/PARALLELISATION_PAR_LOTS_20260911.md) rend d'ailleurs
une identité de boule terminale et jamais un identifiant union-find, ce qui est
exactement la séparation sur laquelle repose ma découpe. Ce que cette note
ajoute est le **chiffre** du plafond, pas l'intuition. C'est aussi l'objet de la
[proposition de graphe filtré](../docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md),
qui remplace la dépendance temporelle des ancres par un graphe statique, et de
sa [réduction aux naissances](receipts_filtered_graph_20260911/README.md). Cette
piste ne vient pas en premier dans l'ordre d'exécution, mais elle vise le seul
obstacle que ni l'optimisation du noyau ni le parallélisme ne peuvent lever.

## 5. Sur le contrat lui-même

À K1..10 et 50k, la tour retient 27,3 M nœuds, 27,3 M références de parents,
16,5 M contributions et 27,2 M références verticales. Les émettre en une seconde
exige près de cent millions d'enregistrements par seconde avant tout calcul
géométrique. À K1..5, la tour retient 4,2 M nœuds pour 27,2 s aujourd'hui ;
la même composition de leviers la placerait autour de sept secondes.

Le constructeur énonce déjà ses contrats avec un « repli K1..5 si nécessaire » :
je ne revendique donc aucune nouveauté sur ce point, seulement une confirmation
chiffrée. Une seconde à K=10 avec sortie explicite n'est pas atteignable par ces
leviers ; K=5 est à un seul ordre de grandeur et constitue la cible défendable.

## 6. Limites déclarées

Les mesures sont à 8 000 et 16 000 sur cette machine, en une exécution par
configuration, avec `--static-threads=1`. Le 389,7 s de référence à 50k vient
d'une autre machine et d'une version antérieure du moteur : l'allocation du § 3
transporte des proportions, elle ne prédit pas un temps. Les facteurs de
parallélisme supposent une efficacité que personne n'a mesurée, et les lignes
statique1 contre statique4 des reçus n'en sont pas une : leurs écarts de MEB
valent exactement les MEB évitées par le semis après échange, donc elles
comparent deux moteurs. Aucun contrat 50k, 1 s ou 100 ms n'est revendiqué ici.

## 7. Deux points de suivi

Ma campagne CTest locale complète sur le moteur courant est close :
**449 tests sur 449, zéro échec**, 3 426 s. Cela clôt le chiffre promis dans mes
notes précédentes.

Le constructeur a par ailleurs intégré loyalement la réfutation de mon prototype,
en notant que le facteur 2,9 annoncé ne qualifie ni la réparation ni la tour.
C'est exact ; le chiffre de remplacement, 1,33x pour ce qui survit, est au § 4 de
ma note MEB.

La porte permanente census→tour est **livrée** par `324f6192` et vérifiée ligne
à ligne : elle alimente `build_full_ball_tower` avec le `balls` du vrai census
jusqu'à K=10, l'oracle rationnel servant de juge et jamais de source, avec quatre
mutants causaux contrôlant la ligne de diagnostic exacte.

**Coût mesuré, comme promis** : `ctest -R '^mhgp7_census_tower'` sur les onze
tests rend **3,50 s** au total. J'avais estimé environ 3,4 s à partir des
captures du reçu, donc l'estimation tombe à 3 % près. L'objection de coût
n'existait pas : cette porte est parmi les moins chères d'une suite qui porte
des tests individuels à 145 s et 423 s.
