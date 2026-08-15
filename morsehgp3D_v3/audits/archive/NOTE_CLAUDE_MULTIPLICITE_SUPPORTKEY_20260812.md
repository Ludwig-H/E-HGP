# Note de Claude — partition fermée du ledger et histogramme de multiplicité

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note ferme numériquement le premier trou et ouvre l'instrument demandé
pour le second, relevés par
[`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md).

Le contre-audit des sections 7, 10 et 11 du même fichier retire toutefois trois
surinterprétations : les issues ne sont pas des propriétés causales
orthogonales, les `52 693` clés pending ne sont pas la borne après RLE (le
tableau contient `263 825` clés non dégénérées), et les « 1 200 cycles » ne sont
pas mesurés. Il ne reçoit pas davantage le titre du commit `64cf6fe` : une RLE
spatiale locale est exacte, mais sa petite réplication n'est pas prouvée par un
seul `terrain,n=400`; elle paie un solve par clé et par lot. Les nombres
ci-dessous restent des observations historiques sans transcript complet.

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
explique `130 033` occurrences anticipées et `3` occurrences finales sur
`7 820 379`, soit `1,663 %`. La branche anticipée domine, mais la branche finale
n'est pas vide.

## 2. L'histogramme de multiplicité par `SupportKey`

Ajouté sous `--multiplicity`, réservé aux petits nuages : c'est un instrument de
mesure, jamais une structure du chemin produit. Chaque `SupportKey` reçoit un
compteur cumulé et le maximum des issues observées; chaque occurrence n'est donc
pas classée séparément. L'identité
`somme des multiplicités = occurrences` est publiée avec son écart.

Relevé sur `terrain`, `n=400`, `smax=11`, `work-cap=20000` :

| issue | clés distinctes | occurrences | moyenne | p50 | p95 | max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| jamais possédé | `144 235` | `686 480` | `4,759` | `4` | `13` | `167` |
| non positif | `66 897` | `676 132` | `10,107` | `8` | `25` | `68` |
| owner et positif au moins une fois, rang non classé | `52 693` | `852 605` | `16,181` | `11` | `44` | `268` |

`multiplicite_total_occurrences=2 215 217` contre `lifts=2 220 024`, écart
`4 807` : ce sont exactement les occurrences dégénérées, dont l'instrument ne
retient ni la clé ni l'issue. Les `23 926` acceptations ne forment pas une
population de l'histogramme : une clé pending rejetée au rang reste dans la
dernière ligne, car l'issue provisoire `4` domine l'issue rang `3`.

## 3. Ce que ces nombres disent, et ce qu'ils ne disent pas

Mes ratios `42/55/510` sont **retirés**. Ils divisaient des occurrences de trois
populations par les seules acceptations. La mesure honnête est :

- une clé qui atteint `pending` au moins une fois possède **16,2 occurrences en
  moyenne**, médiane `11`, p95 `44`, maximum `268`; ce total inclut ses rejets
  owner dans les autres cellules;
- une clé pour laquelle aucune occurrence émise n'est owner possède `4,76`
  occurrences en moyenne; cette classe peut contenir des clés intrinsèquement
  non positives, puisque ces occurrences n'atteignent pas toutes le test de
  positivité;
- une clé intrinsèquement non positive pour laquelle au moins une occurrence
  owner est vue, et qui n'atteint donc jamais pending, possède `10,1`
  occurrences en moyenne. La positivité est une propriété du `SupportKey`, pas
  une issue qui varierait entre ses occurrences.

Le nombre `52 693` compte seulement les clés ayant atteint pending. C'est un
floor trivial de ce sous-ensemble dans cette observation figée, mais ni le coût
après RLE ni un minorant universel de Source S ou de H0 : les trois lignes
totalisent `263 825` clés distinctes non dégénérées. Les `4 807` occurrences
dégénérées représentent entre une et `4 807` clés supplémentaires. Une
géométrie par clé demanderait donc entre `263 826` et `268 632` lifts, soit une
compression comprise entre `8,26` et `8,41`. Atteindre `52 693`, facteur
`42,13`, exigerait un oracle gratuit pour owner et positivité, pas le seul RLE.

Ces trois classes ne justifient pas trois traitements causaux distincts. Le
résultat utile est seulement l'étagement suivant :

1. le RLE `SupportKey` conserve les contextes compacts mais ne construit qu'une
   géométrie par clé, puis recherche l'unique owner dans le run entier;
2. l'acuité q3 et des bornes nécessaires q4 peuvent rejeter une partie des clés
   intrinsèquement non positives avant le lift complet, mais leur gain causal
   exige des flags orthogonaux `positive/owner/relevant`, pas la classe de stade
   maximal;
3. un prune amont owner peut être étudié séparément. Le diamètre fournit la
   condition monotone `D2*S2<=4*Umin` sans centre; connaître `beta` exact pour
   q3/q4 demande en revanche un solve déterminantal comparable au lift. La
   classe « aucune occurrence owner » ne mesure pas à elle seule le gain de ce
   prune.

## 4. Calibration de coût

Sur `terrain`, `n=400`, l'exécution historique annonce `2 220 024` lifts et
`0,881 s` `user`. Ce temps englobe subdivision, bornes, bitsets, enveloppes,
census et table de mesure. Sans fréquence ni compteur de cycles, aucun débit ou
nombre de cycles par lift n'en est déductible. Cette valeur n'est pas un
benchmark.

GCP non utilisé.
