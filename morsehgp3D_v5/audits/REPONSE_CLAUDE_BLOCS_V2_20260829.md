# Réponse Claude — sonde de bloc v2 : les quatre rétractations appliquées, et le chiffre qui change (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `51ca037b`. Vos quatre rétractations sont acceptées sans réserve et
appliquées dans `bench/block_witness_probe.cpp` v2. Les quatre demandes du
« prochain probe » sont livrées : vrai idéal `min_exact_ball_depth`, baseline
sur toutes les ancres actives, masse des blocs capés, pondération par travail.

## 1. Votre réfutation des $8^{3}$ coins est meilleure que la mienne — je retire la mienne

J'avais trouvé indépendamment un contre-exemple (boîtes de côté 3, coordonnées
à deux chiffres, recherche aléatoire) et commencé à le graver. **Le vôtre est
strictement meilleur** : plan, symétrique, à boîte $C$ **plate**, et il nomme la
cause au lieu de la constater — `q3_power` n'est pas séparément convexe dans le
carrier. Vérifié indépendamment avec le `q3_form`/`q3_power` du dépôt :

```text
|ab|² = 1600 pour les trois ; ab arête maximale STRICTE et angle aigu : OUI, OUI, OUI
q3_power(a,b,x-;z) = -57 600 000     x- = (20,24,0)
q3_power(a,b,x0;z) = +38 400 000     x0 = (30,24,0)   <- point ENTIER intérieur du segment
q3_power(a,b,x+;z) = -57 600 000     x+ = (40,24,0)
```

`mhgp5_q3_skinny_center` est déjà câblée. J'ai **supprimé** ma fixture
redondante `tests/block_corner_refutation.cpp` : deux fixtures pour le même
fait, dont une moins bonne, ne valent pas mieux qu'une.

## 2. Ce que la v2 corrige, et pourquoi cela va dans les deux sens

**Rétractation 1 (la plus importante).** `tb` comptait les témoins **communs** à
toutes les boules du bloc — un **minorant** de ce qui décide, pas l'idéal. Le
vrai critère est : le bloc meurt ssi **chacun** de ses triplets meurt, donc ssi
$\min_{t} \mathrm{depth}(t) \ge h_3$. La v2 le calcule, avec sortie anticipée à
$h_3$ (le minimum n'a pas à être connu au-delà du seuil), ce qui le rend
**moins** cher que l'ancien compte commun.

**Rétractation 2.** La baseline porte maintenant sur **toutes** les ancres
actives du bloc : il meurt au niveau paire ssi chacune a $h_3$ témoins de
fuseau. C'est nettement plus strict que la seule paire `(ra.first, rb.first)`.

**Rétractations 3 et 4.** Les blocs sans triplet valide sont publiés comme
**cible et non comme acquis** — aucun classifieur de boîtes ne les reconnaît
encore ; et les blocs capés sont comptés en blocs **et** en triplets.

## 3. Le résultat corrigé

$n = 8000$, $h_3 = 9$, 3 000 blocs échantillonnés à pas constant :

| | `scanline` | `terrain` |
|---|---|---|
| blocs sans triplet valide (cible, non acquis) | 49,3 % | 53,0 % |
| blocs capés (non jugés) | 4 | 0 |
| triplets valides par bloc jugé | 76,5 | 17,6 |
| **idéal vrai** ($\min_t \mathrm{depth} \ge h_3$), en **blocs** | **74,1 %** | **73,1 %** |
| baseline paire (toutes ancres), en **blocs** | 25,5 % | 19,3 % |
| **idéal vrai, pondéré par le TRAVAIL** | **99,7 %** | **99,5 %** |
| **baseline paire, pondérée par le TRAVAIL** | **78,9 %** | **76,2 %** |

**En blocs, l'avantage est plus grand que je ne l'avais dit** (74 % contre 20 à
26 %, soit 3 à 4 fois, contre « deux fois » en v1). **En travail, il est
beaucoup plus petit** : la baseline paire capture déjà 76 à 79 % du travail.

La raison est nette et elle corrige mon intuition : **le certificat de paire tue
déjà les blocs LOURDS.** Un bloc lourd a un gros cover, donc beaucoup de
témoins, donc $W_3 \ge 9$ facilement. Les blocs que la paire rate sont les
**petits**, qui pèsent peu. Mon « deux fois plus de morts » comptait des blocs,
pas du travail.

**Ce qui reste vrai, et c'est la bonne façon de le dire :** le travail
**survivant** passe de $21{,}1\ \%$ à $0{,}3\ \%$ sur `scanline` et de
$23{,}8\ \%$ à $0{,}5\ \%$ sur `terrain` — une réduction du **résidu** d'un
facteur ≈ 70 et ≈ 48. C'est cela que la fibre achèterait, si un certificat de
boîtes atteignait l'idéal.

## 4. Ce que ce chiffre n'est toujours pas

- **C'est l'idéal**, pas un certificat de boîtes. Tout l'écart reste à payer, et
  votre § « certificat sûr » dit que la route par patches perdra les couplages
  (trois médiatrices séparées, coplanarité ignorée en q3) : le certificat
  réalisable sera **strictement en dessous** de 99,7 %.
- **La baseline n'est pas la production.** Elle n'évalue que $W_3$. La chaîne
  réelle ajoute histogramme de coins, secteurs $K=8$ et grille de cellules, donc
  la production tue **plus** que 78,9 %, et la marge réelle de la fibre est plus
  petite que 21 points.
- **Une taille, un échantillon, pas de reçu.** Aucun exposant n'en sort.

## 5. Ce que je fais ensuite, sauf avis contraire

Votre ordre d'implémentation est clair et je le prends tel quel :

```text
RectId(A,B), core IDs, h_a, h_b
  -> handles C + fate DEAD_OUTSIDE_WINDOW
  -> EMPTY/NONE_ACUTE/NONE_OWNER certifiés, sinon masque de patches
  -> médiatrices AB/AC/BC
  -> h0 par patch + minima h_a/h_b
  -> bloc entièrement mort, split borné, ou pending
```

Je commencerais par les **certificats `EMPTY`/`NONE_ACUTE`/`NONE_OWNER` par
boîtes**, parce que c'est eux qui attaquent les 49 à 53 % de blocs sans triplet
valide — la seule masse que la v2 identifie comme entièrement à prendre et que
rien ne reconnaît aujourd'hui.

## 6. Questions

- **V64.** Le renversement du § 3 — avantage en blocs plus grand, avantage en
  travail plus petit — change-t-il votre priorité entre la fibre et le
  center-cover counter-only, ou les deux convergent-ils vers le même premier
  incrément ?
- **V65.** Pour les 49 à 53 % de blocs sans triplet valide, un certificat
  `NONE_ACUTE` par boîtes existe-t-il en $O(1)$ ? La condition d'acuité est
  $\lVert 2x - a - b \rVert^{2} > \lVert b-a \rVert^{2}$ ; sur des boîtes, un
  majorant du membre gauche et un minorant du membre droit se calculent
  exactement. Est-ce suffisant en pratique, ou la lentille
  ($\lVert x-a \rVert \le \lVert ab \rVert$) est-elle la vraie cause de vacuité ?
- **V66.** Confirmez-vous que la pondération par travail que j'emploie
  (triplets valides × candidats de cover) est la bonne unité, ou préférez-vous
  les rescans réellement évités, qui demanderaient de rejouer la chaîne
  complète par bloc ?
