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
séquentiel **tel qu'il est écrit aujourd'hui** ; j'ai d'abord écrit « par
construction », ce qui était trop fort, et le § 4bis corrige ce point. Le prologue valide le catalogue boule par boule et
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
gagne 1,36x, 4,12x ou 1,33x, parce que la géométrie finit parallélisée de toute
façon. **La conclusion stratégique ne dépend donc pas du sort de mon prototype**,
et elle a survécu à sa réfutation, à sa réparation et à sa mesure réelle.

## 4. Ce que cela dit de l'ordre des travaux

Le noyau MEB d'abord, parce que son gain mesuré de 1,357x porte sur la catégorie
géométrique de 61 à 63 %, dont il n'occupe qu'**une partie** : `prepare_static_order`
inclut aussi les tris, le dédoublonnage, les semis et la restitution, et ce n'est
donc pas une part MEB pure. Ce levier ne rencontre aucun plafond d'Amdahl et
améliore aussi le mono-thread. Le prologue ensuite, dont la parallélisation fait passer
le plafond de 2,56x à 4,26x à 16 000 : le laisser séquentiel, c'est abandonner
15 % gratuitement.

Puis vient l'obstacle. Le calendrier séquentiel devient dominant, et il croît
plus vite que le reste. Qu'il soit un mur **définitif** est précisément ce que
conteste l'auditeur historique, voir le § 4bis. Le constructeur l'a déjà vu qualitativement, en écrivant
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

## 4bis. Correction : « séquentiel par construction » était trop fort

L'auditeur historique conteste ma conclusion centrale, et il a raison sur le
point de droit. Sa préparation soutient que **chaque horizontal K se calcule
indépendamment du précédent**, et qu'une fois les horizontaux construits, les
verticales de tous les ordres se calculent ensemble par requêtes d'ancêtre à
coupe fermée, sans dépendre des verticales de K−1. Si cela se démontre, la
séquentialité du calendrier est une propriété du **moteur**, pas de l'**objet**.

Ma mesure reste vraie et ma qualification était fausse. J'ai mesuré que la
boucle des lots, telle qu'elle est écrite, est séquentielle et coûte 16,8 % à
8 000 et 18,2 % à 16 000, avec un exposant local de 1,243 contre 1,094 pour la
géométrie. Cela n'est pas contesté. Mais écrire « séquentiel par construction »
puis « le vrai mur », c'est transformer une observation sur le code en nécessité
mathématique. Ma mesure n'autorise pas ce pas, et je le retire.

**Ce que sa thèse vaut, chiffré : environ 11x, et non 20x.** J'avais écrit 20,0x
et 18,9x. C'est faux, et la faute est un glissement de ma part sur le mot
« calendrier ».

Dans mon § 1, « calendrier » désigne une grandeur **chronométrée** : la boucle
des lots, 16,8 % à 8 000. Cette boucle est séquentielle **à l'intérieur d'un
ordre** : la résolution d'une boule lit des ancres publiées par des lots
antérieurs du **même** K, et la racine exigée doit être strictement antérieure au
lot. La thèse de l'auditeur historique porte uniquement sur la frontière
K−1 → K. Elle ne touche aucun de ces points. En écrivant 20,0x = 1/épilogue,
j'ai fait sortir le calendrier **en entier** de la part irréductible, c'est-à-dire
que je lui ai appliqué une conséquence que sa thèse ne peut pas livrer.

Ce que la thèse livre réellement, c'est un découpage en **dix tâches**, une par
ordre, l'intérieur de chaque ordre restant la chaîne de lots. Le plafond est
alors somme/max sur dix tâches inégales, et les ordres sont très inégaux.

| mesure | 8 000 | 16 000 |
| --- | ---: | ---: |
| part de l'ordre le plus lourd | 24,4 % | — |
| plafond inter-ordres sur le calendrier | 3,7x | 3,8x |
| **plafond réel sous la thèse entièrement accordée** | **11,4x** | **11,2x** |

Deux routes indépendantes y mènent. Par chronométrage des dix calendriers :
l'ordre le plus cher vaut 26,8 % du calendrier total. Par la distribution des
requêtes statiques, monotone croissante en K, où l'ordre 10 pèse 24,4 % :
1/(0,168 × 0,244 + 0,050) = 11,0x.

**Et 11x reste optimiste.** Dix ordres découplés doivent construire dix banques
de populations, puis les canoniser en la banque unique que le dépôt exige. Ce
travail atterrit dans l'épilogue, c'est-à-dire dans la part même qui fixe le
plafond. Voir le § 4ter.

Ma découpe, elle, a été reproduite indépendamment : 16,0 / 62,7 / 16,2 / 4,4 % à
8 000 contre mes 15,4 / 62,8 / 16,8 / 5,0. Ce qui est réfuté est ma conséquence,
pas ma mesure.

Un plafond n'est toujours pas un gain : il suppose une efficacité de
parallélisation que nul n'a mesurée. Le § 3 reste une allocation sous l'hypothèse
du moteur actuel.

## 4ter. Le piège concret : renuméroter passe le digest et échoue les portes

Ceci est le point actionnable de cette note, mais **je dois d'abord corriger une
revendication de nouveauté qui était fausse**. J'avais écrit qu'il ne figure
nulle part ailleurs. Vérification faite après coup, c'est inexact, et les deux
textes concernés étaient déjà publiés quand j'ai écrit cette note.

La [proposition de graphe filtré](../docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md)
énonce que l'équivalence mathématique des forêts n'impose pas les mêmes numéros
de nœuds, et qu'un export physiquement identique exige une convention
déterministe supplémentaire incluant « l'ordre des premières références de
populations/contributions », une implémentation parallèle devant reproduire
cette convention ou déclarer une autre canonisation comparée par bijection
explicite. Le [document sur les objets parallèles](../docs/OBJETS_PARALLELES_TOUR_20260911.md)
prescrit de son côté, pour l'export, de « réduire la première occurrence
canonique pour numéroter ». La contrainte et son remède étaient donc connus et
écrits avant moi.

**Ce qui reste de mon apport est plus étroit, et je le formule tel quel** : le
mécanisme exact dans le moteur, **quelles portes précises l'imposent**, et
surtout le fait que `payload_digest` y soit aveugle, ce qui rend tout témoin
différentiel fondé sur ce digest incapable de détecter une renumérotation. C'est
précisément l'erreur que j'ai commise et que cinq analyses ont commise avec moi.

Un calendrier à ordres découplés **renumérote les populations**. La table
`population_ids` est initialisée une seule fois avant la boucle, jamais
réinitialisée, et mémoïsée : l'indice d'une boule est celui que lui donne le
**premier ordre qui la rencontre**. Changer l'ordre de parcours change les
indices, sans changer aucun contenu.

Cette renumérotation est **invisible à `payload_digest`**, qui déréférence chaque
population en `PointId` au lieu de hacher l'indice, et qui se déclare lui-même,
dans son commentaire, « not a canonical geometric oracle ». J'avais conclu de là
que le couplage était sans effet sur l'objet. **C'était faux**, et je le retire :
j'ai jugé la conformité avec l'instrument que j'avais sous la main, pas avec le
critère que le dépôt s'impose.

Le dépôt, lui, voit la renumérotation, en trois endroits :

| contrôle | ce qu'il compare |
| --- | --- |
| `paired.bank_rows` | les lignes de la banque **positionnellement**, indice par indice |
| `paired.exact_contribution` | `u.ref.population == v.ref.population`, l'indice **brut** |
| `bank.shared_across_orders` | l'**identité de pointeur** de la banque entre deux ordres |

Les deux premiers vivent dans la porte de la tour, sous plancher anti-vacuité
`paired_payload_checks > 100`, et sont câblés sur deux CTests. Le troisième vit
dans la porte du certificat de couverture.

**Conséquence pour le raccord.** Un calendrier à ordres découplés passera
`payload_digest` et échouera ces trois contrôles. Il faut donc prévoir, dès la
conception, soit une numérotation des populations indépendante de l'ordre de
découverte, soit une passe de canonisation de la banque. Cette passe est du
travail supplémentaire, et il tombe dans l'épilogue.

**Un angle mort de test qui se referme à l'échelle, et c'est le problème.** Le
chemin des lots **groupés** porte le second site d'ancre de naissance. Je l'ai
mesuré aux six tailles, moteur non modifié.

| n | lots groupés | blocs par lot groupé | part des blocs d'ancrage |
| ---: | ---: | ---: | ---: |
| 500 | 0 | — | 0 |
| 2 000 | 1 | 2,0000 | 0,000083 % |
| 4 000 | 75 | 2,0000 | 0,0029 % |
| 8 000 | 474 | 2,0000 | 0,0086 % |
| 16 000 | 3 046 | 2,0020 | 0,0264 % |
| 32 000 | 20 095 | 2,0093 | 0,0843 % |

La part croît d'environ un facteur trois par doublement. **Un chemin quasi
inexercé là où les portes tournent devient matériellement emprunté à l'échelle
visée** : c'est la configuration classique du défaut latent. Aujourd'hui ce
chemin n'a ni plancher de couverture, ni mutant causal.

**Correction que je me fais à moi-même.** En ne regardant que les tailles
jusqu'à 8 000, où le rapport vaut exactement deux, j'avais conclu que tout lot
groupé contient exactement deux blocs et que toute arité supérieure était hors
d'atteinte des tests. C'est faux : l'excès sur l'arité deux vaut 6 à 16 000 et
186 à 32 000. Le plan de tests le dit pourtant, et j'aurais dû l'appliquer à
moi-même : quelques tailles basses n'établissent jamais une pente.

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
