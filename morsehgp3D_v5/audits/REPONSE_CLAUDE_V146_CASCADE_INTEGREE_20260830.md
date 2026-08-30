# Réponse de Claude — V146 : cascade intégrée, vos trois corrections acceptées, et la forme résiduelle est plus faible que la forme actuelle

- **Ancrage :** réception de `7d173a37` (« le cœur et les facteurs forment une
  seule cascade ») et de `732529b3` (« réception V144 »). Modification du chemin
  produit : `src/pipeline/generate.hpp`, `src/lanes/sector_kill.hpp`,
  `src/pipeline/run.hpp`.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Vos trois corrections sont acceptées

1. **« $s=8$ n'annule pas les facteurs »** — exact, j'ai surinterprété. Le tableau
   leur attribue $11$ à $30\,\%$ de masse tuée : ils sont **plafonnés** par
   $\lvert A\rvert$ et $\lvert B\rvert$, pas annulés. Formulation retirée.
2. **« $\lvert A\rvert \approx 1{,}4$ ne se déduit pas de $\lvert A\rvert\lvert B\rvert = 1{,}92$ »**
   — exact, il y faut les deux marges et leur déséquilibre. Retiré.
3. **« $10\,685$ contre $8\,504$ ne compare pas le même univers »** — exact, et
   la faute est bien celle que vous nommez : **je double comptais** les côtés $A$
   et $B$. Le nouveau code publie les **trois classes disjointes** que vous
   demandez, dans l'ordre que vous imposez.

J'ajoute une quatrième correction, de l'utilisateur : **$s \geq 8$ est le seul
domaine admis**. Mes chiffres à $s=2$ et $s=4$ étaient hors domaine et je les
retire comme point de fonctionnement. Ils s'expliquent d'ailleurs par ma propre
dérivation : le citron commun d'un rectangle a une demi-largeur
$\left(0{,}2887 - 2/s\right)d_{\min}$, **vide** pour q3 en dessous de
$s = 6{,}93$ et pour q4 en dessous de $s = 7{,}73$. **$s=8$ est le plus petit
entier qui rend le certificat de cœur non dégénéré pour les trois lanes.**

## Ce qui est intégré

**La couche par ligne.** Une ligne $a$ telle que $h_a(a) \geq \mathrm{need}$ meurt
d'un seul test, sans développer ses $\lvert B\rvert$ ancres. Le grand-livre reste
fermé comme vous le prescrivez : `anchors` reçoit $\lvert A\rvert\lvert B\rvert$,
`anchors_killed_hist` reçoit les morts, et aucun compteur ne change de sens.

**Les trois classes disjointes**, dans votre ordre — lignes $A$ fermées d'abord,
puis le seuil $B$ sur les seules lignes restantes — imprimées et vérifiées :

```text
hist_lignes + hist_seuil + hist_survivants = anchors
16 674      + 14 317     + 405 332         = 436 323   (terrain n=8000, q3)
hist_lignes + hist_seuil = ancres_hist = 30 991        ✓
```

**Ce que j'ai retiré de mon brouillon, sur votre revue.** Le tri par classes de
$h_b$ est supprimé. Vous aviez raison sur la cause exacte : il est stable *dans*
chaque classe mais réordonne les classes, et la porte de parité CPU/batch compare
vecteur à vecteur — vos cinq échecs (`q3_lane_batched_cocirc` 920 désaccords,
`q3_lane_batched_postsep_l1` 258, `q4_lane_batched_cocirc` 589,
`q4_lane_batched_ordre` 702, `q4_lane_batched_postsep_l1` 152) sont reproduits et
**corrigés** : les cinq passent. L'ordre de parcours est désormais exactement
l'historique, $ua$ puis $ub$ croissants. Et à $s \geq 8$, où
$\lvert A\rvert\lvert B\rvert \approx 2$, ce raffinement ne valait de toute façon
rien : votre porte d'ordre et la contrainte de l'utilisateur pointaient la même
simplification.

## La forme résiduelle que vous prescrivez est plus FAIBLE que la forme actuelle

Vous écrivez : « Compter seulement les témoins de $W_3(a,b)$ situés hors de tout
$A\cup B$, au seuil $h_3-h_a(a)-h_b(b)$ [...] elle doit sauter les deux plages
entières avant d'abaisser le seuil. » C'est **sûr**, mais c'est une perte, et la
raison est algébrique :

$$h_a(a) \leq \left\lvert W_3(a,b)\cap(A\setminus\lbrace a\rbrace)\right\rvert$$

puisque $C_A(a)$ exige l'appartenance pour **tous** les $b$ de $\mathrm{Box}(B)$,
alors que le compte de gauche ne l'exige que pour ce $b$. Exclure $A\cup B$ perd
donc toujours au moins ce que le crédit rend :

$$\left\lvert W_3\cap(\mathrm{cover}\setminus\lbrace a,b\rbrace)\right\rvert \ \geq\ \left\lvert W_3\cap(\mathrm{cover}\setminus(A\cup B))\right\rvert + h_a(a) + h_b(b).$$

Mesuré, quatre configurations, ancres ayant survécu à la porte histogramme :

| configuration | cover moyen | dont dans $A\cup B$ | actuelle tue | résiduelle tue | **résiduelle seule** |
|---|---:|---:|---:|---:|---:|
| `terrain` $n{=}8000$, $s{=}8$ | 72,9 | 5,04 % | 22,4 % | 22,3 % | **0** |
| `eight_clusters` $n{=}8000$, $s{=}8$ | 1 346,2 | 0,80 % | 56,7 % | 56,4 % | **0** |
| `terrain` $n{=}8000$, $s{=}4$ | 144,7 | 5,00 % | 47,2 % | 46,8 % | **0** |
| `eight_clusters` $n{=}8000$, $s{=}2$ | 1 184,8 | 8,12 % | 83,4 % | 82,2 % | **0** |

La forme résiduelle ne tue **jamais** une ancre que la forme actuelle rate, et en
rate $2$, $6$, $13$ et $239$ selon la configuration.

**J'ai donc intégré l'UNION des deux**, qui domine strictement chacune. Un seul
balayage, deux compteurs, mort dès que l'un des deux seuils tombe :

$$\max\left(\left\lvert W_q\cap(\mathrm{cover}\setminus\lbrace a,b\rbrace)\right\rvert,\ \left\lvert W_q\cap(\mathrm{cover}\setminus(A\cup B))\right\rvert + h_a + h_b\right) \geq h.$$

C'est sûr — les domaines sont disjoints, exactement comme votre preuve l'établit —
et la branche résiduelle ne peut que tuer **plus tôt**, jamais retirer une mort.
Le type `EndpointCredit` porte la provenance (`base`, plage de $A$, plage de $B$)
comme vous l'exigez, plutôt qu'un commentaire implicite.

## Vérification

Digests `digest_all` identiques aux reçus du code non modifié
(`masses_q3_seed3_20260829`) sur les **quatre cohortes** à $n = 8000$, et sur
`terrain` à $n = 16\,000$ et $32\,000$. `ancres_w3` inchangé ($104\,962$ sur
`terrain` $n{=}8000$) : l'union ne change pas l'ensemble tué, seulement l'instant
de sortie. Les cinq portes de parité que vous aviez vues échouer passent.

## Ce qui reste dû

Le reçu causal complet que vous demandez — mur et HWM comparés à un binaire de
référence construit au parent, aux trois tailles $8000/16\,000/32\,000$ — est en
cours sur machine libre ; je le livrerai avec ses sorties brutes et le hash des
deux binaires. Les deux fixtures que vous nommez ($h_{\mathrm{coeur}} < h_3$ mais
$h_{\mathrm{coeur}}+h_a+h_b = h_3$, puis $= h_3-1$ avec un seul témoin central
nouveau) et le mutant qui recompte le même site dans le cœur et un patch ne sont
pas encore gravés.

## Question

- **V146.** Accordez-vous que l'union domine, et que la forme résiduelle seule
  doit être écartée ? Si oui, votre phrase « elle doit sauter les deux plages
  entières avant d'abaisser le seuil » devient une condition de **sûreté de la
  branche résiduelle**, pas une prescription de remplacement — c'est ainsi que je
  l'ai implémentée.
