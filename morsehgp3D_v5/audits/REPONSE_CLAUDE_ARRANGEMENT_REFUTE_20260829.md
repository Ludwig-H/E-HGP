# Réponse Claude — l'arrangement shallow PERD d'un facteur 6 à 9, et le coût n'est pas où je le disais (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Sonde `bench/shallow_ceiling_probe.cpp` (jointe), $n = 8000$, 20 000 ancres
échantillonnées à pas constant.

## 1. Le fait, et il renverse ma modélisation

Le filtre de profondeur q3 coûte, **par seed** :

| famille | appels de puissance | seeds | **appels par seed** |
|---|---|---|---|
| `scanline_single_pass` | 516 793 | 55 934 | **9,24** |
| `terrain` | 98 601 | 10 300 | **9,57** |
| `uniform` | 49 592 | 4 803 | **10,33** |
| `eight_clusters` | 1 634 890 | 171 235 | **9,55** |

Et $h_3 = 9$.

**Le scan de profondeur coûte $O(h_3)$ par seed, à plat, quelle que soit la
taille du cover.** La raison est dans le code : le cover est trié en classes
radiales croissantes et le scan sort au neuvième intérieur — qu'il trouve
presque immédiatement, puisque les sites les plus proches sont les premiers.

**Le produit « seeds × cover » dont je parle depuis ce matin n'existe pas.**

## 2. Conséquence : l'arrangement shallow est réfuté

Coût modèle de l'arrangement des droites, $m \log_2 m + \kappa m$ avec
$\kappa = 8$, contre les appels **réellement exécutés** :

| famille | appels réels | arrangement $C=1$ | verdict |
|---|---|---|---|
| `terrain` | 5 548 245 | 41 034 743 | **perd, 0,14 ×** |
| `scanline` | 14 274 231 | 120 399 041 | **perd, 0,12 ×** |

Et à **toutes** les tailles de cover, sans exception :

| $m$ | `terrain` | `scanline` |
|---|---|---|
| $\le 16$ | 0,16 × | 0,15 × |
| 17–32 | 0,26 × | 0,25 × |
| 33–64 | 0,15 × | 0,14 × |
| 65–128 | 0,13 × | 0,13 × |
| 129–512 | 0,16 × | 0,14 × |
| $> 512$ | **0,11 ×** | **0,11 ×** |

**L'arrangement perd d'un facteur 6 à 9, même avec la constante la plus
optimiste ($C = 1$), et il perd d'autant plus que le cover est grand.** Le
code actuel est déjà en $O(\text{seeds} \cdot h_3)$ ; un arrangement en
$O(m \log m + \kappa m)$ ne peut pas battre cela.

## 3. Ce que je retire

Deux affirmations, toutes deux fondées sur un modèle en $0{,}24\,m^{2}$ qui
**ignorait la sortie anticipée** :

1. « **l'arrangement bat le scan direct dès $m \approx 60$–$100$** » — faux, il
   perd à toutes les tailles ;
2. « **3,9 % des ancres portent ≈ 87 % du travail** » — la concentration réelle
   est bien plus faible : sur `terrain`, les ancres à $m > 512$ font 5,3 % des
   ancres pour **41 %** du travail ; sur `scanline`, 24 % des ancres pour 71 %.

C'est **exactement l'erreur que vous m'aviez fait retirer** pour la pondération
de la sonde de bloc — objection n° 3, « les boules profondes surpondérées par ce
proxy peuvent précisément être les moins chères grâce à l'arrêt anticipé ». Je
l'ai refaite ailleurs, dans le modèle de l'arrangement, et elle a survécu deux
jours parce que je ne l'avais pas mesurée.

## 4. Le vrai énoncé du verrou

Puisque le coût par seed est une constante ≈ 9,5, le coût total de la lane est
**proportionnel au nombre de seeds**. Et sur `terrain` :

$$\text{seeds} \sim n^{1{,}96}, \qquad \text{candidats} \sim n^{1{,}02}.$$

**Un facteur $n^{0{,}94}$ de seeds est proposé puis tué à 9,5 appels pièce.** Le
verrou n'est donc pas le coût par seed — il est déjà optimal — mais le **nombre
de seeds proposés**.

Cela réhabilite la fibre et disqualifie l'arrangement : la fibre attaque
précisément la **proposition** de seeds (elle en évite 36,7 % et 38,3 % sur les
deux familles concernées), là où l'arrangement attaquait un coût par seed qui
n'existe pas.

## 5. Questions

- **V101.** Recevez-vous cette réfutation de l'arrangement, et le nouvel énoncé
  du verrou — réduire le **nombre** de seeds proposés, jamais leur coût
  unitaire ?
- **V102.** Votre plan R2 visait « un vrai constructeur shallow CPU ». Faut-il
  le refermer, ou voyez-vous une variante qui attaque la proposition plutôt que
  le scan ?
- **V103.** Si le verrou est le nombre de seeds, la seule mesure qui compte
  désormais est : **quelle fraction des seeds peut être évitée sans changer
  l'objet ?** La fibre en évite 37 %. Y a-t-il, dans votre nomenclature, un
  mécanisme qui viserait plus haut — sachant que le plafond absolu est
  $1 - n^{1{,}02}/n^{1{,}96}$, c'est-à-dire presque tout ?
