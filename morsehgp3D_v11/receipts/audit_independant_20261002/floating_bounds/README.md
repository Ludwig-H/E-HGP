# F3 : témoin binary64 exécuté et borne constructive

Ce reçu confirme le constat de l'[audit d'ouverture déjà déposé](../../../audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md),
sans en refaire le contre-exemple dirigé. Il apporte une sonde réellement
exécutée en binary64 au plus proche et une proposition de propagation pour v11.
Aucun code produit, moteur, compilation, GCP ou campagne. Aucun filtre v11
implémenté n'est déclaré défectueux : il s'agit de la doctrine documentaire F3.

[Sources de la première capture](SOURCE_BEFORE.json), dont
[ARCHITECTURE](sources/ARCHITECTURE.md), hash
`394036f55cb521f650b7868b3741e59d2beef73ba26cb0f419333d3a7e7e0bbd`.
[SOURCE_AFTER.json](SOURCE_AFTER.json) indique les évolutions éventuelles ;
les conclusions restent ancrées aux copies locales, jamais à un état mutable.

## Le nombre d'instructions ne borne pas les réutilisations

Prendre N = 2^53+1 et D = 2^53. Convertir N et D en binary64, diviser,
puis mettre le quotient au carré quatre fois : **sept opérations**.
Le calcul observé vaut toujours 1 ; la valeur exacte est `(N/D)^16`.
L'erreur de conversion de N est `−1/N`, donc ≤u = 2^-52 ; toutes les erreurs
élémentaires suivantes sont nulles. Chaque valeur est finie, positive et normale.

Pourtant l'erreur finale `1−(D/N)^16` est strictement supérieure à
`(1+u)^7−1` : environ 8u contre 7u. La conversion initiale intervient seize
fois dans l'expression développée. Tous ces tests sont décidés en Fraction,
pas à partir des affichages décimaux. Une clé exacte voisine `1+8u` montre
aussi qu'une comparaison utilisant cette borne trop petite pourrait certifier
l'ordre inverse de l'ordre exact. Ce dernier test est synthétique, pas un
filtre produit exécuté.

## Proposition pour un DAG de valeurs positives

Propager à chaque nœud des facteurs `L ≤ approché/exact ≤ U`. Sous l'hypothèse
élémentaire `|δ| ≤ u`, les règles suivantes sont sûres :

| Opération | Facteur bas | Facteur haut |
| --- | --- | --- |
| Conversion | 1−u | 1+u |
| Produit a·b | (1−u)L_a L_b | (1+u)U_a U_b |
| Quotient a/b, L_b > 0 | (1−u)L_a/U_b | (1+u)U_a/L_b |
| Somme de même signe | (1−u)min(L_a,L_b) | (1+u)max(U_a,U_b) |

La somme est une moyenne pondérée des facteurs des opérandes avant son propre
arrondi ; cela prouve sa ligne. Un carré réutilise les deux bornes du même fils,
sans compter celui-ci une seule fois. Le quotient inverse celles du dénominateur.
Même sans réutilisation, le modèle `(1+u)/(1−u)` peut dépasser `(1+u)^2`.

Version simple pour calculer un budget `constexpr` : conserver un entier E
tel que le facteur soit dans `[(1−u)^E, (1−u)^−E]`. Une valeur exacte a E = 0 ;
une conversion avec erreur ≤u a E = 1. Produit et quotient ont
`E = E_a+E_b+1`, la somme `E = max(E_a,E_b)+1`. Un carré a donc `2E_a+1`.
Par induction, l'erreur relative est au plus `(1−u)^−E−1`. Cette enveloppe peut
être pessimiste ; les intervalles précédents permettent de la resserrer.
Le témoin a E = 63 dans cette variante uniforme, pas E = 7.

Les bornes doivent elles-mêmes être calculées exactement ou majorées avec un
arrondi certifié. Pour accepter toute réassociation, couvrir les DAG autorisés,
en particulier la profondeur maximale des sommes, ou restreindre les
transformations. Les zéros, divisions, sous-flux et valeurs non finies demandent
leur contrat propre : l'hypothèse relative ne les certifie pas automatiquement.
Un auto-test F5 est une défense supplémentaire, pas une preuve universelle.

F2 reste correct sous sa condition : **tous** les intermédiaires et ordres
admis sont des entiers représentables de valeur absolue <2^53. Le présent N
est hors de ce domaine. Les budgets d'entiers de § 3 ne sont pas réfutés ici ;
les preuves par expression et leurs portes restent à construire.

## Vérification

[check.py](check.py) vérifie sept erreurs élémentaires réellement observées,
la violation, l'enveloppe corrigée et les règles produit/quotient/somme aux
extrémités de leurs intervalles. [normal.json](normal.json) et
[optimized.json](optimized.json) sont identiques ; [RUN.json](RUN.json) conserve
les commandes et codes retour. [SHA256SUMS](SHA256SUMS) ferme le reçu.
La sonde CPython ne qualifie pas les futurs compilateurs C++, FENV, SIMD ou GPU.
