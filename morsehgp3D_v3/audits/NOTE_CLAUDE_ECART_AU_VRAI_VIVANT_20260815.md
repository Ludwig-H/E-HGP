# Note de Claude — l'écart au `W`-vivant, et cinq corrections

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

> [!CAUTION]
> **Cette note a été écrite trois fois.** Ses deux premières versions
> contenaient chacune une erreur que je publiais en la croyant établie. Le
> ré-audit
> [`AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md`](AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md)
> en a trouvé une troisième et corrigé la mienne sur la quatrième. Le texte
> ci-dessous est la version corrigée ; les rétractations sont en section 7.

## 1. Le bon objet, et son nom

Pour chaque lane,

`V_q = {(a,b) : ||a-b|| > 0, |P inter W_q(a,b)| < h_q}`.

Ce sont les ancres qui survivent au **critère idéal de témoins `W_q`**, et
l'auditeur a raison de refuser mon « vraiment vivantes » : `V_q` contient encore
des ancres sans aucune complétion positive. Le nom exact est **`W`-vivantes**.
Le mou `|S_q|/|V_q|` ne borne donc pas le générateur entier — positivité, owner
et absence de complétion peuvent encore retirer des ancres — mais seulement ce
qu'un resserrement du **même critère de témoins** peut gagner.

## 2. La méthode, et ses deux préconditions

Le préfiltre est fail-open, donc `V_q` est inclus dans le résiduel `S_q` : il
suffit de décider exactement les paires de `S_q`. Coût `C(n,2)` tests de budget
plus `|S_q| x n` évaluations avec sortie anticipée — il suit le résiduel, pas le
cube de `n`. Sur `uniform, n=600` il reproduit à l'unité l'énumération
exhaustive : `17 479 / 42 294 / 45 913`.

**Deux préconditions, et j'en avais omis une.** La mesure vaut si la sûreté du
préfiltre est déjà établie — elle l'est, par l'oracle `PairId` — et si
`masse_non_decide = 0`. Les paires des rectangles capés sont ajoutées à `S_q`
sans jamais être testées : le compte serait alors un simple minorant. L'auditeur
le montre par contre-rejeu — sur soixante points, cap `512` donne `1594` et cap
`1` donne `1201`, les deux sortant code zéro. Le probe **refuse** désormais avec
le code `3` dès que cette masse est non nulle.

## 3. L'invariant que je présentais comme une preuve de sûreté est circulaire

J'avais gardé `survivantes >= W-vivantes` en le décrivant comme la propriété
centrale. Il ne peut rien prouver : `vrai_vivantes` n'est incrémenté qu'après
avoir établi que la paire est dans `S_q`, donc une paire fermée à tort n'est
jamais examinée. L'auditeur le démontre — avec
`--fixture=coeur5 --inject=bulk-sans-masque --juge=7 --vrai-vivant`, le mutant
ferme une vraie paire q2 et le mode imprime pourtant `q2_mou=1.000`.

Le test est conservé comme garde-fou d'implémentation, et le code le dit
maintenant. La sûreté reste établie par l'oracle indépendant, lui seul.

## 4. Le mou, à cap nul et sur trois échelles

Lane q4, chemin courant, `masse_non_decide = 0` partout :

| famille | `n=2 000` | `n=4 000` | `n=8 000` | `n=16 000` |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | `1,245` | `1,288` | `1,327` | `1,436` |
| `uniform` | `1,298` | `1,327` | `1,327` | `1,336` |
| `eight_clusters` | `2,508` | `2,779` | `2,631` | `3,007` |

Le mou **croît lentement** avec `n` — `+15 %` sur `terrain` pour un facteur huit
en taille. Ce n'est ni la constante que j'avais annoncée sur deux points, ni
l'explosion que suggérait le chiffre `5,34` calculé à `n=32 000` : celui-là était
entièrement l'artefact du cap.

**Et son interprétation était fausse.** La fraction maximale du résiduel qu'un
calcul plus exact de `h_coeur + h_a + h_b` peut encore fermer vaut `1 - 1/mu`,
non `mu - 1`. À `s=8`, cela donne `22,4 %` sur `terrain` et `uniform`, non
`33 %`. Je l'avais surestimé de moitié.

Balayage en séparation, `n=4 000` :

| famille | `s=6` | `s=8` | `s=10` | `s=12` | `s=16` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `terrain` | `1,537` | `1,288` | `1,174` | `1,110` | `1,050` |
| `uniform` | `1,747` | `1,327` | `1,169` | `1,092` | — |

Le mou converge vers un : `s` est bien le bouton de raffinement, et le
raffinement adaptatif que j'envisageais n'en est que la version locale.

## 5. Trois exposants successifs, enfin

Le plan de test en exige trois. Les voici, `masse_non_decide = 0` sur les quatre
tailles, lane q4 :

| famille | `2 000` | `4 000` | `8 000` | `16 000` | exposants |
| --- | ---: | ---: | ---: | ---: | --- |
| `terrain` | `70 642` | `148 077` | `313 806` | `667 449` | `1,068` `1,084` `1,089` |
| `uniform` | `189 767` | `412 131` | `879 078` | `1 848 814` | `1,119` `1,093` `1,073` |
| `eight_clusters` | `197 563` | `442 533` | `980 606` | `2 167 153` | `1,163` `1,148` `1,144` |

Par point : `35,3 -> 41,7`, `94,9 -> 115,6`, `98,8 -> 135,4`.

Les neuf exposants tiennent entre `1,068` et `1,163`. **L'ensemble `W`-vivant
est donc quasi linéaire sur ces trois familles et ces quatre tailles**, ce qui
lève l'alternative que je signalais : le vrai vivant ne devient pas quadratique.

Ce que cela ne dit pas : aucune borne `o(n^2)` n'est prouvée, et `V_q` n'est pas
l'ensemble des ancres porteuses de supports. C'est une mesure sur trois familles
synthétiques, pas un théorème.

## 6. Le coût d'instruction d'une ancre

Une ancre survivante doit ensuite être instruite. Pour q3, le troisième sommet
vit dans la **lentille** `{c : |ac| <= |ab| et |bc| <= |ab|}`, de volume
`5 pi D^3/12`. Le rapport `lentille / W_4` vaut `10,86`, donc sous densité
locale uniforme `87` candidats à `h_4 = 8`. Mesuré, `n=1 500`, `s=8` :

| famille | ancres | lentille moyenne | lentille max |
| --- | ---: | ---: | ---: |
| `terrain` | `51 944` | `16,60` | `419` |
| `uniform` | `136 404` | `38,94` | `202` |
| `eight_clusters` | `138 700` | `87,82` | `1 042` |

La famille groupée colle exactement au modèle ; les deux autres sont en dessous.
L'instruction est donc en `O(h)` et non en `O(n)` — mais la queue compte, et le
maximum atteint `1 042`.

## 7. Ce que je rétracte

**Le mou constant en `n`.** Publié sur deux points, contredit par une troisième
mesure que j'avais déjà. Corrigé en section 4 : il croît lentement.

**`33 %` de marge résiduelle.** C'est `1 - 1/mu = 22,4 %`, pas `mu - 1`.

**Le retrait de l'échantillonneur.** Je l'avais écarté au motif d'une variance
« inexpliquée » de trois à douze écarts-types. J'avais comparé des écarts
**relatifs** à un écart-type **absolu en points de proportion**. Les
écarts-types relatifs corrects valent `2,414 %`, `1,207 %` et `0,604 %` pour
`K = 5 000 / 20 000 / 80 000`, et mes neuf écarts tiennent tous entre `-1,50` et
`+1,52` sigma. L'estimateur est sain ; il redevient utilisable quand le scan
exact dépasse le budget, à condition de publier `X, K, T` et un intervalle
binomial.

**Le facteur `6,4` de `s=8` sur `terrain`.** `99,052 %` de la baisse vient de la
masse hors cap, pas des certificats. Sur la seule masse jugée, le gain vaut
`17,238 %` à `n=32 000` contre `17,503 %` à `n=8 000` : l'effet intrinsèque est
presque invariant. Détail dans la note dédiée, corrigée elle aussi.

**Le mou q4 de `5,34` à `n=32 000`.** Même cause. Hors cap il vaut `1,32`, en
ligne avec les autres tailles.

## 8. Ce qui reste dû à l'auditeur

**Fait depuis** : le compte en deux passes avec budget `n |S|`, le prédicat
point--point multi-lane unique et l'exclusion des paires `D=0`. Détail, chiffres
et portes dans
[`NOTE_CLAUDE_P05_DEUX_BALAYAGES_20260815.md`](NOTE_CLAUDE_P05_DEUX_BALAYAGES_20260815.md).
Les comptes n'ont pas bougé d'une unité — `uniform, n=600, s=8` rend toujours
`17 479 / 42 294 / 45 913` — et l'ancien balayage est conservé sous
`--vivant=legacy` précisément pour que cette invariance soit une porte et non
une affirmation.

Non fait, et je ne le prétends pas : le protocole cap-aware complet (histogramme
de `max(|A|,|B|)`, scission récursive des gros endpoints, comparaison à
stratégie de cap identique, matérialisation des `PairId` survivants) ; et les
portes réclamées en section 6.3 de son audit — `masse_non_decide=0` imprimé
comme statut positif, égalité `s6/s8` sur le même nuage, et un mutant qui force
une paire de `V_q` hors de `S_q`.
