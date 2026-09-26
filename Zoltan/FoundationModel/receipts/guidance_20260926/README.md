# Guidage FULL : contre-exemples et algèbre

26 septembre 2026. Base de conception fba0d0225 ; Python 3.12.1.
Les scripts ci-dessous sont autonomes et n'utilisent que la bibliothèque
standard. Les quatre commandes ont terminé avec code 0, stderr vide.

**Résultat : 10 fixtures PASS, 23 variantes incorrectes réfutées.**
Pour chaque script, les sorties normale et optimisée sont identiques octet
pour octet. Les portes utilisent des exceptions explicites, jamais `assert`.
Les variantes sont des alternatives algébriques/géométriques évaluées dans
les fixtures ; il ne s'agit pas de mutations compilées du moteur.

## Commandes depuis la racine du dépôt

~~~bash
python3 Zoltan/FoundationModel/reference/verify_guidance_k1.py > Zoltan/FoundationModel/receipts/guidance_20260926/guidance_k1_normal.json
python3 -O Zoltan/FoundationModel/reference/verify_guidance_k1.py > Zoltan/FoundationModel/receipts/guidance_20260926/guidance_k1_optimized.json
python3 Zoltan/FoundationModel/reference/verify_guidance_algebra.py > Zoltan/FoundationModel/receipts/guidance_20260926/guidance_algebra_normal.json
python3 -O Zoltan/FoundationModel/reference/verify_guidance_algebra.py > Zoltan/FoundationModel/receipts/guidance_20260926/guidance_algebra_optimized.json
~~~

## Sources et sorties

| artefact | SHA256 |
| --- | --- |
| [verify_guidance_k1.py](../../reference/verify_guidance_k1.py) | `54fd8c195c59908e480831aea25d5a24d3e7e97e9692364000dbab4883bc0ca5` |
| [guidance_k1_normal.json](guidance_k1_normal.json) | `e8e649aaeb2ba5af7566dbd0c81514122c21765fd14cf4b2b19e4cf24ea4905d` |
| [guidance_k1_optimized.json](guidance_k1_optimized.json) | `e8e649aaeb2ba5af7566dbd0c81514122c21765fd14cf4b2b19e4cf24ea4905d` |
| [verify_guidance_algebra.py](../../reference/verify_guidance_algebra.py) | `c87e6d49fe59b2d2108d2a6eba1633c16c3a814f95e26f8651a89ec4e12338e5` |
| [guidance_algebra_normal.json](guidance_algebra_normal.json) | `410fefe48abd0857c616c16a080ac912b8db785118451e5eda67b4ec1fc30c6f` |
| [guidance_algebra_optimized.json](guidance_algebra_optimized.json) | `410fefe48abd0857c616c16a080ac912b8db785118451e5eda67b4ec1fc30c6f` |

Chaque JSON porte le hash de son propre script. Un changement de source
requiert une nouvelle capture ; ces reçus décrivent ces sources précises.

## Géométrie K1 : quatre fixtures, dix variantes

Petits sites rationnels colinéaires plongés dans R³. La référence énumère
toutes les arêtes de distance carrée au plus 4r² et calcule leurs composantes.
Elle ne lit aucune sortie v9.

| fixture | résultat positif | erreur réfutée |
| --- | --- | --- |
| pont supprimé | fusion des extrémités à r²=1/4 pour {0,1,2}, r²=1 pour {0,2} ; inclusion sur 30 requêtes | invariance sous décimation, inclusion inversée, contact strict pour coupe fermée |
| deux complétions | même V={0,2}, enseignants de trois sites donnant labels 1/0 à r²=9/16 ; probabilité conditionnelle 1/2 et Brier minimal 1/4 sous mélange équiprobable | récupération exacte depuis V ; ambiguïté supprimée par enseignant plus riche |
| horizon | {0,4} non connecté jusqu'à r²=1, fusion réelle à r²=4 ; fusion au bord fermé observée | remplacer censure par H, déconnexion éternelle, censurer le contact au bord |
| fuite par chemins enseignants | compilateur jouet de V identique pour les deux complétions ; connexité héritée différente | masquer seulement les coordonnées ou restreindre les composantes enseignantes |

Le compilateur jouet ne qualifie pas l'absence de fuite d'un vrai forward
PTv3. Le futur raccord doit refaire ce contrôle avec ses propres entrées.

## Algèbre de guidage : six fixtures, treize variantes

Matrices d'affectation rationnelles fournies, sans revendication de
réalisabilité géométrique HGP. Les IDs communs sont déclarés ; aucun
appariement de deux acquisitions distinctes n'est calculé.

| fixture | résultat positif | erreur réfutée |
| --- | --- | --- |
| jointure et mesure | G conserve les deux marginales sur les IDs communs | jointure par position, mesures indépendantes |
| réserve et vide | masse non représentée explicite ; absence de cible marquée null | renormalisation présentée comme couverture complète, cible zéro/uniforme inventée |
| même vue douce | T a lignes (5/6,1/6) et (1/6,5/6) ; (0,6) devient (1,5) | identité et aller-retour sans lissage |
| crop | moyenne commune 0 ; moyenne complète 5 ; pertes distinctes 0/25 | dénominateur de masse complète, confusion accord/complétion |
| mêmes poids enseignants | accord exact sur même support, mais accord constant aussi de perte 0 | poids communs ou stop-gradient réputés exclure l'effondrement |
| énergie de graphe | énergie 18 pour deux tokens distincts, 0 pour constants ; variance globale positive possible entre composantes effondrées | toutes les arêtes positives préserveraient les distinctions ; variance globale suffirait |

La cinquième fixture ajoute un exemple de cible fixe de **distance carrée
entre lignes d'affectation** : perte 9/2 pour features constantes, 0 pour les
lignes d'affectation elles-mêmes. Cet exemple borne la réfutation ; ce n'est
ni la BCE proposée pour K1, ni une garantie anti-effondrement d'un réseau.

## Portée

Le [contrat de guidage](../../GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md)
utilise ces contre-exemples pour choisir ses opérateurs et son premier
prétexte. Aucun résultat appris, export natif, poids p3 exact, benchmark
LiDAR ou temps moteur n'est revendiqué. GCP et données KITTI non utilisés.
Les reçus de la [tranche coupes](../cut_algebra_20260926/README.md) sont
distincts et inchangés.

Contrôle documentaire : `python3 tools/check_docs.py` a validé 780 fichiers
Markdown actifs. Ce contrôle exclut Zoltan ; un appel ciblé à sa fonction
`validate` sur `Zoltan/README.md` et les 15 Markdown de FoundationModel
a trouvé zéro erreur. `git diff --check` passe aussi. La présentation
historique est préservée et hors de ce contrôle ciblé.
