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

Un rejeu local avec les emprises canoniques conserve néanmoins un signal utile
pour l'ablation `s=3` : `sum_N=1 754 188/3 809 732/8 485 770` à
`n=4000/8000/16000`, soit des pentes `1,119/1,155`, des moyennes
`438,5/476,2/530,4` et des maxima `946/1018/1085`. Il comporte
`23/893/1356` rectangles tronqués et prend `7,35/12,80/20,92 s` CPU. Ce
diagnostic réfute l'explication par la seule densification ; il ne change pas
l'identité `sum_N=2*residual_pair_mass`, ne couvre toujours ni q3/q4 ni les
crédits projectifs, et ne qualifie aucun temps produit. `s=3` est donc une
ablation prometteuse à porter au vrai reporter, pas une configuration reçue.

## 3. Définition non ambiguë de la fenêtre d'arêtes `E_q(a)`

Le certificat projectif ferme une paire entière ; il ne prouve pas qu'une paire
nue est owner. Son sens d'énumération peut donc suivre le stockage : poser
`GenerationRank=(Morton48,PointId)`, orienter chaque paire une seule fois du
plus petit vers le plus grand rang, et définir :

```text
E_q(a) = { b : GenerationRank(b)>GenerationRank(a) et le reporter projectif
                 n'a pas fermé (a,b) avec h_q=smax+1-q
                 GroupCredits disjoints et rejouables }.
```

Tous les états `OPEN`, `MIXED`, `UNDERFULL`, capés, non reçus ou dégénérés
appartiennent à la fenêtre. Il s'agit d'une sur-approximation d'arêtes candidates,
pas de l'ensemble inconnu des vrais co-sommets. Son invariant sémantique est :
pour tout support pertinent `S`, si son arête maximale canonique est `{u,v}`,
alors l'endpoint de plus grand `GenerationRank` appartient à la fenêtre de
l'autre endpoint. `PairKey=(min PointId,max PointId)` et le tie-break de l'owner
restent non orientés. Les autres sommets sont produits ensuite par la lentille
et le shallow de cette arête. C'est cet invariant que l'oracle petit `n` juge.

Les deux lectures à ne plus confondre sont donc :

- « survivants des crédits projectifs » est la définition calculable de
  `E_q(a)` ;
- « arêtes maximales canoniques de vrais supports » est le sous-ensemble de
  vérité qui doit obligatoirement rester ouvert.

Les survivants du **certificat central q2** forment une autre fenêtre d'arêtes,
`E_2^central`. Elle peut rester une ablation q2, mais ne remplace pas
`E_3/E_4` : le prune central q2 concerne la boule diamétrale de la paire, pas
toutes les sphères q3/q4 passant par elle. Le code du pin ajoute en outre les
deux orientations, tandis que `E_q` conserve seulement l'orientation par
`GenerationRank`.

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

## 5. Le vrai `CanonicalEdgeWindowReporter-q4-v0`

Pour chaque lane q3/q4 et chaque ancre :

1. proposer des membres de groupes ; une banque bornée peut manquer des
   crédits, jamais fermer sur un premier omis ;
2. rejouer activation, couverture conique et disjonction des vrais `PointId` ;
3. traverser le `PointTree` avec des tâches
   `(AnchorId,BNodeKey,possible_cell_mask,open_lane_mask)` ;
4. émettre chaque cible exactement une fois soit dans un suffixe fermé, soit
   dans un `OpenEdgeSpan` de `E_q(a)` ;
5. au cap, émettre le span entier ouvert et une continuation authentifiée ;
6. calculer `sum_a|E_q(a)|` comme la somme des populations des spans ouverts,
   avec orientation unique par `GenerationRank`, et non depuis la masse d'un front q2
   indépendant.

Le ledger exige, par lane :

```text
closed_target_mass + open_target_mass = n*(n-1)/2
canonical max-edge of every true support is in OpenEdgeSpans
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

### 5.1 Commencer par q4 et non par les trois lanes

Le plus petit falsificateur est `PWC0-A/CanonicalEdgeWindowReporter-q4-v0`. Il demande
`h_4=8` crédits, ferme donc au moins autant de cibles que les seuils `h_3=9` et
`h_2=10` pour une même suite de crédits, et produit la fenêtre qui doit être la
plus petite. Si `sum_a|E_4(a)|`, ses tâches ou ses octets restent denses, il est
inutile d'écrire le shallow q4 ou de dupliquer d'abord le reporter pour les
autres lanes. Un vert q4 ne reçoit pas q3/q2 ; il autorise seulement leur ajout
avec masque partagé.

Le P0 prend les 48 chambres signées/permutées avec leurs propres rayons
extrêmes, leur convention half-open et leurs propres `CreditKey`. Une chambre
`OPEN/MIXED` est ensuite la seule à être raffinée dans ses neuf sous-cellules.
Le mode 432 fixe reste l'ablation de rappel maximal. Une mauvaise fenêtre aux
48 chambres ne réfute donc pas encore la route projective ; elle refuse cette
résolution grossière, puis déclenche le raffinement `48 -> 9` uniquement sur
les chambres ouvertes.

### 5.2 Ce qui est réutilisable dans `cell_credits`, et ce qui ne l'est pas

`activation_height`, les 432 tables de rayons et l'enveloppe Andrew entière
sont de bonnes primitives propositionnelles. Le probe courant n'est toutefois
pas une banque reçue :

- ses identifiants d'union sont des indices locaux du pool, pas les vrais
  `PointId` persistants ;
- `rank_counts` est incrémenté rayon par rayon avant de savoir si les trois
  rayons passent : un échec tardif laisse donc une télémétrie non
  transactionnelle ;
- le commentaire « plus petite activation, donc plus proche » est faux. Dans
  `U00`, `s_near=(1,-2,0)` a norme carrée `5` et activation `16`, tandis que
  `s_far=(4,0,0)` a norme carrée `16` et activation `5` ;
- `pool<=48` ne peut pas contenir le pire reçu q4 de huit crédits de neuf IDs,
  soit jusqu'à `72` IDs. Un pool plus petit reste licite seulement s'il publie
  `UNDERFULL/OPEN` ;
- le ledger final rebalaie toutes les cibles en `O(n^2)` et ne produit ni
  `BNodeKey`, ni span, ni continuation.

La transaction correcte construit le crédit dans un temporaire, convertit
chaque index local en vrai `PointId`, rejoue les trois carriers, vérifie la
disjonction avec les crédits déjà commis, puis seulement fusionne compteurs et
digest. Une proposition manquée agrandit `E_4(a)` ; elle ne peut jamais fermer
sur le premier candidat omis.

### 5.3 Boucle minimale du reporter

Pour chaque ancre `a`, la banque calcule le cutoff du huitième crédit dans
chaque chambre reçue. Le reporter traverse ensuite l'arbre des cibles avec des
tâches `(AnchorId,BNodeKey,chamber_mask,state)` :

```text
last_GenerationRank(BNode) <= rank(a)  -> DROP_ORDER
first_GenerationRank(BNode) <= rank(a) -> SPLIT_ORDER
direction et hauteur toutes fermees    -> CLOSED_SPAN
direction ou seuil tous ouverts        -> OPEN_SPAN
frontiere/melange                       -> SPLIT_TARGET
quantum/cap atteint                     -> OPEN_SPAN + ContinuationKey
```

Les `OPEN_SPAN` partitionnent exactement le suffixe de cibles
`GenerationRank(b)>GenerationRank(a)` non
fermées par la banque reçue. Le compteur logique est la somme de leurs
populations ; aucun `PairId` n'est développé sur le chemin produit. Le juge
`n<=64` les développe seulement pour exiger l'égalité avec le classement
ponctuel du **même** bank, puis vérifie que l'arête maximale canonique de chaque
support q4 exhaustif appartient à la fenêtre orientée correspondante.

Cette orientation rend le domaine cible contigu dans l'ordre Morton et borne
le filtre d'ordre à la seule frontière du suffixe. Garder `a<b` par `PointId`
serait encore exact, mais des IDs quasi aléatoires dans chaque `BNode`
forceraient des splits d'ordre potentiellement linéaires par ancre. La fixture
`id0=(10,13,10), id1=(10,10,10), id2=(12,11,11), id3=(10,10,11)` grave la
distinction : l'arête owner unique est `PairKey=(0,3)`, tandis que son sens de
génération Morton est `id3 -> id0`.

Les mutants indispensables partagent un ID entre deux crédits, confondent
index local et `PointId`, omettent une chambre, changent la stricte
d'activation, traitent un cap comme `CLOSED`, inversent l'owner et emploient le
premier omis par distance comme cutoff. Les compteurs bloquants sont séparés :
propositions, activations, tests d'enveloppe, crédits commis, cellules
`UNDERFULL`, tâches créées/consommées/en attente, spans ouverts/fermés,
population logique, octets lus/écrits, HWM et temps. `tasks_created` doit valoir
`tasks_consumed+tasks_pending`.

Les fates de spans sont exclusifs et vérifient
`input_mass=closed_mass+open_mass+pending_mass` par lane. Tant que
`pending_mass>0`, `sum_a|E_q(a)|` n'est pas une fenêtre finale.

Le P0 feuille publie explicitement `anchor_root_seeds=n`. Il ne prétend pas
avoir supprimé les recherches par ancre ; il falsifie d'abord la densité de la
fenêtre avec le plus petit objet sémantiquement exact. Si la fenêtre est sparse
mais ce compteur de graines ou les tâches domine, le palier `PWC0-B`
universalise les crédits sur un `ANode` et remplace les `n` graines par une
jointure `ANode×BNode` à graine unique.

La banque bornée reste propositionnelle. Une fenêtre dense à `P=96` refuse ce
proposer, pas tout certificat projectif. La porte publie donc `P=48/96/192`, le
taux `UNDERFULL` et la cause précise de chaque span ouvert. La fermeture n'est
monotone que si ces banques sont des préfixes emboîtés et conservent les crédits
déjà commis. Un `NO-GO` global suppose soit un cap produit et une arène dérivés
d'un layout/preflight, soit une ablation stabilisée.

Un vert de `E_4` reste nécessaire mais non suffisant. Pour chaque arête ouverte,
chaque site actif fournit une forme : le second ledger est
`M=sum_(a,b in E_4)m_ab`. `EdgeActiveFormCounter-v0` doit le calculer par un
dual-tree exact `(EdgeSpan,CNode)` et publier tests, blocs factorisés, hits,
tâches, maximum par arête, octets/HWM et continuations. Même `|E_4|=O(n)` ne
permet pas de financer le shallow si `M` est dense.

## 6. Décision immédiate

- conserver le compteur de degré du pin sous le nom
  `central_q2_residual_degree_ledger`, avec l'identité `2*residual_mass` comme
  gate ;
- retirer `ProjectiveWindowCounter-v0`, `REFUSÉ s=2`, `VERT s=3` et
  « décide autrement que la masse » de ses claims ;
- implémenter `PWC0-A/CanonicalEdgeWindowReporter-q4-v0` : 48 chambres indépendantes,
  puis raffinement adaptatif des seules chambres ouvertes dans leurs neuf
  sous-cellules ;
- ne lancer ni G4, ni shallow, ni join générique avant ce compteur et le second
  ledger `EdgeActiveFormCounter-v0` ; écrire
  `PWC0-B` seulement si `PWC0-A` reçoit une fenêtre sparse mais un coût de
  graines/tâches rouge.

Le contrat `50000/1s` reste ouvert.
