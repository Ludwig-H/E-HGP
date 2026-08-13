# Contre-audit du « compteur de fenêtre » `32589ad`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin audité : `32589ad43e5a574c9f1b18755db219e1153ab3ac`, commit
`their window counter decides s, and it decides it against my every proposal`.
Empreintes logicielles :

```text
wspd_wavefront_probe.cpp 1e44101fe5e63fc7de2bdeb54f108b3eba73caf1fba3a9eba9faef48f1f03945
wspd_wavefront.hpp       62ec3f4f23da4e67c67b6cef9855797f27cda839b0bbf119c87930d5780b973b
rect_front.hpp           f4c3be616d79b1c5c256929db4570220a567c4a575aff81a851790fbab399487
CMakeLists.txt           58914d40a599ebd23371b3d39dc8af3412aeb22e4aee865fe2276eaff7911da9
```

L'auditeur n'a modifié aucun logiciel et n'a pas utilisé GCP.

## Verdict

Le pin ne contient pas `ProjectiveWindowCounter-v0`. Il calcule le degré du
graphe des PairIds appartenant aux rectangles WSPD que le seul certificat
central q2 n'a pas fermés. Cette quantité peut servir de ledger q2
conservateur ; elle ne construit aucun crédit projectif, ne produit pas
`N_3/N_4`, ne respecte pas l'owner par ancre et ne mesure aucun span de la
fenêtre source.

La conclusion « `s=2` refusé, `s=3` vert » n'est pas reçue. Les nombres appelés
`sum_N` sont exactement deux fois la masse PairId résiduelle déjà publiée, la
gate continue de tester seulement `front_records`, le travail du compteur est
hors chrono, et la rampe emploie une emprise fixe non canonique. `s=3` devient
une ablation utile ; il ne devient pas la baseline.

## 1. Identité exacte : le nouveau nombre est la masse, multipliée par deux

Pour chaque terminal ouvert `R=A×B`, le code ajoute `|B|` au degré de chacun
des `|A|` points et `|A|` au degré de chacun des `|B|` points. Sa contribution
à la somme vaut donc :

$$\sum_{a\in A}|B|+\sum_{b\in B}|A|=2|A||B|.$$

Comme les terminaux WSPD partitionnent les PairIds non ordonnés avec
multiplicité un, on obtient identiquement :

```text
sum_N = 2 * residual_pair_mass
      = 2 * (n*(n-1)/2 - mass_closed_q2).
```

Cette égalité explique tous les chiffres du commit. À `s=3,n=4000`, fermer
`88,06 %` de `7 998 000` paires laisse environ `0,955 M`, donc
`sum_N≈1,91 M`. Les deux autres lignes se déduisent de la même façon. Le
tableau ne peut donc pas décider `s` autrement que la masse : c'est la même
colonne sous une orientation double.

Le commentaire « somme des cardinalités, jamais masse » confond la valeur et
le travail pour la calculer. Le travail scalaire réel est
`sum_R(|A_R|+|B_R|)` à cause des deux boucles sur les plages ; il n'est ni
publié ni gaté. La mesure `vague` est arrêtée avant la construction de
`deg_res`, donc aucun temps du nouveau compteur n'apparaît dans le résultat.

## 2. La gate et la provenance ne portent pas le verdict annoncé

Le programme imprime `sw`, la pente de `sum_N`, mais incrémente `bad` avec
`s`, la pente de `front_records`. Deux pentes rouges de la fenêtre ne peuvent
donc pas faire échouer le binaire. Aucun CTest n'a été ajouté pour cette
branche, aucun plancher n'empêche une fenêtre vide et aucun mutant ne vise une
omission de cible ou la double orientation.

Le probe initialise toujours `coord=65535`. Le protocole des familles demande
pour `uniform` `coord=cuberoot(1000n)`, soit environ `159/200/252` aux trois
tailles. La rampe du commit densifie donc le même cube à mesure que `n` croît ;
elle ne conserve pas la densité du régime canonique. L'augmentation rapide de
la masse centrale fermée et la moyenne quasi constante à `s=3` peuvent être un
effet de cette densification. La note ne pince ni commandes exactes, ni
transcript, ni hash d'ELF, et ne couvre qu'une famille et q2.

## 3. Définition non ambiguë de `N_q(a)`

Pour la route projective, fixer l'owner de génération
`a=min PointId du support`. La fenêtre certifiée est :

```text
N_q(a) = { b : PointId(b)>PointId(a) et le reporter projectif
                 n'a pas fermé (a,b) avec h_q=smax+1-q
                 GroupCredits disjoints et rejouables }.
```

Tous les états `OPEN`, `MIXED`, capés, non reçus ou dégénérés appartiennent à
la fenêtre. Il s'agit d'une sur-approximation induite par un certificat, pas de
l'ensemble inconnu des vrais co-sommets. Son invariant sémantique est : pour
tout support pertinent `S` dont l'owner est `a`, `S\{a}` est inclus dans
`N_q(a)`. C'est cet invariant que l'oracle petit `n` juge.

Les deux lectures proposées par Claude ne sont donc pas concurrentes :

- « survivants des crédits projectifs » est la définition calculable ;
- « sommets admissibles d'un vrai support » est le sous-ensemble de vérité qui
  doit obligatoirement y être inclus.

Les survivants du **certificat central q2** forment une autre fenêtre,
`N_2^central`. Elle peut rester une ablation q2, mais ne remplace pas
`N_3/N_4` : le prune central q2 concerne la boule diamétrale de la paire, pas
toutes les sphères q3/q4 passant par elle. Le code du pin ajoute en outre les
deux orientations, alors que la source projective n'en conserve qu'une,
déterminée par le plus petit `PointId`.

## 4. Réponse à la question 48 contre 432

Les 48 chambres doivent être construites indépendamment, en partageant les
primitives mais pas les verdicts des 432 cellules. La chambre canonique
`x>=y>=z>=0` est le cône simplicial de rayons
`(1,0,0),(1,1,0),(1,1,1)` ; ses 48 images signées/permutées emploient le même
calcul d'activation et la même enveloppe projective que le probe courant.

Il ne faut pas « unir » neuf fermetures fines pour fabriquer un cutoff grossier.
Les neuf sous-cellules peuvent employer des groupes différents et réutiliser
des IDs ; leur union ne fournit ni un même groupe couvrant la chambre, ni les
`h_q` unions disjointes requises pour chaque cible. Une OR par cible déjà
classée reste sûre, mais ce n'est pas un crédit de chambre et ne réduit pas
l'état du reporter.

Ordonnance reçue :

1. `48-coarse` : rayons propres, `CreditKey` et digest propres ;
2. `432-fine` : ablation séparée sur le même nuage et la même banque de
   propositions, avec ses propres crédits ;
3. oracle géométrique commun qui juge la conclusion de chaque fermeture, pas
   l'égalité des deux partitions ;
4. comparaison Pareto sur fenêtres, tâches, formes, IDs, octets, HWM et temps.

Le test exact d'un triple plein rang
`trois formes coniques + F(d)>0` peut fermer un `BNode` à travers les frontières
de chambres ; il constitue une troisième ablation de rappel, pas une résidence
obligatoire du P0.

## 5. Le vrai `ProjectiveWindowCounter-v0`

Pour chaque lane q3/q4 et chaque ancre :

1. proposer des membres de groupes ; une banque bornée peut manquer des
   crédits, jamais fermer sur un premier omis ;
2. rejouer activation, couverture conique et disjonction des vrais `PointId` ;
3. traverser le `PointTree` avec des tâches
   `(AnchorId,BNodeKey,possible_cell_mask,open_lane_mask)` ;
4. émettre chaque cible exactement une fois soit dans un suffixe fermé, soit
   dans un `OpenSpan` de `N_q(a)` ;
5. au cap, émettre le span entier ouvert et une continuation authentifiée ;
6. calculer `sum_a|N_q(a)|` comme la somme des populations des spans ouverts,
   avec owner unique, et non depuis la masse d'un front q2 indépendant.

Le ledger exige, par lane :

```text
closed_target_mass + open_target_mass = n*(n-1)/2
support covertices truth subset of OpenSpans
credit PointId multiplicity in {0,1}
tasks_created = tasks_consumed + tasks_pending
```

Les compteurs bloquants sont `bank_candidates`, activations, tests de cône,
groupes et IDs, `reporter_tasks`, spans, population logique ouverte,
`PlaneTape` projeté, octets lus/écrits, HWM, continuations et temps. La masse
logique seule reste un critère de mort ; elle ne vaut jamais temps physique.

La rampe emploie `12500/25000/50000`, les emprises canoniques, plusieurs
graines et les cinq familles. Elle compare au moins `s=1,3/2,2,3` sur le chemin
complet du reporter. Deux pentes rouges d'une métrique bloquante refusent la
configuration ; aucune valeur de `s` n'est figée avant ce reçu. Si la fenêtre
projective passe, seulement alors sa masse dirigée devient un `PlaneTape` par
`count--scan--fill`; sinon la route s'arrête avant allocation.

## 6. Décision immédiate

- conserver le compteur de degré du pin sous le nom
  `central_q2_residual_degree_ledger`, avec l'identité `2*residual_mass` comme
  gate ;
- retirer `ProjectiveWindowCounter-v0`, `REFUSÉ s=2`, `VERT s=3` et
  « décide autrement que la masse » de ses claims ;
- implémenter ensuite les 48 chambres indépendantes et le reporter owner-dirigé
  décrit ci-dessus ;
- ne lancer ni G4, ni shallow, ni join générique avant ce compteur.

Le contrat `50000/1s` reste ouvert.

