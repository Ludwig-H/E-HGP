# Mesure Claude — le raffinement post-séparation restaure la décroissance de la fraction (28 août 2026)

Ancrage : `bench/mhgp5_rect_probe --descente-seule` au HEAD `3e785622`,
compteurs déterministes (vérifiés identiques entre 8 fils locaux et 48 fils
G4). Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`mode=mesure`, `public_status=not_claimed`.

## 0. Je corrige encore une conclusion de la journée

J'avais écrit que le raffinement post-séparation « perd sur q4 » (rapport
0,27 : 1 sur `scanline` q4). **C'était le mauvais critère** : je comparais des
visites de nœuds à une *estimation au prorata* du cover évité, à une seule
taille. Le critère qui décide — celui que l'audit exigeait — est l'effet sur
les **visites payées**, et il faut le lire sur **l'exposant**, pas sur un
rapport ponctuel.

## 1. La mesure

`scanline_single_pass`, lane q4, raffinement seul (aucun cover, aucun seed,
aucun corps contrefactuel) :

| $n$ | paires avant | tuées | fraction de $\binom{n}{2}$ avant | **fraction après** |
|---|---|---|---|---|
| 8 000 | 804 786 | 42,8 % | 2,5153 % | 1,4389 % |
| 16 000 | 2 114 704 | 52,1 % | 1,6522 % | 0,7913 % |
| 32 000 | 6 951 708 | **65,4 %** | 1,3578 % | **0,4698 %** |

Le taux de mise à mort **croît** avec $n$ (42,8 → 52,1 → 65,4 %). Et la
grandeur qui décide, l'exposant :

| intervalle | exposant de la fraction **avant** | **après** | exposant des ancres avant → après |
|---|---|---|---|
| 8 000 → 16 000 | −0,61 | **−0,86** | $n^{1{,}39}$ → $n^{1{,}14}$ |
| 16 000 → 32 000 | −0,28 | **−0,75** | $n^{1{,}72}$ → **$n^{1{,}25}$** |

**Le raffinement ramène l'exposant des ancres de 1,72 à 1,25**, et l'écart
grandit avec $n$. Rappel du § 3 bis de `MESURE_CLAUDE_MUR_JUSQU_A_200K` : la
linéarité demande une fraction en $n^{-1}$ ; on passe de $-0{,}28$ à $-0{,}75$.

## 2. Le coût, mesuré et non estimé

À $n = 32\,000$ : **580 417 374 nœuds visités** pour 4 546 210 paires tuées,
soit **128 visites de nœud par paire tuée** (1 613 913 évaluations de cœur,
profondeur maximale 16).

En face, ce qu'une paire tuée épargne : son cover d'ancre. La sonde d'étages
mesure, au même $n$ sur la même famille, **708 593 623 visites de handles pour
1 673 861 covers**, soit **423 visites de site par cover**.

**Rapport ≈ 3,3 pour 1 en faveur du raffinement**, et il croît puisque le taux
de mise à mort croît. C'est la première fois qu'un mécanisme mesuré attaque la
quadraticité elle-même plutôt que sa constante.

## 3. Ce qui n'est pas établi, et que je ne revendique pas

- **Ce n'est pas une mesure de temps de production.** Le raffinement n'est pas
  implémenté dans le pipeline ; ces chiffres sont ceux d'une sonde. Le critère
  de l'audit — baisse du **temps** et des **visites payées** dans le vrai flux
  — reste à établir.
- Les deux masses comparées (128 et 423) viennent de **deux sondes
  différentes** au même $n$ et sur la même famille, pas du même run. Le
  rapport 3,3 est un ordre de grandeur.
- **Rien au-delà de 32 000** pour le raffinement : 100 000 et 200 000 sont en
  cours. Deux intervalles ne font pas une loi, et l'exposant « avant » lui-même
  passe de −0,28 à −0,02 entre 100 000 et 200 000 : c'est là que le raffinement
  doit être jugé.
- **La lane q3 n'est pas mesurée à grande échelle** ici, ni `terrain`, ni
  `eight_clusters`.
- **La route q2 doit rester interdite** : l'audit a gravé la contre-fixture
  `refine-hist-wakeup` (quatre positions, `s=1`, `smax=3`, `h2=2`) montrant
  qu'un témoin du frère peut « revivre » dans l'histogramme enfant et faire
  émettre une boule supplémentaire en q2, faute de prétest ponctuel.
- Le critère `separated` **n'est pas héréditaire** (fixture 1D
  `x = {0, 99, 100, 512, 612}`, `s = 8`) : les enfants ne sont pas des
  rectangles WSPD, et il ne faut pas appeler ce post-traitement une nouvelle
  WSPD. Le front canonique reste terminal à la première séparation.

## 4. Ce que je propose de faire ensuite, dans cet ordre

1. Finir la série d'exposants du raffinement à 100 000 et 200 000, sur q3 et
   q4, `scanline` puis `eight_clusters` (mesure locale, sans session payante).
2. Si l'exposant tient, implémenter le raffinement **en q3/q4 seulement**,
   avec : ledger de masse de paires
   (`emitted + postsep_killed = base_alive`), comparaison du multiensemble
   littéral des couples d'indices sur petits arbres, **route q2 interdite**,
   fixture `refine-hist-wakeup` gravée, et abandon si le temps et les visites
   payées du flux de production ne baissent pas.
3. Ne rien revendiquer avant une campagne appariée digests inchangés.
