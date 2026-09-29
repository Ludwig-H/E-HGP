# Contre-audit de la tête numérique — 29 septembre 2026

H3 reste ouvert dans l'API générale, y compris lorsque tous les λ calculés sont finis : la multiplication par des poids valides peut faire déborder les stabilités puis changer la sélection EOM. Une fusion à niveau zéro admise par le validateur produit aussi NaN. Les contrôles manuels dans la plage annoncée pour u18 concordent avec le juge ; ils ne qualifient pas le producteur ou son contrat général de multiplicité.

`phase=exploration_v10_hors_registre`, `backend=cpu_reference`, `profile=public_PointDendrogram_API_joint_numeric_domain`, `mode=independent_readonly_numeric_audit`, `public_status=not_claimed`. GCP non utilisé. Aucun moteur, banc, fichier d'un autre auditeur, graine ou campagne longue modifié ou lancé.

## Sources examinées et portée des tests

La copie corrigée est `build/v10-fixes/tete/src/morsehgp3D_v10`, déclarée issue de `0bce6cc00`. La copie `tete-verif/src` a les mêmes empreintes des trois fichiers numériques examinés. Le checkout intégré observé à `12aa92110` conserve ses sources de tête antérieures. Aucun correctif conjoint plus récent n'a été identifié dans ces copies : `lambda_of` calcule toujours `pow(level, -0.5*z)`, `validate(d)` admet tout niveau fini ≥ 0, et `validate(p)` contrôle seulement z fini dans (0,16], mcs et sélection.

| Fichier de la copie corrigée | SHA-256 |
| --- | --- |
| `src/head/head.cpp` | `dc34aed57e9248a58858515f731654540d38d9ed919877d81f7ca68476bc6dd5` |
| `src/head/head.hpp` | `8d9e29b1992d8d27cb9c542dadca4bd08631b8de128a8bd62a6fb46f8cad474b` |
| `src/points/dendrogram.cpp` | `2eadd747ff6804003e971bd2cb0d492beb7ee5fbd697d3975fb9822f966ba71f` |

Le nouvel en-tête diffère par ses commentaires de celui cité dans le contre-audit précédent ; les unités numériques gardent ses empreintes. Cette note ajoute une capture indépendante et ne reprend pas sa fixture positive minuscule à z=16.

Le journal du développeur `tete/apres/ctest_gate.txt` porte **11/11 terminés**, code 0, 2 136,30 s ; ses différentiels API/CLI et ASan sont également terminés dans les journaux relus. Il s'agit de résultats **observés**, non rejoués dans cette passe. Le journal distinct `tete-verif/apres/ctest_gate.txt` était encore incomplet à la lecture : unit/head_gate passaient et l'oracle catalogue venait de commencer. Il ne constitue pas une seconde clôture.

Les nouvelles gates contrôlent les paramètres, les structures, les labels et la sélection à partir du condensé produit. Leur génération aléatoire emploie des niveaux positifs multiples de 0,25 et des poids 1 à 3 ; elle n'exerce pas ce débordement pondéré. Le différentiel API constate l'identité avec l'ancien condensé, sans démontrer sa finitude sur tout le domaine public.

## Nouvelle cause : λ finis, stabilités infinies, EOM incorrect

Deux composantes feuilles, un point de poids w dans chacune, une seule fusion. Niveaux feuilles `1e-300`, fusion `9e-300`, z=2, mcs=w, racine autorisée. Les deux validations rendent `ok/none`.

Pour cette fixture, la stabilité racine est `R=2w*λ_fusion` et la somme des deux stabilités filles est `S=2w*(λ_feuille−λ_fusion)`. À z=2, **S>R si et seulement si L_fusion>2L_feuille**. Cette dernière comparaison est exacte sur les entrées binary64 choisies : multiplier L_feuille par 2 ne fait qu'en changer l'exposant, sans débordement ni sous-flux. Le juge ne réutilise aucun champ du condensé du produit.

| Poids par point | Masse entière totale | λ et naissances | Stabilités | EOM produit | Sélection analytique |
| ---: | ---: | --- | --- | --- | --- |
| 1 | 2 | tous finis | toutes finies | deux feuilles | deux feuilles |
| 4 294 967 295 | 8 589 934 590 | tous finis | trois +∞ | racine | deux feuilles |

Sur la seconde ligne, le diagnostic long double donne R ≈ **9,544×10^308** et S ≈ **7,635×10^309**. Ces valeurs sont des approximations dans un type à exposant plus large, pas un oracle exact général. La sélection attendue découle de l'inégalité analytique exacte précédente, avec une grande marge.

La cause est la contribution `weight*(λ_sortie−λ_naissance)` : les λ seuls sont représentables, leurs produits pondérés ne le sont plus. EOM reçoit ensuite `sub=∞` et `stability[root]=∞` ; `sub > stability[root]` est faux et le départage en faveur du parent remplace une inégalité stricte réelle. Réduire le domaine aux « λ finis » ne suffit donc pas à fermer H3.

Il ne s'agit pas d'un débordement de masse u64. Le validateur borne le nombre de points à 2^32−1, chaque poids est un u32, et les sous-arbres validés sont disjoints ; la masse totale est au plus (2^32−1)^2 < 2^64. La sûreté de ce compte ne certifie pas celle des produits et sommes flottants.

## Contrôles dans la plage u18 et zéro interne

Le même arbre manuel avec niveaux **1/4 et 3/4**, poids u32 maximaux et mcs=w reste fini pour z=1 et z=2. Le juge prévoit la racine à z=1 et les deux feuilles à z=2, ce que le produit rend. Pour z=1, les enfants gagnent exactement si L_fusion>4L_feuille ; pour z=2, si L_fusion>2L_feuille. Une différence de sélection entre ces deux exposants est donc sémantiquement normale : une porte « équitable » doit juger chaque exposant suivant sa propre définition.

Ce sont des valeurs dans la plage publiée `[1/4,2^64]`, **pas une exécution du producteur u18**. Le producteur courant attache des points de poids unitaire dans la tour ; cette passe n'a pas qualifié son domaine complet, ses z extrêmes ou une extension pondérée. Elle n'établit aucun défaut du producteur u18 et ne clôt H3 pour lui sur la seule base de ces deux exemples.

Une autre fixture conserve les niveaux `{0,1/4}` mais place la fusion au rang zéro, avec deux poids 2, mcs=2 et z=1. Elle satisfait les deux validations. Les naissances et sorties filles valent +∞ et deux stabilités deviennent **NaN par ∞−∞**. C'est une fusion zéro autorisée par l'API structurelle ; aucune preuve n'est donnée que le producteur de sites u18 dédupliqués la fabrique. La convention documentée de stabilité infinie d'une feuille née au niveau zéro ne décrit pas cette stabilité NaN d'un cluster né à λ=∞.

## Borne conditionnelle de finitude pour les événements u18 positifs

En unités de la grille entière, une boule contenant deux sites distincts distants d'au moins 1 a un rayon d'au moins 1/2, donc un niveau positif L ≥ 1/4. Le constructeur publie ces unités directement : `point_dendrogram` mélange les rayons carrés du catalogue et les distances carrées entières de l'entrée core, sans conversion en mètres. Pour K ≥ 2 sans multiplicité, D_K(x) ≥ 1 ; l'entrée cover de poids K+extra ≥ 2 utilise des boules contenant au moins deux sites distincts.

Sous l'hypothèse publiée que tous les niveaux **effectivement consommés** sont dans [1/4,2^64], les plages sont `2^-32 ≤ λ ≤ 2` pour z1 et `2^-64 ≤ λ ≤ 4` pour z2. Une stabilité de cluster est la somme de durées non négatives de ses points, chacun compté une fois dans ce cluster : elle est au plus `M*maxλ`. Le `best` EOM est la somme d'une antichaîne de clusters à ensembles de points disjoints ; la même borne s'applique. Avec `M ≤ (2^32−1)^2 < 2^64`, ces quantités réelles sont **strictement inférieures à 2^66**. Pour le producteur unitaire actuel, `M=n ≤ 2^32−1`, la borne est plus forte : **strictement inférieure à 2^34**.

La marge jusqu'au plafond binary64 est immense. Pour les opérations arrondies ordinaires, une réserve conservatrice de `2^70` suffit : moins de `2^34` accumulations dans ces passes, et `N*2^-53 < 2^-19`, très loin d'un facteur deux d'erreur accumulée. Cela établit l'absence de dépassement par poids/sommes dans ce domaine positif sous les hypothèses déclarées ; ce n'est ni une certification d'un libm quelconque, ni un ordre EOM exact, ni une garantie d'absence de coalescence des dates double.

La même borne s'étend en réel à **tout z admis, 0 < z ≤ 16** : `log2(L) ∈ [−2,64]` implique `log2(λ) ∈ [−512,16]`, donc `2^-512 ≤ λ ≤ 2^16`, entièrement dans les doubles normaux. Les stabilités et `best` sont alors **< 2^80** pour M < 2^64 (**< 2^48** pour le producteur unitaire), avec une réserve conservatrice arrondie de `2^84` dans les mêmes conditions. Cela exclut le débordement par poids/sommes et le sous-flux des valeurs λ dans ce domaine positif, sans ordre EOM exact et sans couvrir les niveaux zéro. L'absence de sous-flux de λ ne signifie pas absence de tout sous-flux intermédiaire : si z est le plus petit double sous-normal positif, le calcul binary64 `−0.5*z` peut s'arrondir à −0 ; la finitude reste acquise mais cette échelle ne conserve pas un exposant strictement négatif. Les conditions z1/z2 restent la priorité pratique de cette note.

Les niveaux zéro demandent une condition supplémentaire. `validate(p)` autorise mcs=1. À K1 core, les points entrent à zéro ; si la condensation ouvre leurs feuilles de masse unitaire, leurs sorties valent +∞ et leurs stabilités sont infinies. Avec **mcs ≥ 2**, au moins deux sites, poids unitaires, aucune fusion zéro et des feuilles zéro portant chacune un seul point, ces feuilles sont trop légères : `drop_subtree` les fait sortir au λ de leur fusion positive, sans lire leur date zéro. Un seul site n'a pas cette fusion protectrice. Des poids capables d'atteindre mcs sur une feuille zéro enlèveraient aussi la protection.

La lecture du producteur confirme `cloud.w[s] != 1 → multiplicity_unsupported` (`tower.cpp:1169`) et `point_weight.assign(n,1)` (`tower.cpp:1873`). Ce contrat producteur est plus étroit que l'API générale et que le témoin MR pondéré. La [lecture archivée](../../../receipts/audit_continu_20260929/head_numeric_corrected/producer_readout.txt) fixe ces observations, sans exécuter de tour. Sous les conditions positives ci-dessus, le risque de débordement montré par notre contre-exemple général est exclu pour u18 z1/z2. Une clôture globale de H3 pour toute tête u18 doit encore porter ses conditions K/entrée/mcs/zéro et son domaine de multiplicité : les seules préconditions publiques `validate(d)` et `validate(p)` ne le font pas.

Si les niveaux sont ultérieurement exprimés en mètres avec pas Δ, la borne géométrique devient `L ≥ Δ²/4`, donc `maxλ = 2/Δ` à z1 et `4/Δ²` à z2. Il faut refaire le domaine conjoint avec cette unité. Le pas fixe de 1 mm donnerait des plafonds 2 000 et 4 000 000, encore très sûrs pour ces masses ; un changement d'unité arbitraire n'hérite pas de la borne entière.

## Capture indépendante et gate proposée

La [sonde](../../../receipts/audit_continu_20260929/head_numeric_corrected/joint_head_probe.cpp), le [reçu](../../../receipts/audit_continu_20260929/head_numeric_corrected/receipt.json) et les sept fichiers natifs nécessaires sont archivés dans `receipts/audit_continu_20260929/head_numeric_corrected/`. L'archive est bit-identique à la copie examinée. Sources fermées avant/après ; compilation Release de deux unités uniquement, g++ 13.3.0, puis exécution code 0. Empreinte de la sonde `9a830470b13a667d8daaf0459a8fd5fc72783d15f28201bff5f818862d93f894` ; binaire `38d88d982da5ea53541a6c144f7499d3d8fbcaebf8958c1a768500c4d17bba67`.

Le code 0 confirme ici les contrôles et les **deux défauts reproduits** ; il n'est pas une porte de conformité verte. La première tentative de lancement précédait la fin d'une compilation rendue en session active : son code 127 et la collecte prématurée sont conservés dans [launch_before_compile_completion.json](../../../receipts/audit_continu_20260929/head_numeric_corrected/launch_before_compile_completion.json), séparément du binaire terminé et clos. Ce défaut du harnais n'est pas un résultat géométrique.

Une future gate de domaine conjoint peut conserver ces cinq petits cas : contrôles finis dans la plage déclarée pour z1/z2 avec masses maximales ; même petite échelle avec poids 1 puis poids maximal ; fusion zéro interne. Les cas hors domaine doivent être refusés avant condensation, ou calculés suivant une politique publiée qui garde les comparaisons EOM définies. Le garde doit porter sur niveaux, z **et masse pondérée**, ainsi que sur le rôle du niveau zéro ; contrôler z seul ou la finitude de λ seul ne suffit pas.
