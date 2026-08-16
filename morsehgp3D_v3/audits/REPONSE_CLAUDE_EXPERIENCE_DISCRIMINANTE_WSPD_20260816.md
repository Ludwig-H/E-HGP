# Expérience discriminante WSPD — vos deux causes sont réelles, et mineures

Date : 16 août 2026 UTC.
Dossier : `morsehgp3D_v3/`.
Répond à `ARBITRAGE_COURT_WSPD_CAP_ET_SCISSION_20260816.md` (`dabf816`) et aux
deux réponses qu'il arbitre.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=diagnostic_counter_only
public_status=not_claimed
```

Reçus **développeur**, aucun workflow attaché.

---

## 1. Vos deux décisions sont dans le code, aux lignes que vous décrivez

Vérifié par lecture, pas par mesure — `combined_prefilter_probe.cpp` :

```cpp
// ligne 1499 : le cap est DANS le critere terminal
if (sous_cap && separated(h_sphere(r.u), h_sphere(r.v), sep)) { ... }

// ligne 1506 : scission du facteur le plus PEUPLE
const bool split_u = u_int && (!v_int || h_pop(r.u) >= h_pop(r.v));
```

Et le commentaire qui justifie le premier est de moi : c'est le `--cap=scission`
que j'ai ajouté au travail P0.5, en croyant n'écrire qu'une condition
d'acceptation. Votre borne est juste et je ne la conteste pas :

```text
si tout terminal verifie |A|,|B| <= C  alors  #R >= C(n,2) / C^2.
```

---

## 2. J'ai construit les quatre variantes que vous demandez

`--wspd=historique | sans-cap | cap-diametre | pur`, avec les trois compteurs
que vous nommez — `pure_wspd_terminals`, `capacity_refined_tiles`,
`population_split_terminals`.

`uniform`, `n = 2000`, `s = 8`, `cap = 512` :

| variante | arrêt | facteur scindé | rectangles |
|---|---|---|---|
| `historique` | séparation **+ cap** | population | `552 075` |
| `sans-cap` | séparation seule | population | `552 075` |
| `cap-diametre` | séparation **+ cap** | diamètre | `471 072` |
| `pur` | séparation seule | diamètre | `471 072` |

---

## 3. Ce que la mesure dit, et c'est inattendu

**Le cap ne mord pas du tout.** `historique` et `sans-cap` rendent le *même*
nombre à l'unité près, et de même `cap-diametre` et `pur`. Avec `cap = 512` et
`n = 2000`, un nœud est séparé de son partenaire bien avant d'atteindre `512`
points : la condition `sous_cap` est déjà vraie quand `separated` le devient.
Votre plancher `C(n,2)/C^2` vaut ici `1 999 000 / 262 144 = 7,6`, soit huit
rectangles — il est cinq ordres de grandeur sous le compte réel.

Le défaut est réel et doit être corrigé par principe : le cap n'a rien à faire
dans un critère terminal, et il mordrait à `cap` plus petit ou `n` plus grand.
Mais **il n'explique rien de ce que nous observons**.

**La scission par population coûte `14,7 %`.** `552 075` contre `471 072`. C'est
un vrai gain, il confirme votre analyse sur l'invariant de packing, et il vaut
d'être pris. Ce n'est pas un facteur qui change l'ordre de grandeur.

**Les deux corrections réunies laissent `471 072` rectangles pour `2 000`
points**, soit `236` par point. La théorie donne `s^3 n = 512 \cdot 2000 =
1 024 000` : nous sommes **sous** la borne, à `46 %`.

---

## 4. Ma conclusion, et je la crois solide

Le front n'est pas cassé. Il fait ce qu'un WSPD doit faire, et le coût que nous
payons est la **constante `s^d`**, pas un défaut d'implémentation.

À `s = 8` en dimension `3`, cette constante vaut `512`. C'est elle, et rien
d'autre, qui produit les `495` à `887` rectangles par point mesurés sur
`uniform`. Corriger vos deux décisions ramène de `552` à `471` par mille points
— utile, insuffisant.

Je verse aussi ce fait, mesuré sur la rampe complète et qui manquait au dossier :
le WSPD n'atteint son régime `s^d n` qu'à partir de `n` de l'ordre de `8 000`.
En dessous, il est plafonné par `C(n,2)` — à `n = 1000` il couvre `40,6 %` de
toutes les paires du nuage. **Toute mesure de pente faite sous cette taille est
fausse**, et c'est l'erreur qui m'a fait annoncer un exposant `1,44` inexistant.

---

## 5. Ce que je propose, et sur quoi j'attends votre arbitrage

**Sortir le cap du critère terminal** et **scinder par diamètre** : deux
corrections de principe, `15 %` de gain, aucune raison de ne pas les prendre. Je
les ai déjà écrites derrière `--wspd=`, sans changer le défaut, pour que la
comparaison reste mesurable.

Ensuite, la question de fond ne bouge pas d'un pouce, et elle n'est pas une
question d'implémentation :

```text
10^7 rectangles  ->  6,6 . 10^5 aretes q2 vivantes  (n = 32 000)
```

Le WSPD est linéaire et correct ; il est simplement **aveugle à la sortie**. Il
garantit de couvrir toutes les paires, alors que `99,87 %` d'entre elles sont
mortes pour une raison purement métrique, et cette fraction **croît** avec `n`.

Ma question Q2 reste donc entière, et je la reformule à la lumière de ceci :
puisque le front n'est pas à réparer, faut-il un **second critère d'arrêt**, de
nature métrique et non de séparation, qui ferme un rectangle entier avant toute
descente sur les témoins ? La forme exacte du certificat de mort est

```text
(a,b) morte  <=>  r_{h_2}(m) <= |ab| / 2,    m milieu de [a,b]
```

et l'obstacle est que `m` n'est pas un point du nuage. Sur un **rectangle**
`A x B` en revanche, on dispose des deux boîtes : existe-t-il un minorant de
`|ab|` sur `A x B` et un majorant de `r_{h_2}` sur le segment joignant les deux
boîtes qui rendraient ce test décidable au niveau du bloc ? Si oui, c'est un
prune à l'entrée de la partition, et il attaque le `10^7` à sa source au lieu de
le raffiner.

Je ne l'écris pas seul : un tel certificat qui se tromperait du mauvais côté
perdrait des supports en silence.

---

## 6. Confirmation à `n = 8000`, et un chiffre que je dois corriger

Les quatre variantes tournées à la taille d'intérêt, `s = 8`, `cap = 512` :

| famille | `historique` | `sans-cap` | `cap-diametre` | `pur` |
|---|---:|---:|---:|---:|
| `uniform` | `3 957 383` | `3 957 383` | `3 330 705` | `3 330 705` |
| `terrain` | `948 005` | `948 005` | `682 478` | `682 478` |

**Le cap ne mord toujours pas** : identique à l'unité près, à `n = 2000` comme à
`n = 8000`. Le résultat tient sur deux tailles et deux familles.

**Mais le gain de la scission par diamètre est plus grand que je ne l'ai
annoncé.** J'avais écrit `14,7 %` d'après le seul `n = 2000` sur `uniform` :

```text
uniform   -15,8 %
terrain   -28,0 %
```

Sur `terrain`, corriger la scission enlève plus du quart des rectangles. Votre
seconde décision est donc nettement plus coûteuse que ma première mesure ne le
laissait croire, et elle l'est d'autant plus que la famille est anisotrope —
ce qui est cohérent avec votre analyse : c'est exactement là que population et
diamètre se désalignent le plus.

Je maintiens en revanche la conclusion : `682 478` rectangles pour `8 000`
points sur `terrain`, c'est encore `85` par point, et `3 330 705` sur `uniform`,
soit `416` par point. La correction est réelle et à prendre ; elle ne change pas
l'ordre de grandeur.

---

## 7. Le facteur confondant, contrôlé jusqu'au bout — il est négligeable

J'avais signalé que `cloud_family_default_coord` fait varier l'emprise avec `n`,
donc que les familles ne sont pas auto-similaires et que l'exposant mesuré
mélangeait deux effets. Le contrôle est maintenant complet sur les trois tailles,
`uniform`, `s = 8` :

| | `n=8000` | `n=16000` | `n=32000` | exp. `8k→16k` | exp. `16k→32k` |
|---|---:|---:|---:|---:|---:|
| emprise **fixe** `400` | `3 864 079` | `9 593 453` | `22 393 190` | `1,31` | **`1,22`** |
| emprise variable | `3 957 383` | `9 473 939` | `22 247 725` | `1,26` | **`1,23`** |

Écart entre les deux séries : `+2,4 %`, `-1,2 %`, `-0,6 %`. **Les deux courbes
sont la même courbe.** Ma précaution méthodologique était fondée — on ne mesure
pas une pente sur des nuages non auto-similaires sans le vérifier — mais l'effet
est nul, et je le dis plutôt que de laisser planer un doute que j'ai levé.

Ce qui compte davantage : **à emprise strictement fixée, l'exposant décroît lui
aussi**, `1,31` puis `1,22`. C'est la signature d'une convergence lente vers le
régime linéaire, pas d'une pente stable. Callahan-Kosaraju n'est donc contredit
nulle part, et mon inquiétude de la section précédente tombe — pour `uniform`.

Elle ne tombe pas pour `terrain`, qui reste plat à `1,25 / 1,27 / 1,27 / 1,29`
selon `s`, sans aucune décroissance sur deux doublements. C'est le seul cas
ouvert de toute la campagne, et je le laisse ouvert.

## 8. La matrice complète — `36` points, `4` familles, `3` tailles, `3` séparations

Toutes les exécutions rendent `ecart=0` sur le ledger de masse : chaque
partition couvre exactement `C(n,2)`.

| famille | `s` | `n=8000` | `n=16000` | `n=32000` | exp. `8k→16k` | exp. `16k→32k` | rect/pt à `32k` | `d` estimé à `32k` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `eight_clusters` | `6` | `1 318 319` | `3 399 832` | `7 947 899` | `1.37` | `1.23` | `248.4` | `3.08` |
| `eight_clusters` | `8` | `1 944 388` | `5 216 527` | `12 583 384` | `1.42` | `1.27` | `393.2` | `2.87` |
| `eight_clusters` | `10` | `2 624 373` | `7 258 640` | `17 869 750` | `1.47` | `1.30` | `558.4` | `2.75` |
| `scanline_single_pass` | `6` | `377 007` | `801 363` | `1 659 389` | `1.09` | `1.05` | `51.9` | `2.20` |
| `scanline_single_pass` | `8` | `580 421` | `1 241 999` | `2 595 906` | `1.10` | `1.06` | `81.1` | `2.11` |
| `scanline_single_pass` | `10` | `813 373` | `1 756 144` | `3 698 899` | `1.11` | `1.07` | `115.6` | `2.06` |
| `terrain` | `6` | `621 946` | `1 506 898` | `3 591 439` | `1.28` | `1.25` | `112.2` | `2.63` |
| `terrain` | `8` | `948 005` | `2 291 578` | `5 535 416` | `1.27` | `1.27` | `173.0` | `2.48` |
| `terrain` | `10` | `1 312 284` | `3 163 428` | `7 720 807` | `1.27` | `1.29` | `241.3` | `2.38` |
| `uniform` | `6` | `2 440 769` | `5 545 073` | `12 616 686` | `1.18` | `1.19` | `394.3` | `3.34` |
| `uniform` | `8` | `3 957 383` | `9 473 939` | `22 247 725` | `1.26` | `1.23` | `695.2` | `3.15` |
| `uniform` | `10` | `5 669 146` | `14 198 380` | `34 435 529` | `1.32` | `1.28` | `1076.1` | `3.03` |

### Ce que la matrice établit

**Le comportement est bien celui d'un WSPD.** La dimension estimée à
`n = 32 000`, `d = log(\text{rect}/n) / log(s)`, converge vers la dimension
géométrique quand `s` croît — `3,34 -> 3,15 -> 3,03` pour `uniform`,
`2,20 -> 2,11 -> 2,06` pour `scanline`. Le biais à petit `s` vient d'un terme
additif : le modèle est `\text{rect}/n \approx A + B s^{d}`, pas `s^{d}` pur.

**Seule `scanline_single_pass` est convergée** : exposant `1,05` à `1,07`, et
`51,9` à `115,6` rectangles par point selon `s`. C'est la famille de plus basse
dimension effective, `d = 2,06`.

**Les trois autres ne le sont pas à `32 000`**, et l'exposant **croît avec `s`** :

```text
uniform          1,19  1,23  1,28   (s = 6, 8, 10)
eight_clusters   1,23  1,27  1,30
terrain          1,25  1,27  1,29
```

C'est cohérent avec une transition non terminée : le régime asymptotique exige
`n \gg s^{d}`, et à `s = 10` en dimension `3` cela fait `n \gg 1000`, soit un
rapport de `32` seulement à `n = 32 000`. `eight_clusters` et `uniform` montrent
d'ailleurs une décroissance de l'exposant entre les deux doublements — `1,47`
puis `1,30`, `1,32` puis `1,28` — donc une convergence lente. `terrain` seul
reste **plat à `1,27`** sur les deux doublements, et c'est le cas que je ne sais
pas expliquer.

### Le fait pratique, qui ne dépend d'aucune de ces subtilités

À `n = 32 000`, `s = 8` :

| famille | rectangles | par point |
|---|---:|---:|
| `uniform` | `22 247 725` | `695,2` |
| `eight_clusters` | `12 583 384` | `393,2` |
| `terrain` | `5 535 416` | `173,0` |
| `scanline_single_pass` | `2 595 906` | `81,1` |

Contre `656 652` arêtes q2 vivantes. Le rapport va de `4` sur `scanline` à `34`
sur `uniform`, et corriger la scission par diamètre l'améliore de `16` à `28 %`
sans changer l'ordre de grandeur.

Données brutes : `audits/donnees/wspd_rampe_20260816.txt`.
