# Questions de Claude — route sparse `directes + gateways`, avant implémentation

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Objet : je réoriente vers la route des sections 7 et 8 de
[`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md),
avec une cible GPU. Ces six questions sont celles où votre réponse change
l'architecture que j'écris, pas seulement son réglage. La première peut
invalider la route sur le régime cible.

## Ce que j'apporte à l'entrée

Ma génération locale certifiée (`HEAD` `6693639`) émet, par ancre et sans table
globale, tous les couples (support propre positif `U`, boule `B`) avec
`p + q <= 11`. Or une **coface directe d'ordre `k`** est exactement
`Q = U union I` avec `I` l'ensemble des intérieurs stricts et `k = p + q - 1`.
Ma sortie est donc, si je ne me trompe pas, précisément la **source directe
complète** que la section 7 exige, pour `k <= 10`.

Mesures (`--mode=arity`, juge exhaustif des trois arités égal à la génération
locale, `q2/q3/q4 = 681/884/202` à `n = 70`) :

| famille | `n = 4000` | q2 | q3 | q4 | total par point |
| --- | --- | ---: | ---: | ---: | ---: |
| terrain | | 19,83 | 44,04 | 4,19 | 68,07 |
| scanline_single_pass | | 20,13 | 43,18 | 4,89 | 68,20 |
| scanline_overlap_multiecho | | 23,04 | 51,27 | 17,44 | 91,75 |
| uniform | | 33,98 | 135,88 | 122,28 | 292,15 |

**Q0.** Confirmez-vous l'identification « mes activations = vos cofaces
directes d'ordre `<= 10` » ? Si oui, la borne « au plus quatre bras stricts par
événement direct » porte sur ces `68` enregistrements par point pour
`terrain`, soit au plus `272` bras par point avant déduplication.

## Q1 — la porte régulière tient-elle sur le régime LiDAR ?

C'est ma question décisive. La section 2 exige, pour chaque coface non-Gabriel
pertinente, un **support minimal `U(Q)` unique et essentiel**, aucun label
extérieur exactement sur la frontière, et tout label de `I(Q)` strictement
intérieur.

Or `terrain` est construit avec un sol plat et un jitter `{0,1,2}` en `z` :
`cloud_families.hpp` annonce explicitement des « coplanarités massives
ASSUMÉES ». Les familles scanline ajoutent des pas anisotropes entiers et des
multi-échos verticaux quantifiés. Sur une grille u16, ces régimes produisent
massivement des points cosphériques et coplanaires — donc exactement les
supports non uniques et les égalités de frontière que la porte exclut.

Si la porte régulière échoue précisément sur les familles cibles, la route
sparse rend `unsupported_degeneracy` là où on en a besoin, et le gain est nul.

1. Quelle est la fraction attendue de cofaces à support non unique sur ces
   familles, et existe-t-il un moyen de la **mesurer** avant d'implémenter ?
2. Le quotient du plateau cosphérique que la section 2 évoque
   (« quotienter le plateau par une preuve dédiée ») a-t-il une forme
   exécutable, ou faut-il conserver la coface Gamma entière ?
3. Existe-t-il une variante de l'étoile silencieuse qui reste exacte avec un
   support **multiple** — par exemple en choisissant `u_0` canonique dans
   l'union des supports minimaux plutôt que dans un support unique ?

## Q2 — clé de niveau exacte, et sa largeur en bits

Le fold atomique groupe par **niveau exact**. Les niveaux sont les rayons
carrés `beta` des miniboules, rationnels :

- q2 : `beta = d^2/4`, donc `d^2` entier suffit ;
- q3 et q4 : `beta` est un rationnel dont le dénominateur vient du déterminant
  de Cayley--Menger.

Pour grouper les trois arités dans un même lot il faut comparer ces rationnels
entre eux, donc un dénominateur commun.

1. Existe-t-il une **clé entière canonique** de niveau, comparable entre arités
   sans multiprécision non bornée ? Sur la grille u16, quelle est la borne de
   bits du numérateur et du dénominateur réduits pour `q = 2, 3, 4` ?
2. Si la comparaison exige la multiprécision, quelle largeur fixe suffit — ce
   qui déciderait de la faisabilité GPU d'un tri par niveau ?
3. Le tri doit-il porter sur `beta` seul, ou sur `(beta, clé canonique de la
   coface)` pour que les ex æquo soient déterministes sans être
   séquentialisés ?

## Q3 — les bras stricts et la déduplication

La section 8 dit que seules les facettes obtenues en retirant un élément du
**support** sont des bras stricts, et qu'il y en a au plus quatre par
événement direct.

1. Une même facette `F` est bras strict de plusieurs cofaces directes. La
   déduplication doit-elle se faire sur l'ensemble de labels de `F`, ou sur
   `(F, beta(F))` ? Deux cofaces distinctes peuvent-elles proposer le même `F`
   avec des `beta(F)` différents — auquel cas ma clé serait fausse ?
2. `T_F = (F \ {u_F}) union {z_F}` peut ne pas être une facette du cœur. Le
   resolver doit alors descendre. Cette descente est-elle bornée en profondeur
   par une quantité que je peux calculer, ou peut-elle être arbitrairement
   longue ?
3. Dans la branche `|J_F| = 0`, il faut « tous les co-minimiseurs exacts ». Le
   minimum porte-t-il sur `beta(Q)` des cofaces directes incidentes à `F`, et
   les co-minimiseurs sont-ils les cofaces atteignant ce minimum exact ?

## Q4 — le fold est-il parallélisable, ou est-ce le nouveau goulot ?

Le fold DSU par niveau exact est la seule étape intrinsèquement ordonnée. À 50 k
sur `terrain` j'attends de l'ordre de `10^7` arêtes et un grand nombre de
niveaux distincts.

1. Les lots de niveaux **distincts** peuvent-ils être contractés en parallèle
   dès que leurs composantes sont disjointes, ou l'ordre des niveaux est-il
   strictement contraignant ?
2. Un schéma de type Borůvka parallèle sur les arêtes triées par niveau
   préserve-t-il l'atomicité des lots d'égalité exigée par la spécification ?
3. Existe-t-il une caractérisation permettant de traiter les niveaux par
   blocs — par exemple « un lot ne peut fusionner que des composantes déjà
   fermées en dessous de son niveau » — qui autoriserait un fold par vagues ?

## Q5 — ce que la sortie doit contenir, exactement

Je veux éviter de matérialiser plus que nécessaire.

1. Pour `normalized_horizontal_h0`, la sortie est-elle exactement : les
   composantes de facettes à chaque niveau exact, plus l'union des `PointId` par
   composante, plus les naissances `beta(F)` — et rien d'autre ?
2. Faut-il conserver les **identités des facettes** dans la sortie, ou
   seulement les unions de labels ? La proposition 6+ insiste sur l'égalité
   facette par facette ; est-ce une exigence de **preuve** ou de **sortie** ?
3. La précision « filtrer aussi les sommets par leur poids de naissance
   `beta(F)` » implique-t-elle un événement de naissance séparé par facette dans
   le stream, en plus des arêtes ?

## Q6 — ce qui reste à prouver de mon côté

Je liste ce que je considère non prouvé chez moi, pour que vous puissiez me
contredire :

1. la queue de bord de ma génération n'est pas bornée : la médiane des
   candidats est plate (59 à 61 sur un facteur huit en `n`) mais p99 et maximum
   croissent ; la fermeture par cône a supprimé le balayage d'univers, mais je
   n'ai pas encore mesuré ses visites aux trois tailles contractuelles ;
2. la fenêtre de support des arités supérieures sature empiriquement, elle
   n'est pas certifiée ; ma tentative de la fermer par Jung était fausse et est
   rétractée ;
3. je n'ai aucun reçu, aucune `BallKey`, aucun owner exact-once : je compte des
   enregistrements, je n'en émets pas encore ;
4. aucune mesure `uniform` à 50 k, aucun port CUDA, aucune session G4.

## Ce que je fais sans attendre

Émission des cofaces directes comme records `(U, I, beta, BallKey)`,
déduplication des bras stricts, et requête `J_F` — cette dernière est bornée
par ma localité puisque `beta(F) < beta(Q)`. La session G4 reste fermée tant
que la gate locale ne l'est pas.

GCP non utilisé pour cette note.
