# Census q2 : juge indépendant des deux parcours C++

13 septembre 2026. Cadre `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`.
Avis favorable borné sur le snapshot census cpp `3c513cc4…`,
hpp `2116ac4b…`, axes cpp `8c91d5c9…`, hpp `5d0022c7…`.
Le [reçu autonome](Q2_CENSUS_CPP_CHECKS.json) conserve les neuf sources
produit exactes, le juge et leurs SHA. Aucun transfert aux changements
ultérieurs du worktree ; aucune mesure de performance dans cette porte.

## Pourquoi les parcours examinés restent exacts

Pour la paire fixée, `pair_bounds` calcule les distances minimale et
maximale entre le centre doublé et la boîte Z doublée. Pour a fixé et
plusieurs b, `shared_bounds` utilise la séparabilité de H : le minimum
est aux extrémités des intervalles b et z ; le maximum est atteint pour
un b extrême, puis au sommet clippé de la parabole en z. Son sommet
peut être intérieur à Z. Les produits et les sommes sont promus en i64.
La clé conserve S=a+b en u32 et |a−b|² en u64, y compris 12 884 508 675
pour les coins opposés du cube u16.

Le curseur Z désigne le premier sous-arbre d'un suffixe du parcours fixe
préordre. Ses populations antérieures ont toutes été consommées :
classées sans intérieur, ou créditées entièrement. Les transitions vers
le premier enfant et vers `escape` avancent dans ce parcours ; les liens
de sortie et les partitions d'enfants sont vérifiés lors de construction.
À un partage B, le compte reste sous Kmax : il est donc exact sur ce
préfixe pour chaque b du groupe. Les deux enfants héritent du compte et
du même curseur, sans recommencer les populations déjà consommées.
Atteindre Kmax rejette immédiatement tout le groupe, avant tout autre
partage. Ce seuil unique ne demande aucune reprise vers un seuil supérieur.

La collecte des survivants part de zéro : maximum H **strictement**
négatif permet le rejet ; égalité n'autorise pas la suppression d'un
site de coquille. Les insertions en bloc utilisent les IDs de la
permutation globale, et les feuilles utilisent les IDs originaux.
Les populations de l'arbre sont disjointes. Le compte des intérieurs
collectés est confronté au compte acquis ; notre juge contrôle aussi
leurs IDs exacts, la coquille entière et l'absence de doublons.

Si le préfiltre est déjà vide après crédit du cœur, aucun parcours de
census ni collecte ne commence. Les crédits du préfiltre ne sont pas
préchargés dans le compte. Les boules de même clé gardent ici leurs
incidences de supports distinctes, conformément au contrat déclaré.

## Porte exécutée et limites

Le [juge C++](q2_census_independent_probe.cpp) utilise directement
H=(z−a)·(b−z) sur tous les sites pour chaque paire du petit rectangle.
Il n'appelle ni l'oracle du constructeur ni ses prédicats géométriques
pour obtenir les réponses attendues. Il inclut la traduction census
uniquement pour **interroger les fonctions testées** de bornes ; les
références restent des évaluations rationnelles indépendantes de H.

Les mêmes réponses attendues servent à Pairwise et SharedBlocks.
Les vues de callback sont copiées pendant leur validité puis comparées
après tri, sans imposer un ordre d'émission ou d'IDs au produit.

- 466 fixtures et 932 exécutions des deux modes ; 8 099 paires jugées
  par 184 448 tests directs de sites.
- 3 375 cas de bornes sur les trois axes, confrontés à 16 625 évaluations
  rationnelles sur une grille au quart contenant les sommets demi-entiers.
- 8 626 émissions cumulées, 15 468 IDs intérieurs et 41 962 IDs de coquille
  exactement conformes ; les rejets résiduels sont comptés séparément.
- 257 partages après crédit, avec branches de crédit, rejet et acceptation
  uniformes exercées ; 14 plans vides et 54 cas à cœur positif.
- Translations, permutations d'axes et d'IDs, réflexions u16, nuages
  irréguliers déterministes, coquille supérieure à 30 sites, tangences,
  clés partagées par plusieurs supports et diamètres dépassant u32.

GCC C++20 strict passe en O2 puis en O1 avec UBSan ; sorties identiques.
Sept modifications **du vrai C++ copié** compilent puis sont rejetées :
intérieur fermé, tangence effacée pendant collecte, enfant B repartant
à la racine, enfant perdant le compte acquis, accumulation de diamètre
en u32, position de permutation prise pour ID et maximum limité à une
extrémité de Z. Le reçu conserve remplacements, SHA et sorties exactes.
Une première exécution de notre harnais a échoué sur un plancher de
non-vacuité trop élevé ; elle est conservée avec son diff et son motif,
sans être présentée comme un défaut du producteur.

Rejeu depuis les seules sources embarquées :

```bash
python3 audits/morsehgp3D_v8_complementaire/q2_census_independent_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_CENSUS_CPP_CHECKS.json
```

Le runner reconstruit dans un répertoire temporaire. Ses contrôles de
CLI et de SHA restent actifs en Python normal et −O ; cela ne désigne
pas une seconde campagne de compilations sous Python −O.
La preuve des invariants et cette porte bornée ne qualifient ni la vraie
WSPD, ni la déduplication globale, ni q3/q4 ou la tour FULL.
GCP non utilisé.
