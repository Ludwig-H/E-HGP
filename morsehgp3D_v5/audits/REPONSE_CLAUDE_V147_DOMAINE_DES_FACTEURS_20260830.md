# Réponse de Claude — V147 : sur le bon dénominateur, **100 % des morts d'ancre exigent $h_a+h_b$**, et le domaine de ces crédits est trop petit d'un facteur 4,6

- **Ancrage :** pin `351faccc`. Cinq analyses indépendantes et cinq réfutations
  adversariales sur la cascade des trois crédits. Toutes les mesures ci-dessous
  ont été **reproduites chiffre pour chiffre par un réfuteur indépendant**.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Je corrige mon propre § V144 : `core` ne tue **rien** seul

J'écrivais « `core` fournit $6{,}4$ à $7{,}9$ des $9$ requis, $h_a+h_b$ fournissent
le solde » et j'en tirais que les facteurs sont des *finisseurs*. C'est une
**erreur de catégorie**, et la réfutation la nomme : sur un rectangle vivant, le
cœur est **plafonné à $h_3-1$ par construction** — `generate.hpp` n'émet que des
rectangles de `core <= 8` pour $h_3 = 9$. Comparer une moyenne **censurée par le
filtre qui a produit la population** à une moyenne libre n'a pas de sens ; et
l'histogramme plat du cœur sur $0..8$ est précisément la signature de cette
censure.

Sur le seul dénominateur qui tranche — **les morts d'ancre** —, $n=2000$, $s=8$,
graine 3, $\sim 40\,000$ ancres par cohorte :

| cohorte | `core` max sur rectangle vivant | morts par `core` seul | morts par $h_a+h_b$ seuls | **morts exigeant les deux** |
|---|---:|---:|---:|---:|
| `eight_clusters` | 8 | **0 / 6 753** | 1 088 (16,1 %) | **83,9 %** |
| `terrain` | 8 | **0 / 2 604** | 0 (0,0 %) | **100 %** |
| `uniform` | 8 | **0 / 1 391** | 0 (0,0 %) | **100 %** |
| `scanline` | 8 | **0 / 3 518** | 150 (4,3 %) | **95,7 %** |

**Aucune mort n'est attribuable au cœur seul, et $100\,\%$ des morts exigent
$h_a+h_b$.** L'exigence de l'utilisateur — « il faut $h_{\mathrm{coeur}}$ **mais
aussi** $h_a$ et $h_b$ » — est donc confirmée sur le bon dénominateur, et plus
fortement que je ne l'avais formulée : les facteurs ne sont pas des finisseurs
optionnels, ils sont **indispensables à chaque mort**.

## Où passent les témoins : dans les pointes, hors des boîtes

Le citron $W_3(a,b)$ a deux **pointes** ouvertes à $60$ degrés en $a$ et en $b$,
qui portent jusqu'à $\lVert ab\rVert/\sqrt{3} = 0{,}577\,\lVert ab\rVert$. Or la
séparation impose $\mathrm{rayon}(\mathrm{Box}(A)) \leq D/s = 0{,}125\,D$ à
$s = 8$. **La boîte est donc trop petite d'un facteur $4{,}6$ pour atteindre la
pointe qui la concerne.**

Mesuré, $n=2000$, $s=8$, graine 3 :

| cohorte | témoins dans les **pointes** | témoins dans $\mathrm{Box}(A)\cup\mathrm{Box}(B)$ | contraste densité extrémités/corridor |
|---|---:|---:|---:|
| `eight_clusters` | **80,6 %** | **7,8 %** | 94,6 (inter-amas) |
| `scanline` | 55,5 % | 8,3 % | 3,55 |
| `terrain` | 40,1 % | 6,8 % | 1,35 |
| `uniform` | 36,1 % | 3,6 % | 1,17 |

Sur `eight_clusters`, $72{,}8\,\%$ des témoins ($81{,}9\,\%$ pour les ancres
inter-amas voisines) sont **dans les pointes mais hors des deux boîtes** :
**aucun des trois crédits ne peut les réclamer.** Le corridor inter-amas est vide
($1{,}76$ point dans la boule médiane de rayon $D/4$) alors que les extrémités
portent $66{,}4$ et $65{,}1$ points — exactement le régime que l'utilisateur avait
identifié.

## Le plafond, si le domaine des facteurs était la bonne boule

En remplaçant le domaine de $h_a$ par $B(a, d_{\min}/2)$ — même certificat exact
aux huit coins de $\mathrm{Box}(B)$ — le plafond mesuré est :

| cohorte | morts d'ancre : actuel → réalisable → plafond | **masse de seeds : actuel → réalisable → plafond** |
|---|---|---|
| `eight_clusters` | 17,0 → 57,2 → 63,3 % | **33,4 → 86,7 → 91,5 %** |
| `scanline_overlap` | 9,6 → 30,7 → 37,9 % | 18,7 → 61,7 → 69,3 % |
| `scanline_single` | 8,8 → 25,9 → 33,8 % | 18,4 → 58,1 → 69,2 % |
| `terrain` | 6,5 → 19,1 → 26,3 % | **13,1 → 38,7 → 50,3 %** |
| `uniform` | 3,5 → 17,1 → 25,9 % | 8,6 → 37,2 → 51,2 % |

C'est un facteur $2{,}6$ à $4{,}3$ sur la masse éliminée, **à $s=8$**, dans le
domaine admis. C'est de loin le plus grand levier mesuré depuis deux jours.

## Mais les deux implémentations proposées sont FAUSSES, et c'est exécuté

1. **Élargir la plage du témoin dans `corner_histograms` double compte.** Le
   crédit de cœur est produit par `count_universal_witnesses`
   (`src/spindle/witness_count.hpp`), dont `witness_detail::credit_weight`
   soustrait exactement `range_of(a)` et `range_of(b)`, c'est-à-dire les plages
   **fines**. Remplacer la plage du témoin par celle d'un ancêtre fait compter
   tout témoin de $A_k \setminus A_f$ **deux fois**. C'est mot pour mot le mutant
   `bucket-not-disjoint`. Exécuté à `732529b3`, $n=800$, graine 3 : code 4,
   $3\,170$ violations sur `terrain`, $4\,760$ sur `scanline`, et surtout
   **`viol_profondeur = 9` sur $1\,500$ seeds audités par la vraie forme
   `q3_power`** — donc des seeds de profondeur réelle $< 9$ sont éliminés.
   **L'objet change.** Le run sain rend code 0 et zéro violation.
2. **Les deux cônes ne sont pas disjoints à l'égalité.** Le certificat
   $B(a, d_{\min}/2)$ et son symétrique se recouvrent quand
   $\lVert a-b\rVert^2 = d_{\min}^2$ avec un test non strict : contre-exemple
   entier $a=(0,0,0)$, $b=(8,0,0)$, $z=(4,0,0)$, et un second à boîtes non
   dégénérées séparées jusqu'à $s=100$. Or $\lVert a-b\rVert^2 = d_{\min}^2$ vaut
   pour $15{,}6$ à $46{,}4\,\%$ des ancres réelles — les rectangles feuille-feuille
   sont un cas **nominal**. Honnêteté du réfuteur : le surcrédit n'a pas été
   atteint sur $3$ millions de tirages, mais la condition nécessaire est banale.

## Ce que cela pose

Le levier est réel et mesuré ; sa mise en œuvre ne l'est pas. La difficulté est
précisément celle que votre note du 30 août anticipe : **la disjonction des
domaines**. Élargir le domaine de $h_a$ oblige à retrancher le même élargissement
au cœur, or le cœur est calculé dans un autre en-tête et sa soustraction est
figée sur les plages fines.

## Questions

- **V147.** Peut-on élargir le domaine de $h_a$ de $\mathrm{Box}(A)$ à une boule
  $B(a,\rho)$ **sans toucher au cœur**, en exigeant que le domaine élargi soit
  disjoint de la source du cœur par construction — par exemple en n'admettant que
  les témoins situés dans la pointe **strictement au-delà** de
  $\mathrm{Box}(A)\cup\mathrm{Box}(B)$, et en retranchant symétriquement ce cône
  de la source du cœur ? Ou faut-il, comme le suggère la réfutation, un
  `ar.core` dont la provenance porte explicitement le domaine soustrait ?
- **V148.** Quel $\rho$ garde les deux cônes disjoints dans **tous** les cas, y
  compris $\lVert a-b\rVert = d_{\min}$ ? $\rho < \lVert a-b\rVert/2$ **strict**
  suffit-il, et faut-il le rapporter à $\lVert a-b\rVert$ (par ancre) plutôt qu'à
  $d_{\min}$ (par rectangle) ?
- **V149.** Confirmez-vous la correction de mon § V144 — le cœur étant censuré à
  $h_3-1$ sur tout rectangle vivant, sa moyenne ne se compare pas à celle des
  facteurs, et $100\,\%$ des morts exigent $h_a+h_b$ ?
