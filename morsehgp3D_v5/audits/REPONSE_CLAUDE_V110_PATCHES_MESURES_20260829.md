# Réponse de Claude — V110 à V113 : la route « WSPD $\to$ 64 patches » mesurée, et les « 64 » sont un curseur, pas une constante

- **Ancrage :** `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`, § « Certificat sûr :
  center-cover conditionné par $C$ » et § V84, aux pins `ac02e3c7` et `dc01fdf0`.
  Sonde `morsehgp3D_v5/bench/q3_patch_block_probe.cpp` (counter-only, jamais une
  décision d'objet). GCP non utilisé.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Ce qui est implémenté

Vos cinq étapes, à la lettre. Boîte des centres du rectangle par
$\lVert ab\rVert/2 \leq \lVert c-a\rVert = \lVert c-b\rVert \leq \lVert ab\rVert/\sqrt{3}$
(l'angle opposé à l'arête maximale d'un triangle aigu est dans $[60,90)$ degrés),
pavage $K\times K\times K$ à bornes entières en échelle $K$, masque de
médiatrices $AB$, $AC$, $BC$ par intervalles entiers, puis crédit témoin
$g_{AB}[j]$ par la borne concave aux huit sommets
$L_A(Q,z)=\min_{\mathrm{sommets}}\left[\mathrm{dist}^2(C,K\,\mathrm{Box}(A))-\lVert C-Kz\rVert^2\right]$,
et $\max(L_A,L_B)>0$.

Deux invariants exécutables, vérifiés à chaque configuration :

1. aucun bloc à masque vide ne contient de vrai seed aigu ;
2. tout vrai seed d'un bloc mort par crédit est **effectivement profond**,
   vérifié par la vraie forme `q3_power` sur les témoins du rectangle.

## V110 — le masque de médiatrices est inerte, le crédit fait tout

Le fate `EMPTY` tue $0{,}0$ à $0{,}6\,\%$ des blocs. Vous l'aviez annoncé
(« ils conservent donc un sur-ensemble ») : les trois intervalles séparés sont
si larges que le masque porte $\geq 8$ bits dans $98\,\%$ des blocs. Tout le
pouvoir de coupe vient du **crédit témoin**, aucun du masque. Le masque ne sert
donc qu'à restreindre l'ensemble des patches à tester — un rôle de coût, pas de
décision.

## V111 — la composition `core + g_AB` est fausse, et j'ai la fixture

En composant par une **somme**, j'obtenais $19{,}3\,\%$ de seeds retirés sur
`terrain` $n=2000$ — et **228 violations** de l'invariant 2 : des seeds déclarés
morts qui ne sont pas profonds. Avec votre composition
$\mathrm{base}_j=\max(\mathrm{core},g_{AB}[j])$, zéro violation. Les deux
crédits reconnaissent bien les mêmes sites, exactement comme votre note de
`dc01fdf0` le dit. Fixture minimale permanente : `terrain`, $n=2000$,
graine $3$, $K=4$, mutant `credit-sum-core-gab`, $228$ violations attendues.

Votre $f=\min_a h_a(a)+\min_b h_b(b)$ au niveau rectangle est en revanche
**inerte** : moyenne mesurée $0{,}00$ sur toutes les configurations. Un seul
point sans témoin universel dans la plage suffit à annuler le minimum. Seule la
version par ancre (vos bitsets $P[t]$ et le seuil $\tau_i(c)$) peut le récupérer.

## V112 — le pouvoir de coupe, à l'échelle : `uniform` et `scanline` tiennent, `terrain` décroche

Échantillon de $\sim 3000$ rectangles **tirés par hachage sur toute la liste
vivante** — le préfixe donne des chiffres faux d'un facteur $5$, la vague WSPD
triant les rectangles par niveau. $K=4$ (vos $64$ patches), graine $3$,
$f=0$, donc **minorant** :

| famille | $n=2000$ | $n=4000$ | $n=8000$ | $n=16000$ |
|---|---:|---:|---:|---:|
| `terrain` — blocs morts | 14,5 % | 14,7 % | 13,2 % | 17,4 % |
| `terrain` — **seeds proposés retirés** | **14,4 %** | **10,3 %** | **4,9 %** | **6,3 %** |
| `uniform` — blocs morts | 45,8 % | 49,0 % | 49,3 % | 49,8 % |
| `uniform` — **seeds retirés** | **57,7 %** | **62,7 %** | **60,8 %** | **61,7 %** |
| `scanline` — blocs morts | 16,5 % | 18,0 % | 24,1 % | 24,5 % |
| `scanline` — **seeds retirés** | **19,1 %** | **17,0 %** | **21,3 %** | **22,4 %** |

Zéro violation sur les douze configurations.

Lecture. Sur `uniform` la route retire **six seeds proposés sur dix**, stable, et
sur `scanline` **plus d'un sur cinq**, croissant. Sur `terrain` la fraction
**décroche** — et le diagnostic est net : la fraction de *blocs* morts reste
stable ($14{,}5 \to 13{,}2$), c'est la fraction de *seeds* qui tombe. Le
mécanisme tue donc les blocs **légers** et rate les blocs **lourds**, ceux
mêmes qui portent la masse sur `terrain`.

## V113 — les « 64 » ne sont pas une constante : c'est un curseur, et il n'est pas saturé

Si le mécanisme rate les blocs lourds, c'est que leur boîte de centres est
grande, donc que les patches y sont grossiers. Test direct, mêmes rectangles :

| famille, $n$ | $K=2$ (8 patches) | $K=4$ (64) | $K=8$ (512) |
|---|---:|---:|---:|
| `terrain`, 2000 | 0,5 % | 12,9 % | **28,3 %** |
| `terrain`, 8000 | — | 6,2 % | **18,0 %** |

Doubler la finesse par axe **double à triple** le retrait, aux deux tailles. À
$n=8000$, $K=8$ rend $18{,}0\,\%$ là où $K=4$ rendait $6{,}2\,\%$ : la
résolution compense plus que la décroissance. Le pouvoir de coupe de la route
est donc **limité par la résolution du pavage**, pas par la géométrie du régime,
et $64$ n'est pas un point de saturation.

C'est le premier mécanisme mesuré qui attaque la **proposition** avec un ordre
de grandeur non trivial, et c'est aussi le premier dont le levier soit
explicitement réglable.

## Ce que je ne prétends pas

- La version **ponctuelle** du crédit mesure le plafond du pouvoir de coupe, pas
  un coût acceptable : elle paie $\sim 1400$ à $2500$ évaluations de témoins par
  rectangle. C'est votre DFS masqué en version **nœud** qui doit rendre la
  route abordable, et je ne l'ai pas mesuré. Le gain publié ici est exact ; le
  coût publié est un majorant grossier.
- $f=0$ : ces chiffres sont un **minorant** du pouvoir de coupe réel.
- Une graine, échantillon de $3000$ rectangles : les fractions sont robustes,
  les masses absolues non. Trois graines suivront avec le harnais de reçu.

## Questions

- **V114.** Le coût du pavage $K^3$ est en $K^3$ par rectangle, mais le crédit
  s'amortit sur $|A||B|$ ancres et tous les handles. Voyez-vous une raison de
  principe de rester à $K=4$, ou $K$ doit-il devenir un paramètre mesuré, voire
  **adaptatif** au diamètre de la boîte de centres — grossier sur les petits
  rectangles, fin sur les gros, qui sont exactement ceux qui portent la masse de
  `terrain` ?
- **V115.** Le crédit par nœud `center_witness_phi32_lattice_min` remplace-t-il
  intégralement la version ponctuelle, ou faut-il garder la version ponctuelle
  comme oracle borné de non-régression du pouvoir de coupe ? Je n'ai pas de
  moyen de mesurer la perte de relaxation du passage point $\to$ nœud sans les
  deux.
- **V116.** Sur `terrain`, les blocs lourds survivent. Avez-vous, dans votre
  nomenclature, un fate qui viserait spécifiquement un bloc **dense** — où
  $|A||B||C|$ est grand — plutôt que le bloc moyen ? C'est là qu'est la masse,
  et c'est exactement là que le pavage uniforme est le plus grossier.
