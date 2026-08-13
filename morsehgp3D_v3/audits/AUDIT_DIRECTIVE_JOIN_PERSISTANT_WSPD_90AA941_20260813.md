# Directive de déblocage — WSPD terminale, masque central et join persistant

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit s'adresse à Claude. Il ne modifie aucun code, ne reçoit aucun claim
produit et n'emploie pas GCP.

## 1. Verdict au pin `90aa941`

Le pin observé est
`90aa941b46bab4927c36947f1faecab761773236`, commit
`put both probes on the machine, and measure the front that is actually
candidate`.

Le nouveau `wspd_front` contient la première bonne ossature de la route :

- à `leaf=1`, la récursion self/cross partitionne inductivement toutes les
  paires ;
- le prédicat de séparation L-infini est entier et conforme à la convention
  doublée annoncée ;
- les positions coïncidentes sont refusées avant calcul ;
- la géométrie scientifique n'est appelée qu'après matérialisation des
  terminaux ;
- le petit oracle exige déjà une occurrence de chaque paire attendue.

Cette ossature doit être conservée. En revanche, la passe témoin courante ne
mesure pas encore le candidat industriel. Chaque terminal recrée une file à
`C=root`, appelle jusqu'à trois fois le classifieur scalaire et compte ces trois
appels comme une seule évaluation. Les deux seuls CTests WSPD quittent avant
cette passe. Une session G4 de cet état mesure donc le cardinal de la
partition, puis une ancienne DFS sous-comptée ; elle ne mesure pas un masque
commun ni un join persistant.

## 2. Réparations P0 avant d'interpréter une rampe

### 2.1 Fermer le domaine du front

Le CLI accepte `leaf>1`, mais un self-bloc feuille de taille supérieure à un
est omis. La première version reçue doit soit imposer `leaf=1` avant tout
calcul, soit posséder une ABI distincte qui développe canoniquement les paires
intra-feuille. Accepter le paramètre puis compter sur le ledger final pour
échouer n'est pas un contrat.

L'oracle doit en plus :

- rejeter toute clé diagonale `p==q` ;
- rejeter toute clé supplémentaire ou hors domaine ;
- exiger `nombre_de_cles=C(n,2)` puis multiplicité exactement un ;
- survivre à une permutation qui conserve les vrais `PointId` ;
- tuer séparément une omission et un doublon de masses compensables.

Les indices de vecteur `(ia,ib)` ne sont pas des identités persistantes. Le
tape doit porter `TreeEpoch`, `ANodeKey`, `BNodeKey`, un owner d'orientation et
`RectId=digest(epoch,min(AKey,BKey),max(AKey,BKey))`. Les ties de l'arbre et de
toute file finissent par `PointId/NodeKey`, jamais par l'ordre d'insertion.

### 2.2 Construire un vrai masque central

Le calcul commun demandé est une API propre, et non une boucle autour de trois
appels à `rect_classify`.

Pour un rectangle `A×B`, calculer une seule fois `Dlo`. Pour chaque nœud ou ID
témoin `C`, calculer une seule fois `Vhi`, puis rendre :

```text
mask = 0 si Dlo <= 0
bit q2 si Vhi < Dlo
bit q3 si 3*Vhi < Dlo
bit q4 si 209*Vhi <= 56*Dlo
```

Les bits sont imbriqués `q4=>q3=>q2`. Cette passe n'appelle ni
`rect_h_interval`, ni le fallback `E2max*X2max`, ni un produit vectoriel. Un
échec de bit vaut `UNKNOWN`, jamais `NONE`, `KEEP`, `POSITIVE` ou
`SOURCE_EMPTY`.

Sous u16, `Dlo`, `Vhi` et leurs coefficients tiennent en u64. Le P0 doit donc
publier `wide_products=0`. Le fallback plus large est une passe séparée et son
gain marginal doit justifier son coût.

## 3. Le test de viabilité qui évite une descente témoin inutile

Il est possible de savoir en temps constant si **aucun point du réseau u16** ne
peut passer le masque central tant que `A` et `B` restent inchangés.

Pour chaque axe, poser :

```text
slo = A.lo + B.lo
shi = A.hi + B.hi
g_i(z) = max(abs(2*z-slo), abs(2*z-shi))^2
```

Alors `Vhi(z)=g_0(z_0)+g_1(z_1)+g_2(z_2)`. Chaque `g_i` est convexe sur les
entiers. Son minimum u16 est atteint parmi les entiers floor/ceil voisins de
`(slo+shi)/4`, écrêtés au domaine. Huit combinaisons suffisent donc pour
obtenir exactement `Vbest=min_z Vhi(z)`.

Conséquences exactes :

```text
si Vbest >= Dlo, aucun singleton C ne passe le cœur q2
si 3*Vbest >= Dlo, aucun singleton C ne passe le cœur q3
si 209*Vbest > 56*Dlo, aucun singleton C ne passe le cœur q4
```

Ces refus ne sont pas des `NONE` du spindle : ils disent seulement que
raffiner `C` est inutile pour **ce certificat central**. Le scheduler doit
alors raffiner `A/B`, essayer un autre certificateur ou déléguer. Cette porte
évite de descendre l'arbre témoin quand les boîtes endpoint sont simplement
trop larges.

Le minimiseur donne aussi le meilleur centre de proposition. Une fenêtre
Morton autour de ce centre est un tier très bon marché ; elle n'a cependant
aucune garantie de rappel spatial et ne doit jamais être le seul producteur
présenté comme « la solution ».

## 4. La solution architecturale : raffinement local et héritage des crédits

Augmenter globalement la séparation WSPD de `s=2` à `s=12` multiplie la
constante cubique relative par `(12/2)^3=216` et ne garantit aucun témoin. La
bonne opération est un raffinement **local** des seuls rectangles encore
ouverts.

État minimal :

```text
RelationState {
  RectId, ANodeKey, BNodeKey, open_mask,
  credit_count[3], proof_ids[<=10],
  mixed_c_span, scheduler_epoch
}
```

Les propriétés monotones donnent la reprise exacte :

- un nœud `C` qui était `ALL` pour `A×B` reste `ALL` pour tout sous-rectangle ;
- un nœud `C` réellement `NONE` pour tout le produit reste `NONE` après
  restriction de `A/B` ;
- seuls les nœuds `MIXED` sont reclassifiés ;
- les `proof_ids` restent valides, distincts et hérités ;
- une scission de `A` ou `B` ne réintroduit jamais la racine du front
  géométrique déjà classifié.

Cette dernière propriété ne permet pas de jeter les endpoints relatifs. Après
`A=A0∪A1`, un ID de `A1`, interdit comme endpoint pour le parent, peut devenir
un témoin valide de `A0×B`. Le state conserve donc les propositions rejetées
pour ce seul motif dans un sidecar `endpoint_blocked`, à filtrer de nouveau
pour chaque enfant, ou rejoue le producteur borné sur l'enfant. Cette reprise
n'invalide aucun crédit positif et ne redémarre pas la classification
géométrique complète depuis `C=root`.

Le scheduler compare en entier les marges des enfants :

```text
M2 = Dlo - Vhi
M3 = Dlo - 3*Vhi
M4 = 56*Dlo - 209*Vhi
```

Il choisit canoniquement la scission `A`, `B` ou `C` qui améliore le plus les
bits encore ouverts par octet créé. Si `Vbest` interdit un bit, une scission
`C` n'est pas candidate. Une égalité, un quantum épuisé ou un manque de rappel
rend un état `DELEGATED_RESIDUAL` complet. La vérité scientifique ne dépend
jamais de cet heuristic.

Sur GPU, cet état devient une wavefront SoA globale :

1. `count` des enfants et des résultats ;
2. scan stable ;
3. `fill` par `RectId/NodeKey` ;
4. suppression des bits saturés `10/9/8` ;
5. itération ou handoff.

Il ne faut ni `std::priority_queue` par rectangle, ni scratch maximal par
thread, ni copie d'une antichaîne entière à chaque enfant. Les spans vivent
dans une arène immuable et les enfants portent des deltas ou des références
partagées.

## 5. Producteurs de témoins à comparer, pas à confondre

Le même ABI permet une ablation honnête sur les records ouverts :

1. `MortonWindow` : `W=32/L=16`, tier 0 à coût strictement borné ;
2. `OctreeReport` : range-report/best-first batché, saturé à douze IDs, avec
   cap fail-open ;
3. `OrderCorridor` : requêtes orthogonales dans les chambres unimodulaires,
   seulement si son gain par octet dépasse les deux premiers.

Chaque ID proposé est recertifié par le masque central sur **tout** le
rectangle et dédupliqué par `(RectId,PointId,lane)`. Une banque vide ou capée
reste déléguée. Pour q4/q3/q2, les seuils sont respectivement huit, neuf et dix.
Les huit IDs q4 sont réutilisés pour q3/q2 ; un neuvième n'est cherché que pour
q3, puis un dixième pour q2.

`MortonWindow` seule ne mérite donc pas une promotion architecturale. Sa
discontinuité peut manquer un amas placé à une case voisine. Elle reste utile
si elle ferme assez de rectangles avant la wavefront octree. La bonne métrique
est `fermetures supplémentaires / (IDs lus + nœuds visités + octets)`, par
famille et par lane.

## 6. Ce qui vient après le filtre central

Un record q3 délégué doit rechercher exhaustivement les carriers d'une arête
owner. Pour une paire concrète `a,b`, un point `x` est carrier aigu exactement
dans la sur-approximation :

```text
H=(x-a) dot (b-x) < 0
||x-a||^2 <= ||b-a||^2
||b-x||^2 <= ||b-a||^2
```

Pour q4, conserver toute la lentille et apparier deux points avec
`acute(x)||acute(y)` ; un seul des deux doit être aigu. Viennent ensuite
`||x-y||^2<=||b-a||^2`, rang affine trois, positivité stricte, owner, census et
shell. Le flux témoin du census reste le nuage complet.

Ni l'échec du cœur, ni un corridor vide, ni un cap ne prouvent
`SOURCE_EMPTY`. Cette issue exige une couverture exhaustive de la relation
carrier. Aucun de ces étages ne matérialise une mosaïque de Delaunay d'ordre
supérieur : seulement arbre de points, relations factorisées, carriers locaux
et arrangements shallow temporaires.

## 7. Gates exactes du prochain commit

Avant une interprétation G4, le binaire WSPD doit avoir au moins un CTest
non-oracle qui traverse la passe 2. Les portes minimales sont :

- `classified_terminals == terminal_records` et
  `classified_internal_records == 0` ;
- par lane, `closed_records+delegated_records=terminal_records` et
  `closed_mass+delegated_mass=C(n,2)` ;
- `Dlo_calls=terminal_records`, jamais trois fois ce nombre ;
- `Vhi_calls`, `H_calls=0` pour le P0 central, IDs lus et octets explicites ;
- `planned=filled=consumed`, `repeated_task=0`, HWM et digest du tape ;
- invariant final indépendant du quantum, du nombre de threads et de la
  permutation d'entrée conservant les `PointId`.

Fixtures centrales :

```text
A=B=C={(0,0,0)}                         -> aucun bit
a=(0,4,0), b=(12,16,0), z=(8,8,4)      -> frontière q3, bit q3 absent
a=(0,0,0), b=(2,1,1), z=(1,0,0)        -> frontière q4, bit q4 absent
a=(0,0,0), b=(24,16,2), z=(18,12,3)    -> égalité 209*V2=56*D2 acceptée
a=(0,0,0), b=(0,0,28), z=(2,7,14)      -> tue le mutant 56 vers 57
a=(0,0,0), b=(10,0,0), z=(1,0,0)       -> échec central mais q3/q4 vrais : UNKNOWN
```

Le juge borné développe exhaustivement les points réels de petits nœuds et
évalue directement `H`, `E2`, `X2` en BigInt ou largeur reçue. Quatre tirages
avec remise par nœud sont un fuzz utile, pas une preuve d'universalité.

La rampe physique est `12500/25000/50000` sur `uniform` et
`eight_clusters`, avec deux pentes consécutives au plus `1,35` pour : tape
WSPD, tâches jointes, appels `Dlo/Vhi`, IDs lus, nœuds octree, octets et HWM.
La masse déléguée est un ledger sémantique, pas une pente de travail.

## 8. Ordre de travail remis à Claude

1. Recevoir la partition WSPD à `leaf=1`, son identité et son oracle renforcé.
2. Remplacer les trois classifieurs par `central_mask(Dlo,Vhi)` et ajouter un
   CTest qui exécute réellement cette passe.
3. Ajouter `Vbest` et publier combien de records imposent un split `A/B` avant
   toute recherche témoin.
4. Émettre le tape SoA et une wavefront `MortonWindow` strictement
   propositionnelle.
5. Ajouter le raffinement local avec héritage des crédits, sans retour à
   `C=root`.
6. Mesurer CPU puis device les compteurs physiques ; garder ou supprimer la
   fenêtre Morton selon son gain marginal.
7. Ajouter `OctreeReport`, puis éventuellement le corridor, uniquement sur les
   résidus.
8. Raccorder les carriers et le shallow q4 seulement après réception de cette
   tranche.

Le p95 `<=200 ms` d'une tranche résidente peut servir de seuil d'ingénierie ;
il n'est pas le contrat. Le seul verdict produit reste le `warm_e2e` complet,
avec construction, transferts, source, census, fold, dix forêts, verticales et
certificat minimal. Le statut `50000` sous une seconde reste `NO-GO`.

## 9. Rejeu local du pin

La cible WSPD a été construite en Release et les deux portes existantes rendent
`2/2 PASS`. Elles ne couvrent que l'oracle de multiplicité et le refus
`n>64` ; toutes deux quittent avant la passe géométrique.

Un diagnostic local `n=2000`, quantum `64`, confirme le compromis à mesurer :

| famille | séparation | front/point | masse q3 fermée | masse q4 fermée | nœuds C comptés |
| --- | ---: | ---: | ---: | ---: | ---: |
| uniforme | 1 | 10,768 | 0,00 % | 0,00 % | 1 269 545 |
| uniforme | 2 | 20,537 | 0,00 % | 0,00 % | 2 434 938 |
| uniforme | 4 | 60,019 | 3,06 % | 1,62 % | 7 299 412 |
| huit amas | 2 | 12,762 | 0,00 % | 0,00 % | 1 419 083 |
| terrain | 2 | 7,388 | 0,01 % | 0,01 % | 759 983 |
| terrain | 4 | 17,767 | 15,01 % | 8,81 % | 1 922 776 |

Ces nombres sous-comptent les appels réels jusqu'à un facteur trois, mais ils
suffisent à trancher : augmenter globalement `s` achète peu de q3/q4 et gonfle
fortement le front. La voie à poursuivre est `s` faible plus raffinement local
persistant, pas un grand `s` ni une nouvelle DFS par terminal.

GCP non utilisé par cet audit.
