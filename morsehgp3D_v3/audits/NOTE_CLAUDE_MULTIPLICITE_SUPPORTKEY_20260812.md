# Note de Claude — partition fermée du ledger et histogramme de multiplicité

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note ferme les deux trous relevés par
[`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md).

Le contre-audit de la section 7 du même fichier retire toutefois trois
surinterprétations : les issues ne sont pas des propriétés causales
orthogonales, les `52 693` clés pending ne sont pas la borne après RLE (le
tableau contient `263 825` clés non dégénérées), et les « 1 200 cycles » ne sont
pas mesurés. Les nombres ci-dessous restent des observations historiques sans
transcript complet.

## 1. La partition ferme désormais exactement

Le rejet de rang **anticipé** était compté globalement mais n'était attribué ni
à une arité ni à un groupe. Il l'est maintenant, avec un compteur séparé de
groupes. L'identité publiée est, pour chaque arité,

`lifts = degeneres + owner + positivite + acceptes + rang_final + rang_anticipe`

et l'écart est imprimé. Sur `terrain`, `n=1 500`, `smax=11`,
`work-cap=20000`, sans filtre d'axe :

| arité | lifts | dégén. | owner | positivité | acceptés | rang final | rang anticipé | écart |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| q2 | `1 206 409` | `0` | `1 159 553` | `0` | `28 808` | `0` | `18 048` | `0` |
| q3 | `3 479 927` | `23` | `3 189 508` | `128 767` | `63 804` | `0` | `97 825` | `0` |
| q4 | `3 134 043` | `19 064` | `2 887 422` | `207 254` | `6 140` | `3` | `14 160` | `0` |

`early_rank_groups=129 970`. La somme des trois colonnes de rang anticipé vaut
`130 033`, exactement le nombre d'occurrences que le contre-audit avait calculé
comme non attribuées. Sa prédiction est donc confirmée au chiffre près.

Je retire par conséquent l'affirmation « le rang n'explique rien » : le rang
explique `130 033` occurrences sur `7 820 379`, soit `1,66 %`, et il le fait par
la branche anticipée, jamais par la branche finale.

## 2. L'histogramme de multiplicité par `SupportKey`

Ajouté sous `--multiplicity`, réservé aux petits nuages : c'est un instrument de
mesure, jamais une structure du chemin produit. Chaque occurrence est notée avec
son issue la plus avancée; l'identité `somme des multiplicités = occurrences`
est publiée avec son écart.

Relevé sur `terrain`, `n=400`, `smax=11`, `work-cap=20000` :

| issue | clés distinctes | occurrences | moyenne | p50 | p95 | max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| jamais possédé | `144 235` | `686 480` | `4,759` | `4` | `13` | `167` |
| non positif | `66 897` | `676 132` | `10,107` | `8` | `25` | `68` |
| possédé quelque part | `52 693` | `852 605` | `16,181` | `11` | `44` | `268` |

`multiplicite_total_occurrences=2 215 217` contre `lifts=2 220 024`, écart
`4 807` : ce sont exactement les tuples dégénérés, dont l'instrument ne retient
pas l'issue. Les acceptations valent `23 926` supports.

## 3. Ce que ces nombres disent, et ce qu'ils ne disent pas

Mes ratios `42/55/510` sont **retirés**. Ils divisaient des occurrences de trois
populations par les seules acceptations. La mesure honnête est :

- un tuple qui atteint `pending` — positif et possédé quelque part — est examiné
  **16,2 fois en moyenne**, médiane `11`, p95 `44`, maximum `268`;
- un tuple jamais possédé nulle part coûte `4,76` occurrences;
- un tuple non positif en coûte `10,1`.

Le nombre `52 693` compte seulement les clés ayant atteint pending. Il ne borne
pas le coût après RLE : les trois lignes totalisent `263 825` clés distinctes
non dégénérées, soit un facteur diagnostique
`2 215 217/263 825=8,40` occurrences par clé. Atteindre `52 693` exigerait en
plus des prunes parfaits pour toutes les autres clés.

Les trois populations appellent trois traitements distincts, et c'est le
résultat utile de cette mesure :

1. les `852 605` occurrences de tuples ayant atteint pending sont justement
   réduites par le RLE `SupportKey`; des tests owner spécialisés peuvent encore
   en réduire le coût;
2. les `676 132` occurrences non positives demandent un test de positivité
   **plus précoce** que le centre complet;
3. les `686 480` occurrences jamais possédées demandent un filtre géométrique
   supplémentaire — le test de rayon `beta` dans l'intersection courante des
   intervalles est la candidate naturelle, et elle n'exige pas le centre.

## 4. Calibration de coût

Sur `terrain`, `n=400`, l'exécution historique annonce `2 220 024` lifts et
`0,881 s` `user`. Ce temps englobe subdivision, bornes, bitsets, enveloppes,
census et table de mesure. Sans fréquence ni compteur de cycles, aucun débit ou
nombre de cycles par lift n'en est déductible. Cette valeur n'est pas un
benchmark.

GCP non utilisé.
