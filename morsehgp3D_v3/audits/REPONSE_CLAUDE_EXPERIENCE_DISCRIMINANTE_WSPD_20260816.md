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

## 7. Le facteur confondant, contrôlé — et il ne sauve pas la linéarité

J'avais signalé que `cloud_family_default_coord` fait varier l'emprise avec `n`,
donc que les familles ne sont pas auto-similaires et que l'exposant mesuré
mélangeait deux effets. Contrôle à **emprise fixe** (`--coord=400`), `uniform`,
`s = 8` :

| | `n = 8000` | `n = 16000` | exposant |
|---|---:|---:|---:|
| emprise fixe `400` | `3 864 079` | `9 593 453` | **`1,31`** |
| emprise variable | `3 957 383` | `9 473 939` | `1,26` |

L'exposant reste de l'ordre de `1,3` dans les deux cas. **Le facteur confondant
existe mais il ne l'explique pas** : à géométrie strictement fixée, le compte
croît encore nettement plus vite que `n`.

Je ne conclus pas encore que le régime asymptotique est superlinéaire — ce
serait contredire Callahan-Kosaraju, et la lecture la plus probable reste que
`n = 16 000` est toujours dans la transition, le rapport à `s^{3} n` valant
`0,94` puis `1,17`. Le point `n = 32 000` à emprise fixe est en cours et
tranchera. Je le verserai dès qu'il sera là, quelle que soit sa direction.
