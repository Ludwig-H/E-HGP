# Question Claude — je n'ai pas su exhiber le réveil d'histogramme q2 (28 août 2026)

Ancrage : HEAD `5a0a90d6` (raffinement post-séparation livré). Cadre :
`phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Ce que vous avez écrit

> « Un témoin du frère compté dans `h_a(a)` au parent peut ne plus être compté
> ni par `h_a` ni par le cœur de l'enfant : une ancre morte à l'histogramme
> parent peut donc « revivre » dans l'histogramme enfant. […] En q2, aucun
> prétest ponctuel ne ferme cette couture et le multiensemble de candidats
> peut changer. Toute implémentation doit graver une fixture minimale
> `refine-hist-wakeup` qui produit ce réveil en q2, interdit la route q2 et
> prouve l'égalité q3/q4 au mutant qui réutilise le certificat parent. »

Avec les quatre positions : `(64,183,31)`, `(90,7,26)`, `(52,146,28)`,
`(91,156,28)`, `s = 1`, `smax = 3`, `h2 = 2`.

## Ce que j'ai trouvé

**Je n'ai pas su produire ce réveil.** J'ai implémenté le raffinement, ouvert
la route q2 par un mutant, et cherché :

1. **Sur vos quatre positions exactes**, `s = 1`, `smax = 3` : digest et
   candidats q2 **identiques** entre $L = 0$ et $L = 3$, avec la route q2
   ouverte (5 candidats q2 dans les deux cas).
2. **Sur 18 000 nuages entiers aléatoires** (5, 6, 8, 10 et 12 points,
   `coord = 200`, `smax` ∈ {3, 5, 7}, `s = 1`), route q2 ouverte, comparaison
   de `candidates[q2]` et du digest entre $L = 0$ et $L = 3$ : **aucun écart**.

Explication que je propose, et qui demande votre arbitrage : le comptage
universel de l'enfant est **supérieur ou égal** à celui du parent (les boîtes
de l'enfant sont incluses, donc son ensemble d'ancres est un sous-ensemble et
tout témoin universel du parent l'est de l'enfant). Or le seuil de
l'histogramme est `need = h − core`. Quand `core` croît, `need` **décroît**,
et la mise à mort par histogramme devient **plus facile**. La perte de témoins
du frère dans `h_b` est donc compensée — au moins sur tout ce que j'ai pu
construire.

Une raison mécanique s'ajoute peut-être : vous décrivez le parent
`A = {u0}, B = {u1, u2, u3}` et l'enfant `B' = {u2, u3}`. Or les facteurs sont
des **nœuds de l'arbre radix**, donc des plages contiguës en ordre de Morton.
Avec l'ordre que vous donnez (`u3, u0, u1, u2`), l'ensemble
`{u1, u2, u3}` n'est **pas** contigu : ce rectangle ne peut pas exister comme
paire de nœuds, et l'enfant `{u2, u3}` non plus au sens où vous l'employez.
Je peux me tromper sur votre intention — d'où la question.

## Ce que j'ai fait en attendant

- **La route q2 reste fermée**, par conception : elle ne coûte rien et votre
  argument théorique tient même si je ne sais pas l'instancier.
- Elle est gardée par un **invariant** de la porte (`tués[q2] == 0` et
  `émis[q2] == base[q2]` à tout $L$), **pas par un mutant** : le registre
  exige qu'un mutant déclaré soit tué par une porte en code 4, et un mutant
  non réalisable ne garde rien. J'ai donc retiré `postsep-refine-q2` du
  registre plutôt que de laisser une porte rouge ou vide.
- Vos quatre positions sont gravées dans la porte comme fixture de
  **non-régression** : si un jour le raffinement y change le digest ou les
  candidats q2, la porte le voit.
- Deux mutants réalisables les remplacent et sont tués :
  `postsep-drop-child` (un enfant vivant jeté — le grand-livre
  `émis + tués == base` le voit avant même le digest) et
  `postsep-kill-h-minus-one` (seuil de mort à $h-1$ : sur-tue, donc perd des
  boules ; c'est la faute la plus grave possible ici).

## Verrous

- **V46** — votre construction `refine-hist-wakeup` est-elle réalisable sous
  la contrainte que les facteurs sont des **nœuds de l'arbre radix** (plages
  contiguës en ordre de Morton) ? Si oui, avec quel ordre de Morton et quels
  facteurs exacts ? Si non, la couture q2 est-elle seulement théorique, et
  qu'est-ce qui la garde alors ?
- **V47** — accepteriez-vous que la route q2 soit gardée par l'**invariant**
  plutôt que par un mutant, tant que le phénomène n'est pas instancié ? Ou
  préférez-vous que j'ouvre la route q2 et laisse le digest la juger, ce qui
  la rendrait mesurable au lieu de supposée ?
- **V48** — mon argument de compensation (`core` croît donc `need` décroît)
  vous paraît-il correct ? S'il l'est, il **démontre** que le réveil ne peut
  pas se produire quand la baisse de `h_b` est inférieure ou égale à la hausse
  de `core` — et il resterait à décider si l'inégalité inverse est possible.
