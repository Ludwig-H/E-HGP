# Réponse Claude — V84 : le rétrécissement est d'un facteur 4, mais une bande n'en capte qu'une direction (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`, quatre familles, $n = 8000$.

## 1. Le rétrécissement, mesuré

Le certificat de **paire** doit résister au **disque entier** des centres, de
rayon $\rho = D/(2\sqrt{3})$. Le certificat de **bloc** ne doit résister qu'à
l'ensemble **réel** des centres du bloc. Le rapport des deux rayons borne ce
qu'une construction par bornes peut espérer :

| famille | rayon réel / $\rho$ (moyenne) | pire cas | blocs |
|---|---|---|---|
| `scanline` | **0,274** | 0,891 | 740 |
| `terrain` | **0,220** | 0,746 | 759 |
| `uniform` | **0,257** | 0,813 | 898 |
| `eight_clusters` | **0,267** | 0,788 | 771 |

**Connaître $x \in C$ rétrécit l'ensemble des centres d'un facteur ≈ 4 en
rayon**, donc ≈ 16 en aire dans le plan bissecteur. C'est l'explication
géométrique des 92 à 98 % du plafond : ce n'est pas un effet de bord, c'est un
rétrécissement massif et régulier sur les quatre familles.

## 2. Mais ma proposition ne capte qu'une direction — et je le dis avant qu'on la code

Dans le plan bissecteur, la contrainte issue de $C$ est
$2c \cdot (x-a) = \lVert x \rVert^{2} - \lVert a \rVert^{2}$ : pour $(a,x)$
**fixés**, c'est une **droite**. Quand $a$ parcourt $\mathrm{Box}(A)$ et $x$
parcourt $\mathrm{Box}(C)$, cette droite balaie une **bande**.

Une bande ne contraint **qu'une direction**. Le long de la bande, le centre
reste borné seulement par le disque, donc par $\rho$. La région
disque ∩ bande a donc une extension $\approx 2\rho$ dans un sens et
$\approx 2w$ dans l'autre, où $w$ est la demi-largeur de bande.

**Ordre de grandeur de $w$.** Avec la séparation WSPD $s = 8$, les boîtes ont un
diamètre $\lesssim D/8$ ; le déplacement de la droite quand $x$ varie de $\delta$
est de l'ordre de $\delta$, soit $w \approx D/8 = 0{,}125\,D$, à comparer à
$\rho = 0{,}289\,D$ : **$w \approx 0{,}43\,\rho$**.

D'où la comparaison honnête, en aire :

| région | aire relative |
|---|---|
| disque du niveau paire | $\pi\rho^{2} \approx 3{,}14\,\rho^{2}$ |
| disque ∩ bande ($w = 0{,}43\rho$) | $\approx 1{,}7\,\rho^{2}$ |
| **ensemble réel des centres** (mesuré) | $\pi (0{,}27\rho)^{2} \approx 0{,}23\,\rho^{2}$ |

**La bande divise l'aire par ≈ 1,8 ; le rétrécissement réel est de ≈ 13.** Ma
construction du § 4 de la note précédente capte donc une fraction du gain
disponible, pas sa totalité. C'est une limite de ma propre proposition, et je
préfère la poser maintenant plutôt que la découvrir après.

## 3. Ce qui reste vrai, et ce qu'il faudrait pour aller plus loin

Reste vrai : la formulation par le centre échappe aux deux échecs mesurés — la
forme de Gram et le couplage sur $a$ qui rendent le repli par intervalles
inerte, et les coins que `mhgp5_q3_skinny_center` a réfutés. En $a$ le minimum
est exact et séparable par axe ; en $c$ la forme est affine. **C'est la seule
formulation calculable dont je dispose.**

Ce qui manque : une seconde contrainte, pour fermer la direction que la bande
laisse libre. Trois pistes, toutes non mesurées :

1. **Resserrer $w$ en découpant $C$.** $w$ est proportionnelle au diamètre de
   $\mathrm{Box}(C)$ ; un niveau de split le divise par deux. Mais j'ai déjà
   mesuré qu'un split payé pour tuer plus tôt ce qui mourait déjà coûte $+34\ \%$
   de mur : ici il achèterait du **certificat**, pas de la mort anticipée, donc
   le cas est différent — mais il doit être mesuré, pas supposé.
2. **Augmenter $s$.** $w \approx D/s$ : passer de $s = 8$ à $s = 16$ diviserait
   $w$ par deux, au prix de plus de rectangles. Le compromis est mesurable
   puisque $s$ est déjà un paramètre.
3. **Borner l'extension le long de la bande** par une contrainte que je n'ai pas
   trouvée. C'est là que j'aimerais votre avis : existe-t-il, pour un bloc, une
   borne sur la position du centre **le long** de la médiatrice de $ax$, autre
   que le disque d'acuité ?

## 4. Questions

- **V85.** La piste 3 existe-t-elle ? Si non, disque ∩ bande plafonne à une
  réduction d'aire de ≈ 1,8 contre ≈ 13 disponible, et il faut choisir entre
  split de $C$ et augmentation de $s$.
- **V86.** Entre ces deux-là, laquelle mesureriez-vous d'abord ? $s$ me paraît
  la moins risquée : c'est un paramètre existant, le digest doit rester
  identique, et l'effet sur le nombre de rectangles est déjà instrumenté.
- **V87.** Le rapport 0,22–0,27 est remarquablement stable sur quatre familles
  très différentes. Y voyez-vous une raison structurelle — par exemple que
  l'extension du cover d'ancre et celle d'un handle sont liées par la
  construction de l'antichaîne — qui permettrait de le **borner** au lieu de
  le constater ?
