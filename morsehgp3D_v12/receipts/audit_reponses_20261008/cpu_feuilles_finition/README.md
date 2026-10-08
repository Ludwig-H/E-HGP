# CPU après FULL M : feuille et finition

**Passer de 24 à 16 sites par feuille n'est pas une piste nouvelle et n'a pas amélioré le catalogue dans la dernière
comparaison disponible.** La [session I contre-jugée](../session_i_catalogue/README.md), source `d2f39fe82`, mesure
déjà K5/W48 sur le même code, une prise de dix passes par cellule, neuf chaudes. Catalogue C, en ms :

| trame | feuille 16 | feuille 24 | rapport 16/24 |
| --- | ---: | ---: | ---: |
| ng00 | 330,736 | 327,790 | 1,009 |
| ng01 | 289,396 | 280,876 | 1,030 |
| ng02 | 333,756 | 331,990 | 1,005 |

Les différences des médianes de poste montrent la compensation : à 16, feuilles −23 à −28 ms et émission −10 à
−13 ms, mais parcours +35 à +42 ms et finition +1 à +6 ms. Ce ne sont pas des différences appariées ni une somme
de médianes ; un seul processus par cellule ne suffit pas pour une adoption statistique. F2 avait aussi mesuré les
deux tailles ([reçu historique](../../audit_performance_20261007/mesures/README.md)). La recherche bornée dans les
reçus de développement, microbancs et audits récents n'a pas trouvé de nouvelle comparaison **FULL** 16/24.

La référence historique v11 CPU à 200/163/195 ms `domain` employait 16 ; la v12 FULL M emploie le défaut 24 de
`full_probe.cpp`. Cela ne permet pas d'attribuer leur écart à ce paramètre : sessions, architectures et frontières
diffèrent. Le pin v11 `ac081a06f…` et ses sources sont explicites dans la capture ; il n'y a aucun port nouveau.
Les 2 500 contre 12 926 sous-ensembles de tailles 2..4 d'une feuille pleine de 16/24 sont seulement une borne de
présentations potentielles **par feuille**. Le nombre de feuilles, les filtres et les présentations réellement
visitées changent ; le rapport 5,17 n'est ni un gain prévu ni un rapport de travail du nuage.

## Frontière actuelle établie

La [session M](../session_m_admission/README.md), source `957e9784`, donne les valeurs suivantes. Les parts sont
calculées par passe puis médianées, pas comme rapports des médianes. Neuf journaux CPU déjà admis, 36 chaudes.

| trame | C (ms) | part C/FULL | parcours | feuilles | émission | finition |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 318,767 | 83,55 % | 68,984 | 156,298 | 24,294 | 60,718 |
| ng01 | 272,698 | 84,27 % | 59,448 | 126,535 | 24,064 | 55,856 |
| ng02 | 320,924 | 83,42 % | 70,985 | 148,243 | 28,235 | 65,236 |

La finition reste **déjà parallèle et commune** à CPU/GPU : `Assembly::finish` rassemble les lots, puis
`finish_stage<PoolExecutor>` effectue clés, tris radix stables, vérification/réparation exacte, rangs/CSR, table et
sorties. Depuis T2-d-C, les cinq sorties de type identique et taille exacte sont adoptées sans copie par
`PoolExecutor::adopt` ; les niveaux doivent encore être matérialisés. Aucun nouveau constat « finition en série »
ou « recopier la finition GPU sur CPU » n'est fondé. Les tris sautent déjà les octets constants ; leur nombre de
passes ne vaut donc pas systématiquement la largeur maximale des clés. `simt::claim` CPU est déjà linéaire.

Le poste émission n'est pas un second calcul général : `fill_body` copie/convertit les émissions mémorisées quand
la feuille étroite en a au plus 64 ; seules les feuilles débordantes ou virtuelles sont rejouées. Il faut publier
`leaves_rewritten` avant de lui attribuer un coût de recalcul. Les calculs de paires uniques et l'arrêt effectif du
census CPU sont également intégrés. `leaves.cpp`, les corps J3/census/paires, `simt.hpp` et le radix n'ont pas changé
entre I et M ; la finition et le Pool ont changé. Le code catalogue cité est encore identique au Git `72f622a5`
(la sonde FULL a évolué), sans qualification héritée d'un binaire nouveau.

Même une finition gratuite laisserait, toutes choses égales par ailleurs, des médianes C de
**258,324 / 217,451 / 255,562 ms**, et FULL de **321,280 / 268,144 / 320,037 ms**. Soustraction effectuée dans
chaque passe avant médiane : borne conditionnelle de cette exécution, jamais prédiction d'une architecture.
Le popcount logiciel reste une piste causale indépendante déjà proposée et vérifiée vers assembleur
([proposition](../cpu_popcount/README.md), [wrappers](../cpu_popcount_asm/README.md)) ; aucune preuve de gain natif
ne s'y ajoute ici. Ces points restent rattachés à CST-0233/0234.

## Mesure proposée, sans exécution

Priorité : **ventiler la finition sur le code retenu**, puis seulement confirmer l'effet du paramètre feuille
dans FULL si le développeur le juge utile après ses modifications CPU. Aucune modification moteur nécessaire.

1. Figer un seul code, les options Release/u21, le Pool et les empreintes des deux sondes. Pour chacune de ng00–02,
   rejouer `catalogue_probe` CPU K5/W48 avec `--leaf=24 --passes=10 --digest --cache=0`, puis éventuellement 16.
   Les diagnostics existants séparent `levels_ns`, `sort_ns`, `assemble_ns`, `table_ns`, et publient les chaînes
   réparées, les réécritures et le grand livre. Une prise sert au diagnostic, sans règle d'adoption ni gain acquis.
   `assemble_ns` inclut rassemblement, contrôles, émission CSR, matérialisation et copies Pool ; il ne sépare pas
   encore allocation, préfaute et bande passante. Ne pas attribuer ses 56–65 ms à un seul mécanisme sans mesure.
2. Si la comparaison FULL 16/24 est poursuivie : même ELF, mêmes données/IDs entiers, u21/K5/W48, CPU, cache 0,
   mode recouvert explicite, mêmes budgets. Cinq tours appariés, trois bras par trame : 24-référence, 24-répétition
   A/A, 16 ; ordre alterné et figé avant départ. Dix passes par processus, la première exclue. Cela fait
   45 processus / 450 passes / 405 chaudes. Archiver chaque argv, code, JSONL et SHA du binaire avant/après la
   dernière prise, échec compris. Ne pas optimiser la taille indépendamment sur chaque trame après mesure.
3. Comparer FUL1 sur **toutes** les passes entre bras avant toute interprétation de temps. Publier les compteurs
   sans imposer leur égalité entre 16 et 24 : le découpage change le travail. Recalculer P/C/G/FULL et les quatre
   sous-postes C par passe ; G recouvert est une fenêtre, TMVR une queue. Médianes par processus, ratios appariés,
   intervalle et A/A déclarés avant campagne ; ne jamais sommer les médianes ni importer le verdict MES-FULL 24.

Commande FULL proposée (gabarit, chemins à fournir par le développeur) :

```text
PROBE_FULL --trame=XYZ,IDS,ng00 --k=5 --leaf=16 --threads=48 --passes=10 --digest --cache=0 --recouvert
```

Le JSON FULL actuel ne répète pas `leaf` : l'admission de ce facteur doit donc se lier à l'argv archivé et à son
code/pin. Le JSON catalogue, lui, porte `leaf`. Les temps de la sonde catalogue sont des diagnostics séparés et
ne doivent pas remplacer C observé dans FULL. Ce plan n'est ni une demande de lancement ni une campagne jouée.

## Rejeu de cette lecture

```sh
python3 -B check.py --repo DEPOT --returned-full DOSSIER_FULL_M
python3 -B -O check.py --repo DEPOT --returned-full DOSSIER_FULL_M
```

Les deux sorties égalent `results.json` : hashes Git, égalités de sources annoncées, nombres historiques de I et
arithmétique sur les neuf JSONL M déjà admis. Aucun nouveau test de moteur, compilation, accès GCP ou payload de
données. La capture ne duplique ni sources entières ni journaux. Pas de modification produit ou registre.
