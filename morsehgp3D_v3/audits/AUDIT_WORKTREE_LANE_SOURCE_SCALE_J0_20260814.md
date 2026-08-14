# Contre-audit du worktree `lane_source_scale_probe` — J0

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot lu sans modifier le prototype :

- `HEAD=a369452f665cf13480b5d8039d22449e16e9ba57` ;
- `prototype/lane_source_scale_probe.cpp` non suivi, 547 lignes,
  SHA-256 `03b5e1a3d1ca31e3f2559dbb4418d37a86632f60078ed333d7aa3b4ef618c52f`.

GCP n'a pas été utilisé. Ce contre-audit ne reçoit aucun logiciel, aucun
chiffre J0 et aucun SLO. Il n'autorise aucune structure de Delaunay, d'aucun
ordre. Il conserve trois lanes autonomes : un fuseau ou un index géométrique
pur peut être partagé, mais aucun verdict ou record q2, q3 ou q4 ne fait
autorité pour une autre arité.

## Verdict

La sonde est un **diagnostic tronqué de candidats**, pas encore une mesure de
la taille de l'objet. Deux P0 indépendants sont reproduits :

1. le garde `diam_max <= 0,75*dmax` peut rendre vert alors que des supports
   situés au-delà de la coupure manquent ;
2. l'owner q3 n'est pas total lorsque deux arêtes maximales ont le même plus
   petit `PointId`, et le mode `--verifie` ne possède aucun juge q3 pour voir le
   doublon.

Même après ces deux réparations, les compteurs ne portent ni `I_B/U_B`, ni
`BallKey`, ni disposition du shell. Le sujet et son brute q2/q4 comparent des
cardinalités de profondeur stricte avec les mêmes prédicats, pas les records
contractuels à une autorité indépendante. Ils ne peuvent donc pas encore
alimenter les exposants exacts de J0.

## 1. Ce qui est mathématiquement sain

Pour une arête diamétrale ponctuelle `ab`, la requête finale dans la boule de
centre milieu et de rayon `D=|ab|` est un sur-ensemble sûr des autres sommets,
des intérieurs et du shell d'une miniboule positive q2, q3 ou q4. Le filtrage
de puissance qui suit reste nécessaire. L'arrondi entier du milieu utilisé par
la grille est compensé par la marge de deux unités, puis l'appartenance au
sur-ensemble est retestée exactement avec `U=2z-(a+b)`.

Les trois fuseaux stricts implémentent les bonnes inégalités. Avec
`g=D2-|U|^2` et `Q=D2|U|^2-(U.d)^2`, les tests sont `g>0` en q2,
`3g^2>4Q` en q3 et `g^2>2Q` en q4. Les seuils de mort sont correctement
`smax-1`, `smax-2`, `smax-3`, soit `10/9/8` sous `smax=11`. Les égalités
restent shell et ne créditent aucun intérieur.

La structure de contrôle ne forme pas une cascade de verdicts. `a2`, `a3` et
`a4` sont calculés séparément ; q4 ne lit pas le verdict q3, et q3 ne lit pas le
verdict q2. La liste de points aigus construite dans la boucle q4 est un préfixe
géométrique interne à cette lane, pas une sortie q3. Cette propriété doit être
conservée.

Sur `uniform,n=60,coord=1000,dmax=5000,smax=11`, la compilation autonome et le
rejeu borné donnent `brute_q2=1038`, `ancre_q2=1038`, `brute_q4=3283` et
`ancre_q4=3283`. C'est une vérification utile de l'énumération sur cette entrée,
mais seulement en cardinalité et avec les mêmes routines `bien_centre` et
`interieur_strict` des deux côtés.

## 2. P0 — la sentinelle de coupure rend un faux vert

Commande reproduite avec le générateur intégré :

```text
/tmp/mhgp3v_lane_source_scale_probe --family=two_lines --points=10 --smax=11 --threads=1
```

Elle retourne le code zéro et annonce :

```text
candidats : q2=20 q3=0 q4=0 total=20
coupure : diam_max=4.0 dmax=547 rapport=0.007
```

Le même sujet avec son mode borné :

```text
/tmp/mhgp3v_lane_source_scale_probe --family=two_lines --points=10 --smax=11 --threads=1 --verifie
```

retourne le code un et réfute la source :

```text
verifie : brute_q2=45 ancre_q2=20 brute_q4=0 ancre_q4=0
DESACCORD: l'enumeration par ancre differe du brute force
```

Vingt-cinq paires q2 sont absentes alors que le rapport observé vaut seulement
`0,007`, très loin de la sentinelle `0,75`. Le maximum observé est conditionné
par la coupure et ne dit rien des sorties qu'elle a omises. Cette fixture tue
définitivement le mutant `observed_max_certifies_dmax`.

La porte exacte reste celle de la réponse Q16 : une `NeutralPairPartition`
conserve toutes les paires ; chaque masse non descendue porte un certificat de
localité propre à sa lane ou une continuation ; `unresolved_pair_mass=0` avant
tout chiffre nommé taille de l'objet. Les calottes peuvent fermer des
incidences, mais l'absence de certificat ne se remplace pas par une marge
empirique.

## 3. P0 — l'owner q3 duplique une `SupportKey`

Le code ne compare, lors d'une égalité de longueurs, que le plus petit endpoint
de l'arête candidate. Si celui-ci est aussi celui de l'ancre, il omet de
comparer le second endpoint.

Fixture entière dans le plan `z=0` :

```text
A, PointId 0 = (0,0,0)
P, PointId 1 = (3,4,0)
B, PointId 2 = (4,3,0)
```

Le triangle est strictement aigu et ses longueurs carrées sont
`AP^2=AB^2=25`, `PB^2=2`. L'`EdgeKey` canonique maximal est `(0,1)`. Pourtant
le test courant déclare owners les ancres `(0,1)` et `(0,2)`, car leurs minima
valent tous deux zéro. La même q3 est donc comptée deux fois.

La réparation attendue n'est pas une dépendance à q2 : `Lane3` compare seule
les `EdgeKey=(min_id,max_id)` complets, lexicographiquement, entre toutes les
arêtes de longueur maximale de son triangle. Une fixture permanente doit
comparer le multiensemble des vraies `SupportKey` q3 au brute force. Le mode
`--verifie` courant ne calcule que q2 et q4 ; il ne peut pas tuer ce mutant.

## 4. P0 contractuel — profondeur seule, shell absent

Les trois compteurs finaux acceptent selon le seul nombre d'intérieurs stricts.
Le contrat exige le census complet `(I_B,U_B)`, puis le rang fermé et la
disposition. Une égalité de puissance, un extra-shell ou plusieurs `PointId`
sur une même position ne figurent dans aucun record produit. Les familles
multiecho rendent cette omission immédiatement pertinente.

Sous `RelevantGP`, un extra-shell doit mener à un refus typé, jamais à une
acceptation silencieuse. Sous `Plateau`, il faut transporter tout le shell et
sa multiplicité. Appeler ces nombres `cand_q*` est correct ; les appeler
supports, taille de l'objet ou sorties exactes ne l'est pas.

Le juge borné partage en outre `c8::bien_centre` et
`c8::interieur_strict` avec le sujet q4, et ne compare que deux cardinalités.
Il peut certifier l'absence d'écart de boucle sur les cas exercés, pas le
prédicat, l'owner, `I_B/U_B`, `BallKey`, ni l'absence d'une compensation
manque--doublon. La porte J0 doit comparer les ensembles de records complets à
un oracle indépendant pour q2, q3 et q4.

## 5. Domaine, ressources et portée industrielle

Le parseur `atoll` accepte des suffixes : `--points=20junk` exécute vingt
points et retourne zéro. `--coord=70000` est également accepté et exécuté,
alors que le profil annoncé est u16. Les options `dmax`, `threads`, `coord` et
leurs produits n'ont pas de caps industriels : `dmax*dmax` peut déborder,
`threads` peut provoquer une allocation non bornée et le cast vers `int`, la
grille dense alloue `dim_x*dim_y*dim_z`, tandis que la table d'offsets alloue
un cube de côté proportionnel à `dmax/cell`.

Le préflight doit donc imposer un parse entier strict, `0<=coord<=65535` sur
les coordonnées réellement générées, des bornes de threads et de rayon, des
produits d'allocations vérifiés, un budget mémoire/HWM et des fates explicites
`PENDING_CAP`/continuation. Le bandeau du prototype doit aussi reprendre les
cinq champs canoniques de cette tranche ; `backend=cpu_reference` et
`mode=diagnostic_exact_borne` ne remplacent pas le cadre déclaré ci-dessus.

Enfin, cette sonde n'est pas encore le producteur WSPD demandé. Elle parcourt
les paires locales d'une grille, puis, par ancre, les points aigus et les
couples de lentille ; chaque q4 survivant rescane encore `inner`. Son pire cas
est quintique et sa grille peut être proportionnelle au volume de la boîte,
pas au nombre de points. Elle n'établit donc aucune complexité à 50 000 points,
encore moins le contrat d'une seconde sur G4.

## 6. Gates avant une mesure J0 ou G4

1. Graver `two_lines,n=10` comme réfutation de la sentinelle et interdire tout
   exposant J0 tant que la masse d'ancres n'est pas conservée/certifiée.
2. Canoniser l'owner q3 sur l'`EdgeKey` complet et ajouter un juge q3 par
   records, avec la fixture isocèle ci-dessus.
3. Produire et comparer `SupportKey`, owner/primary, `I_B`, `U_B` et `BallKey`
   sous les deux profils de dégénérescence ; aucun accord de cardinalité seul.
4. Recevoir les préconditions u16, le parseur, les caps, continuations,
   compteurs de visites, octets et HWM.
5. Remplacer la coupure empirique par la partition WSPD neutre complète et les
   certificats propres aux trois producteurs autonomes.

Avant ces portes CPU, lancer cette sonde à 50 000 sur G4 ne produirait qu'une
mesure tronquée non interprétable. GCP non utilisé.
