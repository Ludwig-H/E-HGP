# MorseHGP3D v2 — mesures

> Faits mesurés le 8 août 2026 sur le codespace (CPU, pas de GPU). Aucun statut
> public, aucun budget revendiqué.

## 1. Exactitude

`./build/mhgp_oracle` compare le pipeline à un oracle exhaustif sur 24 nuages
entiers tirés au hasard ($n\in[8;12]$, $K\in[2;4]$) :

| porte | contenu | résultat |
| --- | --- | ---: |
| P0 | catalogue des sphères critiques contre l'énumération de **tous** les supports de taille 1 à 4 | **identique**, 24/24 |
| P2 | $\#\pi_0\left(\Gamma_k(a)\right)$ par force brute sur $\binom{n}{k}$ et $\binom{n}{k+1}$, contre la forêt | **496 cas, 0 désaccord** |

P0 et P2 passent aussi bien avec le voisinage adaptatif certifié qu'avec le
voisinage exhaustif : la procédure de croissance du §3.1 ne perd rien sur ces
nuages.

Un prototype rationnel indépendant (Python, `Fraction`) avait auparavant validé
le même modèle contre le même oracle sur une trentaine de configurations
$n\in[10;12]$, $K\in[3;4]$ — 0 désaccord sur plus de 1 000 comparaisons de
partitions, en comparant non seulement le nombre de composantes mais la
**partition induite sur les minima**.

Quatre défauts ont été trouvés — **deux par les portes, deux par les audits** —
et corrigés :

- `well_centered4` perdait le signe de l'orientation, `sphere4` ayant normalisé
  `den > 0` : le test de bon centrage dépendait alors de l'ordre des sommets et
  acceptait des tétraèdres non bien centrés (3 à 6 par nuage) ;
- les supports de taille 1 n'étaient pas émis ;
- l'élagage par cliques de faces perdait 0,63 % du catalogue (§3 bis) ;
- les unions d'un lot de même niveau étaient appliquées **événement par
  événement**, produisant une chaîne de fusions binaires au lieu d'une
  multifusion contractée. Sur le carré à l'ordre 1, la forêt comptait trois
  nœuds de niveau 1 enchaînés au lieu d'un nœud à quatre enfants. Les composantes
  après le lot étant les mêmes, **P2 ne pouvait pas le voir** : c'est la limite
  d'un oracle qui ne compare que des nombres de composantes. La régression R6 le
  couvre ; l'obligation d'un oracle comparant la *structure* du merge tree reste
  ouverte.

## 1 bis. Ce que le second audit a invalidé dans ces mesures

Les chiffres du §2 et du §3 ont été relevés **avant** la suppression de
l'élagage par cliques de faces, dont le §2 de
`WARNING_AUDIT_IMPLEMENTATION_2.md` a montré qu'il est faux. Ils portent donc sur
un catalogue **incomplet** : la taille mesurée est un **minorant** et le temps
mesuré un **minorant** lui aussi. Ils restent publiés parce que l'ordre de
grandeur de l'objet et l'exposant du coût sont les deux faits utiles, et qu'ils
ne peuvent que croître. Les mesures d'après correction sont à refaire.

Les portes P0 et P2 passaient déjà avant la correction : c'est précisément ce que
l'audit reproche à l'oracle actuel — il partage ses primitives avec le chemin
testé, et sa comparaison ne porte que sur les identifiants de support. La
fixture R3 comble ce trou pour ce cas précis ; l'obligation générale reste
ouverte (`DESIGN.md` §10.1, point 2).

## 2. Taille de l'objet

$K=10$, donc $s_{\max}=11$, nuage uniforme dans le cube :

| $n$ | sphères critiques | par point | $\lvert W_p\rvert$ moyen | doublements max | points non certifiés |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 200 | 45 952 | 229,8 | 199,0 | 2 | 0 |
| 500 | 138 278 | 276,6 | 499,0 | 3 | 0 |

**La taille du catalogue est de l'ordre de 250 à 280 objets par point.**
Extrapolée à $n=50\,000$, elle donne $1{,}2$ à $1{,}4\cdot10^{7}$ sphères
critiques — **le même ordre de grandeur que les $1{,}8\cdot10^{7}$ records utiles
estimés par `CONTRAT_50K_BILAN.md` §3**. Les deux chemins, très différents,
convergent sur la taille de l'objet mathématique : c'est un argument fort en
faveur du modèle, et cela fixe le plancher que tout pipeline devra payer.

À ces tailles, $\lvert W_p\rvert=n-1$ : la majoration a priori ne mord pas encore,
parce qu'elle exige au moins $s_{\max}$ points de dénominateur positif **par
cône**, soit environ $s_{\max}\times m$ points au total ($m=42$ cônes, donc
$\approx460$). Le régime où $\lvert W_p\rvert$ se détache de $n$ commence donc
au-delà de $n\approx10^{3}$ ; il n'est pas encore mesuré ici.

## 3. Coût, et pourquoi le contrat n'est pas atteint

| $n$ | temps catalogue |
| ---: | ---: |
| 200 | 2,61 s |
| 500 | 40,18 s |

Le rapport $40{,}18/2{,}61=15{,}4$ pour un facteur 2,5 en $n$ donne un exposant
empirique $\log_{2{,}5}15{,}4=2{,}98$ : le coût observé est **cubique**, ce qui
est exactement $\Theta\left(n\cdot\lvert W_p\rvert^{2}\right)$ avec
$\lvert W_p\rvert=n$.

C'est le verrou, et il est structurel dans cette version : l'énumération teste
toutes les paires du voisinage pour construire le graphe des faces admissibles.
Deux termes le composent, et un seul est irréductible :

1. $\lvert W_p\rvert$ doit se stabiliser quand $n$ croît. La majoration du §3.1
   le permet en principe — elle donne, en régime uniforme,
   $\lvert W_p\rvert\approx s_{\max}\,m/(\cos\theta-\sin\theta)^{3}$, soit
   $\approx1\,700$ pour $m=42$ — mais **elle ne le garantit pas** : le §2 de
   l'audit rappelle que le pire cas reste $\Theta(n)$ par point.
2. Même à $\lvert W_p\rvert\approx1\,700$, le terme quadratique vaut
   $1{,}4\cdot10^{6}$ paires par point, soit $7\cdot10^{10}$ à $n=50\,000$ :
   hors budget de plusieurs ordres de grandeur.

Après suppression de l'élagage faux, l'énumération des supports de taille
quatre parcourt les triplets dont les trois paires respectent la borne de
diamètre $2\tau$ : le coût par point passe de $\Theta(m_p^{2})$ à
$\Theta(m_p^{3})$, et le coût total à $\Theta\left(\sum_p m_p^{3}\right)$.
L'écart au budget se creuse donc, ce qui ne change pas la conclusion mais en
renforce l'urgence.

**Le chemin est donc identifié et il est unique** : remplacer l'énumération
quadratique par le *peeling* local du §3, qui ne visite que les cellules
réellement présentes du $\leq(s_{\max}-2)$-level dual — donc $\Theta$ de la
sortie locale, environ 250 objets par point au lieu de $1{,}4\cdot10^{6}$
candidats. C'est le seul travail qui sépare cette version d'un budget
compatible avec le contrat ; il est indépendant de tout le reste (arithmétique,
catalogue, forêt, oracle sont en place et validés).

## 3 bis. Mesure d'après correction, à $n=200$

| grandeur | avec l'élagage faux | après suppression |
| --- | ---: | ---: |
| sphères critiques | 45 952 | **46 243** |
| quadruplets examinés | 512 033 | **258 739 800** |
| temps catalogue | 2,61 s | **92,54 s** |

L'élagage perdait donc **291 sphères critiques sur 46 243**, soit 0,63 % du
catalogue — assez peu pour n'être vu par aucun accord moyen, assez pour rendre le
résultat faux. C'est exactement la raison pour laquelle une porte d'exactitude ne
peut pas être un benchmark. Le prix de la correction est un facteur 35 en temps
et 505 en candidats examinés : le coût par point est passé de $\Theta(m_p^{2})$
à $\Theta(m_p^{3})$, comme annoncé.

## 4. Ce qui n'a pas été mesuré

- Aucune exécution GPU. Aucun kernel CUDA n'existe dans cette version.
- Aucune mesure au-delà de $n=500$ (le coût cubique l'interdit sur cette
  machine).
- La distribution de $\lvert W_p\rvert$ en régime $n\gg s_{\max}m$, qui décide de
  la viabilité de la borne certifiée, reste à établir.
- Les flèches verticales et le `coverage_log` ne sont pas produits (obligations
  de preuve V.1 et C.1 de `DESIGN.md`).
