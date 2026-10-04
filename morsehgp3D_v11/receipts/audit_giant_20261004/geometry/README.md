# Revue indépendante num/index/catalogue — 4 octobre 2026

Source figée : `0f5e8a207f2974e262cd40a8882b97af1da396af`. Cadre :
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Cette capsule contient une lecture des sources et un témoin scalaire exact ; **aucun
build, test natif, fit ou appel GCP**. Les 238 copies proviennent des blobs Git du
pin, et non d'un chantier mutable. Leur fermeture est contrôlée dans
`SOURCE_AFTER.json` et `SHA256SUMS`.

## Constat nouveau : le format de points refuse un tétraèdre u24 valide

Dans `bench/points_export.cpp:266–273`, `fixed` accepte trois mots de 64 bits et
refuse avec `tower_invariant` tout mot supérieur non nul. Les lignes 327–330
appliquent cette garde à **tous** les niveaux du catalogue, avant les ordres demandés.
La portée u24 du moteur ne suffit donc pas à celle de cet export.

Posons `L=2^24−1` et les quatre sites unitaires
`(0,0,0),(L,L,0),(L,0,L),(0,L,L)`. Ils sont distincts, dans le domaine u24.
Leur centre est `(L/2,L/2,L/2)`, avec quatre poids barycentriques `1/4` ; aucun
support de deux ou trois sites ne donne cette boule. Elle a `p=0`, `m=4`,
`qmin=4`, donc satisfait l'admission du catalogue `p+qmin≤K+1` dès `K=3`.
Cette admission ne signifie pas qu'elle est forte à k=3.

La factory q4 (`src/num/sphere.cpp:63–102`) conserve le dénominateur
`D=4L³` et trois coordonnées `N_j=±2L⁴`. Sa matérialisation produit le
Level **non réduit** `12L⁸ / 16L⁶`, de valeur `3L²/4`.
Le numérateur a **196 bits**, le dénominateur **148 bits** : ils respectent les
budgets u24 `204/152` (`src/num/budgets.hpp:26–27`). Le mot du numérateur
d'indice 3 vaut **11**. La garde de `fixed` rend donc nécessairement
`tower_invariant` sur ce niveau valide. C'est une conséquence de la source
épinglée et d'un calcul exact, **pas une observation d'exécution du binaire**.
Le fichier final n'est pas publié lors de ce refus ; le protocole transactionnel
supprime le fichier pending.

Le vérificateur Gram/Fraction indépendant contrôle les trois profils et les
24 permutations du tétraèdre. Les profils u18/u21 donnent respectivement des
numérateurs de 148/172 bits et franchissent cette garde. Les sorties normal et
`python -O` sont identiques. La campagne actuelle de points u21 n'est pas remise
en cause ; le prochain raccord POINTS/u24 doit soit versionner un format assez
large/variable, soit déclarer et contrôler la restriction de profil avant export.
Une porte native u24 sur ce tétraèdre, avec lecture exacte du dump, sera utile.

## Résultat de la relecture du noyau

Aucun nouveau défaut num/index/catalogue établi. La matrice `COVERAGE.md` précise
les invariants effectivement relus et les portes présentes. Comparaison avec
`c40f40798375a0fc37917499401f16876cccbd2a` : **157 fichiers identiques** ; seule
la documentation `tests/catalogue/README.md` diffère parmi les 158 comparés.
Toutes les sources produit num/index/catalogue et leurs codes de test capturés
sont inchangés. Les anciens résultats restent à relier à leurs capsules G4 ; cette
identité de sources ne constitue pas une nouvelle exécution ou qualification.
Le README du pin borne explicitement ASan18 à num/index/tower, sans catalogue/FENV,
et n'annonce pas Clang.

Le risque de débordement des sommes d'incidences de cet export n'est pas retenu
sous les gardes actuelles : `B<2^32−1`, `|I|+|U|≤max_leaf≤1024` donnent
`P≤1024B<2^42` et `8P+8n<2^46`. Ce raisonnement concerne le code actuel et ne
se transporte pas à une future coquille globale non bornée.

Limites : pas de test natif nouveau, ni d'inventaire indépendant des archives G4,
ni d'allocation massive ou de mesure de performance. Cette revue ne couvre pas
FULL/descentes, sélection de points, EOM ou théorie statistique, traités séparément.
Les implémentations et interfaces de num/index/catalogue ont été relues ; les
registrations de leurs tests ont été inventoriées et des corps de tests ciblés ont
été inspectés, sans prétendre avoir relu exhaustivement tous les harnesses ou
toutes les sources historiques v7–v10. Les copies de dépendances servent à la
fermeture et ne sont pas toutes réauditées dans cette capsule.
