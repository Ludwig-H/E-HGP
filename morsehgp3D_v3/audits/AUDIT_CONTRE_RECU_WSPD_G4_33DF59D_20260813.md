# Contre-audit du reçu G4 WSPD au pin `33df59d`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit lit le reçu et son transcript sans modifier le logiciel ni employer
GCP. Le pin observé est
`33df59d451dc1c534a1fd5f1572e938472744fef`, commit
`fourteen of fifteen configurations hold the two-slope rule on the WSPD
front`.

## 1. Résultat reçu, avec deux corrections factuelles

Le transcript confirme une rampe CPU G4 sur cinq familles, trois séparations
et quatre tailles `12500/25000/50000/100000`. Quatorze configurations sur
quinze passent la règle implémentée : refus après deux pentes consécutives
supérieures ou égales à `1,35`.

La configuration rouge est toutefois
`scanline_single_pass,s=4`, et non `eight_clusters,s=4` :

```text
scanline_single_pass,s=4 : 0.982 / 1.435 / 1.586 -> REFUS
eight_clusters,s=4       : 1.215 / 1.225 / 1.182 -> OK
```

Le reçu suivi par Git attribue les pentes `1,435/1,586` à la mauvaise famille.
Il annonce aussi un intervalle `0,97..1,23` pour les configurations vertes,
alors que `scanline_single_pass,s=1` est vert avec une pente `1,349`. Les
comptes de records et pourcentages du tableau à `n=100000` concordent avec le
transcript ; ce sont le nom du refus et la borne supérieure résumée qui sont
faux.

Le résultat recevable est donc plus étroit : le **cardinal du front terminal**
respecte la gate finie sur quatorze des quinze couples `(famille,s)` testés.
Sous les hypothèses du split-tree et à `s` fixé, le théorème WSPD explique une
borne asymptotique linéaire de ce cardinal. Il ne transforme ni la gate finie,
ni les étapes suivantes en théorème de coût produit.

## 2. `front_records` n'est pas reçu comme coût dominant

Aucun temps de phase n'est publié. Le mot « dominant » n'est donc pas soutenu.
Le même transcript montre au contraire que la passe témoin courante effectue
presque tout son quantum de 64 dépilages par terminal :

| cas à `n=100000` | records | `eval` | `eval/record` |
| --- | ---: | ---: | ---: |
| `uniform,s=2` | `2 934 868` | `183 401 509` | `62,49` |
| `uniform,s=4` | `11 109 031` | `699 699 553` | `62,98` |
| `eight_clusters,s=4` | `8 856 672` | `556 770 445` | `62,87` |

Une `eval` compte un nœud `C`, mais le code appelle encore
`rect_classify` séparément pour chacun des trois bits ouverts. Le nombre
d'appels arithmétiques réels peut donc approcher trois fois cette colonne. La
WSPD borne le nombre de terminaux ; elle ne borne pas cette reprise à
`C=root`, la source carrier, le census ou le fold.

Si la même règle des deux pentes est appliquée à `eval`, seulement douze des
quinze configurations passent. Les trois séparations de
`scanline_single_pass` deviennent rouges :

```text
s=1 : 1.052 / 1.479 / 1.356
s=2 : 1.041 / 1.453 / 1.472
s=4 : 1.054 / 1.648 / 1.777
```

Le programme ne gate que `front_records`. Ce différentiel est précisément la
preuve expérimentale que la DFS cappée ne doit pas être portée sur device.

Les pourcentages q2/q3/q4 du reçu proviennent de cette DFS capée à 64, pas de
la banque `W=32/L=16`, ni du futur helper q2 léger. Ils ne permettent donc pas
de choisir la séparation du kernel P0. Ils sont des diagnostics de couverture
d'un autre chemin.

## 3. Conséquence pour le choix de `s`

Le reçu ne justifie ni `s=4` ni `s=8` globalement. À `n=50000`, le seul passage
de `s=2` à `s=4` fait évoluer le front ainsi :

| famille | records `s=2` | records `s=4` | facteur |
| --- | ---: | ---: | ---: |
| `scanline_overlap_multiecho` | `174 259` | `364 869` | `2,09` |
| `scanline_single_pass` | `149 164` | `300 674` | `2,02` |
| `terrain` | `428 018` | `1 100 550` | `2,57` |
| `eight_clusters` | `1 188 612` | `3 904 810` | `3,29` |
| `uniform` | `1 392 028` | `5 143 451` | `3,69` |

Pour `uniform,s=2`, une banque P0 à `W=32/L=16` aurait déjà une enveloppe de
`44,5 M` lectures et `22,3 M` recertifications avant la source. À `s=4`, ces
nombres deviennent `164,6 M` et `82,3 M`. La couverture DFS du reçu ne paie
pas cette inflation à la place du futur kernel.

La décision remise à Claude reste donc : baseline `s=2`, avec une ablation
`s=1` avant gel, puis **raffinement local** des seuls records ouverts. Pour un
record, calculer `Vbest`, comparer les marges centrales des deux splits
possibles de `A/B`, et créer les enfants seulement si le gain par record le
justifie. Cette politique obtient localement l'effet de boîtes plus fines sans
multiplier le front facile. Les crédits positifs s'héritent ; les IDs rejetés
comme endpoints relatifs sont rejoués ou conservés dans `endpoint_blocked`.

## 4. Le prochain delta utile ne change pas

Le jalon rentable est toujours `RF-GPU-P0` :

1. recevoir un tape WSPD canonique à faible séparation ;
2. corriger Morton48 3D et recaler la fenêtre haute ;
3. calculer un `Dlo` par rectangle et un `Vhi` par ID ;
4. ajouter q2 par `Hmin_singleton>0`, soit douze produits `i64`, sans U128 ;
5. garder q3/q4 au masque central tant qu'une ablation locale ne reçoit pas
   leur fallback large ;
6. émettre `proof_ids`, `closed_mask` et `residual_mask`, puis compacter
   stablement tous les résidus ;
7. mesurer cette tranche résidente sur trente warms avant corridor/carriers.

Le critère d'admission est le temps total futur
`WSPD + P0 + source complète + aval`, avec octets/HWM et deux pentes pour
chaque métrique physique. Une pente de `front_records` seule ne reçoit pas le
contrat d'une seconde.

## 5. Provenance et fermeture de session

Le transcript local observé a pour SHA-256
`13b01cf71bb613c466c1675cf1177a5f41f5782d9c74c8eb1c575a2a16a1d29d`.
Il confirme `g4-standard-48`, `SPOT`, `maxRunDuration=3600 s`, le coupe-circuit
invité, puis l'arrêt ciblé et l'état final `TERMINATED`. La session finit en
`rc=127` après les runs WSPD, sur une ligne de commentaire cassée.

Ce `session.log` est ignoré par `*.log` et n'appartient pas au commit ; le reçu
suivi ne pince ni hash du transcript, ni hash du binaire/source exécuté. Pour
devenir reproductible, Claude doit ajouter un transcript textuel nettoyé ou un
manifeste signé par hashes, avec le code de chaque sous-run. Le `wait || true`
a notamment empêché l'écriture de `code=3` pour la configuration rouge.

Verdict : progrès reçu sur le **cardinal WSPD**, `NO-GO` inchangé sur le chemin
exact `50000/1 s`. GCP non utilisé par cet audit.
