# Réponse Claude — V80 confirmée, et le repli par intervalles est INERTE : 0 % capté (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`.

## 1. V80 — le rapport simple/patches tient, et s'améliore avec la taille

| famille | $n=8000$ | $n=16\,000$ | $n=32\,000$ |
|---|---|---|---|
| `scanline` — certificat unique | 98,2 % | **99,8 %** | **99,7 %** |
| `scanline` — patches nécessaires | 1,8 % | 0,2 % | 0,3 % |
| `terrain` — certificat unique | 98,9 % | **99,3 %** | **99,9 %** |
| `terrain` — patches nécessaires | 1,1 % | 0,7 % | 0,1 % |

Ma crainte que les blocs plus gros portent des supports plus divers, donc plus
de patches, est **infondée** : c'est l'inverse. La machinerie de patches vaut
0,1 à 0,7 % à $n = 16\,000$ et $32\,000$. V79 en sort renforcée.

## 2. Le repli par intervalles dirigés : implémenté, validé, et inerte

J'ai implémenté exactement votre spécification — arithmétique d'intervalles
entiers dirigés sur $\Pi(a,b,c;z) = G\,(y \cdot y) - y \cdot W$, avec le carré
prenant zéro comme minimum s'il traverse zéro, et la borne de $G$ intersectée
avec $[0, +\infty)$ par l'identité de Gram.

**Validation d'abord**, pour qu'un résultat négatif ne soit pas un bug : sur
**200 000** triplets à boîtes **ponctuelles**, l'arithmétique reproduit
`q3_power` **exactement** — 200 000 exacts, 0 simplement encadrant, 0 borne
fausse. L'implémentation est juste.

**Résultat sur les vraies boîtes**, sur les blocs du gain marginal :

| famille | blocs captés | appels captés | part du marginal | coût |
|---|---|---|---|---|
| `scanline` | **1** sur 740 | 3 355 | **0,0 %** | 266 142 évaluations |
| `uniform` | **0** sur 898 | 0 | **0,0 %** | 437 986 évaluations |

Vous aviez averti : « cette route est sûre mais **risque d'être lâche** à cause
des dépendances d'intervalles ». La mesure dit qu'elle n'est pas lâche, elle est
**inerte**. La cause est structurelle : $a$ apparaît dans $d = b-a$, dans
$u = c-a$ **et** dans $y = z-a$. L'arithmétique d'intervalles les traite comme
indépendants et perd tout le couplage ; l'élargissement est catastrophique bien
avant que $\Pi_{\mathrm{sup}}$ puisse passer sous zéro.

## 3. L'état du verrou, dit franchement

| | part du travail résiduel |
|---|---|
| certificat **idéal** (connaît les triplets) | 92 à 98 % |
| dont atteignable **sans patches** | 96 à 99,9 % de ce gain |
| certificat de **boîtes** par intervalles sur $\Pi$ | **0 %** |

**Le plafond est élevé, la machinerie lourde est inutile, et aucun certificat de
boîtes connu ne capte quoi que ce soit.** C'est là qu'est le verrou, et il n'est
ni dans les patches ni dans la vacuité.

## 4. La direction que je propose, et pourquoi elle échappe au problème

Ne pas encadrer $\Pi$. Revenir à la formulation par le **centre**, qui élimine
la forme de Gram et le couplage sur $a$. Avec $c$ le circumcentre, « $z$
strictement intérieur » équivaut à $\lVert z-c \rVert^{2} < \lVert a-c \rVert^{2}$,
soit

$$\psi(c,a) = 2\,c \cdot (z-a) + \lVert a \rVert^{2} - \lVert z \rVert^{2} > 0.$$

Deux propriétés, et elles sont exactement ce qui manque à $\Pi$ :

1. **en $a$, le minimum se calcule EXACTEMENT et par axe.** $\psi$ est
   $2c \cdot z - \lVert z \rVert^{2} + \sum_i (a_i^{2} - 2 c_i a_i)$, et chaque
   terme est une parabole convexe en une variable : son minimum sur
   $[\mathrm{lo}_i, \mathrm{hi}_i]$ est atteint en $\mathrm{clamp}(c_i)$. Aucun
   élargissement, aucune énumération de coins — et c'est bien le **minimum**
   qu'il faut, pas le maximum ;
2. **en $c$, $\psi$ est AFFINE.** Son minimum sur un convexe est donc atteint en
   un point extrême, et il suffit d'un **sur-ensemble convexe** de l'ensemble
   des centres réalisables.

Tout le problème se ramène ainsi à **borner l'ensemble des centres**, qui est
une région plane du plan bissecteur — le disque de rayon $D/(2\sqrt{3})$ du
niveau paire, resserré par la bande que $C$ impose. C'est précisément votre
« center-cover resserré par $C$ », et cette formulation-là ne souffre ni de la
forme de Gram, ni du couplage sur $a$, ni de coins (que la fixture
`mhgp5_q3_skinny_center` a réfutés).

## 5. Questions

- **V82.** Confirmez-vous le retrait du repli par intervalles sur $\Pi$ comme
  route de certificat — en gardant sa valeur d'**oracle indépendant**, où il
  reste utile puisqu'il est exact sur boîtes ponctuelles ?
- **V83.** La reformulation du § 4 vous paraît-elle la bonne, et voyez-vous un
  obstacle que je ne vois pas à borner l'ensemble des centres par
  disque ∩ bande, en entiers exacts ?
- **V84.** Si oui, je propose de mesurer d'abord le **plafond de cette
  construction-là** — combien de témoins un disque ∩ bande crédite, contre les
  témoins communs du certificat idéal — avant d'écrire quoi que ce soit dans le
  chemin produit. C'est la méthode qui a écarté quatre directions aujourd'hui.
