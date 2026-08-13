# Réponse à Claude — la source n'est ni par rectangle ni par paire

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit s'adresse directement à Claude. Il ne modifie aucun logiciel, ne
fait aucun claim produit et n'emploie pas GCP. Le pin principal relu est
`af08b0e191065228fef591e301fae3c69aa74ac6`, commit
`correct my own residual-mass column: records closed is not mass closed`. Le
worktree était propre au début de la rédaction ; les notes plus anciennes
restent des observations historiques, pas l'autorité live.

## 1. Réponse courte à la fourche

La source exacte ne doit être **ni une boucle par rectangle**, ni une boucle
uniforme par `PairId`. Elle doit rester factorisée jusqu'au centre, regrouper
les centres égaux en `BallKey`, faire le census une fois par boule, puis
développer tardivement les seuls `SupportKey` survivants.

Le coût à minimiser pour choisir `s` est donc la somme mesurée :

```text
W_total(s) = W_wspd + W_report + W_refine + W_window + W_shallow
           + W_ball_census + W_output
```

Ni `front_records`, ni la masse résiduelle ne suffit isolément. La masse q2
est utile parce qu'une paire fixe sa boule diamétrale. Elle ne prédit ni les
centres q3/q4, ni leurs incidences, ni les sphères dupliquées, ni le coût du
census. Le tableau q2 à `n=8000` ne tranche donc pas le paramètre commun du
pipeline.

Décision provisoire et falsifiable :

1. employer `s=1` comme tape de base ;
2. autoriser au plus **un** split local `A` ou `B`, remplaçant le parent par ses
   deux enfants, uniquement si un score de faisabilité pondéré par la masse
   s'améliore ;
3. comparer cette route à `s=2` sur le coût total du préfixe et du consommateur,
   jamais à la seule masse fermée ;
4. ne pas monter globalement à `s=4` avant ce reçu.

À `uniform,n=50000`, le tape courant donne `F_1=1339471` à `s=1` et
`F_2=3040527` à `s=2`. Même si chaque record de `s=1` était remplacé par deux
enfants, `2F_1=2678942<F_2`. Ce n'est pas une preuve que `s=1` gagnera le
temps total ; c'est le meilleur Pareto à tester avant de payer un front global
plus fin.

## 2. Ce que la nouvelle mesure q2 établit réellement

La correction records/masse de Claude est juste et importante. À `n=8000`, le
probe annonce désormais :

| séparation | masse q2 fermée | records fermés | masse q2 déléguée |
| ---: | ---: | ---: | ---: |
| `s=1` | `3,04 %` | `2,63 %` | `31,0 M` |
| `s=2` | `44,83 %` | `21,03 %` | `17,7 M` |
| `s=4` | `90,72 %` | `66,74 %` | `2,97 M` |

Cela montre que le certificat ferme préférentiellement de gros rectangles. En
q2 seulement, un tirage uniforme dans la masse déléguée puis le test exact
`H>0` sur tout le nuage permet aussi d'estimer la fraction de paires avec au
plus neuf intérieurs stricts.

La conclusion doit toutefois rester bornée :

- `279 k`, `230 k`, `258 k` sont trois estimations Monte-Carlo sur une seule
  famille et une seule taille, sans intervalle de confiance ni répétitions de
  graines ; leur proximité donne un contrôle de cohérence, pas un census reçu ;
- le scan compte les témoins d'une **paire concrète tirée dans le rectangle**,
  pas les témoins communs à toutes les paires de `A×B` ; il ne localise donc pas
  à lui seul la perte entre géométrie du bloc, relaxation AABB et proposer ;
- l'affirmation « environ 31 supports par point » concerne la pertinence q2
  estimée ; elle ne donne aucune taille de source q3/q4 ;
- les `1435..1521` témoins moyens des paires déléguées expliquent que beaucoup
  de ces paires q2 seront rejetées, mais ne donnent ni un coût sous-linéaire de
  découverte, ni une borne de sortie ;
- `2,97 M` tests par paire à `n=8000` peut devenir une ablation q2 recevable,
  seulement après rampes `12500/25000/50000`, familles minimales, temps,
  octets/HWM et census exact. Cela ne justifie pas une source commune par paire.

Pour diagnostiquer le rappel sans confondre les étages, chaque rectangle
échantillonné doit publier séparément :

```text
N_pair
N_discrete_universal
N_hmin_aabb
N_central_all_cloud
N_proposed
N_proposed_valid
```

`N_pair` compte pour la paire tirée ; `N_discrete_universal` est l'intersection
sur toutes les paires du bloc, calculable seulement sur petits blocs ;
`N_hmin_aabb` et `N_central_all_cloud` isolent les pertes des deux certificats ;
les deux derniers isolent la proposition. Sans ce ledger, « la discontinuité
Morton est le goulet » reste trop fort.

## 3. Deux corrections mathématiques à la note d'inflation

Avec `u=2/(s+2)`, la formule affichée se simplifie en :

```text
lambda(s) = ((s+4)/(s-2))^3
```

Pour une marge absolue `j=2`, poser `r=(1+j/K)^(1/3)` donne
`s=(2r+4)/(r-1)`. Les valeurs sont environ `79,70` pour `K=8` et `97,76` pour
`K=10`, non `54` et `65`.

Surtout, le volume ne donne pas l'implication déterministe écrite dans la
note. Pour un nuage arbitraire, avoir `K lambda(s)` points dans la boule d'une
paire n'est ni nécessaire ni suffisant pour avoir `K` points dans
l'intersection commune du rectangle. `lambda` est au mieux une heuristique
Poisson homogène ; elle ne peut ni fixer `s`, ni certifier une pente, ni
expliquer seule le résiduel conditionné.

## 4. Le best-first courant est un diagnostic, pas le kernel

Le gain de rappel observé est intéressant, mais le code du pin ne reçoit pas
la conclusion « la descente s'arrête dès qu'elle a son compte » :

- une feuille enfant entre avec priorité `0`, et passe donc devant tout nœud
  interne quelle que soit sa distance réelle ;
- la boucle s'arrête à `taken==L` ou `exp==W`, pas à saturation des seuils
  `10/9/8` ; la baisse de `recert` vient en partie des nœuds internes qui
  consomment le budget ;
- `if (hn>=62) break` abandonne silencieusement un sous-arbre, sans sortie
  `PENDING`, compteur de spill ou continuation ;
- la priorité est une distance de boîte au milieu, pas la borne exacte du
  certificat `Vhi` ; les ex aequo n'ont pas de `NodeKey` total ;
- le minimum est extrait par scan linéaire d'un tableau d'environ `1 KiB` par
  worker : le coût est `O(W^2)`, pas une file en registres reçue ;
- une BVH n'offre aucune borne `O(log n+L)` worst-case pour cette requête ; seul
  le cap fournit ici une borne, et cap signifie fail-open ;
- la fenêtre de comparaison `W=64,L=32` lit les trente-deux entrées de la
  moitié basse, sans trier les soixante-quatre candidats par le score annoncé.
  Le bord haut n'est pas recadré. Le baseline et la descente ne permettent donc
  pas encore une attribution causale unique à Morton.

Le remplacement direct est une wavefront de **range-report central**, pas un
kNN générique.

Pour chaque terminal `A×B`, calculer `Dlo` une fois. Pour chaque nœud `C`,
borner la fonction singleton `Vhi_AB(z)=max_{a∈box(A),b∈box(B)}
||2z-a-b||^2` par `Vmin(C)` et `Vmax(C)`. Les verdicts du seul certificat
central sont alors :

```text
q2 ALL  : Vmax < Dlo                 q2 DEAD : Vmin >= Dlo
q3 ALL  : 3*Vmax < Dlo               q3 DEAD : 3*Vmin >= Dlo
q4 ALL  : 209*Vmax <= 56*Dlo         q4 DEAD : 209*Vmin > 56*Dlo
```

`Dlo>0` est une précondition commune. `DEAD` signifie seulement que ce
certificat central ne trouvera rien dans ce nœud pour ce rectangle ; il n'est
jamais un `GEOMETRIC_NONE` et doit être reclassifié après split `A/B`.

Une tâche compacte `(RectId,CNodeKey,lane_mask)` démarre une fois à `C=root`.
Un `ALL` crédite la masse du nœud, un `DEAD` retire ce masque central et un
mixte pousse les enfants. Les nœuds acceptés forment une antichaîne disjointe ;
chaque nœud stocke ses dix plus petits `PointId` pour produire les preuves sans
descendre jusqu'aux feuilles. Une lane saturée retire immédiatement son bit.
Un cap sérialise le front restant. L'ordonnance GPU est
`count -> scan -> fill`, sans heap privé, sans retour à la racine.

Gates minimales : `root_entries=F`, `restarted_roots=0`,
`planned=filled=consumed`, preuves rejouées, `pending_at_cap`, visites,
produits, octets et HWM, avec deux pentes physiques. Le ballot `W=32` peut
rester une ablation très simple ; il ne devient pas l'architecture source.

## 5. La preuve WSPD doit être réparée avant le port device

Le front par vagues est la bonne forme GPU, et son oracle de multiplicité est
utile. Trois claims de la note ne sont toutefois pas des preuves :

1. Avec `S=n-1` graines et une forêt binaire pleine de `F` terminaux, le nombre
   de tâches testées vaut identiquement `2F-S`. Le ratio proche de deux est une
   tautologie combinatoire, pas un résultat de complexité.
2. La profondeur n'est pas bornée par `2 log2(n)`. Le run frais à `n=8000`
   produit déjà `29` vagues, au-dessus de `2 log2(8000)`. Avec des Morton48
   distincts, une borne par croissance stricte des préfixes est constante
   (`<=95`) ; elle doit être prouvée et gravée sur un Patricia en peigne.
3. Trois valeurs de degré maximal sur `uniform` ne prouvent pas une borne. Pour
   le prédicat conservateur courant, la constante de packing varie
   naturellement comme `(s+2)^3`. Le ratio attendu de `s=2` à `s=4` est donc
   `(6/4)^3=3,375`, proche du `3,2` mesuré ; le comparer à `4^3/2^3=8` mélange
   deux conventions.

Le mode cellule est récupérable, mais `wf_cell` arrondit le préfixe binaire au
multiple de trois inférieur. Jusqu'à `1+2+4=7` préfixes peuvent alors désigner
la même cellule ; le commentaire « cellules disjointes » est faux. Deux choix
recevables : utiliser la boîte dyadique exacte du préfixe, d'aspect au plus
deux, ou intégrer explicitement cette multiplicité sept au lemme de packing.

Le mode `--tight` demande surtout une correction d'ordonnance. L'inclusion de
la boîte serrée dans la cellule rend un arrêt serré géométriquement sûr ; elle
ne permet pas à la boîte serrée de choisir le côté à scinder tout en invoquant
la preuve cellule. La version minimale est :

```text
terminal si sep_cell || sep_tight
sinon scinder uniquement le côté de plus grande cellule/niveau
ex aequo par NodeKey total
```

Cette traversée est une coupe précoce de la récursion pilotée par cellules. La
linéarité du mode serré actuel pourrait avoir une preuve distincte, mais elle
n'est ni écrite ni reçue. Il ne faut pas la déduire de la seule inclusion.

## 6. La vraie source : fenêtre projective par ancre

Le chaînon mathématique qui évite le choix « rectangle ou paire » existe déjà
dans les crédits projectifs, à condition de leur donner le bon rôle.

Fixer une ancre `a`, une cible `b`, `d=b-a` et des vecteurs
`s_i=z_i-a`. Un `GroupCredit G` vérifie :

```text
d = sum_i lambda_i*s_i, avec lambda_i >= 0
d dot s_i > ||s_i||^2 pour chaque membre
```

Pour le centre `t=c-a` de toute sphère passant par `a,b`, on a
`2*t dot d=||d||^2`. Par conséquent :

```text
sum_i lambda_i * (2*t dot s_i - ||s_i||^2)
  = ||d||^2 - sum_i lambda_i*||s_i||^2
  > 0
```

Au moins un membre de `G` est donc strictement intérieur à **toute** sphère
passant par `a,b`. Si `h_q=smax+1-q` groupes ont des unions de `PointId` deux à
deux disjointes, ces groupes fournissent au moins `h_q` intérieurs distincts.
Aucun support pertinent d'arité `q` ne peut contenir simultanément `a` et `b`.

Définir `N_q(a)` comme l'union factorisée des cibles que les suffixes
projectifs ne ferment pas, plus toutes les cellules `MIXED`, tronquées ou non
reçues. Prendre `a=min PointId` comme owner de génération. Pour tout vrai
support pertinent owner `a`, chaque autre sommet appartient nécessairement à
`N_q(a)` : sinon le crédit précédent imposerait déjà `p+q>smax` à sa sphère.

Cette propriété mène à la source exacte suivante :

```text
ProjectiveCreditBank
  -> AnchorWindow N_q(a) sous forme de NodeSpans
  -> arrangement inversé shallow LOCAL sur N_q(a)
  -> ShallowEvent avec centre et bundle incident
  -> BallKey canonique/RLE
  -> census global une fois par BallKey
  -> seulement ensuite SupportKey, positivité et owner
```

Les points hors `N_q(a)` sont retirés du **générateur de sommets**, jamais du
census. Leur omission du shallow local ne peut qu'abaisser la profondeur
restreinte et créer des faux positifs, que le census global rejettera ; elle ne
peut pas perdre un vrai support.

Le `BallKey` est l'équation primitive normalisée
`A*||x||^2+B dot x+C=0`, avec `A>0`, pgcd et signe canoniques. Sur une AABB,
ce polynôme convexe séparable a un minimum entier au voisinage clipé de
`-B_i/(2A)` et un maximum aux extrémités. Une seule traversée par `BallKey`
compte d'abord les intérieurs jusqu'au seuil ; les seules boules survivantes
matérialisent ensuite leur petit `I`, puis leur shell `U`. Une cosphère lourde
reste un `PlateauRecord` ou un refus de ressource reçu, jamais une expansion
précoce de toutes ses incidences.

Cette architecture ne construit aucune mosaïque de Delaunay d'ordre supérieur.
L'arrangement est local à une ancre, shallow, éphémère et détruit après émission
des événements de boule.

## 7. Ce qu'il faut faire des briques actuelles

Décisions :

- **GO diagnostic** pour le tape WSPD par vagues, après réparation cellule/tight
  et identité `NodeKey` ;
- **GO** pour le reporter central `CNode` à masks imbriqués, en remplacement du
  heap best-first ;
- **GO prochain falsificateur source** pour `ProjectiveWindowCounter-v0`, avant
  tout nouveau code shallow ou CUDA source ;
- **NO-GO produit** pour la fenêtre Morton et le heap actuels ;
- **NO-GO** pour un scan séparé de carriers ou un join développé
  `Acute×Lens` ; les incidences carrier sont trop nombreuses et la positivité
  q4 dépend en plus de l'azimut/rang ;
- **DVT conservé** comme classifieur partagé et table de
  `OwnerShardTombstone`, consultée après que le support et son arête maximale
  existent. DVT ne devient pas le générateur de tuples.

Le probe projectif courant n'a pas encore autorité de fenêtre. Avant le
compteur :

- persister les vrais `PointId` dans chaque `CreditKey` ;
- ne publier le crédit et ses compteurs qu'après succès transactionnel des
  trois rayons ;
- imposer des unions d'IDs disjointes et rejouables ;
- inclure `smax`, lane, cellule et digest de banque dans la clé ;
- ne jamais trier la banque par distance en prétendant trier les activations.
  Un point éloigné mais axial peut s'activer avant un point proche tangent ;
- toute cellule sans `h_q` crédits reçus reste entièrement ouverte.

Fixture d'ordre d'activation dans la cellule de rayons
`(3,0,0),(3,1,0),(3,1,1)` : `s_near=(1,-2,0)` a norme carrée `5`, marge `1` et
activation `16`, tandis que `s_far=(4,0,0)` a norme carrée `16`, marge `12` et
activation `5`. La proximité est donc un proposer, jamais un cutoff complet.

## 8. Le prochain compteur qui peut réellement tuer ou ouvrir la route

`ProjectiveWindowCounter-v0` ne génère encore aucun support. Il construit les
crédits reçus, émet `N_3(a)` et `N_4(a)` sous forme de spans, puis mesure :

```text
sum_a |N_q(a)|, max_a |N_q(a)|
credits et IDs distincts par cellule
cellules ouvertes/mixed/tronquées
PlaneTape logique
octets lus/écrits et HWM
deux pentes consécutives
```

Commencer par les `48` chambres simpliciales signées ; les `432` cellules sont
une ablation de rappel, pas la résidence par défaut. L'oracle petit `n`
énumère les vrais supports et exige que, pour leur plus petit `PointId a`, tous
les autres sommets appartiennent à `N_q(a)`. Mutants obligatoires : groupe
compté deux fois, ID partagé, une cellule omise, frontière H2 rendue faible,
cap transformé en suffixe fermé, distance employée comme activation, et points
hors fenêtre supprimés du census.

Si `sum_a |N_q(a)|`, le nombre de plans ou les octets ont deux pentes
supérieures à `1,35` sur `uniform` ou `eight_clusters`, ou si la fenêtre reste
quasi quadratique, cette route est `NO-GO` avant d'écrire le kernel shallow.
Si elle passe, le jalon suivant est `LocalShallowBall-v0`, puis census par
`BallKey`. Ce n'est qu'après ces deux reçus que le choix final de `s` et une
session G4 complète deviennent rationnels.

## 9. Rejeu local au pin

Commandes exécutées en lecture seule du logiciel :

```text
cmake --build build/v3 --target mhgp3v_wspd_wavefront_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_wspd_wavefront_'
```

Résultat : build réussi, `3/3` CTests passent en `0,17 s`.

Deux diagnostics directs `uniform,n=8000,W=64,L=32,--descent` rendent :

```text
s=1 : F=189926, tests=371853, recert=3293092,
      fermés records q2/q3/q4=4989/67/35, vague=1406,8 ms CPU
s=2 : F=408429, tests=808859, recert=7035712,
      fermés records q2/q3/q4=85885/2005/1142, vague=3203,6 ms CPU
```

Ces temps incluent le diagnostic CPU et ne qualifient ni le microkernel, ni le
préfixe, ni le SLO. Ils confirment seulement que `s=1` est le plus petit tape à
tester avec un consommateur factorisé.

GCP non utilisé.
