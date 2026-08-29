# Réponse Claude — V92 : le niveau boîtes atteint 2,0 à 4,3 secteurs sur 8, et l'instabilité est aux FRONTIÈRES (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Sonde `bench/sector_reach_probe.cpp`, $n = 8000$, 3 000 blocs à pas constant.

## 1. Ce que le calcul par boîtes coûte réellement

Ma V90 mesurait le **plancher** — les secteurs atteints par les centres exacts.
Voici ce qu'une implémentation atteindrait, calculé **par boîtes** :

| famille | plancher (centres exacts) | **niveau boîtes** | groupes à 8 secteurs |
|---|---|---|---|
| `scanline` | 1,17 | **2,45** | 10,5 % |
| `terrain` | 1,31 | **3,64** | 26,7 % |
| `uniform` | 1,71 | **4,28** | 25,9 % |
| `eight_clusters` | 1,35 | **1,98** | 3,2 % |

**Sûreté vérifiée : 0 violation** sur les quatre familles — l'ensemble calculé
par boîtes contient toujours l'ensemble exact.

Le calcul est simple parce que le test de secteurs est **par ancre** : la base
$(u,v)$ est donc exacte, et comme $u \perp d$ et $v \perp d$, on a
$p_x \cdot u = (x-m) \cdot u$, **linéaire en $x$**. Les bornes sur
$\mathrm{Box}(C)$ sont donc des intervalles exacts, séparables par axe, en
entiers doublés ($2x - (a+b)$).

**Le gain reste substantiel : exiger le seuil sur 2 à 4,3 secteurs au lieu de
8.** Mais le calcul par boîtes coûte un facteur 2 à 2,8 par rapport au
plancher, et cela doit être dit avant qu'on l'implémente.

## 2. V92 — l'instabilité que vous pressentiez, et où elle est vraiment

Vous demandiez si je sous-estimais une difficulté d'exactitude, « en
particulier près de $q_x = 0$ ». Vous aviez raison qu'il y en a une, mais elle
n'est pas où je la cherchais.

Ma première version produisait 19, 12, 0 et 1 violations de sûreté. Le
diagnostic est net — deux cas fautifs :

```text
intervalle Pu=[-1024,-96] Pv=[-112,0]   centre exact (-32,6 ; 0,0)
intervalle Pu=[-1584,0] Pv=[3652,8776]  centre exact (0,0 ; 742,3)
```

Les centres sont **dans** le rectangle : l'intervalle est correct. Ils sont
**exactement sur une frontière de secteur** ($P_v = 0$, puis $P_u = 0$), et
c'est l'attribution discrète qui diverge.

**L'instabilité réelle est donc aux frontières de secteurs, pas près de
$q_x = 0$** — même si les deux se rencontrent, puisqu'un centre proche de $m$
a une direction mal déterminée. Le traitement est celui que vous suggériez pour
$q_x = 0$ : **marquer les deux secteurs adjacents**, ce qui est le sens
conservateur (on en marque plus, jamais moins). Après ce correctif : **0
violation partout**.

En entiers, ce traitement est même plus simple qu'en flottant : la frontière
d'un secteur est un produit vectoriel **nul** avec un sommet $p_k$ de
l'octogone, ce qui se teste exactement, et le cas nul marque les deux côtés.

## 3. Ce que la mesure laisse comme perte

Les groupes dont le rectangle $[P_u] \times [P_v]$ **contient l'origine**
atteignent les 8 secteurs et ne rapportent rien : 3,2 % sur `eight_clusters`,
mais **26,7 %** sur `terrain` et 25,9 % sur `uniform`. C'est exactement le cas
$q_x \approx 0$ — le handle contient des positions des deux côtés du plan qui
sépare, donc la direction du centre n'est pas contrainte.

C'est la principale perte du niveau boîtes, et elle est concentrée : un **split
de $C$** la réduirait mécaniquement, puisqu'un enfant a moins de chances de
contenir l'origine. Mais je ne le propose pas — j'ai déjà mesuré deux fois
qu'un split payé d'avance ne rapporte pas, et il faudrait ici mesurer le
plafond de ce split avant d'y toucher.

## 4. Questions

- **V94.** Recevez-vous le niveau boîtes (2,0 à 4,3 sur 8) et le traitement
  conservateur des frontières ? C'est le dernier chiffre qui manquait avant
  l'implémentation que vous décriviez en V93.
- **V95.** La perte concentrée du § 3 — 26,7 % de groupes à direction non
  contrainte sur `terrain` — vaut-elle une mesure de plafond du split de $C$,
  ou préférez-vous livrer d'abord la version sans split et voir ce qu'elle
  donne réellement ?
- **V96.** Confirmez-vous que l'implémentation entière doit tester
  l'appartenance à un secteur par produit vectoriel avec les sommets $p_k$ de
  l'octogone — le produit **nul** marquant les deux secteurs — plutôt que par
  un `atan2` comme dans cette sonde de mesure ?
