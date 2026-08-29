# Question Claude — l'ensemble des centres est un SECTEUR, pas une bande : je corrige ma V84 et la machinerie existe déjà (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## 1. Votre correction de ma mesure est juste, et vos chiffres sont meilleurs

Ma V84 normalisait par le $\rho$ de la **première** ancre du bloc tout en
mélangeant les centres de **toutes** ses ancres. Votre version — diamètre des
centres à **ancre fixe**, normalisé par $2\rho_{ab} = D/\sqrt{3}$ de **cette
même ancre** — est la bonne, et le contrôle `0 >1` valide la normalisation.

| famille | diamètre / $2\rho_{ab}$ | si $\ge 2$ centres | pire |
|---|---|---|---|
| `scanline` | **0,113** | 0,138 | 0,863 |
| `terrain` | **0,129** | 0,178 | 0,796 |
| `uniform` | **0,183** | 0,244 | 0,639 |
| `eight_clusters` | **0,112** | 0,132 | 0,827 |

Le rétrécissement est donc plus fort que je ne l'avais mesuré : facteur **6 à 9
en diamètre**, non 4.

## 2. Ce que je corrige à mon tour : ce n'est pas une bande, c'est un secteur

En V84 j'ai écrit que la contrainte issue de $C$ est une **bande**, qui ne
contraint qu'une direction, et j'en concluais que la construction plafonnait à
une réduction d'aire de $\approx 1{,}8$. **C'était faux, et voici pourquoi.**

Le centre est le point de **norme minimale** sur sa droite $L_x$. En effet
$p_x$, composante de $x-m$ orthogonale à $d$, appartient au plan du triangle
(comme $x-m$ et $d$) ; or l'intersection du plan bissecteur de $ab$ avec le
plan du triangle est la droite passant par $m$ de direction $p_x$ ; le centre y
étant, il est un multiple de $p_x$. Donc

$$v = c - m = \frac{q_x}{2\lVert p_x \rVert^{2}}\, p_x, \qquad q_x = \lVert x-m \rVert^{2} - \frac{D^{2}}{4}.$$

**Vérification numérique :** sur 30 589 triangles aigus d'arête maximale $ab$
tirés aléatoirement, l'écart maximal au parallélisme entre $v$ et $p_x$ vaut
$4{,}4 \times 10^{-16}$ — epsilon machine.

**Trois conséquences, et la troisième est la bonne nouvelle.**

1. **La direction du centre est celle de $p_x$**, donc contrainte par
   $\mathrm{Box}(C)$ : l'ensemble des centres est une **plage angulaire**, pas
   une bande. Une bande laissait une direction libre ; un secteur n'en laisse
   aucune.
2. **Le rayon est positif.** $q_x > 0$ est *exactement* la condition d'acuité
   stricte en $x$ ($\lVert 2x-a-b \rVert^{2} > D^{2}$). Tout seed valide a donc
   $t > 0$ : le centre est sur le **rayon positif**, ce qui élimine d'emblée la
   moitié du disque.
3. **La machinerie existe déjà.** `anchor_sector_kill` recouvre précisément ce
   disque par $K = 8$ secteurs d'un polygone convexe à sommets entiers, et sa
   signature expose déjà `u32* sector_counts`. Aujourd'hui elle exige $h_3$
   témoins **dans chaque** secteur. Si $\mathrm{Box}(C)$ ne peut atteindre
   qu'un sous-ensemble de secteurs, il suffit d'exiger le seuil **sur ceux-là** :
   condition strictement plus faible, donc strictement plus de morts, **sans
   aucune arithmétique nouvelle**.

## 3. Ce que cela change au verrou

Le verrou du § 3 de ma note précédente disait : plafond idéal 92 à 98 %,
patches inutiles, certificat de boîtes connu 0 %. La troisième ligne change :
le certificat par secteurs **existe, est exact en entiers, est déjà éprouvé par
une porte et des mutants**, et il n'a jamais été conditionné par $C$ parce que
la fibre $A \times B \times C$ n'existait pas.

Ce n'est plus « trouver un certificat de boîtes » mais « restreindre l'ensemble
des secteurs requis à ceux que $\mathrm{Box}(C)$ atteint ». Le calcul de cet
ensemble est une comparaison de directions : projeter les coins de
$\mathrm{Box}(C)$ sur le plan bissecteur et les situer par rapport aux sommets
$p_k$ du polygone — des produits vectoriels entiers, du même type que ceux déjà
présents dans `sector_kill.hpp`.

## 4. Questions

- **V88.** Le théorème du § 2 vous paraît-il correct, et l'avez-vous déjà
  quelque part ? Il me semble être exactement ce qui manquait en V85, et il
  était sous mes yeux dans la formule $v_x = q_x p_x / (2\lVert p_x \rVert^{2})$
  que j'avais moi-même écrite le 29 au matin sans en voir la conséquence
  géométrique.
- **V89.** La restriction de `anchor_sector_kill` aux secteurs atteignables par
  $\mathrm{Box}(C)$ est-elle saine telle que je la formule ? Ma seule
  inquiétude : le polygone à $K = 8$ secteurs **contient** le disque, donc un
  secteur peut être atteignable géométriquement sans contenir aucun centre
  réel — ce qui rend le test conservateur, jamais faux. Confirmez-vous ce sens ?
- **V90.** Avant d'implémenter, je propose de mesurer le **plafond** : combien
  de secteurs sur 8 un handle atteint-il en moyenne ? Si la réponse est 7 ou 8,
  la restriction ne rapporte rien ; si c'est 1 ou 2, elle rapporte beaucoup.
  C'est une mesure d'une demi-heure et elle décide. Voulez-vous que je la fasse
  avant toute autre chose ?
