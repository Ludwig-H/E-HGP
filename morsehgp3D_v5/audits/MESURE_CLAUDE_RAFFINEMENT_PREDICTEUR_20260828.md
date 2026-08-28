# Mesure Claude — le raffinement post-séparation a un prédicteur, et il ne sauve pas `terrain` (28 août 2026)

Ancrage : `bench/mhgp5_rect_probe` au HEAD `4ecb57d4`, un fil, machine
partagée — **compteurs** citables, temps non citables. Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `mode=mesure`,
`public_status=not_claimed`.

Ce document répond à la demande de l'audit : « ventiler par classe les verdicts
`anchor_kill_cumulated` en `k=1` (`W_q`) et `k=2` (secteurs) ; seule la masse
`k=1` borne le gain de mortalité accessible au certificat universel ».

## 1. Le raffinement post-séparation, mesuré sur cinq cas

Prototype instrumenté (`descente_prolongee`) : on prolonge la descente
ternaire **à l'intérieur** d'un rectangle vivant, en réévaluant le certificat
universel de chaque sous-rectangle, jusqu'à 4 paires ou profondeur 40. Rien
n'est changé à la production ; c'est une mesure.

| cas ($n = 16\,000$) | paires tuées | coût mesuré (nœuds visités) | cover évité (estimation au prorata) | rapport |
|---|---|---|---|---|
| `scanline` q3 | **43,8 %** | 84,4 M | 534,8 M | **6,3 : 1** |
| `eight_clusters` q4 | 40,4 % | 658,7 M | 476,4 M | 0,72 : 1 |
| `scanline` q4 | 21,2 % | 109,2 M | 29,6 M | 0,27 : 1 |
| `uniform` q3 | 6,1 % | 201,0 M | 33,6 M | 0,17 : 1 |
| `terrain` q4 | **5,9 %** | 69,8 M | 4,0 M | **0,06 : 1** |

À $n = 8\,000$, `scanline` q3 donnait 33,7 % et un rapport de 3,4 : 1 : **c'est
le seul cas où le gain croît avec $n$**, donc le seul qui puisse toucher un
exposant plutôt qu'une constante.

**Conclusion nette et défavorable à ma propre piste : le raffinement
post-séparation paie sur un seul cas (`scanline` q3) et perd partout ailleurs
— en particulier sur `terrain` q4, qui est précisément le régime qui ne tient
pas les contrats d'échelle.** Il ne peut donc pas être présenté comme la
réponse au problème ; au mieux comme une optimisation q3 conditionnelle.

Réserve à charge : « cover évité » est une **estimation au prorata** des
paires (je répartis le cover d'un rectangle proportionnellement à ses paires),
presque certainement fausse puisque les ancres lourdes dominent. Seule la
colonne « coût » est une mesure. Et le critère de succès retenu par l'audit
n'est ni l'un ni l'autre : c'est la baisse du **temps** et des **visites
payées**, à établir sur le flux de production, pas sur ce prototype.

## 2. Le prédicteur : la masse `k=1`

`scanline` q3, $n = 16\,000$ — ventilation par cause sur les ancres vivantes
(post-histogramme) :

| cause | ancres tuées | part |
|---|---|---|
| `k=1` — $W_q$ exact, certificat **universel** | 546 779 | **42,7 %** |
| `k=2` — secteurs, **ancre-spécifique** | 94 548 | 7,4 % |
| vivantes après les deux | 639 574 | 49,9 % |

Le lien avec le raffinement, en comptabilité alignée (c'est le point sur lequel
l'audit m'a repris, et il avait raison) :

- le raffinement part de **toutes** les paires des rectangles vivants
  (1 591 516), pas des ancres post-histogramme (1 280 901) : il tue donc
  d'abord, gratuitement, les 310 615 paires que l'histogramme aurait tuées ;
- il reste $696\,537 - 310\,615 = 385\,922$ paires tuées qui relèvent du
  certificat universel, sur une masse `k=1` de 546 779 : **le raffinement en
  récupère 70,6 %**.
- Sur `uniform` q3, la masse `k=1` est de 564 834 ancres et le raffinement ne
  tue que 141 246 paires **au total** : il en récupère donc **moins de 25 %**.

D'où l'énoncé que je propose, et qui est falsifiable :

> **Le gain du raffinement post-séparation est borné par la masse `k=1` du
> cas, et le taux de récupération varie fortement (70,6 % sur `scanline` q3,
> moins de 25 % sur `uniform` q3).** Là où le certificat universel ne tue rien
> — la lane q4, où $W_4$ tue **zéro** ancre sur toutes les familles mesurées —
> le raffinement ne peut rien gagner, et la mesure le confirme (0,06 : 1 à
> 0,72 : 1).

C'est exactement la prédiction de l'audit (« le zéro `W4` déjà observé fait de
ce raccord une priorité **q3** »). La mesure la confirme au lieu de la
supposer, et elle en donne le coefficient.

## 3. Ce que cela impose comme suite

1. Le raffinement n'est **pas** la réponse au passage à l'échelle. Il reste un
   candidat q3, à ne juger que sur le temps et les visites payées du flux de
   production, avec le ledger de masse de paires, la route q2 interdite et la
   fixture `refine-hist-wakeup`.
2. **`terrain` q4 reste entier.** Son coût par ancre croît, et aucun des
   leviers mesurés ne l'attaque : $W_4$ n'y tue rien, les secteurs peu, le
   raffinement rien. C'est là qu'il faut instrumenter le flux réel (V39) avant
   toute conception — c'est la mesure suivante, et elle est en cours
   (`bench/mhgp5_q4_stage_probe`, familles `terrain` / `scanline` / `uniform`
   à 4 000, 8 000, 16 000).
3. Je ne propose aucune conception pour `terrain` tant que je n'ai pas la
   décomposition de son coût par étage. Proposer avant serait viser au hasard.
