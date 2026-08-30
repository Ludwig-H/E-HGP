# Réponse de Claude — V154 : deux pistes de coût que j'ouvre et que je ferme moi-même dans l'heure

- **Ancrage :** pin `4f177beb`. Deux hypothèses de coût sur le test $W_3$ par
  ancre, formulées puis réfutées par mesure. Aucune ne va au chemin produit.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Piste 1 — l'ordre de balayage : réfutée, et j'avais mal lu la géométrie

**Hypothèse.** Le cover est trié par distance au **milieu** de l'ancre
($\mathrm{dist2q} = \lVert 2z-(a+b)\rVert^2$) et `anchor_universal_kill` part de
la classe radiale $0$. Or $80{,}6\,\%$ des témoins du citron sont dans les
*pointes* sur `eight_clusters` : le scan devrait donc parcourir le corridor vide
en premier, et gagner à être inversé.

**Mesure**, $n=2000$, $s=8$, graine 3, verdict identique dans tous les cas :

| cohorte | cover utile moyen | tests/ancre **croissant** (actuel) | décroissant | désaccords |
|---|---:|---:|---:|---:|
| `eight_clusters` | 129,5 | **19,3** | 87,9 | 0 |
| `uniform` | 29,2 | **13,5** | 25,1 | 0 |
| `scanline` | 24,5 | **12,8** | 19,5 | 0 |
| `terrain` | 17,2 | **9,6** | 14,0 | 0 |

**L'ordre actuel est meilleur partout, jusqu'à $4{,}5$ fois.** Mon erreur est
géométrique : un point à $D/4$ de $a$ a $\lVert 2w\rVert^2 = D^2/4$, donc la
classe radiale $2{,}7$ — pas une classe haute. Seul $z=a$ exactement atteint
$\lVert 2w\rVert^2 = D^2$. Les pointes ne sont donc **pas** au bout du tri, et
le tri par le milieu les atteint tôt.

Résultat utile au passage : **le test $W_3$ ne coûte que $9{,}6$ à $19{,}3$
tests par ancre**, bien moins que je ne le supposais.

## Piste 2 — le placement du crédit par point : déjà capturé par l'existant

V150 concluait que le crédit d'extrémité étendu ne gagne rien en **pouvoir** sur
le test $W_3$ par ancre, seulement en **placement** — il se calcule par point et
éviterait de construire le cover de l'ancre. V151 chiffrait l'amortissement
disponible ($65$ à $189$ rectangles par point).

Or la lane possède déjà ce placement : `generate.hpp:1026`,
`pretest_by_query = sc.handle_points >= opt.pretest_query_min_points`, puis
`rect_diametral_candidates` **une fois par rectangle** et
`anchor_kill_from_candidates` **avant** `anchor_cover_from_handles`. Sur les
rectangles à handles denses — exactement ceux où le cover est cher — le test
$W_3$ tourne donc déjà sans construire le cover d'ancre.

Le gain de placement que je proposais est donc, pour l'essentiel, **déjà pris**.
Ce qui resterait est le cas des handles peu denses, où le cover est de toute
façon petit ($17$ points en moyenne sur `terrain`).

## Ce que cela laisse

Les deux pistes ferment, et elles ferment dans le même sens : **le test $W_3$ par
ancre est déjà bien placé et bien ordonné**. La marge n'est pas dans son coût.

Restent, non fermées :

- le **crédit d'extrémité étendu** de V150 — sûr, mesuré, $2{,}9$ à $4{,}6$ fois
  plus de masse éliminée à la porte histogramme, mais dominé en pouvoir par
  $W_3$ ; sa seule valeur restante serait de tuer *avant*
  `rect_diametral_candidates`, pas seulement avant le cover d'ancre ;
- le fait de V147, qui reste le plus solide de la journée : **$100\,\%$ des morts
  d'ancre exigent $h_a+h_b$**, aucune n'est attribuable au cœur seul.

## Question

- **V154.** Le crédit d'extrémité se calcule à partir de `corner_histograms`,
  donc **avant** `rect_diametral_candidates` et avant tout cover. Est-ce que
  déplacer la porte histogramme étendue (avec $h^{\mathrm{ext}}$) *avant* la
  requête diamétrale du rectangle — et non seulement avant le cover d'ancre —
  change quelque chose au grand-livre, ou est-ce déjà l'ordre en vigueur ?
