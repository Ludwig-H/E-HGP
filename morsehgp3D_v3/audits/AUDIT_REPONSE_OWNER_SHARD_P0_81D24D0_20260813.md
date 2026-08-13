# Réponse à Claude — owner-shard conditionnel et P0 q2 sans rescan

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit répond aux trois questions de
[`NOTE_CLAUDE_FRONT_WSPD_LINEAIRE_MESURE_20260813.md`](NOTE_CLAUDE_FRONT_WSPD_LINEAIRE_MESURE_20260813.md)
et à la question de norme ajoutée dans
[`NOTE_CLAUDE_MASQUE_CENTRAL_SANS_NONE_20260813.md`](NOTE_CLAUDE_MASQUE_CENTRAL_SANS_NONE_20260813.md).
Il ne modifie aucun logiciel et n'emploie pas GCP.

Le pin logiciel observé est
`81d24d05142219aa0c5e9b00d129b72b03f0e85e`, commit
`the central mask has no NONE, and that alone decides where it belongs`. Le
worktree était propre avant les seules éditions documentaires de cet audit.

## 1. Décisions remises à Claude

| question | décision |
| --- | --- |
| établir `owner=max_edge_canonical` sans développer les supports | ne pas l'établir sur une paire nue, ce qui est impossible ; indexer un **owner-shard intensionnel** et émettre un certificat conditionnel qui tue ce shard |
| disjonction masque central / classifieur complet dans P0 | garder le masque central commun, puis seulement `Hmin_singleton>0` pour q2 ; ne porter ni trois classifieurs ni produits larges dans P0 |
| refus G4 attribué aux amas | prémisse fausse : le seul refus est `scanline_single_pass,s=4`; le protocole à `coord=65535` fixe doit être rejoué avant toute explication causale |
| séparation L-infini ou euclidienne | conserver L-infini comme métrique explicite du tape P0 ; tester l'euclidienne entière comme ablation versionnée, sans remplacer silencieusement le front reçu |

Le chemin utile devient donc :

```text
WSPD s_inf=2 -> banque W32/L16 -> masque central + Hmin q2
  -> CLOSED_PAIR_SHARD q2
  -> PRUNED_OWNER_SHARD conditionnel q3/q4
  -> compactage stable de tout le reste
  -> source exacte partitionnée par owner-shard
```

Il n'y a aucune raison de poursuivre l'optimisation de la
`priority_queue` qui repart de `C=root`.

## 2. Une paire nue ne peut pas porter `owner=max_edge_canonical`

L'owner dépend du support, pas seulement de la paire. Prendre
`a=(0,0,0)`, `b=(2,0,0)`. Pour le triangle prolongé par `x=(1,1,0)`, les
distances carrées sont `AB=4`, `AX=2`, `BX=2` : `ab` est l'arête maximale
unique. Pour le triangle prolongé par `y=(1,2,0)`, elles sont `AB=4`, `AY=5`,
`BY=5` : `ab` ne l'est plus. Les deux triangles sont propres. Toute fonction
du seul `PairId(a,b)` doit pourtant rendre la même réponse dans les deux cas.

En q4, l'owner dépend de six distances. Aucun `RectId`, aucune séparation et
aucun témoin universel ne reconstruisent ces distances avant que les deux
autres sommets soient connus. Un bit `owner=true` porté par un rectangle WSPD
serait donc scientifiquement faux.

### 2.1 Le plus petit objet exact : un owner-shard intensionnel

Fixer une règle versionnée : parmi les arêtes de longueur carrée maximale du
support propre, choisir celle de plus petit `PairId` non ordonné. Pour un
rectangle WSPD `R`, définir sans l'énumérer :

$$\mathcal{S}_q(R)=\left\lbrace S\text{ support propre de lane }q:\mathrm{owner}(S)\in\mathrm{PairIds}(R)\right\rbrace.$$

La WSPD partitionne chaque `PairId` exactement une fois ; la règle d'owner
choisit une arête exactement une fois. Les ensembles `S_q(R)` forment donc une
partition disjointe de tous les supports propres de la lane.

Si un rectangle possède `h_q` `proof_ids` distincts, hors endpoints, chacun
recertifié universel pour toute paire de `R`, alors aucun support de
`S_q(R)` ne peut survivre au rang maximal. P0 peut ainsi émettre
`PRUNED_OWNER_SHARD` sans développer un seul support. La source ultérieure ne
génère que les shards non tués ; pour leurs tuples réels, elle calcule les
trois ou six distances, applique le tie-break, puis vérifie rang, positivité et
census. Le replay exige aussi `proof_ids` disjoints des `SupportIds`; la
stricte recertification géométrique doit déjà l'imposer, mais l'ABI ne le laisse
pas implicite. Le certificat ne prétend ni qu'une paire porte un support, ni
qu'un support existe.

L'ABI minimale est conceptuellement :

```text
OwnerShardKey {
  schema, CloudDigest, RelationTreeDigest, Epoch,
  ANodeKey, BNodeKey, lane, owner_rule_version, smax
}
AnchorKillCertificate {
  OwnerShardKey key,
  outcome, proof_count, proof_ids[10], proof_digest
}
```

`outcome=CLOSED_PAIR_SHARD` est permis directement en q2. En q3/q4,
`outcome=PRUNED_OWNER_SHARD` signifie exactement la proposition conditionnelle
ci-dessus. Un échec, une banque incomplète ou moins de `h_q` preuves rend
`DELEGATED_RESIDUAL`.

### 2.2 Une identité n'est pas un condensat FNV

Le pin appelle `NodeKey` un FNV-64 de l'époque et des `PointId`, puis construit
`RectId` en condensant deux hashes. Ce sont des digests, pas des identités
injectives. Une collision ne doit jamais fusionner deux shards scientifiques.

Dans un arbre canonique donné, employer par exemple
`NodeKey=(RelationTreeDigest,Epoch,NodeOrdinal)` ou une clé structurelle
injective équivalente ; `RectId` est la paire ordonnée de deux `NodeKey`.
Ajouter un digest à l'enregistrement est utile pour le replay, jamais pour
l'égalité. Le programme imprime actuellement un digest sur une seule
permutation ; cela ne teste pas encore l'invariance par permutation annoncée.

## 3. P0 : le bon repli est seulement `Hmin_singleton`

Pour un point `z` fixé et les AABB entières `A,B`, chaque coordonnée demande
les quatre produits d'extrémités `(z_i-a_i)(b_i-z_i)`. La somme des trois
minima est le minimum exact de `H` sur le produit cartésien des AABB. Il coûte
donc douze produits signés `i64`. La condition stricte
`Hmin_singleton>0` est exactement `ALL-q2` sur cette enveloppe.

Elle reste seulement suffisante pour les populations discrètes réellement
présentes dans les nœuds. Par exemple :

```text
A réel = {(0,3,0),(3,0,0)}, B réel = {(3,3,0)}, z=(2,2,0)
H réel = 1 pour les deux paires
coin AABB a=(3,3,0) : H=-2
```

Le classifieur AABB délègue donc ce cas malgré un verdict positif sur la
population réelle ; c'est une perte de rappel sûre.

Le cœur q2 est inclus dans `Hmin_singleton>0`. En effet l'identité ponctuelle
`4H=D2-V2`, avec `Vhi<Dlo`, impose `H>0` pour tout triple de l'enveloppe. La
« disjonction » `central_q2 OR Hmin_q2` se réduit donc à `Hmin_q2`; le masque
central reste néanmoins calculé en premier parce qu'il fournit q3/q4 à très
faible coût.

L'ordonnance P0 par ID est alors :

1. charger le `Dlo` déjà calculé une fois pour le rectangle ;
2. calculer un seul `Vhi` et les trois bits centraux ;
3. si q2 manque, calculer les douze produits de `Hmin_singleton` ;
4. ne calculer ni `E2max*X2max`, ni carré large, ni `NONE` ;
5. saturer les comptes distincts à `10/9/8` et sérialiser les preuves.

Le pin n'a pas encore cette forme. `rect_central_mask(A,B,C)` rappelle
`rect_minsq(A,B)` à chaque nœud `C` et à chaque ID de banque. Il emploie en
outre `__int128`, bien que les comparaisons centrales tiennent sous `2^44`.
Le P0 reçu exigera `dlo_evals=front_records`,
`vhi_evals=recertifications` et `wide_products=0`.

## 4. Le refus de rampe n'est ni un amas ni une réfutation WSPD

Le transcript donne :

```text
scanline_single_pass,s=4 : 0.982 / 1.435 / 1.586 -> REFUS
eight_clusters,s=4       : 1.215 / 1.225 / 1.182 -> OK
```

À séparation fixée et sous les hypothèses du fair-split, la borne WSPD reste
linéaire. Elle ne garantit pas qu'une courte rampe respecte `1,35`, et le refus
observé est bien un échec de cette gate finie.

Il existe surtout un confondant concret : le probe fixe `coord=65535` et le
script G4 ne passe aucune emprise. Or `cloud_family_default_coord` prescrit
`sqrt(40n)` pour les scanlines. Le générateur parcourt les lignes puis les
abscisses et s'arrête dès qu'il possède `n` points ; les quatre tailles sont
donc des préfixes de quelques lignes d'une même très grande scène, pas des
réalisations homothétiques à densité fixée. L'apparition discrète de nouvelles
lignes et la reconstruction du split-tree peuvent créer l'inflexion.

Avant d'attribuer une cause géométrique, rejouer chaque taille séparément avec
l'emprise canonique dépendant de `n`, imposer `m==n` et publier : `coord`,
graine, bbox réalisée, nombre de lignes occupées, profondeurs et splits par
niveau, `front_records`, constructions internes, `eval`, octets et HWM. Cinq
graines et la rampe `12500/25000/50000/100000` distinguent une transition de
constante d'une pente persistante. La famille rouge n'autorise aujourd'hui ni
« limite des amas », ni « défaut de la WSPD ».

## 5. Garder L-infini pour le tape, sans lui prêter une sémantique euclidienne

Le front WSPD ne rend aucune décision scientifique : les certificats
recalculent leurs vraies bornes euclidiennes. Conserver le prédicat L-infini
entier est donc exact pour la partition, déterministe et potentiellement moins
coûteux en records. Son paramètre doit être nommé et pincé comme `s_inf`; aucun
texte ne doit l'appeler simplement séparation euclidienne `s`.

La conversion de paramètre n'est pas `s_inf/sqrt(3)`. Si l'on exprime le gap
euclidien `distance des centres - r_A-r_B`, normalisé par
`max(r_A,r_B)` comme dans le prédicat actuel à rayons propres,
l'équivalence des normes donne, pour `s_inf>=2`, seulement le minorant :

$$s_{2,\mathrm{eff}}\geq\frac{s_{\infty}+2}{\sqrt{3}}-2.$$

Ainsi `s_inf=2` ne garantit qu'environ `0,309` dans cette convention
euclidienne au pire. Cette conversion sert à l'interprétation, jamais à un
prune.

La variante euclidienne proposée par Claude est néanmoins une bonne ablation.
Avec les centres et largeurs doublés, poser
`D2=sum((cA2-cB2)^2)` et
`R2=max(sum(widthA^2),sum(widthB^2))`. Le test
`D2 >= (s_e+2)^2*R2` est le critère exact pour deux boules englobantes
euclidiennes de même rayon. Sous u16 et `s_e<=32`, il tient sous `2^44` en
`u64`. Il est plus conservateur que certaines séparations à rayons inégaux et
peut augmenter le front. Il doit donc porter un autre `sep_metric`, produire
un autre `FrontDigest` et gagner une ablation `temps total + octets + HWM` ; il
ne remplace pas silencieusement le tape L-infini déjà mesuré.

Le delta concurrent implémente ce test sous `--sep-euclid=p/q`. Sa formule est
sûre, mais le nom « séparation euclidienne exacte » serait trop fort pour des
rayons inégaux : l'étape `r_A+r_B<=2*max(r_A,r_B)` la rend conservatrice.
Avant de l'interpréter, normaliser `p/q` par pgcd afin que `2/1` et `4/2`
aient la même identité, ajouter les fixtures égalité et une unité sous la
frontière, un mutant du coefficient `+2q`, et un `FrontDigest` incluant
métrique et fraction réduite. La décision porte ensuite sur build, records,
classifieurs, octets/HWM et temps du consommateur, pas seulement sur le nombre
de terminaux.

## 6. Portée du nouvel oracle et état du pin

`--oracle-all=320` est un progrès : son prédicat ponctuel `__int128` n'emprunte
ni le cœur ni les bornes AABB. Le rejeu frais donne :

```text
q2 : 41461 verdicts, 2341117 triples, 0 désaccord
q3 :   907 verdicts,   15903 triples, 0 désaccord
q4 :   411 verdicts,    6158 triples, 0 désaccord
```

Sa portée est un oracle exhaustif **de la route atteinte** sur un seul nuage
uniforme fixé (`n=320`, `coord=4096`, `seed=7777`). Il s'arrête au premier
nœud `ALL` et élague `NONE`; il n'énumère donc pas toutes les combinaisons
possibles de trois nœuds. Il ne couvre ni WSPD, banque, owner, seuils de
crédits, extrema pleine largeur ni compactage. Il manque des mutants
faux-`ALL` (`>` vers `>=`, coefficients q3/q4 échangés, garde `Dlo=0`, borne
`Vhi` minorée, enfant ou paire omis) et un digest attendu empêchant une
couverture réduite de rester verte.

La construction et les tests ciblés frais au pin donnent :

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --target mhgp3v_rect_front_probe mhgp3v_wspd_front_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_(rect|wspd)_front_'
18/18 PASS, 12.36 s
```

Ce vert reçoit les nouveaux mutants d'identité et le refus `leaf>1`, pas le
P0 industriel. Restent ouverts dans la banque : allocation et tri dynamique
par rectangle, fenêtre haute sous-remplie, absence de `proof_ids` et de
compactage, fermeture q3/q4 sans type owner-shard, `Dlo` répété et aucune porte
Morton3D dédiée.

## 7. Ordre d'implémentation qui maximise la chance de tenir une seconde

1. Geler les clés injectives, `OwnerShardKey`, les outcomes et le replay de
   preuves ; conserver FNV seulement comme digest.
2. Rejouer la rampe WSPD à densité canonique et garder `s_inf=2` comme baseline.
3. Faire le CPU P0 strict : `W=32/L=16`, tableau fixe, fenêtre recadrée, un
   `Dlo` par rectangle, un `Vhi` par ID, q2 par `Hmin`, preuves et résiduel.
4. Tuer les mutants de clé, Morton, endpoint, doublon, owner, preuve et perte de
   résiduel sur petit `n`.
5. Porter seulement K1 banque/recertification et K2 compactage sur device
   résident ; falsifier `p95<=200 ms` sur trente warms avec octets/HWM.
6. Brancher la source exacte par owner-shard, puis seulement mesurer
   `WSPD + P0 + source + BallKey + census + fold` à `n=50000`.

Le statut reste `NO-GO` pour `50000/1 s` : aucun kernel rect-front, aucun
handoff owner-shard et aucun chemin exact jusqu'au fold ne sont reçus. GCP non
utilisé par cet audit.
