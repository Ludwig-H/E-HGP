# Audit du reçu d'échelle Yao48 q2 au commit 2e49dcf

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le snapshot CPU q2 de la campagne est **NO-GO avant G4**. Sur les familles
`terrain`, `scanline_single_pass` et `scanline_overlap_multiecho`, plusieurs
compteurs de travail ont deux exposants successifs strictement supérieurs à
`1,35`. Seule la famille `uniform` passe cette gate. Ce résultat refuse
l'ordonnance CPU courante, qui associe encore trop de survivantes à trop de
nœuds du classifieur; il ne réfute ni le théorème Yao48 ni une future
implémentation tuilée ou dual-tree.

Les douze ledgers ferment et les compteurs sont utiles comme falsificateurs.
Les temps ne sont pas qualifiables : la machine a compilé et exécuté d'autres
contrôles pendant la rampe. Cette campagne n'est ni un benchmark exclusif, ni
une exécution G4, ni un `warm_e2e`, ni un reçu de payload.

## Pincement et portée de la provenance

Les fichiers audités sont :

| objet | SHA-256 |
| --- | --- |
| [`scale_counters_raw.txt`](../receipts/yao48_scale_20260811/scale_counters_raw.txt) | `acf8e89248131cc7fdce3246f559d380acbee4ce67548ac9fb5e26efdd67d889` |
| [`exponents_derived.txt`](../receipts/yao48_scale_20260811/exponents_derived.txt) | `f2d9783211d884fef821a45961d428ee645bad656685d1520337957f54d2776f` |
| `prototype/pair_yao48_source.cpp` de la campagne | `68d7435f36af85987885a2b55702282728afaa879c214ebf54814506e8ef861b` |
| `prototype/yao48_source.hpp` de la campagne | `59720b420052aeb889cc05afdf557a8006a606ef2129c3751114ce3bc51068bd` |
| binaire Release observé | `31f3a9a17a06aaf5f2d78ec84d6c49f1cfff526a3178a00ac607729f2c8d8334` |

La boucle séquentielle observée a employé ce même binaire pour les quatre
familles et les trois tailles, avec `leaf_size=8`, `bank_pops=512`,
`chamber_visits=100000`, la banque ponctuelle `exact` et
`max_work=30000000000`. Elle a terminé ses douze cas avec `rc=0` et `DONE`.
Les sources q2 n'ont pas changé entre `c8dedf0` et le commit de reçus
`2e49dcf`.

Cette provenance est reconstruite depuis le transcript de session; elle n'est
pas auto-authentifiée par les deux fichiers. Ceux-ci omettent le `HEAD`, les
hashes source et ELF, le build-id, le compilateur, l'hôte, les horodatages par
run, la commande complète, `max_work`, `chamber_visits` et le mode de banque.
Ils qualifient donc un falsificateur historique pincé, jamais le successeur
`fc3e6d5` ni un produit.

Un contrôle séparé sur le binaire successeur a reproduit exactement tous les
compteurs de la ligne `terrain, n=12500`; son temps, différent, a été ignoré.
Ce contrôle ne remplace pas une nouvelle rampe authentifiée.

## Recalcul indépendant

Pour un compteur de travail positif `W`, les deux exposants sont calculés sur
les valeurs brutes, avant arrondi :

$$e_1=\log_{2}\left(\frac{W_{25000}}{W_{12500}}\right),\qquad e_2=\log_{2}\left(\frac{W_{50000}}{W_{25000}}\right).$$

Les 72 exposants du tableau dérivé ont été recalculés : aucune erreur
arithmétique n'a été trouvée. Les compteurs chargés suivants suffisent au
verdict :

| famille | compteurs avec `e1>1,35` et `e2>1,35` | exposants publiés représentatifs |
| --- | --- | --- |
| `terrain` | visites de coupe, survivantes, boîtes et tests du classifieur; aussi reçus région/radiaux, visites de lots et tombstones tardives | `cvis=1,39/1,38`; `surv=1,40/1,43`; `kbox=1,56/1,64`; `ktests=1,48/1,52` |
| `scanline_single_pass` | visites de cônes et de coupe, survivantes, boîtes et tests; aussi reçus région/radiaux, visites de lots et tombstones tardives | `cone=1,368/1,351`; `cvis=1,46/1,55`; `surv=1,51/1,69`; `kbox=1,72/1,83`; `ktests=1,69/1,67` |
| `scanline_overlap_multiecho` | visites de coupe, survivantes, boîtes et tests; aussi reçus région/radiaux, tombstones ponctuelles et tardives | `cvis=1,45/1,42`; `surv=1,45/1,48`; `kbox=1,69/1,65`; `ktests=1,56/1,56` |
| `uniform` | aucun compteur de travail publié | maximum représentatif `kbox=1,28/1,09` |

Le second exposant `cone` de `scanline_single_pass` vaut exactement
`1,351283465`; son affichage `1,35 <<ROUGE` est juste mais ambigu. Les secondes
sont imprimées comme des entiers dans le tableau dérivé alors que leurs
exposants utilisent les valeurs brutes à trois décimales. Les temps ne font de
toute façon pas partie de la décision de travail.

Les masses de paires couvertes croissent naturellement presque comme
`C(n,2)`. Elles mesurent une couverture, pas un coût exécuté, et sont exclues
de la gate d'exposant. Les inclure rendrait même `uniform` rouge et
contredirait le but de la gate. La règle s'applique aux visites, prédicats,
événements effectivement émis, records, octets et high-water, avec comparaison
sur les valeurs non arrondies.

## Identités fermées et coût tardif

Pour chacun des douze cas, les contrôles suivants passent :

- `region_mass+point_tombstones+survivors=C(n,2)`;
- `classifier_tombstones+census=survivors`;
- `full_chambers+underfull_chambers=48n`;
- `radial_mass<=region_mass`;
- code de sortie nul.

Ces égalités de masse ne sont pas un catalogue rejouable. Le mode normal ne
matérialise ni les `CensusRecord`, ni les reçus Yao/radiaux, ni les sorts par
paire. Elles prouvent que le comptage s'additionne, pas que chaque paire a le
bon sort.

À 50 k, le classifieur reçoit encore entre 23,0 et 50,0 millions de
survivantes. Parmi elles, `91,85 %` à `97,94 %` finissent en tombstone tardive.
Les quatre familles paient chacune entre 1,70 et 2,35 milliards d'évaluations
de boîtes et de points, sans compter la voie liste. Le résiduel census est
proche du linéaire, mais il est obtenu après ce produit état--nœud massif.

## Conseil d'implémentation

Le prochain cran ne doit pas être un simple port CUDA de cette boucle.

1. Conserver ce producteur CPU comme oracle de sorts agrégés et falsificateur
   de compteurs.
2. Avant le classifieur, essayer le certificat strict depuis l'une ou l'autre
   extrémité lorsque la banque opposée est déjà disponible dans la tuile ou un
   cache borné authentifié. L'ownership reste unique et le reçu engage le côté;
   cette symétrisation est exacte.
3. Remplacer le parcours `survivante x nœud` par le certificat dual-tree exact
   de la [note q2](NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md) : pour
   une boîte cible `Q` et un nœud témoin `W`, calculer le minorant séparable
   `L_p(Q,W)` aux quatre couples d'extrémités par axe. Une antichaîne de plages
   témoins disjointes, de masse dix et avec `L_p(Q,W)>0` partout tombstone `Q`;
   l'égalité reste active. Les comptes, offsets 64 bits et sorties résiduelles
   sont ensuite produits par `count--scan--fill`.
4. Rejouer une nouvelle rampe mono-binaire authentifiée. Un gain sur la seule
   fraction de masse coupée ne suffit pas : il faut faire chuter les
   tombstones tardives, les boîtes par survivante et les deux pentes chargées.

Cette route évite toujours toute matrice globale de paires, cellule, coface ou
incidence d'ordre supérieur.

GCP non utilisé pour cet audit.
