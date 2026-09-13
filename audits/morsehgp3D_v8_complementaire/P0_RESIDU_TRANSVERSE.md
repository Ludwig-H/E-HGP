# Deux rangées transverses : les histogrammes locaux parfaits ne suffisent pas

13 septembre 2026. Audit complémentaire v8, CPU mono, entrée u16,
`exploration_v8_hors_registre`, `audit_independant_math_and_architecture`,
`public_status=not_claimed`. Cette note porte sur q2 et un rectangle donné.

**Pool, DualBlocks et Tubes laissent tous m² paires sur cette famille,
alors que seules O(hm) paires q2 passent leur profondeur exacte.**
Le problème n'est donc pas seulement le coût de construction des crédits :
leur universalité sur tout le facteur opposé peut empêcher tout crédit.
Rendre l'histogramme plus précis ou choisir de meilleurs témoins globaux
ne résout pas ce cas. C'est une raison concrète de comparer la sélection
de sous-rectangles, prévue dans P0, avant un port général.

## 1. Preuve locale exacte

Prendre $A=\lbrace(1000,i,0):0\leq i<m\rbrace$ et
$B=\lbrace(60000,j,0):0\leq j<m\rbrace$, indices entiers. Les deux facteurs
sont séparés selon la convention v8 dès que 59000≥s(m−1). Le nuage A∪B
ne possède aucun témoin extérieur aux facteurs.

Pour tout z=(1000,k,0) d'A et toute ancre a=(1000,i,0), le site
b=(60000,k,0) de B donne H=(z−a)·(b−z)=0. Ainsi **aucun z n'est
universel sur B**, même si l'on teste les sites plutôt que les coins.
L'argument symétrique vaut dans B. Tous les crédits locaux exacts sont
nuls, pour chaque ancre. Les trois méthodes doivent donc laisser m²
paires avec cette règle de rejet.

Pour une paire fixée a_i,b_j, en revanche, un site de l'un ou l'autre
facteur d'ordonnée k est strictement intérieur à la boule diamétrale si
et seulement si (k−i)(j−k)>0. La profondeur exacte vaut :

$$p(i,j)=2\max(0,|i-j|-1).$$

Avec besoin h=Kmax pour q2, elle reste sous le seuil si et seulement si
|i−j|≤w, où w=⌈h/2⌉. Pour m>w, le nombre exact de paires ainsi conservées
est donc :

$$M_{q2}=(2w+1)m-w(w+1).$$

À h=10, seules les cinq diagonales de part et d'autre de la diagonale
centrale restent, soit **11m−30** paires. Ce sont des paires sous seuil,
avant dédoublonnage des boules éventuelles ; ni des nœuds FULL ni la
sortie complète des voies q3/q4. La configuration plane est volontaire.
La croissance infinie suppose d'accroître aussi la précision et la
séparation ; le profil u16 ne fournit que des cas bornés.

## 2. Ce que le code actuel et le consommateur borné exécutent

Le [reproducteur C++](transverse_residual_probe.cpp) appelle les trois
stratégies réelles, développe leurs descripteurs, puis effectue un census
q2 indépendant, par la formule du milieu et avec arrêt au seuil h.
Ce census scalaire est **un juge borné**, pas une proposition de moteur.
Il confronte chaque profondeur saturée à la formule ci-dessus et compare
les paires physiques survivantes à la bande attendue.

| n=2m, h10 | Paires émises, chaque méthode | Survivantes q2 | Tests ponctuels du census borné, chaque méthode |
| ---: | ---: | ---: | ---: |
| 128 | 4 096 | 674 | 217 253 |
| 256 | 16 384 | 1 378 | 1 256 837 |
| 512 | 65 536 | 2 786 | 7 903 045 |

Aux mêmes tailles, DualBlocks ne paie que 502/1 014/2 038 tâches ; Tubes,
128/256/512 tests de balayage. L'amont évite ici le carré de préparation,
mais ne réduit pas le produit envoyé à l'étape suivante. Le coût du
census dépend de l'ordre de ses scans et n'est pas une borne nécessaire
pour un meilleur index spatial ; le nombre m² de paires développées,
lui, reste une dépense de l'interface actuelle sur ce cas.

Le [reçu](TRANSVERSE_RESIDUAL_CHECKS.json) comprend 24 essais sous C++20
strict/UBSan : trois méthodes, h1/5/10, croissance et s8/10/12. Le mutant
de frontière fermée est compilé et rejeté sur la profondeur indépendante.
Ni composante q3/q4 complète ni tour FULL n'a été exécutée.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/transverse_residual_checks.py --selftest
```

## 3. Issue constructive à comparer

Sur cette famille, les deux listes triées permettent de produire seulement
la bande |i−j|≤w en O(m+M_q2), avec les certificats de profondeur ci-dessus.
Ce raccourci propre à la fixture montre **quel travail il est possible
d'éviter** ; ce n'est pas une règle transposable sans preuve à un nuage 3D.

Dans une architecture générale, raffiner le facteur opposé dans les tâches
résiduelles peut rendre les témoins universels sur un sous-rectangle plus
petit, ou une borne de profondeur par bloc peut éliminer ses paires en
commun. Ces tâches peuvent conserver le rectangle WSPD propriétaire
original : changer le grain du certificat n'impose pas de reconstruire
une nouvelle WSPD. Leurs produits doivent partitionner exactement le
résidu, et les témoins conserver leurs populations et identités disjointes.

Critère utile pour la comparaison suivante : compter les sous-rectangles
visités, leurs certificats, les paires réellement développées et les
opérations aval, puis vérifier les mêmes paires finales. Réduire le temps
du seul préfiltre sur ces rangées ne clôt pas P0. GCP non utilisé.
