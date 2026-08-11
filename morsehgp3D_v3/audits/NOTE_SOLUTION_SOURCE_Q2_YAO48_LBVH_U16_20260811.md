# Note de solution — source q2 par Morton/LBVH, Yao48 strict et census fermé (profil u16)

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note spécifie la **route produit candidate** de la lane q2 décrite par
l'audit courant et par
[`../PROPOSITION.md`](../PROPOSITION.md) §6.2. L'architecture mathématique est
celle de
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md)
(théorème de coupe directionnelle, classification terminale, ledger),
respécialisée au profil u16 de v3 où **toute l'arithmétique décisive tient en
`i64`/`i128` sans cascade dyadique**. Le statut logiciel appartient à
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

Cette architecture n'est pas à réimplémenter de zéro. La ligne enregistrée
contient déjà, comme composants séparés, une frontière CUDA Morton/Yao48
reprenable et un classifieur multi-rang `count--scan`. Leur ancien contrat de
rang fermé accepte une égalité dans le prune et peut s'arrêter sur des contacts;
ces décisions ne sont donc pas celles de v3. Leur portée, leurs mesures et
leurs limites de réemploi sont détaillées dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md).
Ils servent de différentiels et de source de contrats, jamais d'autorité v3.

## 1. Objet calculé

Pour le contrat `hgp_reduced_normalized_h0_v3` à `K=10`, la lane q2 doit
partitionner exactement les `C(n,2)` paires :

- **tombstone** : la paire possède au moins dix `PointId` distincts, hors
  extrémités, strictement intérieurs à sa boule diamétrale — `p>=10`, donc
  `p+q>=12`, bloc H0-inerte avec resolver latent ;
- **record fermé** : sinon, le census fermé complet
  `C(u,v)={x : Phi_{u,v}(x)<=0}`, sa profondeur stricte, sa coquille, son rang
  et son niveau `beta=D^2/4` sont publiés en une passe.

Le prédicat est `Phi_{u,v}(x)=(x-u) dot (x-v)` ; sur u16, `|Phi|<3*2^34` tient
en `i64` avec marge. Le classifieur ponctuel ne suppose aucune position
générale : les égalités `Phi=0` sont des contacts de coquille, comptés fermés
et jamais intérieurs.
Cette robustesse du classifieur ne change pas le domaine produit initial de la
spécification : les positions 3D doivent être deux à deux distinctes et la
politique `RelevantGP` doit accepter le shell utile. Une paire de `PointId`
colocalisés a `D^2=0` et n'est pas un support propre positif q2; elle relève
d'un rejet d'entrée ou d'une future agrégation pondérée, jamais d'une
activation q2 ordinaire.

## 2. Structures résidentes

1. **Ordre Morton** : clé 48 bits (trois axes u16 entrelacés), paires
   `(clé, PointId)` triées ; l'ordre canonique des ex æquo est le `PointId`.
2. **LBVH radix** : arbre binaire sur l'ordre trié, coupé au bit dominant de
   la première différence de clés. Lorsque toute la plage partage la même clé,
   le fallback partage au milieu de l'ordre secondaire `PointId`, ce qui garde
   les colocalisés déterministes. Chaque nœud porte sa boîte AABB u16 exacte et
   sa plage `[begin,end)` de positions. Le prototype CPU matérialise ce contrat;
   une construction device de type Karras reste à recevoir séparément.
3. **Ownership exact une fois** : la paire `(i,j)` avec `pos(i)<pos(j)` est
   possédée par l'ancre de position haute `j`, qui ne parcourt que le préfixe
   `[0,pos(j))`. La masse totale possédée est `somme_j pos(j) = C(n,2)`.
   Cette identité permet des reçus de régions sans tableau global; elle ne
   prouve toutefois ni une complexité sous-quadratique, ni l'absence d'un
   traitement ponctuel de tout l'univers.

## 3. Coupe Yao48 stricte fail-open

Par ancre `p`, 48 chambres semi-ouvertes (8 octants × 6 permutations par
magnitudes décroissantes ; les égalités de magnitudes sont routées par une
règle totale documentée — la chambre est un choix de TRAVAIL, seule la preuve
compte). Le certificat engage `K=10` témoins de `PointId` distincts de `p` et
de la cible, avec `D` = maximum de leurs distances carrées à `p` (les plus
proches donnent le meilleur `D`, l'optimalité n'est pas requise). Une banque
sous-pleine n'autorise **aucun** cutoff : fail-open.

Pour une cible `q` de coordonnées canoniques `(x,y,z)` dans la chambre, les
trois inégalités **strictes**

$$x^{2}>D,\qquad (x+y)^{2}>2D,\qquad (x+y+z)^{2}>3D$$

certifient dix intérieurs **stricts** distincts (théorème de coupe
directionnelle, variante stricte) : la paire `(p,q)` est tombstonée sans
visite. Sur une boîte entière du LBVH contenue dans la chambre, les minima par
axe donnent le même certificat pour toutes ses feuilles : le nœud est remplacé
par un **reçu de masse**. Toute égalité descend ; l'échec du certificat ne
classe rien (non-converse gravé en fixture).

Si la cible appartient aux dix candidats certifiés comme les plus proches,
`x^2>D` échoue automatiquement, et cet échec n'est pas un faux négatif
évitable par le onzième. En effet, `A(p;q,w)>0` implique
`||w-p||<||q-p||`; une cible du top-10 possède moins de dix témoins stricts
possibles dans la chambre. Le mode top-nearest exact conserve donc `K=10`.

Pour un réservoir arbitraire, notamment une banque issue d'antichaînes, la
cible peut occuper un des dix slots alors que dix autres témoins utiles
existent. Ce mode conserve `K+1=11` candidats, exclut `q` et recalcule `D` sur
les dix engagés. Si une chambre contient `t<=10` points hors ancre, chaque
cible y possède au plus neuf autres témoins et ce certificat précis est
impossible; `t=0` ne porte aucune cible. La table factorisée des onze candidats
est immuable. Le reçu engage un masque de onze bits dont exactement dix sont
levés; une enveloppe radiale engage un couple `(bank_index,mask)` distinct pour
chaque chambre. La sélection ne peut pas être un état mutable de la banque.
Pour une boîte de cibles, préférer une antichaîne témoin disjointe de la boîte
à une banque de taille `K+|Q|`.

### Banque compressée par antichaîne

La recherche des dix plus proches n'est pas une obligation mathématique. Une
banque peut être certifiée par une antichaîne de nœuds LBVH dont les plages de
feuilles sont disjointes, excluent l'ancre, sont entièrement contenues dans la
même chambre, ne contiennent que des témoins de distance strictement positive
à l'ancre et ont une masse totale au moins dix. Le preflight de positions 3D
distinctes garantit seulement la positivité des distances; l'antichaîne et
son reçu doivent certifier séparément une masse totale au moins dix. Toute
extension aux sites pondérés devra recertifier ces deux obligations. Poser :

$$D_c=\max_i\max_{x\in\mathrm{box}(W_i)}\left\Vert x-p\right\Vert^{2}.$$

Dix feuilles canoniques distinctes de leur union sont alors de vrais témoins
de distance carrée au plus `D_c`. Le reçu chaud conserve plages, masses et
majorant; le juge borné les développe en `PointId`. Raffiner une banque pour
réduire `D_c` est une optimisation guidée par la masse cible, pas une condition
d'exactitude.

Une seconde coupe sûre évite d'exiger une chambre unique pour toute une boîte.
Si une cible échoue à au moins une des trois inégalités Yao, sa distance carrée
à l'ancre est au plus `3D_c`. Pour une boîte dont l'ensemble conservateur des
chambres possibles est `S`, toutes ses feuilles sont donc prunables si toutes
les banques de `S` sont pleines et si :

$$\mathrm{dist}^{2}(p,\mathrm{box})>3\max_{c\in S}D_c.$$

Toute égalité descend. Le reçu engage `S` et toutes les banques référencées;
une fixture traversant une frontière de chambres tue l'oubli d'une chambre.

Cette enveloppe radiale est seulement un premier filtre. Une coupe plus forte
résout exactement, pour chaque chambre compatible `c`, l'intersection entre la
boîte cible et le cône signé/permuté. Dans les magnitudes canoniques
`x>=y>=z>=0`, après clipping signé de la boîte en intervalles
`[lx,ux]`, `[ly,uy]`, `[lz,uz]`, poser `z0=max(lz,0)`,
`y0=max(ly,z0)` puis `x0=max(lx,y0)`. L'intersection du cône fermé est vide si
`z0>uz`, `y0>uy` ou
`x0>ux`; sinon les minima simultanés de `x`, `x+y` et `x+y+z` sont exactement
`x0`, `x0+y0` et `x0+y0+z0`. Employer le cône fermé sur les frontières ne peut
qu'ajouter des points fantômes et reste donc fail-open pour les chambres
semi-ouvertes du routage ponctuel.

Pour chaque intersection non vide, la banque doit être pleine et les trois
comparaisons strictes `x0^2>D_c`, `(x0+y0)^2>2D_c` et
`(x0+y0+z0)^2>3D_c` doivent toutes passer. Si cette condition tient pour
chaque chambre non vide, toute cible de la boîte satisfait sa coupe Yao et le
nœud entier est prunable. Un masque 48 bits propagé sous raffinement, les
versions de banques et les bornes entières forment le reçu; toute égalité
descend. Aucun solveur flottant ni 48 résolutions génériques par enfant n'est
nécessaire.

### Certificat collectif boîte cible--nœud témoin

Les banques directionnelles ne sont pas la seule manière d'exploiter les
témoins déjà trouvés. Fixer une ancre `p`, une boîte cible `Q` et un nœud témoin
`W`. Pour `d=q-p` et `s=w-p`, poser
`A(d,s)=d\mathbin{\cdot}s-\left\Vert s\right\Vert^2`. Alors
`A(d,s)>0` est exactement le prédicat `Phi_{p,q}(w)<0`.

Pour chaque axe `i`, soit `D_i=[d_i^-,d_i^+]` la projection de `Q-p` et
`S_i=[s_i^-,s_i^+]` celle de `W-p`. Le minimum continu exact sur le produit des
deux boîtes est :

$$L_p(Q,W)=\sum_{i=1}^{3}\min_{d_i\in\left\lbrace d_i^-,d_i^+\right\rbrace,\ s_i\in\left\lbrace s_i^-,s_i^+\right\rbrace}\left(d_i s_i-s_i^2\right).$$

En effet, pour `d_i` fixé la fonction est concave en `s_i`, donc son minimum
est à une extrémité; pour `s_i` fixé elle est linéaire en `d_i`, donc encore à
une extrémité. Quatre couples de coins par axe suffisent. Ce minimum de boîtes
est un minorant conservateur pour les feuilles réelles; ses coins fantômes ne
peuvent créer qu'un échec fail-open.

Si une antichaîne authentifiée de nœuds `W_j` possède des plages de feuilles
deux à deux disjointes, vérifie `L_p(Q,W_j)>0` pour chaque nœud et totalise au
moins dix `PointId`, chaque cible réelle de `Q` possède dix témoins stricts
distincts. Le nœud `Q` est donc tombstone sans classifieur ponctuel. `L=0`
descend. Un parent et son descendant ne sont jamais crédités ensemble; une
masse de boîte ne remplace pas la cardinalité de sa plage. Sous raffinement
`Q' subset Q` et `W' subset W`, le minimum ne peut qu'augmenter. Un enfant
cible `Q'` peut donc hériter par référence du **même** nœud témoin `W`, avec les
mêmes identité, plage, masse et version. Si `W` est remplacé par un descendant
`W'`, son identité, sa plage et sa masse changent et le crédit doit être
reconstruit; aucun crédit ne survit non plus à une fusion, un changement
d'ancre ou une mutation du LBVH.

Le profil u16 donne `|A|<=3*65535^2<2^34`; les soustractions et produits sont
élargis avant calcul en `i64`, puis rejoués en `i128` par le juge. Le reçu lie
digest du nuage, epoch du LBVH, ancre, plage cible possédée et, pour chaque
`W_j`, plage, AABB, masse et valeur de `L`. Le juge borné développe les feuilles,
recalcule indépendamment `4*Phi` et exige dix identifiants distincts pour chaque
cible. Les mutants minimaux omettent un coin croisé, changent `>` en `>=`,
créditent parent et enfant, emploient la taille de boîte au lieu de la plage ou
réutilisent un epoch périmé.

L'architecture de test est une traversée duale persistante : un état
`(target_node,witness_frontier,credited_antichain)` raffine d'abord les nœuds
témoins ambigus, puis partage la cible si le même doute persiste. Les crédits
positifs suivent les enfants par référence; aucun rescan de la racine témoin et
aucune matrice `Q times W` ne sont admis. Les feuilles cibles restées sous dix
retombent au classifieur/census exact.

Le split de cible doit conserver le sibling comme domaine témoin. Si
`Q=Q_L union Q_R`, les points de `Q_R` étaient exclus des témoins du parent mais
deviennent admissibles pour `Q_L`, et réciproquement. La frontière ambiguë du
parent doit donc garder les domaines qui chevauchent `Q`; chaque enfant les
hérite et les reclassifie. Une insertion séparée du sibling n'est nécessaire
que si l'implémentation l'avait retiré, et exige alors une déduplication des
plages. Les nœuds dont un majorant exact donne `A<=0` restent éliminés; les
nœuds ambigus persistent ou se raffinent. Une arène immuable avec partage
structurel évite les copies sans perdre ni doubler ces nouveaux témoins.

### Certificat aux deux extrémités

L'ownership Morton ne contraint pas le côté du certificat. L'unique owner d'une
paire `{u,v}` peut essayer la coupe centrée en `u`, puis, si une banque
certifiée de `v` est disponible dans la même tuile ou dans un cache borné et
authentifié, la coupe symétrique centrée en `v`. La paire est tombstonée si
l'un des deux certificats stricts passe; elle reste émise exactement une fois
par son owner de position haute. Le reçu engage le `PointId` choisi comme
centre, la version de banque et le côté utilisé. L'orientation inverse est une
optimisation facultative, jamais une condition de complétude.

Cette symétrisation est exacte même si l'autre extrémité appartient à la
banque : alors `D` majore `||u-v||^2`, tandis que la première coupe exige
`x^2>D` avec `x^2<=||u-v||^2`; le certificat échoue donc automatiquement. La
banque ponctuelle chaude conserve l'enveloppe `O(B*48*K)` pour le top-nearest
ou `O(B*48*(K+1))` pour un réservoir arbitraire de `B` ancres actives; toute
table globale correspondante est interdite. À titre de diagnostic seulement,
une table de onze candidats à 50 k occuperait 105 600 000
octets si les identifiants sont des positions Morton `u32` avec mapping
authentifié, mais 211 200 000 octets avec les `PointId u64` de la ligne
enregistrée, hors `D_c`, masques et offsets. Une porte optionnelle compare les
sorts mono-côté et bi-côté à l'oracle et exige un gain strict non vide du
second côté avant d'en payer le cache.

## 4. Classification terminale et census fermé

Chaque paire survivante `(u,v)` est classifiée par un parcours LBVH avec les
bornes exactes par boîte déjà reçues dans la lane self-join : l'infimum
séparable `L4` et le supremum `U4` de `4*Phi` sur la boîte.

- `L4>=0` : aucun point strict dans la boîte — retirée de la recherche
  d'intérieurs, **rescannée** obligatoirement pour le census fermé si
  `L4=0` peut porter des contacts ;
- `U4<0` : toute la boîte est strictement intérieure — crédit en bloc ;
- sinon descente, feuilles au prédicat exact.

L'arrêt anticipé à dix intérieurs stricts émet la tombstone ; sinon le census
fermé complet est publié (liste `C(u,v)`, profondeur stricte `p`, coquille,
rang fermé, niveau). Le rescan de census ne peut pas être évité par la
recherche stricte : contacts et intérieurs sont deux comptes.

## 5. Ledger et refus

Le ledger ferme simultanément, par lane et par run :

1. `candidate + certified_pruned + unresolved = C(n,2)` avec résidu nul pour
   une publication exhaustive (`unresolved>0` = refus atomique, jamais une
   sortie partielle) ;
2. la partition `tombstone + census` des candidates classifiées ;
3. multiplicité canonique un par paire (ownership rejoué) ;
4. l'identité du nuage (digest), du `leaf_size`, de l'ordre Morton et du
   seuil.

L'égalité globale des masses ne suffit pas : une omission et une duplication
de même cardinal pourraient se compenser. Le reçu produit ferme aussi, pour
chaque ancre, la masse attendue `pos(j)`, les intervalles de régions disjoints,
les cibles ponctuelles et le digest canonique de leur union. Les banques
immuables sont factorisées par `(ancre, chambre, version)` afin que les reçus ne
recopient pas leurs onze identifiants. Chaque reçu référence en plus son masque
d'engagement propre; le juge en recalcule les dix `PointId`, `D`, l'exclusion de
la plage cible et les inégalités strictes.

Aucun tableau global de paires ni de banques ponctuelles `n*48*K` ou
`n*48*(K+1)` n'est matérialisé : les
survivantes du mode mesure sont comptées et hashées, pas stockées; le mode
oracle borné (`n<=256`) tient les sorts par paire pour le juge.

## 6. Juge indépendant et différentiel

Le juge borné réécrit sa propre arithmétique (audit : « le juge de couverture
ne partage pas les prédicats décisifs du sujet ») :

- prédicat recalculé sous la forme distincte
  `4*Phi = ||2x-u-v||^2 - ||u-v||^2` en `i128`, jamais la forme produit du
  sujet ;
- scan exhaustif de chaque paire contre tous les points (`Theta(n^3)`), sans
  Morton, sans LBVH, sans chambres ;
- comparaison de **tous** les sorts (tombstone/census), de toutes les
  profondeurs strictes, de tous les rangs fermés et de tous les census.

Le différentiel bi-mode du sujet (baseline sans coupe Yao48 ni prunes de
boîtes, classification terminale seule) doit rendre des sorts et masses
identiques ; il mesure le gain, il ne juge pas la vérité.

## 7. Portes exigées (planchers, fixtures, mutants)

- planchers : reçus de région, tombstones ponctuelles, tombstones du
  classifieur, census, chambres sous-pleines et survivantes doivent tous être
  exercés par au moins une campagne qui échoue au code 3 si le plancher mord ;
- fixtures : non-converse avec contact exact, égalité
  `(x+y+z)^2=3D` qui doit descendre, prune positif d'une région, chambres
  sous-pleines fail-open, extrêmes u16 et points colocalisés diagnostiques ;
- mutants à code 4 : `strict-to-large`, `d-understated`,
  `chamber-perm-swapped`, `ownership-doubled`, `last-region-omitted`,
  `census-skips-inf-zero`, `threshold-minus-one`,
  `witness-subtrees-overlap` et `chamber-mask-omitted` ;
- politiques de travail : valeurs minimale et ample de la patience et du
  remplissage des banques rendent les mêmes sorts et census. Dans le seul probe
  diagnostique, un plafond de travail annoncé est contrôlé avant et après
  chaque unité comptable, inclut visites de banques, tas, tests ponctuels et
  piles, et ne réussit jamais après l'avoir dépassé; ce plafond n'existe pas
  dans le chemin produit ;
- équivariance : plusieurs permutations des `PointId` rendent le même ensemble
  canonique de sorts et les mêmes records fermés après renommage.

## 8. Exposants avant toute latence

Publier par famille (`uniform`, `terrain`, `scanline_single_pass`,
`scanline_overlap_multiecho`) à `12 500/25 000/50 000` : visites de nœuds,
tests de chambres, opérations de tas, tests `Phi` ponctuels des voies arbre et
liste, banques sous-pleines, survivantes, tailles de census, octets réels et
high-water. Les masses logiques prunées et terminales sont publiées comme
diagnostic de sélectivité, mais leur croissance proche de `n^2` n'est pas du
travail lorsqu'un reçu compact les représente. Deux exposants consécutifs d'un
même compteur de travail ou de stockage au-dessus de `1,35` classent la route
`NO-GO` avant tout port ou toute campagne de latence. Publier au moins six
décimales lorsqu'une pente est proche du seuil; la comparaison de référence est le self-join q2
historique (53 à 724 millions de visites `L4`, 86 millions à 1,365 milliard de
tests ponctuels à 50 k).

Le pire cas de SORTIE reste quadratique (graphe de Gabriel dense) ; la gate
d'exposant juge le régime des familles G4, pas un théorème universel. Une
insuffisance de ressource refuse atomiquement.

GCP non utilisé pour cette note.
