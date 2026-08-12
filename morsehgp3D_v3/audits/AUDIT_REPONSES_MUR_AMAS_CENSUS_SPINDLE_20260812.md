# Contre-audit du mur amas/census — spindle complet avant liste

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document répond aux quatre questions de
[`NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md`](NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md).
Le pin reçu pendant le contre-audit est
`59d098bd1b027a55a381e91499bc3432cc50f192`, commit
`prove the filter changes nothing, then stop paying for it — and measure the
wall the clusters build`. L'ELF Release observé a le SHA-256
`114be24e4c87f1c03814a88e6ec34820ccbb57a473e414d76834819fd76c201f`.
Un delta postérieur non pincé ajoute les compteurs front/rejets, rend la garde
de densité opt-in, propage `--compare-engines` et compare trente-cinq compteurs
des deux moteurs. Son ELF SHA-256 `fed7e39c...` n'est pas une autorité de ce
document. GCP non utilisé.

## Verdict

La suppression de `theta` est reçue mathématiquement et utile. Le CMake grave
trois familles sur le moteur reference, plus causalité/planchers/refus ; il ne
reçoit pas à lui seul la formule « cinq familles et deux moteurs ». Le diff
ajoute sept noms de portes theta et recâble le mutant existant, plutôt que les
« six portes nouvelles » annoncées. La nouvelle note
identifie aussi deux postes effectivement rédhibitoires : la boucle
`C(nlens,2)` et les census répétés sur `kept`. En revanche, quatre conclusions
de la note ne sont pas reçues :

1. les colonnes `candidate_pairs=C(n,2)` et `front_witness_prunes=0` ne
   correspondent pas aux reçus qui portent les colonnes q4/census ;
2. une boule médiane vide n'implique pas un spindle de témoins universels vide ;
3. le quotient `interior_tests/(supports/n*n)` n'est pas une taille moyenne de
   `kept` et n'attribue pas le temps au seul census ;
4. des pentes finies entre `n=100` et `500` sont rouges et compatibles avec un
   régime cubique, mais ne prouvent pas une complexité asymptotique cubique.

La reprise prioritaire est néanmoins claire : **avant `gather_sites`**, compter
les nœuds AABB entièrement contenus dans le spindle q3/q4 complet. Ce
certificat trouve précisément les témoins proches des amas que la petite boule
de milieu ne voit pas. Ensuite seulement, une cutting signée transporte les
identités `always_inside` et les conflits jusqu'au census.

## 1. Les colonnes de front de la note ne viennent pas du même reçu

Sur le même ELF, la même graine et la même famille, les reproductions donnent :

| `n` | note : candidats/prunes | reçu reproduit : candidats/prunes | q4 paires |
| ---: | ---: | ---: | ---: |
| `100` | `4 950 / 0` | `4 950 / 0` | `2 446 467` |
| `150` | `11 175 / 0` | `11 174 / 1` | `8 370 933` |
| `200` | `19 900 / 0` | `19 899 / 2` | `17 892 952` |
| `300` | `44 850 / 0` | `44 831 / 40` | `55 220 207` |

Les colonnes q4 reproduites sont exactement celles de la note, mais les deux
colonnes de front divergent. Le claim « deux faits exacts à chaque taille » est
donc réfuté par le propre binaire pincé. Il faut versionner les reçus bruts,
leur SHA d'entrée, le SHA de l'ELF et la commande ; aucune colonne ne doit être
reconstruite depuis `C(n,2)`.

Le résultat utile reste très rouge : le front de boule médiane ne ferme qu'une
masse minuscule et les paires q4 croissent avec une pente globale d'environ
`2,71` entre `100` et `500`. Le temps donne environ `3,05`, mais ses pentes
adjacentes oscillent fortement. La formulation correcte est « pentes
empiriques rouges compatibles avec un régime cubique », pas « producteur
cubique démontré ».

## 2. Réponse Q1 — le juge indépendant reste premier

Oui. L'ordre 2 reste préalable à la réception de la cutting, mais il doit être
un **oracle borné minimal**, pas une qualification de performance du producteur
legacy. Il énumère à petit `n` et compare les enregistrements complets
`(BallKey,SupportKey,I_B,U_B,ownerPair)` ; ses solveurs rationnels, sa
positivité, son owner et son census ne réemploient pas ceux du sujet.

La mécanique de cutting peut être développée en parallèle, mais aucun prune,
rang, shell, plateau ou owner n'est reçu avant l'accord à cet oracle. Le vieux
producteur sert de second différentiel et de falsificateur de coût, jamais
d'autorité mathématique.

L'attribution actuelle du census doit aussi être retirée. `interior_tests`
compte tous les prédicats des census q3/q4, y compris les supports finalement
rejetés et les arrêts anticipés. `supports/n` additionne q2, q3 et q4, alors
que q2 ne passe pas par ce census. Enfin, `hw_kept` est un maximum, pas une
moyenne. L'égalité numérique de la note est donc une corrélation entre univers
différents.

Le reçu nécessaire publie séparément :

- `census_calls_q3/q4`, acceptés et rejetés ;
- `sum_crossing_entered_q3/q4` et quantiles de `ncross` ;
- tests consommés avant acceptation ou rejet et profondeur d'arrêt anticipé ;
- temps du census, de la génération q4, de l'owner et du payload ;
- `sum_kept`, p50/p95/p99/max par ancre survivante.

Jusqu'à ce reçu, `C(nlens,2)` **et** le census sont deux murs mesurés ; aucun ne
doit être déclaré l'unique cause.

## 3. Réponse Q2 — la masse seule ne décide rien, mais un nœud ALL-spindle oui

La seule masse d'un nœud, même accompagnée d'une AABB ambiguë, ne certifie pas
le budget. Deux ensembles peuvent avoir même masse et même boîte mais des
comptes de témoins différents. Pour
`a=(10,10,10)`, `b=(30,10,10)`, prendre deux nœuds de masse dix et d'AABB
`[1,19] times {10} times {10}` :

- `{1,11,12,13,14,15,16,17,18,19}` contient neuf témoins axiaux universels ;
- `{1,2,3,4,5,6,7,8,9,19}` n'en contient qu'un.

La masse ne peut donc être créditée qu'après une preuve `ALL` géométrique.

### Lemme du nœud spindle

Pour une paire distincte, poser `d=b-a`, `D2=d dot d`,
`U=2z-a-b`, `g=D2-U dot U` et
`Q=D2*(U dot U)-(U dot d)^2`. Les domaines ouverts de témoins universels sont :

$$W_3(a,b)=\left\lbrace z:g>0\text{ et }3g^2>4Q\right\rbrace,\qquad W_4(a,b)=\left\lbrace z:g>0\text{ et }g^2>2Q\right\rbrace.$$

Ces spindles sont des intersections de boules ouvertes, donc convexes. Pour une
AABB fermée `C`, l'inclusion `C subset W_q(a,b)` est équivalente à
l'appartenance stricte de ses huit coins à `W_q`. Les coins sont géométriques,
pas nécessairement des `PointId` u16 ; les prédicats sont évalués en largeur
certifiée. Toute égalité donne `UNKNOWN`.

Si `C subset W_4`, tous ses `PointId` sont intérieurs à toute boule q4
admissible de l'ancre ; sa masse crédite q4 et q3 sans charger les points. Si
`C subset W_3` seulement, sa masse crédite q3. Un nœud contenant `a` ou `b`
échoue automatiquement au test strict ; sa feuille est descendue et
l'endpoint exclu. Les nœuds crédités forment une antichaîne **par lane**. Une
frame descendue depuis un parent `W3-only` porte `q3_already_credited` ; un
enfant `W4` ne crédite alors que q4. Sans ce bit, la même plage serait comptée
deux fois en q3.

À `smax=11`, huit crédits tuent q4 et neuf tuent q3. En général, les seuils
sont respectivement `smax-3` et `smax-2`. La DFS pré-liste conserve deux masks :

```text
node ALL-W4 -> c4 += mass; c3 += mass seulement si la plage n'est pas déjà créditée q3
node ALL-W3 seulement -> c3 += mass et marquer q3_already_credited
  -> ne pas descendre si q4 est déjà morte
  -> sinon descendre encore pour chercher des sous-nœuds ALL-W4
UNKNOWN -> descendre pour toute lane encore vivante
dès c4 >= smax-3 : tuer q4
dès c3 >= smax-2 : tuer q3
si les deux lanes meurent : ne construire ni site_list, ni kept, ni lens
```

Fixture positive : `a=(20,20,20)`, `b=(32,20,20)` et la boîte
`[23,29] times [19,21] times [19,21]`. Ses huit coins vérifient strictement le
spindle q4 ; une masse huit tue q4. Ajouter `z=(26,20,20)` comme neuvième
`PointId` tue q3. Mutants : tester un seul coin, accepter l'égalité, compter un
endpoint, recréditer q3 dans un enfant W4 d'un parent W3-only, ou arrêter un
nœud W3-only alors que q4 reste vivante.

Ce certificat répond directement à l'objection inter-amas. La phrase « tout
témoin universel est nécessairement dans le vide » est fausse. Avec
`a=(10,10,10)`, `b=(30,10,10)` et `z=(11,10,10)`, on obtient
`D2=400`, `U=(-18,0,0)`, `g=76`, `Q=0` : `z`, proche de l'amas de `a`, est
universel q3 **et** q4. Seule la petite boule médiane est confinée au vide.

Le ledger minimal est
`node_all4_mass`, `node_all3_only_mass`, `spindle_node_visits`,
`endpoint_descents`, morts q3/q4 et
`pair_mass_closed+pair_mass_surviving=pair_mass_input`, par lane et digest de
plages. C'est un certificat exact et nouveau ; sa parcimonie sur
`eight_clusters` doit être mesurée, jamais supposée.

Une DFS séparée pour chaque `PairId(a,b)` ne constitue toutefois qu'un oracle
`spindle-node-only` : après un front quadratique, elle ajouterait un autre
rescan quadratique. La route produit doit relever le même prédicat sur un
produit factorisé `A_endpoint times B_partner times C_witness` avant émission
des `PairId`. Des bornes dirigées de `D2`, `U`, `g` et `Q`, ou une validation
des coins extrêmes du produit, doivent prouver qu'une même antichaîne `C` est
`ALL-W3/W4` pour **toutes** les paires du bloc ; `UNKNOWN` subdivise. Le reçu
publie masse de blocs et de paires fermée, visites du classifieur, deux pentes
`<=1,35` et un cap absolu. Aucune liste ni DFS par partenaire n'est admise sur
le chemin 50 k.

### Lift entier du certificat sur `A times B times C`

Le lift suivant remplace « bornes dirigées » par une première autorité exacte.
Pour `a in A`, `b in B`, `z in C`, poser

$$H(a,b,z)=(b-z)\mathbin{\cdot}(z-a),\qquad R(a,b,z)=\left\lVert(b-a)\mathbin{\times}(z-a)\right\rVert^2.$$

Les identités ponctuelles sont `g=4H`, `Q=4R`. Les deux prédicats deviennent

$$\text{q3: }H>0\text{ et }3H^2>R,\qquad\text{q4: }H>0\text{ et }2H^2>R.$$

Le minimum de `H` sur trois AABB fermées est atteint sur leurs coins : la
fonction est affine en `a,b`, concave en `z`, et une vertexisation successive
ne peut augmenter son minimum. Sa séparabilité permet même de calculer
`Hmin` par trois minima de huit évaluations scalaires, soit vingt-quatre
évaluations plutôt que `512`. La fonction `R` est séparément convexe, car elle
est la norme carrée d'une application affine lorsque deux variables sont
fixées. Une vertexisation successive prouve que `Rmax` est atteint parmi les
`8^3=512` triples de coins.

Par conséquent, `Hmin>0 && 3*Hmin^2>Rmax` certifie `ALL-W3`, et
`Hmin>0 && 2*Hmin^2>Rmax` certifie `ALL-W4`. Toute égalité ou tout échec rend
`UNKNOWN`. Le mot exact qualifie les extrema et la sûreté de la décision, pas
sa complétude : `Hmin` et `Rmax` peuvent venir de triples différents. Par
exemple, fixer `a=(0,0,0)`, `z=(10,10,0)` et laisser `b` dans
`[11,100] times [11,100] times {0}`. En écrivant
`x=b_x-10`, `y=b_y-10`, tous les points du bloc vérifient
`H=10*(x+y)` et `R=100*(x-y)^2`, donc q4 strictement ; pourtant
`Hmin=20`, `Rmax=792100` et `2*Hmin^2=800`. Le certificat répond correctement
`UNKNOWN`, jamais `OUTSIDE`.

Sous u16, `|H|<2^34`, tandis que `R`, `2*H^2` et `3*H^2` demandent jusqu'à
69 bits. `H` est formé en `i64`, puis les carrés et comparaisons sont promus
**avant** multiplication vers `u128` ou une représentation équivalente. Les
mutants `i64`, `>=`, coefficient `2/3` inversé et coin omis doivent mourir ;
les fixtures d'égalité q4 `a=(10,10,10),z=(11,10,10),b=(12,11,11)` et q3
`a=(10,10,10),z=(11,9,10),b=(11,8,11)` restent `UNKNOWN`.

L'ordonnance est un self-join canonique de `A times B` qui partage une frontière
persistante de nœuds `C`. Scinder `A/B` partitionne exactement la masse de
paires ; scinder `C` partitionne seulement la recherche des témoins et ne doit
jamais recréditer cette masse. Les crédits `C` sont des reçus de plages
disjointes par lane, hérités lors d'un split d'endpoints sans repartir de la
racine. Si un nœud `C` chevauche les identifiants de `A/B`, il descend jusqu'à
exclure `z=a,b`. Une fermeture annule et comptabilise les tâches tardives de la
lane ; aucun atomique concurrent ne peut fermer deux fois le même bloc.

La broad phase calcule d'abord `Hmin`, puis une majoration d'intervalles de
`R`. Le parcours des `512` triples est l'oracle/fallback des états encore
ambigus et s'arrête dès que q4 puis q3 sont réfutées comme certificats. Il ne
doit pas devenir une boucle systématique `A times B times C`. La gate publie
`classifier_calls`, triples évalués, sorties `ALL4/ALL3-only/UNKNOWN`,
expansions de frontière, tâches annulées, octets/HWM et deux travaux séparés :
classifieur produit et masse résiduelle vers listes/census. Elle impose
`PairId_before_terminal=0`, les identités de masse par lane, l'accord complet
au juge sur petit `n`, puis deux pentes `<=1,35` et des caps absolus sur
`eight_clusters`.

## 4. Réponse Q3 — `kept` ne possède aucune saturation déterministe

Pour une ancre de longueur `D` à densité `rho`, le préfixe brut et la zone de
conflit sont des homothéties. Leur cardinal attendu est de l'ordre de
`rho*D^3`. Si une famille laisse survivre des ancres
`D=Theta((n/rho)^(1/3))`, alors `kept=Theta(n)` est possible. Le cap
`kKeptCap=2048` n'est pas une saturation mathématique : son dépassement produit
un overflow/refus.

Sous un PPP stationnaire et le **vrai** filtre par spindle, la survie d'une
longue ancre possède une queue exponentielle en `rho*D^3`, à un facteur
polynomial près. Une moyenne conditionnelle peut donc converger, sans que le
high-water converge vers une constante ; une croissance logarithmique n'est
qu'une enveloppe heuristique tant que la loi conditionnelle et le mélange des
longueurs ne sont pas démontrés. Trois tailles et l'exposant ajusté `0,46` ne
distinguent ni ce régime, ni les bords, ni l'incomplétude du front médian.

La gate publie `mean/p50/p95/p99/max kept`, bucketés par `rho*D^3`, ainsi que
`overflow_kept=0` et la marge au cap. Sous le modèle Poisson, elle compare
également `kept/(rho*D^3)` à la constante de volume de la zone de conflit ; le
maximum n'est jamais interprété comme une moyenne ou une saturation.

Le résultat industriel visé est simple : le pré-list spindle-node réduit
d'abord les ancres longues ; la cutting transporte ensuite `A_K/C_K`. Elle ne
supprime les rescans que si `sum_conflicts_at_census` et `W_census` ferment
leurs pentes et caps ; des lignes presque concurrentes peuvent garder de grands
conflits. Aucun gain n'est acquis avant ce ledger.

## 5. Réponse Q4 — une contre-fixture, pas une porte qui exige le défaut produit

La famille doit rester permanente, mais la porte actuelle détourne
`--min-prunes=1` pour exiger que le chemin échoue. Elle deviendra rouge lors
d'une amélioration saine et bloquera alors le produit pour la mauvaise raison.

Il faut deux portes distinctes :

1. une porte **historique/diagnostique** nommée, par exemple
   `midball_legacy_eight_clusters`, qui pince le mode `midball-only`, la famille,
   le hash d'entrée et ses compteurs réellement observés ; une évolution du
   diagnostic met son reçu à jour, elle ne constitue pas un échec produit ;
2. une porte produit positive `spindle_prelist_eight_clusters`, avec planchers
   `prelist_spindle_node_prunes>0` et `pair_mass_closed>0`, identité de masse,
   accord à l'oracle, caps de visites et high-water.

Les compteurs doivent nommer leur certificat : `midball_prunes`,
`spindle_node_prunes`, `cutting_patch_deaths`. Un compteur générique ne doit
pas mélanger des univers différents. De même, `front_mass_closed` compte
actuellement tous les points d'un nœud, y compris les identifiants `<=a`; ce
n'est pas la masse de `PairId` non ordonnés et il doit être renommé ou corrigé.

## 6. Ordre de reprise vers 50 k

1. graver la divergence des colonnes de la note comme fixture de provenance ;
2. construire le juge indépendant borné `(S,I_B,U_B,owner)` ;
3. recevoir `spindle-node-only` comme oracle sur petites ancres contre un scan
   ponctuel, puis relever le certificat sur `A times B times C` avant PairId ;
4. lancer `front-only` sur `eight_clusters`, puis `uniform`, avec ledger de
   masse et pentes/caps ;
5. seulement si le front pré-liste est vert, recevoir la cutting signée et les
   niveaux q4 contre le juge ;
6. remplacer le rescan census par le replay `A_K/C_K`, puis mesurer séparément
   `W_census`, `J_pos` et `H_out` ;
7. CUDA/G4 reste suspendu jusqu'à ces fermetures CPU et au payload officiel.

Le résultat `theta` retire un coût inutile ; il ne change pas ce séquencement.
Le contrat `50 000/1 s` reste entièrement ouvert.

## 7. Rejeu des portes au pin

La configuration Release inventorie `560` CTests, dont `43` préfixés
`mhgp3v_anchor_`. Sur l'ELF `114be24e...` attaché aux sources du commit, la
commande
`ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_anchor_'` rend
`43/43` en `141,39 s`. Ce vert reçoit les portes locales existantes, notamment
theta et les petits `eight_clusters`; il ne reçoit ni l'oracle indépendant, ni
le classifieur spindle proposé ici, ni CUDA/G4, ni le payload officiel.

GCP non utilisé.
