# Robustesse : déplacements, aberrants et sous-échantillonnage

Contrelecture indépendante de Q1(a–c) au pin
`ee2df036282cad79dbb233934d5c3880243004cf`. Les deux documents de contexte et
leurs empreintes figurent dans `sources.json` ; ils sont consultables par
`git show <pin>:<path>`. Les sources du développeur sont intactes.

`replay.py` est autonome : Python standard/Fraction, aucun import du produit,
aucune dépendance tierce, aucun binaire, banc ou appel cloud. Le script dérive
les intersections des boules et les dates contraintes d'un triangle ; il ne
construit pas une mosaïque générale. Les résultats bornés ne qualifient aucun
moteur natif.

## 1. Moins de k aberrants n'interdit ni création ni pont

Boules fermées, k=2, r=2, coordonnées sur un axe de R3 :

- P={0,6,12} ne possède aucune composante d'ordre 2. Ajouter le seul site 1
  crée une lentille, de trace sur l'axe [-1,2].
- P={0,2,5} possède deux lentilles séparées, de traces [0,2] et [3,4].
  Ajouter le seul site 3 produit notamment la lentille de la paire {2,3},
  de trace [1,4], qui relie les deux composantes. L'union a pour trace [0,5].

Pour des sites collinéaires, chaque intersection de boules est convexe et
contient la projection axiale de chacun de ses points. Les intersections
de ces pièces sont donc non vides exactement lorsque leurs traces axiales
s'intersectent. Le comptage des composantes sur l'axe est ici également
celui de la multi-couverture dans R3.

La propriété correcte, pour m sites ajoutés, est

\[
\Omega_k^P(r)\subseteq\Omega_k^{P\cup O}(r)
\subseteq\Omega_{k-m}^P(r),\qquad k>m.
\]

Un centre couvert requiert donc au moins k-m bons sites ; cela n'interdit
aucune des deux modifications ci-dessus. À un rayon fixé, la séparation
stricte dist(P,O)>2r, avec m<k, suffit en revanche pour l'égalité des régions :
une boule de bon site et une boule aberrante ne se rencontrent pas, et
les seuls aberrants n'atteignent pas k. Cette condition dépend du rayon.

## 2. Quantification : la bijection doit survivre au comptage

P={1/10,1/5,10,20}, k=2, r=1/10, arrondi au plus proche sur une grille
de pas 1, même origine. Les labels deviennent {0,0,10,20}, avec déplacement
maximal δ=1/5. Le support unique {0,10,20} reste à distance de Hausdorff
1/5 de P, mais sa multi-couverture d'ordre 2 est vide même à r+δ=3/10.
La région initiale possède une composante, de trace [1/10,1/5]. À l'origine,
d2 passe de 1/5 à 10 après fusion des deux unités de comptage.

Avec les labels et leurs multiplicités conservés, les inclusions à k fixé
restent vraies après déplacement δ. Dédupliquer avec poids unitaires
supprime une unité et change le modèle. Des poids entiers conservant la
masse originale permettraient les mêmes bornes de transport sur la région
pondérée ; aucune implémentation pondérée de la mosaïque n'est qualifiée ici.

Pour une grille isotrope de pas h, arrondi au plus proche et origine commune,
chaque retour se déplace d'au plus √3 h/2 en R3. Ce seul fait ne certifie
pas le changement de comptage produit par les collisions.

## 3. Un net non pondéré peut perdre une composante

Le sous-échantillon {1/10,10,20} du même P est un net de distance 1/10.
Pourtant sa région d'ordre 2 est vide à r+1/10=1/5. La proximité des
supports ne contrôle donc pas d_k à k fixé.

Pour S⊆P et d=|P\S|, les bornes exactes sont

\[
\Omega_{k+d}^P(r)\subseteq\Omega_k^S(r)
\subseteq\Omega_k^P(r).
\]

Cela autorise disparition et fragmentation ; cela ne prouve pas une
identité de composantes à ordre inchangé. Un transport vers des centres
échantillonnés avec poids des préimages, déplacement maximal δ et masse
totale conservée, donne au contraire les inclusions à seuil de masse fixé
avec rayon r+δ. Il s'agit d'un modèle pondéré explicitement différent.

## 4. Deux absences de stabilité géométrique au même niveau

**Perte de composante.** P={0,2,L,L+1}, P'={−δ,2+δ,L,L+1}, k=2, r=1.
Le script choisit L=100, δ=1/10. Le déplacement apparié est δ et la
stabilité de d2 est respectée. La composante ponctuelle {1} disparaît ;
l'autre composante reste, de trace [L,L+1]. La distance de Hausdorff des
régions est au moins L−1, librement agrandissable malgré δ fixé petit.

**Même composante, même homotopie, saut de la réalisation alpha.**
T={(-4,0),(4,0),(1,2)}, plongé dans R3. Son centre circonscrit est
(0,−11/4), son rayon carré a=377/16. Le troisième site est strictement
intérieur à la boule diamétrale de la base. La date alpha de la base est
donc la date contrainte a de la face, et non sa MEB de rayon carré 16.
Les deux petits côtés sont Gabriel, de dates 29/4 et 13/4.

Comparer T_-=(1−s)T et T_+=(1+s)T au niveau carré fixé a, avec 0<s<1/10.
Le script prend s=1/100. Le déplacement apparié maximal est 8s. Le
complexe T_- est le triangle plein ; T_+ est le V des deux petits côtés.
Les deux complexes sont connexes et contractiles, avec les mêmes trois
sites. Le point (0,0) appartient au triangle initial ; sa distance au V
final vaut (1+s)8/√29. Le carré est obtenu par projections rationnelles
sur les segments, dont le paramètre 20/29 sur le premier côté.

Ainsi le déplacement tend vers zéro tandis que la borne inférieure
géométrique ne tend pas vers zéro, même pour une composante identifiée
ayant H0 et H1 inchangés. Ce saut n'est pas une erreur de dates : base et
face entrent simultanément. Cette paire libre, de même naissance, peut
être effondrée en protégeant les sommets pour résoudre ce témoin précis ;
aucune réduction canonique ou garantie générale n'en est déduite.

## 5. Théorème de comptage à utiliser dans la tour

Supposer une bijection entre les sites conservés, de déplacement maximal δ,
d sites supprimés et e sites ajoutés. Pour tout centre y et tout r≥0,

\[
N_{P'}(y,r+\delta)\ge N_P(y,r)-d,
\quad N_P(y,r+\delta)\ge N_{P'}(y,r)-e.
\]

Chaque site initial dans la boule reste dans la boule agrandie, sauf s'il
est supprimé ; c'est toute la preuve. Donc

\[
\Omega_k^P(r)\subseteq\Omega_{k-d}^{P'}(r+\delta),
\quad
\Omega_k^{P'}(r)\subseteq\Omega_{k-e}^{P}(r+\delta),
\]

pour les grades positifs pertinents. Les grades non positifs donnent la
borne triviale R3. Si d=e=0, l'ordre k est conservé. Sinon, le décalage
d'ordre est essentiel. Ces inclusions induisent les applications de
composantes ; elles ne sont pas des bijections à la même coupe. Pour les
mosaïques A, le passage se fait par les équivalences naturelles avec Ω,
dans la catégorie homotopique ou sur l'homologie/π0, et non par des
inclusions géométriques de réalisations A_P dans A_P'.

Les calculs du script contrôlent quelques instances exactes de ces bornes,
pas tous les centres ou nuages. Une erreur de masse η sur les boules donne
de même un décalage de seuil t→t−η ; un couplage de masses dont au plus η
est transporté à distance supérieure à δ donne les deux décalages
(r,t)→(r+δ,t−η). Ne pas confondre ce η de comptage avec un budget d'erreur
géométrique de réduction.

## Rejeu et fermeture

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Les premières exécutions normal et −O réussissent avec **108 contrôles**,
sorties exactement identiques, sans champ temporel. `attempts.json`
conserve commandes, codes de sortie et stderr exacts ; aucun échec n'a été
rencontré. Les résultats sont `proof.json` et `proof.optimized.json`.
