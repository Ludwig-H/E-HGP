# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans construire aucune structure de Delaunay, y
compris le graphe ou la triangulation d'ordre un.

Cadre courant :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_bounded_oracles_and_g4_diagnostic
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v3 n'est ni promue dans le registre officiel, ni qualifiée GPU, ni déclarée
exacte sur son domaine public. Le contrat reste ouvert : à `n=50000` et
`K_max=10`, aucun échantillon ne qualifie encore le payload complet sous
`p95 warm_e2e<1 s` sur G4. Aucun kernel GPU G4 bout-en-bout n'est reçu. Un reçu
antérieur sur VM G4 est explicitement `GPU_RUN=NO` et mesure `78,8 s` CPU sur
`uniform,50000`, sans BallKey, census, fold ni payload. La nouvelle rampe J0
est également CPU-only et diagnostique ; elle ne produit toujours ni
BallEvent, fold ou payload.

Le profil v3 courant reste exactement `quantized_u16_input_only`. Le contrat
normatif binary64 demeure un profil distinct et ouvert : un arrondi ponctuel
fixe, non injectif et sans certificat ne préserve pas le HGP, car deux points
situés de part et d'autre d'une frontière InSphere arbitrairement proche peuvent
être envoyés sur la même position. Une similarité-lattice exacte enregistrée
est un sous-domaine sûr ; une mesure u16 générale ne qualifie ni le payload ni
le SLO binary64.

La recette G4 q4 courante reste non reçue : son parser mélange sweeps et
exact-once de sorte que le cardinal attendu est impossible, cherche des champs
du probe supprimé, décide avant de rapatrier une réfutation et ne porte pas de
deadline globale. Le noyau qu'elle lancerait garde en outre le P0 des
identités d'entrée ci-dessous. Aucun nouveau reçu 50k ne doit être
déduit de ces probes CPU.

La première sonde J0 de `acd792d` reste elle aussi diagnostique. Sur la famille
intégrée `two_lines,n=10`, elle retourne code zéro avec vingt candidats q2 et
un rapport `diam_max/dmax=0,007`, alors que son propre brute force en trouve
quarante-cinq. Porter la coupure à soixante-quatre espacements ne change pas ce
faux vert. Son owner q3 duplique en outre un triangle aigu isocèle quand deux
arêtes maximales partagent le même plus petit `PointId`, et aucun juge q3
n'existe dans `--verifie`. Les onze CTests J0 passent, mais ne couvrent ni ces
deux P0, ni le shell. Aucun exposant de sortie, chiffre 50k ou SLO ne doit être
déduit de cette sonde avant conservation de toute la masse d'ancres et
comparaison des records complets. Le contre-audit reproductible est
[`audits/AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md`](audits/AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md).

Son brut concurrent localise néanmoins le mur. Sur `uniform,50000`, il compte
`21 432 482` candidats à `smax=11` en 38 s CPU et `4 004 994` à `smax=6` en
4 s CPU. Le repli ne réduit donc ce ledger que d'un facteur `5,35`, pas douze.
La route paie encore `6 091 112 797` paires de lentille à `smax=11`, soit
`623,5` par q4 retenu. Sur `eight_clusters,12500,smax=11`, elle atteint
`24 135 659 695` paires puis refuse la coupure ; aucun chiffre amas 25k/50k
n'existe. Le verdict est `INCOMPLET_OU_TRONQUE`, pas une réception J0. Voir
[`audits/AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md`](audits/AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md).

La première restriction du certificat de calottes aux directions
« admissibles à `r` » est réfutée : un partenaire d'un support de diamètre
`D>=r` peut être plus loin que `r`, et une cellule doit intersecter la région
potentielle plutôt que contenir une même calotte aux trois sommets. La version
sûre marque fail-open les directions vérifiant
`2(s.u)>max(r,||s||)`, puis applique indépendamment les seuils `10/9/8`.
[`audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md`](audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md).

`95b41b7` implémente l'intersection et les partenaires globaux ; ses mesures
annoncent `88,00 %` sur `uniform` et `14,33 %` sur `terrain`. Elles restent à
recertifier : le classifieur arrondit `||s||` vers le haut et peut omettre une
cellule potentielle. Le correctif exact est de propager
`T2=max(r*r,||s||^2)` et de comparer les carrés, sans `sqrt`. La fixture et les
trois comparaisons entières sont dans l'audit Q18--Q20. Même après ce patch, le
résiduel terrain, q3 et la masse d'ancres hors cutoff restent à fermer.

La route constructive qui en résulte est maintenant spécifiée. Une
`NeutralPairPartition` WSPD conserve exactement la masse ; chaque lane tente
son propre cœur `W2/W3/W4Depth`, puis scinde ou continue. Lane3 évalue
directement le centre ambiant de ses propres `Third3`. Lane4 construit ses
propres seeds et énumère le niveau shallow de leurs formes affines sur le plan
médiateur : au plus `2*r4` groupes de roots par ligne, sans aucun produit
`lentille x lentille`. Le best-first exact emploie la quadratique séparable
`Q_theta=q*A-p*B` sur le Morton BVH. Proposition, invariants de masse et ordre
d'implémentation :
[`audits/NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md`](audits/NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md).

Le premier raccord confirme cette direction. À `n=6000,smax=6`, le mode axial
conserve les `89 796` q4 du chemin plat et remplace `48 791 131` couples par
`830 044` roots proposés, soit environ `59x` moins. Le mur reste `25--28 s`
parce que chaque seed balaie encore tout son CSR témoin : le prochain jalon est
donc d'abord le branchement de `census_replay` pour supprimer le second scan,
puis la descente `Q_theta` sur BVH pour supprimer le premier. `DEBORDEMENT`
doit continuer ou refuser, jamais être compté comme une mort. `2c14313` porte
le flat scan sur device comme baseline de parité, mais ne contient pas encore
cette descente. L'attempt G4 du pin `11130cb` compile sous CUDA 12.9, mais finit
`rc=127` avant le rapatriement du brut et avant le verdict ; sa cible est
certifiée `TERMINATED`, mais aucune parité ni aucun débit n'est reçu. Préserver
et récupérer le fichier distant avant toute relance qui effacerait `~/ax`.
`6be6bd8` accélère déjà la construction du lot par une grille partagée ; sa
prochaine porte compare le CSR complet à un scan naïf sur petit `n`. Pour
borner ensuite la HWM, un tuilage exact est possible : propriétaire par sommet
lexicographique minimal et halo entier `ceil(3*dmax/2)`, avec recollement des
multiensembles et IDs globaux. Ce lemme ne certifie pas le cutoff lui-même.
Audit et gates J1.5/J2 :
[`audits/AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md`](audits/AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md).

## Verdict actuel

Historiquement, le pin logiciel `6e815d28b3e229a0161eb00d6fa0c9a272efac5d` contient quatre avancées
directement liées à l'idée de support complet, puis plusieurs diagnostics de
réduction du broad phase WST3/WST4 :

- `Corner8BallDepth` reçoit un certificat q4 `ALL_INTERIOR` sur un produit de
  boîtes de supports et une boîte témoin ; il évite centre et division, mais ne
  propose ni supports, ni owner, ni positivité ;
- `WST3CandidateCover` route une fois le troisième sommet dans l'antichaîne du
  rectangle qui contient l'arête-owner ;
- son produit non ordonné couvre de même les deux sommets restants d'un
  quadruplet ; `f7ab7bb` ajoute le certificat Corner8 bisigne et `52585c5`
  cesse de supprimer les couples témoins non séparés.

Le commit intermédiaire `3703097` avait inversé le signe du filtre d'acuité.
`a73161c` applique désormais l'identité correcte `E+X-D=-2H`, garde une face
aiguë lorsque `H<0`, ajoute la fixture du tétraèdre régulier de tie et rétracte
les gains issus du signe faux. Cette réparation reçoit le signe scalaire de la
fixture, pas encore une relation q4 positive : le juge WST4 ignore toujours
les copies sous les ancres non-owner, les `PointId` distincts et les
barycentriques.

Ce troisième point n'est pas encore `OwnedCK-WST3/WST4`. Le probe ne filtre ni
les occurrences provenant d'arêtes non-owner, ni les diagonales de `PointId`,
ni le bien-centrage q4. Il classe désormais les blocs qui peuvent contenir une
face aiguë, mais n'applique ce bit qu'à un compteur : il ne produit pas encore
la relation q3 positive. Son juge choisit l'owner a posteriori et ne regarde que
ce rectangle : il prouve la **couverture adressée par owner**, pas l'exact-once
de toute la relation émise. Son tie-break emploie en outre le rang Morton au
lieu du vrai `PointId`, et le probe rejette les positions dupliquées au lieu
d'en conserver la multiplicité.

La distinction coût/couverture est impérative. Arrêter la descente dès la
racine resterait une couverture adressée par owner, mais sans sélectivité.
L'échelle rationnelle réduit fortement le nombre de
descripteurs sur la rampe finie, sans réduire la masse logique des quadruplets
qu'ils représentent ni recevoir une borne de packing pour les nœuds LBVH. Le
comportement actuel du code est `diag^2*num<=rayon^2*den`, donc un rapport plus
grand raffine. Le nom d'option, un commentaire local et les portes CMake
conservent la convention inverse : il n'existe donc pas encore de contrat
versionné non ambigu. La réparation est soit de renommer ce paramètre
`--finesse=num/den`, soit de conserver un ratio diamètre/rayon et d'inverser la
comparaison ; deux portes réciproques doivent distinguer les deux sens.
Le compteur WST4 emploie correctement l'identité
`sum binom(|C_i|,2)+sum_{i<j}|C_i||C_j|=binom(sum_i |C_i|,2)` sans boucle
quadratique sur les blocs, mais reconvertit encore l'entier u128 en `double`
pour l'affichage. Owner, injectivité, positivité et profondeur doivent précéder
tout `fill`.

Le diagnostic Corner8 reste exécuté après la matérialisation et le count du
broad phase : il n'épargne encore aucun produit WST4. `52585c5` marque les
couples `C,D` non séparés sans les supprimer, ce qui répare une perte de
complétions. Il compte aussi la décomposition récursive de `Sym2(C)`, mais ne la
route pas : la jonction Corner8 parcourt toujours les couples plats `u<=v` et
soumet encore la diagonale `{C,C}` comme deux boîtes identiques.
Le comptage récursif lui-même atteint `459477476` nœuds à `n=4000`, contre
`141468` couples plats : pour la télémétrie, `|C|-1` suffit ; pour la source,
`Sym2` doit rester paresseux et ne descendre qu'au besoin.

Le théorème bisigne commis est sûr sous ses préconditions, mais son caller est
incomplet. Dès qu'un nœud témoin certifie l'un des deux signes de `J`, il ne
descend plus ce nœud, même si la lane active exige l'autre signe. Il faut porter
un masque de lanes actives par tâche, créditer seulement les bits conclus et
continuer les bits restants. Aucune CTest actuelle ne juge cette jonction, ses
vrais `PointId`, ses deux ledgers ou sa causalité avant le produit.

Le nouveau `--masse` ne reçoit pas les conclusions de son titre de commit. Il
pondère le rectangle par une masse brute, puis choisit les **blocs** `C,D`
uniformément au lieu de les pondérer par leurs populations ; le tirage n'est
donc pas uniforme parmi les quadruplets. Son cumul est en `double`, l'owner
ignore le tie par vraie `EdgeKey` et la positivité est provisoirement forcée par
`const bool centre=true`. `retenus` ne teste que `I<=7`, sans extra-shell, et
`--masse` est même accepté sans `--ordre=4` : un rejeu `n=40` sur le chemin q3
rend `OK` avec une ligne q4 factice. La CTest à regex reproduit une seule sortie
biaisée ; zéro succès sur 3000 tirages ne prouve ni une fréquence `10^-8`, ni
que la masse q4 positive est du déchet, ni que le rayon est le filtre dominant.
Le sampler combinadique u128 exact, ses strates de rejet et son plafond
`samples*n` sont spécifiés dans `PROPOSITION.md`.
Le mode exhaustif `--supports-retenus`, introduit brut à `f1b78c0`, applique
désormais le test entier `c8::bien_centre` commis à `34cf05d`, puis `I<=7`.
Il énumère donc la population ponctuelle `Positive4 intersect {I<=7}` des
`SupportKey`, pas encore des événements HGP : shell, `BallKey`, RLE et provenance
des supports cosphériques restent absents. Sa « localité de rayon » est le
rapport plus-grande-arête/espacement ; ce n'est qu'un proxy à facteur deux
**après** positivité, pas le circumrayon. Son rang sans tie `PointId` correspond
au plus à une clique kNN mutuelle non déclarée. Enfin, le cap `n=260` autorise
jusqu'à `47 627 157 760` tests in-sphere puis `575 990 939 160` comparaisons de
distance : c'est une borne d'entrée, pas un budget. L'énumération globale des
4-ensembles n'a pas besoin d'owner pour l'exact-once ; l'owner est requis pour
son raccord à `OwnedCK`. Elle hérite en outre du rejet global des positions
dupliquées du probe, contraire au profil `PointId` déclaré.

`c8::bien_centre` est mathématiquement cohérent sous entrée u16 authentifiée :
Cramer et les quatre signes de faces équivalent aux quatre poids
circumcentriques strictement positifs. Au `HEAD`, les `18/18` portes ciblées
Corner8/WST passent sur deux rejeux en `13,39--19,31 s`. Les diagnostics
individuels donnent `0,00--0,02 s` pour `mhgp3v_corner8_centre` et
`5,10--9,61 s` pour
`mhgp3v_supports_retenus_localite`; le catalogue contient `839` CTests. Cela remplace
l'échec transitoire antérieur à `08dec609`, mais ne constitue pas une réception
indépendante : les portes restent à regex, la seconde omet `--ordre=4`, aucun
budget de travail n'est imposé, et les fixtures ne couvrent ni permutations,
orientation opposée, extrêmes u16, hors-profil, ni mutants de poids nul ou
négatif. L'ABI publique prend des `i64` sans preflight u16 et fusionne
non-positif, dégénéré et entrée invalide dans un booléen. Les commentaires CMake
qui disent encore que ce compteur n'applique pas la positivité, ou que les
`4096` supports du bloc sont tous bien centrés sans appeler ce prédicat, sont
des claims périmés. Le titre « H4 trente n » n'est pas reçu : la population est
`Positive4 intersect {I<=7}`, pas `H4_rank`, et vaut déjà `61--73 n` sur les
deux tailles uniformes contre `30--35 n` seulement sur les amas. Ni le
shell/RLE, ni une source 50k, ni une pente ou un coût de bout en bout ne sont
mesurés.

Votre idée mathématique est donc reçue sous sa forme exacte : la source finie
exhaustive n'a besoin que des supports minimaux positifs complets. q2 teste une boule
diamétrale par paire de positions distinctes, donc `D>0`. Deux IDs au même site
ne forment pas q2 entre eux, mais chacun reste endpoint avec tout autre site et
leur multiplicité est conservée dans le census. q3
teste une circum-boule par triangle strictement aigu ; q4 une circumsphère par
tétraèdre affinement indépendant dont les quatre poids circumcentriques sont
strictement positifs. Pour q3, le centre est
intrinsèque au plan, mais la boule et son census sont ambiants en dimension
trois. Les sphères incidentes à une ancre partielle ne sont que le domaine d'un
prune facultatif, jamais la source.

En dégénérescence, un support positif témoigne une miniboule candidate :
Carathéodory donne l'équivalence entre l'existence d'une telle base de taille
au plus quatre et `c in conv(U_B)`. Il ne suffit toutefois pas à construire le
`BallEvent`. Sous le contrat régulier Reani--Bobrowski, le census du shell
complet doit encore vérifier `c in relint(conv(U_B))`, puis décider rang fermé,
disposition et provenance. Sous `RelevantGP`, une lane pertinente
`|I_B|+|S|<=smax` vérifie `U_B=S`, donc la positivité suffit. Hors de ce domaine,
la source conserve la `BallKey` et rend `unsupported_degeneracy/plateau_pending`
tant qu'une politique dégénérée indépendante n'est pas reçue.

Cette unicité fournit aussi un backend de census partagé : après normalisation
et RLE, une `BallKey` peut être parcourue par la même somme de trois
quadratiques convexes `A*z_j^2+B_j*z_j`. Ses extrema sur une AABB u16 entière
sont exacts par axe. `active_arity_mask` efface q2/q3/q4 respectivement à
`10/9/8` intérieurs. Pour q4, le théorème `Q4SeedAxisTopR4` permet de transporter
la liste exacte `I_B/U_B`. Le commit `33766f6` conserve les IDs, porte le buffer
de shell à sa capacité prouvée de 163, publie les comptes requis et type les
plateaux ; `a369452` refuse désormais `MORT_GAP` et tout apex dont le compte
retenu atteint `r4`. Son replay n'est toutefois pas encore une autorité : ni la
sélection ni l'API n'imposent l'injectivité/disjonction des `PointId`. Le backend
reste donc l'autorité/fallback jusqu'à réception de cette précondition. Cela supprime les
census répétés par support, pas une sortie réellement lourde.

La source WSPD cible suit directement les trois contrats de miniboule : q2
garde les paires dont la boule diamétrale a au plus neuf intérieurs ; q3 garde
les triangles strictement aigus dont la miniboule ambiante en a au plus huit ;
q4 garde les tétraèdres bien centrés dont la circumsphère en a au plus sept.
Les seuils de mort sont `10/9/8` sous `smax=11`.

Ces trois lanes sont des producteurs autonomes. Ils peuvent lire le même
`PointStore`, le même index Morton et une `NeutralPairPartition` immuable, mais
leurs queues, records, caps, continuations et preuves de complétude sont
disjoints. `PairAnchor3` est créé dans `Lane3`, jamais lu dans la sortie q2 ;
`Q4Seed3` est créé dans `Lane4`, jamais lu dans la sortie q3. Un préfixe
ternaire interne à q4 n'est pas un support ni un événement q3.

La sonde J0 actuelle n'implémente pas encore ce fork : elle construit un même
vecteur `acu`, consommé par q3 avec son bit owner puis décodé par q4. Cela ne
crée pas de propagation de verdict, mais reste un record inter-lanes interdit
pour l'architecture. Les producteurs reçus devront posséder séparément
`Third3/owner3` et `Q4Seed3/primary4` ; seul un calcul géométrique pur et
immutable peut être mis en cache en commun.

Un J0 de dimensionnement exact doit couvrir toutes les ancres de la
`NeutralPairPartition`. Le diamètre maximal observé sous un `--dmax` ne prouve
pas l'absence de support au-delà de la coupure. Une masse sautée doit porter un
certificat local exact ou rester une continuation ; sinon les compteurs sont
des ledgers tronqués sans ordre garanti, pas la taille de l'objet. Avant owner,
shell et source complète, omissions et surcomptes peuvent coexister.

Le nouveau déblocage interne à `Lane4` supprime le produit de cellules. Pour un
`Q4Seed3` aigu
exacte, les centres compatibles sont sur un segment axial et chaque site a une
puissance affine `A_z-tau*B_z`. Si `p` sites sont intérieurs sur tout le
segment, toute circumsphère de profondeur au plus sept a son apex parmi les
`8-p` premières racines `B>0` ou les `8-p` dernières racines `B<0` : au plus
seize groupes, avec ties complets. Pour un apex retenu **et shallow**, sur des
`PointId` injectifs et disjoints, le replay de ces extrêmes reconstruit
exactement `I_B/U_B`; l'API courante doit encore imposer ces préconditions. Pour
une arête owner portant `m_e` lignes carrier, il y a
au plus `8m_e` centres q4 shallow lorsque `m_e` compte toutes les lignes de
sites admissibles ; le générateur limité aux `m_e^acute` faces aiguës a la
borne de propositions `16m_e^acute`. C'est le remplacement déterministe de
`binom(m_e,2)`.

La voie active est un fork :

```text
NeutralPairPartition
  |- Lane2(Pair2 -> Q2MidballDepth10)
  |- Lane3(PairAnchor3 x Third3 -> Q3MiniballDepth9)
  `- Lane4(PairAnchor4 x Q4Seed3 x Fourth4 -> Q4SeedAxisTopR4 -> Positive4)
```

Les trois sorties rejoignent seulement ensuite `BallKey/RLE`. `WST4`,
`CellPair` et `Sym2` restent des diagnostics de masse historiques, pas
l'ordonnance P0. Le contrat est précisé dans
[`audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).

Un déblocage arithmétique accompagne cette réduction. Pour tout support complet
`S={a,a+m_1,...,a+m_k}`, `k=1,2,3`, poser `G=M^T M`,
`Delta=det(G)` et `r=adj(G) diag(G)`. Les numérateurs des poids sont les `r_i`
et `2 Delta-sum r_i`, tandis que
`Phi_S(z)=Delta||z-a||^2-(z-a)^T M r` décide exactement intérieur, shell et
extérieur. Cette même forme redonne la boule diamétrale q2, la boule ambiante
q3 et, en q4, `Delta=O^2` et `Phi=O*J` sans choisir le signe d'orientation.
Elle fournit directement une `BallForm` primitive. Le support q4 ponctuel se
réduit ensuite à la forme `T/Qbar` qui tient en i128 sur u16 ; un produit de
boîtes au signe indécis garde le numérateur corrélé en i192/i256 et rend
`PENDING` s'il ne peut pas le borner. La formule et les portes requises sont
dans `PROPOSITION.md`.

Le théorème, la portée conditionnelle de Corner8, les coutures exact-once et les
réponses aux questions de coût sont reçus dans le
[`contre-audit support-complet`](audits/AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md).

Les autres prototypes restent des accélérateurs non reçus : Midball/HC/SOC/Jung
certifient des ancres partielles ; `--borne-sup` perd encore des fermetures avec
la vue combinée ou BJD ; `BallFormToBallEvent-v0` déborde son contrat u16 ; le
probe nommé 0B n'émet ni dix forêts, ni verticales, ni payload. Leur état exact,
les hashes et les contre-fixtures sont centralisés dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Les titres de
commits et les taux locaux restent des diagnostics, jamais des claims produit.

## Route active

La prochaine chaîne à fermer est :

```text
0A  BallForm -> BallKey -> census -> BallEvent exact
0B  oracle exhaustif borné -> lots -> dix forêts -> verticales -> payload
1   manifeste NoDelaunay -> NeutralPairPartition exacte et immuable
2a  Lane2 autonome -> Pair2 -> MidballDepth10
2b  Lane3 autonome -> PairAnchor3 x Third3 -> Q3MiniballDepth9 ambiant
2c  Lane4 autonome -> PairAnchor4 x Q4Seed3 x Fourth4
    -> Q4SeedAxisTopR4 -> Positive4 -> I_B/U_B avec cap/fate reçus, aucun Sym2
3   réunir seulement les sorties reçues dans BallKey/RLE puis 0A/0B
4   porter les trois producteurs sur device, puis mesurer warm_e2e sur G4
5   ouvrir séparément tout nouveau profil numérique
```

Cette séquence bloque une réception produit, pas l'audit de coût. Une piste
parallèle `counter-only` peut recevoir sur petit `n`, contre l'oracle exhaustif,
`NeutralPairPartition -> Lane4(Q4Seed3Block -> Q4SeedAxisTopR4)`. Elle doit mesurer les morts
`T2/permanents/gaps`, les groupes de racines, les splits, les comparaisons
larges, les octets et la HWM ; elle ne ferme ni 0A, ni 0B et ne crée aucun
claim public.

Une source incomplète peut être comparée dans le sink de référence, mais ne
publie jamais un succès. Il n'existe pas de watermark monotone par ancre : les
runs sont scellés, triés et mergés par niveau exact avant le premier commit
d'un lot. « Streamé » signifie mémoire résidente bornée, jamais fold en ligne
sur une source non scellée.

## Contrat d'identité

Les couches restent distinctes :

- `PrimitiveSphereKey` : cinq coefficients primitifs de
  `A||z||^2+B dot z+C`, avec `A>0`, avant census ;
- `BallKey` : identité de nuage, profil et schéma exact ajoutés à la clé
  primitive ;
- `SupportKey` : vrais `PointId` triés, jamais positions Morton ou indices
  d'un buffer ;
- `BallEvent` : `BallKey`, supports, owners, niveau exact, `I_B/U_B`, lanes,
  provenance, complétude du census et disposition transactionnelle.

Le fold contractuel ne doit dépendre ni de `__int128` natif, ni du nombre de
limbs du profil. Le probe courant viole encore cette frontière : son
comparateur lit directement `PrimitiveSphereKey` et effectue ses contrôles
après des multiplications signées susceptibles de déborder. Une fois la
frontière reçue, un futur profil binary64 pourra changer `ExactKernel` et le
codec sans réécrire le fold. La cardinalité seule ne motive pas binary64 : la
grille u16 3D contient $2^{48}$ sites distincts ; l'index dense et le `PointId`
sont des codecs séparés.

## Prochaines réparations P0

Avant d'appeler `0A` fermé :

1. construire directement les polynômes q3/q4 sans centre rabattu en `int64`
   et employer une autorité BigInt/rationnelle sur tout `[0,65535]^3` ;
2. juger indépendamment dépendance affine, positivité, clé primitive, niveau,
   census et owner sur des `PointId` non denses ;
3. ajouter epoch/profile/schema, statuts typés, marqueurs de complétude et
   `SupportRecord` atomique ;
4. appliquer `count -> preflight -> fill -> validate -> publish`, avec zéro
   payload sur cap moins un, erreur numérique ou dégénérescence non admise ;
5. borner les générateurs de fixtures et refuser leur capacité plus un ;
6. différencier toute la sortie de `0A`, puis fermer `0B` par générateurs et
   ordres jusqu'au `BenchmarkOutputContract-v1`.

## Ancienne factorisation WST3/WST4, désormais diagnostic

La section suivante conserve seulement les preuves de couverture et les
obligations qui jugent le probe courant. Elle n'est plus une route P0. Aucun
`CellPair` ou `Sym2` ne doit être raccordé aux trois producteurs autonomes.

La factorisation ci-dessous décrit uniquement le probe historique réfuté. Les
flèches ne sont pas une ordonnance et ne doivent jamais être traduites en flux
entre `Lane2`, `Lane3` et `Lane4` :

```text
HISTORICAL_PROBE_ONLY:
CKPairTape(A,B)                          toutes les paires
  -> OwnedCK-WST3(A,B,C)                carrier de l'arête maximale
  -> OwnedCK-WST4(A,B,C,D)              second carrier/apex
  -> BallKey/RLE -> rang/census -> fold
```

Le mot `Owned` est réservé à l'intersection exacte suivante. Un cover de
cellules, même disjoint à l'intérieur d'un rectangle, ne suffit pas :

```text
WST3CandidateCover
  intersecte 3 PointId distincts, OWNER(ab), indépendance et triangle aigu
  -> OwnedCK-WST3

unordered CellPair(WST3CandidateCover)
  intersecte 4 PointId distincts, OWNER(ab), orientation non nulle
  intersecte 4 poids circumcentriques strictement positifs
  -> OwnedCK-WST4
```

Chaque filtre est tri-state sur un bloc. `ALL` autorise le consommateur
factorisé, `NONE` écarte le bloc, `MIXED` remplace atomiquement le parent par
une partition complète ; égalité ou cap donnent `PENDING`, jamais un verdict
inventé. Le tie-break owner porte sur les vrais `PointId`, indépendamment du
rang Morton. Plusieurs `SupportKey` peuvent ensuite partager une `BallKey` :
le RLE garde toutes les provenances et ne paie qu'un census complet par boule.

Pour chaque rectangle CK, une boule `B_R` contenant `A union B` fixe un niveau
Morton. Tout troisième ou quatrième sommet d'un support dont `ab` est l'arête
maximale appartient à `2B_R`; la constante deux est sharp. En écrivant
`a=m-h`, `b=m+h`, `x=m+q` autour du centre de `B_R`, les endpoints donnent
`||m||^2+||h||^2<=R^2`, tandis que la maximalité donne
`||q+h||,||q-h||<=2||h||`, donc `||q||<=sqrt(3)||h||` et `||x||<=2R`.
Intersecter avec `B(c_A,U_AB+r_A)` et `B(c_B,U_AB+r_B)`, où `U_AB` majore
les distances endpoint, resserre encore le domaine sans perdre de carrier.
Dans cette identité combinatoire historique, les cellules non vides donnent
des facteurs ternaires et leurs couples des facteurs quaternaires. Cela ne
constitue aucune preuve de complétude du probe et ne relie aucune lane active.
Cette preuve exige que `CKPairTape` partitionne réellement les paires non
ordonnées et que les cellules half-open forment une antichaîne. Un raffinement
remplace atomiquement le parent par tous ses enfants disjoints. Pour la
diagonale q4, `binom(C,2)` devient tous les `binom(C_i,2)` et tous les
`C_i×C_j`, `i<j`; pour `C×D`, tous les `C_i×D_j`. Le carrier primaire oriente
la sweep mais ne crée jamais une seconde copie du `CellPair`.
Une paire n'est q2 propre que si `D=||b-a||^2>0` : seule cette paire endpoint
dégénérée est filtrée. Une position géométrique peut être bucketisée, mais tous
les `PointId` et leur multiplicité restent présents dans les pools témoins et
les produits ; les paires de chacun de ces IDs vers une troisième position
gardent donc leur multiplicité. Un quotient silencieux changerait la profondeur.

### Miniboule unique et centres critiques finis

Pour un support minimal positif affinement indépendant fixé, la miniboule —
donc la boule canonique de l'événement complet porté par ce support — est
unique. Si son arité est inférieure à quatre, il subsiste une famille de
sphères ambiantes incidentes, mais elles ne sont pas des événements dont ce
support est le support minimal. Pour q2, le centre canonique est exactement le milieu et
`(z-a) dot (b-z)>0` décide l'intérieur strict ; l'égalité est shell, et le
verdict HGP complet conserve le census fermé et le `BallKey`. Pour une ancre
`ab` fixée, q3 prend le pied
auto-centré de la ligne du troisième site dans le plan médiateur, et q4
l'intersection de deux lignes. Le continuum du disque de Jung n'est donc pas la
source des événements : il reste le domaine légitime d'un prune collectif avant
la génération finie.

Ainsi la source HGP n'énumère jamais une famille continue de sphères : q2 teste
la seule boule diamétrale, q3 la seule boule **ambiante** dont le centre et le
rayon sont intrinsèques à chaque triangle aigu, et q4 la seule circumsphère de
chaque tétraèdre bien centré. Les domaines de
centres attachés à une ancre partielle sont exclusivement des accélérateurs de
prune ; leur échec n'ajoute aucun événement et n'autorise aucune cascade de
rang.

Pour un domaine de centres contenant le centre canonique, `C` désigne son
nombre d'intérieurs, `U` les témoins individuellement universels et `D` la
profondeur collective minimale. Le contrat exact est `U<=D<=C`. Ainsi `C<h`
évite un Jung/BJD qui ne peut fermer sans décider les cofaces, `U>=h` ferme, et
le cas `U<h<=C` exige `tau(F)`, une sweep ou un split. Pour toutes les sphères
incidentes non bornées, le cœur commun est seulement le segment ouvert q2 ou le
circumdisque situé dans le plan q3.

Aucune inférence entre lanes n'est définie. `Lane2`, `Lane3` et `Lane4`
construisent et recensent chacune leurs propres supports complets, puis livrent
séparément owner, shell et `BallKey` au sink commun.

q3 recertifie `E+X-D>0` et l'indépendance affine. q4 ne signifie pas « quatre
faces aiguës » : l'autorité est la stricte positivité des quatre
barycentriques du circumcentre. Un `Q4Seed3` aigu adjacent à l'arête maximale,
produit dans `Lane4`, sert seulement à choisir une provenance géométrique
primaire. La jointure q4 teste
`Acute(x) OR Acute(y)`, jamais `AND` : un q4 positif peut n'avoir qu'une seule
face aiguë adjacente. Si les deux le sont, le plus petit `PointId` aigu est le
primaire et supprime le doublon ; l'autre sommet reste un apex arbitraire.

Le probe historique partitionnait les cellules en `A` (`Hmin<0`) et `N`
(`Hmin>=0`), puis comptait
`Sym2(A) disjoint_union Cross(A,N)`. Cette construction reste un diagnostic de
masse réfuté, jamais une entrée de `Lane4`. Pour un `Q4Seed3` exact, les centres vivent sur
une droite et la puissance de chaque apex y est affine : une sweep 1D par lots
égaux remplace l'arrangement 2D comme candidat principal. Les comparaisons
rationnelles peuvent dépasser `i128` sous u16. Un site dont le dénominateur de
sweep est nul contribue une puissance constante négative, nulle ou positive ;
il n'est jamais jeté.

Dans l'architecture active, `Lane4` régénère ses propres `Q4Seed3` depuis la
partition neutre. Elle ne lit ni relation, ni masque `q3_open`, ni tape, ni
événement de `Lane3`.

Le nombre de blocs initiaux vaut conditionnellement `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` pour q2/q3/q4, avec `0<eta<=1` et une
vraie propriété fair/compressed-split. Ces bornes ne couvrent pas tous les
raffinements `MIXED`. Leur masse logique peut rester quadratique ou pire. Les
blocs sont paresseux jusqu'à un consommateur factorisé reçu ou au preflight
atomique d'une vraie sortie. Le rapport complet
et son contre-audit sont
[`audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
et
[`audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md).
Les réponses à Claude et le contre-audit des compteurs porteurs/apex sont dans
[`audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit v2 et la réponse entière `BlockBallDepth8` sont dans
[`audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).

## Déblocages mathématiques prêts après `0A`

Quatre pistes sont assez précises pour être implémentées dans les composants
existants, avec échec fail-open.

### `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q3 exige
`H>0 && 4H^2>EX` et q4 `H>0 && 3H^2>EX`. Le domaine est séparément convexe en
`e,t` et en `a,b,z`.

- Si les 64 couples de coins de `(C-A)×(B-C)` passent, tout le rectangle est
  `ALL`. Un échec est `UNKNOWN`.
- Les 512 triples de coins de `A×B×C` caractérisent exactement `ALL` pour
  l'enveloppe AABB continue. Un coin fictif échouant ne vaut pas `NONE` pour les
  seuls `PointId` stockés.

Le `JungSpindleRect-v0` actuellement branché n'est pas `SOC64/CORNER512` : il
combine des extrema séparés de `D,V,T`. Son diagnostic `n=6000,s=8` gagne
environ trois centièmes de point seulement. Cela réfute cette combinaison sur
les boîtes grossières mesurées, pas les deux théorèmes corrélés. Leur primitive
et leurs 16 portes isolées sont verts au pin stable. L'intégration WSPD active
et son coût transitif restent non reçus, comme borné ci-dessous.

`SOC64` existe désormais en mode shadow et en mode actif optionnel. Le mode
actif est un disjonctif q4 sûr : un `ALL` change le fate, un échec reste
inconclusif. Le message du commit `110fe76` annonce une baisse locale du
résiduel à `n=3000`, mais aucun reçu brut ne permet de la promouvoir en pente
ou en résultat G4. La tentative archivée sous
`receipts/soc64_actif_g4_20260814/` a échoué avant la mesure et s'est arrêtée
`TERMINATED`.

Le parcours actif reste incomplet sous `central-NONE` : si SOC est `UNKNOWN`,
il ne descend pas encore sa propre vue vers les enfants. Le prochain jalon
n'est donc plus un shadow initial, mais une porte locale appariée : union de
preuves par vrais `PointId`, mutant de chevauchement, `pending=0`, coût/HWM,
puis tailles `1500/3000/6000`. Ne pas lancer 50 000 avant cette porte. Un
retour inférieur à `floor=q4` signifie seulement `UNKNOWN_BELOW_FLOOR`, pas
une lane exacte.

Le chemin `--soc64-actif` n'a pas encore de juge propre. Lui adjoindre
`--judge-soc64` ne valide pas ses succès : l'actif promeut d'abord `v=ALL`, puis
le shadow saute ce verdict et peut finir avec zéro verdict jugé. L'autorité
active-only doit recomposer les vrais `PointId` distincts pour chaque fermeture
et séparer ses compteurs de ceux du shadow.

`--judge-vwave` n'est pas cette autorité : il recompte seulement les
singletons du central. Combiné à SOC/BJD sur la fixture `eight_clusters,n=200`,
il rejette 149 fermetures collectives valides et affiche « sans 10 » alors que
la lane q4 exige huit. Cette combinaison doit être refusée jusqu'à un juge qui
recompose chaque type de preuve.

### `JungDiskDepth`, puis LP projectif

Pour une paire ponctuelle owner `ab`, les centres q3 et q4 ne parcourent pas
tout le plan médiateur : avec `y=2c`, ils restent dans les disques exacts de
Jung `||y-d||^2<=D/3` et `||y-d||^2<=D/2`. Dans ce plan 2D fixe, un groupe d'au
plus trois IDs peut certifier qu'au moins un témoin est intérieur pour tout
centre admissible. Neuf groupes disjoints ferment q3, huit ferment q4 avant la
création des carriers de cette paire.

Cette preuve ne ferme pas un rectangle CK : ses endpoints font varier le plan,
le disque et les demi-plans. Il faut scinder jusqu'à une paire/microtile rejoué,
ou prouver un futur `BlockJungDiskDepth` uniforme. Une fixture `2×2` ferme q4
sur la paire basse alors qu'aucun de ses huit témoins n'est même q2 intérieur
sur la paire haute ; tout transfert depuis un représentant reste interdit.

Un candidat de certificat `ALL` uniforme est `BlockJungDualTile`. Écrire `m=(a+b)/2`,
`h=(b-a)/2`, `c=m+w`, avec `w dot h=0` et
`||w||<=kappa||h||`, où `kappa^2=1/3` pour q3 et `1/2` pour q4. Un groupe
ferme une paire si et seulement s'il existe des poids rationnels `lambda_z`
non négatifs de somme un tels que, avec
`alpha=||h||^2-sum lambda_z||m-z||^2` et `p=m-sum lambda_z z`, on ait :

```text
q3 : alpha>0 et 3*alpha^2 > 4*(||h||^2||p||^2-(p dot h)^2)
q4 : alpha>0 et   alpha^2 > 2*(||h||^2||p||^2-(p dot h)^2)
```

Helly borne une base à trois IDs. Un certificat de bloc conserve une même base
et des poids rationnels. À poids fixes, le test exact des 64 couples de coins
décrit ci-dessous prouve les polynômes sur tout `A×B`, sans Bernstein ; un
échec provoque un split fail-open. Tester les coins avec des poids reproposés
séparément reste faux. Pour un témoin singleton, ce dual redonne exactement
`SOC64`.
Cette version bloc est sûre mais incomplète : `for all pair exists lambda`
n'implique pas `exists lambda for all pair`. Un échec ou un dénominateur trop
large rend `MIXED/UNKNOWN`, jamais `NONE`.

La forme entière directement alignée sur le vérificateur pose
`W=sum w_z`, `D=||b-a||^2`, `A=W*D-sum w_z||a+b-2z||^2`,
`P=W*(a+b)-2*sum w_z*z` et
`R=D||P||^2-(P dot (b-a))^2`. Elle teste q3 par
`A>0 && 3A^2>4R` et q4 par `A>0 && A^2>2R`. Sous u16, la largeur i128 annoncée
exige `W<=65535` vérifié avant toute somme. Une primitive CUDA et une profondeur
de bloc restent ouvertes.

Le lift rectangle à poids fixes est désormais résolu mathématiquement. Poser
`A0=-W*(a dot b)+(a+b) dot Z-Q` et
`C0=W*(a cross b)-a cross Z-Z cross b`. q4 vaut
`A0>0 && 2A0^2>||C0||^2`, q3 remplace `2` par `3`. Pour un endpoint fixé,
`(A0,C0)` est affine dans l'autre et chaque lane est un cône de Lorentz convexe.
Les `8×8=64` couples de coins caractérisent donc exactement `ALL` sur
l'enveloppe `A×B`. Au pin `5809bd2`, `bjd_lane_box` rend une lane entière ou
`-1` ; le wrapper contractuel traduit `lane>=floor` en `ALL_GROUP`, sinon en
`MIXED`. La primitive vérifie seulement la base et les poids fournis, n'émet
aucune paire et tient en i128 sous `1<=W<=65535`. Un proposant distinct est
rappelé dans les enfants après split. `ALL_GROUP` ajoute une
hyperarête uniforme ; seule
`tau(E_Q)>=8/9` ferme la profondeur du rectangle. Les contrôles
`A4=4*A0` et `R=4*||C0||^2` fixent les facteurs ; cette
équivalence porte seulement sur le reçu à poids communs, pas sur l'existence
d'un poids différent par paire. Le widening précède `a+b`, les produits et les
normes ; le preflight de `W` somme en type large ou saturant avant tout cast.

Depuis le pin `5809bd2`, le header implémente cette forme à base et poids
communs fixés. La réception logicielle reste ouverte : son selftest direct ne
compare que la forme ponctuelle et des boîtes dégénérées, et le header
géométrique ne porte ni `PointId` ni preuve de disjonction. Son prétest
intérieur porte le pire cas à 65 évaluations. Au pin `694920a`, le raccord WSPD
passe huit CTests ciblées : nominal non vacuaire, refus partiel et modes
vacuaires, fixture collinéaire et trois exécutions mutantes. Le parent
`8fd6f59` refuse en plus `--exige-q4-ouvert` sans `--juge-bjd` et une
cardinalité autre que neuf pour la fixture. Ces tests reçoivent le packing
causal sur leurs campagnes, mais ni une boîte non dégénérée dans le selftest
indépendant, ni `tau(F)`, ni chemin device.

L'ABI stable garde aussi une ambiguïté à supprimer : une `Base` invalide fait
retourner `kLaneNone`. Le callsite q4 courant échoue ouvertement parce qu'il ne
lit que `retour>=q4`, mais un futur consommateur pourrait prendre cette valeur
pour un vrai `NONE`. La réception exige `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`, ou
un `UNKNOWN` explicite pour toute invalidité. Le commentaire minimax de
`jung_dual.hpp` inverse ses quantificateurs, quoique les formules utilisent la
bonne identité.

Une fermeture de profondeur ne peut additionner des scalaires anonymes. Le
packing minimal conserve des groupes de `PointId` deux à deux disjoints et
disjoints de tout singleton/span déjà crédité dans la même vue. La route plus
forte conserve les groupes comme hyperarêtes et teste `tau(F)>=8/9`. Cap ou
juge sauté donne `PARTIEL/UNKNOWN`.

Le greedy égal-poids courant est un no-go comme hot path sous son ordonnance.
À `n=1500`, il réduit la masse q4 ouverte de `12,55 %` sur `uniform` et de
`0,87 %` sur `eight_clusters`, mais ne retire aucune des `32387961` et
`9366805` recertifications ; les médianes CPU utilisateur augmentent
respectivement de `5,47 %` et `8,15 %`. Il faut fermer la proof-tile avant la
descente, générer les bases par coupes et mesurer le coût aval réellement évité.
Un préfiltre exact proposé dans `PROPOSITION.md`, fondé sur le minimum bilinéaire
exact de `A0` et des intervalles coordonnée de `C0`, peut certifier `ALL` en
36 valeurs scalaires avant le fallback des 64 coins.

La dissection live à `eight_clusters,n=1500` trouve huit témoins singleton
exacts pour `89,5 %` de 200 PairId ouverts tirés par masse, contre `26,5 %` de
200 rectangles hachés. Ce signal favorise les microtiles et les preuves
uniformes, mais les deux taux ne se soustraient pas : PairId mass-weighted et
rectangles non pondérés ne sont ni appariés ni munis d'un intervalle, et le
second test n'exerce aucun groupe Jung.

L'ordre de descente live `--ordre-proche` réduit le pending sur ce même nuage
aux petits budgets, mais à finalité `window=256/512` il rend le même
`E4=1071162` que l'ordre Morton. Il compresse donc le budget du certificateur
central ; il ne réduit pas son résiduel géométrique et ne remplace ni Jung
collectif ni `BlockBallDepth8`.

La primitive entière associée ne décide pas elle-même l'existence de ces
poids : elle vérifie un vecteur rationnel fourni. Son contrat exige `D>0`, de
un à trois IDs authentifiés, des poids positifs, une somme capée et le profil
u16 ; tout échec du proposant reste `UNKNOWN`. Le wrapper, et non le tableau de
coordonnées, porte la disjonction des groupes. Une réception `k>1` exige un
juge exact de la faisabilité sur le disque continu ; un accord singleton avec
`SOC64` ou quelques centres tirés ne suffit pas.

Ce juge reste de taille constante par base : avec `u_z=a+b-2z` et `s=2w`, les
centres non couverts satisfont `2*s dot u_z>=D-||u_z||^2` dans le plan
`s dot (b-a)=0`. Le point de norme minimale de trois demi-plans au plus est
l'origine, une projection sur un bord ou l'intersection de deux bords. Le test
strict est `3*r^2>D` pour q3 et `2*r^2>D` pour q4. Une base couvrante `G`
certifie ensuite récursivement `Depth(P,h)` par tous les
`Depth(P minus {z},h-1)`, `z` dans `G`; le DAG porte au plus trois enfants par
niveau. Les groupes disjoints restent le fast path, cette récurrence le juge
borné. Sous `smax=11`, elle demande au plus `3280` recherches de base pour
`h=8` et `9841` pour `h=9`. Ces nombres viennent de la profondeur, pas des noms
q4/q3. L'oracle entier tient en i256 après réduction par
`g_i=D-||u_i||^2` et
`K_ij=D*(u_i dot u_j)-(u_i dot d)*(u_j dot d)` ; le replay Gram brut peut
dépasser 256 bits et reste GMP.

Le packing disjoint n'est pas complet. Une fixture u16 possède six témoins
universels, sept groupes couvrants disjoints au maximum, mais une profondeur
q4 exactement huit : `u=6<p=7<d=8`. Le hot path tente le packing, puis un
`ProofSpanDAG` de suppressions capé ; un cap produit `PENDING`. Sur un bloc CK,
chaque base proposée doit garder une marge uniforme ou provoquer un split du
proof-tile, jamais l'expansion des `PairId`.

Helly avec tolérance comprime en outre tout succès ponctuel. Pour les ensembles
fermés `B_z` de centres où `z` n'est pas intérieur, `Depth(P,h)` signifie qu'il
n'existe, pour **aucun** ensemble `R` de `h-1` IDs au plus, un point commun à
tous les `B_z` restants. Il existe
donc toujours un sous-pool qui certifie déjà le seuil `Depth>=h`, de taille
`eta(3,h)<(h+1)^2` : au plus **80 IDs pour q4** et **99 pour q3**. Un
`ToleranceKernel` porte ces IDs et le vérificateur rejoue exactement leur
arrangement dans le disque. Ce résultat borne le payload, pas sa recherche ni
le nombre de tuiles ; pour un rectangle, le même noyau doit être prouvé sur
tout `A×B` ou provoquer un split. Le détail et la source primaire sont dans
[`PROPOSITION.md`](PROPOSITION.md#noyau-de-helly-avec-tolérance-et-hypergraphe-exact).

La profondeur possède surtout une réduction combinatoire exacte. Former
l'hypergraphe de rang trois dont chaque hyperarête est une base Helly couvrante
donne `d=tau(E)`, le nombre transversal minimal ; le packing actuel n'est que
`p=nu(E)<=d`. Un branch-and-cut alterne alors un petit solveur de transversal
bitset et une HPI qui rend soit un contre-centre, soit une nouvelle base
disjointe. Le device vérifie chaque base géométrique une fois, puis rejoue la
preuve de `tau(E)>=8/9` sans refaire la géométrie à chaque branche.
Le rang de cet hypergraphe décroît avec le domaine des centres : trois IDs sur
le disque pair-level 2D, deux sur l'axe de face 1D, un à BallKey fixée. C'est le
même mécanisme de profondeur à travers q3 et q4.

Après une face aiguë, une seconde porte collective travaille en dimension un.
Sur le segment de centres `J_f` compatible avec `K_4(ab)`, chaque témoin porte
la forme affine `P_z(tau)=A_z-tau*B_z`; il est intérieur lorsque `P_z<0`.
Le seuil q4 fixe-face se décide exactement sans sweep globale : poser
`p=min(8,n_permanents)`, puis conserver les `8-p` seuils `tau<alpha` les plus grands et
les `8-p` seuils `tau>beta` les plus petits. Leur replay groupe les égalités
shell et forme un `AxisToleranceKernel` d'au plus **16 IDs**. C'est un scan
`O(n)` à mémoire `O(8)`, et la spécialisation constructive de
`eta(2,8)=16`; il préserve l'équivalence `Depth>=8`, pas la valeur numérique
au-delà de huit. Les bouts irrationnels exigent jusqu'à environ 207 bits sous
u16, donc i256/quatre limbs. La version bloc groupe une égalité uniformément
prouvée comme shell et ne scinde qu'un ordre indécis. La chaîne devient `Jung edge 2D -> carrier aigu -> noyau axe 1D
-> WST4 symbolique -> BlockBallDepth8 sur carrier×apex -> résiduel`. La sweep
par face n'est autorisée qu'après preflight ; il n'est pas nécessaire
d'énumérer les q4 pour commencer à prouver leur rang.

Le preflight M4 reste lui aussi factorisé. Chaque atome WST4 porte une masse de
quatre IDs distincts calculée par Möbius sur les quinze partitions des quatre
facteurs. Initialiser `M4_pending` à la masse non décidée ; pour une masse `m`,
`ALL_Q` fait `M4_pending-=m; M4_L+=m`, `NONE_Q` fait `M4_pending-=m` et
`MIXED_Q` remplace atomiquement le parent par ses enfants. Ainsi
`M4_U=M4_L+M4_pending`. `M4_L>B_fill` rejette seulement le fill ponctuel ;
`M4_U<=B_fill` certifie sa capacité, mais count final, offsets et publication
exigent encore `M4_pending=0`. Sinon continuer ou passer au shallow. L'identité exacte par arête/`PlaneKey` reste un
microkernel endpoint borné : à 50 000 points, un catalogue global aurait déjà
`1249975000` arêtes et `62496250050000` incidences `(e,z)`. Aucun de ces termes
soustraits n'est saturé séparément avant le résultat positif.

Deux ledgers restent distincts : `M4_raw_[L,U]` est pré-profondeur, tandis que
`residual_output_[L,U]` suit le fill après profondeur et positivité. Une
fermeture `ALL_INTERIOR` crédite `domain_mass_closed` et produit zéro sortie
résiduelle ; elle ne rend jamais `M4_raw=0`.

Le LP global reste un oracle utile, mais son échec ne prouve plus une pénurie
sur le disque Morse : une fixture à huit groupes ferme `JungDiskDepth8` alors
que le LP sur tout le plan échoue dès la profondeur un.

Pour `s_i=z_i-a`, `d=b-a`, `D=||d||^2`, `q_i=||s_i||^2`, poser :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

`G` crédite un intérieur sur toute sphère par `a,b` si et seulement si
`d` appartient au cône positif de `G` et `kappa_G(d)<D`. Un optimum basique
emploie au plus trois IDs. Huit extractions disjointes donnent un fast path q4;
un arbre de suppressions fournit un oracle complet de profondeur universelle
relativement au pool, jusqu'à 3280 appels LP pour `h=8`, donc q4 sous
`smax=11`. Cette propriété porte sur
toutes les sphères par la paire, pas seulement les supports Morse; un échec
reste fail-open pour la source. Ce dernier n'est pas un hot path.

### Pelages inversés collectifs

`OriginOnionDepth-h` inverse une banque autour de l'ancre, retire
successivement les sommets de `conv({0} union P)`, puis teste si la cible
inversée reste strictement dans la dernière coque sans chute de rang. Chaque
couche fournit alors un ID intérieur distinct dans toute sphère par la paire.
Une facette se rejoue sur un BNode par le test séparable
`v||d||^2-u dot d>=1`, sous environ 87 bits. `h=8/9/10` ferme q4/q3/q2.

Ce fast path reste universel. Sur la famille u16 à deux droites, tous les
certificats universels peuvent laisser `n^2/4` paires alors que tous les
triangles sont obtus et la vraie source q3/q4 est vide. La porte par carrier
aigu doit y rendre zéro sans développer de `PairId`.

### Cages de quatre à six sites

Une positive basis inclusion-minimale 3D peut avoir quatre, cinq ou six sites. Les cages
tétra-only sont donc incomplètes. Une cage de six facettes possède au plus huit
sommets de fleur. `SixRoleCageProposer` reste une ablation counter-only ; chaque
groupe doit être validé exactement, et réduire une cage impose de recalculer sa
fleur.

Les preuves, limites de largeur et contre-fixtures sont consolidées dans
[`audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Raffinement local : signal utile, coût non reçu

Le raffinement des seuls terminaux q4 non fermés réduit réellement `E4` à
`n=3000`, `s=8` : `eight_clusters` passe de `4 045 644` à `2 597 699` arêtes
résiduelles et `uniform` de `1 027 538` à `464 599`. Mais à profondeur quatre,
les recertifications passent respectivement de `31 538 327` à `199 169 436`
et de `108 858 186` à `193 020 841`. Sans `F3/C4_carrier/F4/M4_apex`,
BallKeys, census et fold, il
est faux de conclure que le levier « paie ».

La télémétrie de tête double-compte les parents ensuite scindés et imprime
jusqu'à `380,15 %` de masse q2 fermée. Le ledger terminal
`CLOSED/OPEN/PENDING` reste cohérent ; les compteurs de tentatives doivent être
séparés de l'objet final.

La réparation algorithmique proposée est `ProofCarryingLocalRefinement` : un
enfant hérite des CNodes témoins déjà `ALL`, des `NONE` et de leurs IDs ; seuls
les `MIXED` sont rejoués. Cela évite de repartir de la racine à chaque split et
se prête à `count--scan--fill` avec continuations persistantes.

La recette G4 a depuis été exécutée sur CPU. Les quarante processus rendent
zéro, mais `terrain` conserve des continuations q3/q4 à 25 000 et 50 000 : ses
`sum_E4` sont des surensembles, pas des fenêtres finales. Sur
`eight_clusters`, `pending=0` et les pentes restent proches de `1,9` après
profondeur quatre ; cela réfute la configuration centrale mesurée, pas tous les
certificateurs rectangle. Le rapport `1,62` contre `1,57` compare en outre des
unités différentes et ne mesure pas un prix. Détails et réponses :
[`audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md)
et
[`audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## Dégénérescences et sortie lourde

Le profil de coordonnées u16 n'exclut pas les cosphères. Le domaine candidat
utilise une politique `RelevantGP` fail-closed : un extra-shell pertinent rend
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture du domaine reste elle-même à recevoir.

Un `SphereRun` interne conserve l'identité et le census pour garder la décision
réversible. Il n'autorise pas un plateau public. Un quotient saturé ne devient
valide qu'après reconstruction des lots, dix forêts, coverage et verticales.
Si le contrat exige chaque `SupportKey`, une cosphère lourde est une borne de
sortie ; ni RLE ni streaming ne suppriment ce travail.

## Porte de coût

Une pente `sum_E4` ne qualifie rien seule. Chaque campagne publie au minimum :

- masses exclusives `CLOSED/OPEN/PENDING`, avec `pending=0` pour une fenêtre
  finale ;
- `E3/E4`, maximum par ancre, `F2`, `FaceBlock`, q3 shallow,
  `T4_site/acute_face`, accepts primary, morts `T2/permanent/gap`, groupes de
  racines et ties, puis `W4_positive/H4_rank` ;
- `N4_event`, `Z4_const`, `R4_bundle` et `T4_site` séparés ;
- `BallKey` brutes/uniques, supports, census et tailles de shell ;
- sorties `H`, octets, HWM, opérations larges et temps par phase ;
- commandes, seeds, commit, diff, binaire et codes de sortie.

Les diagnostics CPU existants ne sont pas des modèles G4. Aucun petit cutoff
kNN universel n'est reçu : une fixture u16 de exactement 50 000 IDs conserve un
q4 régulier vide tout en plaçant au moins 12 499 distracteurs devant chacun de
ses trois partenaires, depuis chacun de ses quatre sommets. Elle réfute toute
source ancrée `k<=12499`, pas toute architecture locale imaginable. Aucun
arrangement global, aucune structure de Delaunay de quelque ordre que ce soit
et aucun catalogue exhaustif ne deviennent le chemin produit.

Au pin historique `cec4a4f`, le sampler v2 retire `PENDING` et la censure des grosses
lentilles, puis remplace `2 sigma` par une demi-largeur Hoeffding correcte sous
des tirages i.i.d. uniformes. Son implémentation ne reçoit pas encore cette loi :
multiply-high reste sans rejet, les streams SplitMix n'ont pas de contrat
d'indépendance, le delta n'est pas réparti sur les décisions simultanées et
`W4` n'a pas d'intervalle. Le contrôle ne compare pas le décodage à un mode
exhaustif déterministe. `--rang` peut en outre réussir sans `--porteurs`, ignore
les extra-shells et mesure un échantillon conditionnel non pondéré, pas `H4/W4`.

Le nouveau `q4_brute_oracle` reçoit seulement une énumération exhaustive
bornée. Son claim `M4=Theta(n^4)` pour tout nuage est faux : sa propre famille
`two_lines` donne `M4=0`. Ses prédicats recopient le Gram--Cramer/in-sphere du
sujet, son `H4` teste seulement les intérieurs et oublie le shell, et les cas
vides impriment encore certains `NaN`. Les cinq CTests verts ne réparent pas
ces défauts.
Une construction ouverte en coordonnées réelles prouve néanmoins qu'une masse
q4 bien centrée peut être quartique **avant rang**. L'ancienne instanciation par
cubes unitaires u16 n'était pas uniforme (`2093/4096` supports seulement). La
fixture exacte mise à l'échelle utilise les nœuds
`A=(20000,20000,20000)+{0,1}^3`,
`B=(30000,30000,30000)+{0,1}^3`,
`C=(19000,31000,31000)+{0,1}^3`,
`D=(31000,19000,31000)+{0,1}^3` et
`Z=(20000,20000,30000)+{0,1}^3`, avec quarante `PointId` distincts. Ses `4096`
supports sont q4 positifs et les huit IDs de `Z` sont uniformément intérieurs. Par convexité du déterminant
in-sphere normalisé en `z`, huit coins certifient `ALL_INTERIOR`; un seul bloc
du classifieur standalone actuel ferme avant fill. Son raccord à de vrais
supports owner/positifs reste ouvert. Les coins ne certifient pas le verdict inverse
`NONE`.

Ce pin contient aussi le juge direct des flips SOC et `JungDual`. Le premier
tue le mutant de somme lorsqu'il énumère tout, mais imprime encore `accord=OUI`
si son cap laisse des flips non jugés. Le second a une identité entière correcte
sous `sum(weights)<=65535` ; il ne fait cependant qu'essayer sept pondérations,
n'impose pas son cap dans le header et possède un mutant de largeur à overflow
signé. Le juge primal indépendant reçoit une base `k=2`, mais pas encore le cas
ternaire, la profondeur ni la tuile uniforme. Le HEAD inclut aussi les diagnostics de
feuilles et d'ordre de descente ; leurs cohortes et coûts restent à recevoir.

Le raffinement local réduit effectivement `E4`, mais ses parents et enfants
sont encore mélangés dans plusieurs compteurs de tentative. Son coût doit être
jugé après séparation `AttemptStats/TerminalLedger` et avec héritage des preuves
`ALL/NONE`. La campagne CPU G4 a calculé ses pentes après coup, coupe les
métriques physiques et n'exige pas la finalité ; elle n'est pas une campagne de
qualification. Voir
[`audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md)
et son contre-reçu au pin `35fcea8`.

Le détail live, les hashes, les reçus G4 et le worktree ne sont maintenus que
dans [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Les
paragraphes ci-dessus bornent les non-claims et les obligations mathématiques.

## Construire et tester

Depuis la racine du dépôt :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
python3 tools/check_docs.py
```

CUDA reste opt-in avec `-DMHGP3V_ENABLE_CUDA=ON`. Une session GCP éventuelle
doit suivre exclusivement les scripts gardés et les coupe-circuits décrits par
`AGENTS.md`.

## Arborescence documentaire

- [`PROPOSITION.md`](PROPOSITION.md) : proposition technique et mathématique
  consolidée ;
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : unique
  verdict mutable ;
- [`audits/README.md`](audits/README.md) : index court des audits actifs et des
  dépendances historiques encore citées par le logiciel ;
- `oracle/` : juges bornés indépendants ;
- `prototype/` : candidats et probes, sans autorité produit implicite ;
- `receipts/` : diagnostics et reçus, dont le statut est fixé par l'audit.
