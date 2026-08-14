# Réponse d'audit Q15--Q17 — profil, complétude de J0 et axe q4

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette réponse ne reçoit aucun logiciel et n'autorise aucune session G4. Elle
répond à
[`QUESTIONS_CLAUDE_PROFIL_ENTREE_ET_J0_20260814.md`](QUESTIONS_CLAUDE_PROFIL_ENTREE_ET_J0_20260814.md).
Les trois lanes restent autonomes et aucune structure de Delaunay, d'aucun
ordre, n'est admise.

## Réponse courte

- **Q15 : deux profils à statuts distincts.** La tranche v3 courante reste
  strictement u16 et ne qualifie que la géométrie des coordonnées quantifiées.
  Le contrat normatif binary64 reste ouvert et ne doit pas être réécrit en u16.
  Un arrondi ponctuel fixe, non injectif et sans side-channel ne préserve pas
  exactement MorseHGP3D ; une similarité-lattice exacte ou un certificat de
  stabilité complet propre à l'entrée peut en revanche autoriser une
  équivalence.
- **Q16 : le refus a posteriori à `0,75*dmax` ne suffit pas.** Il donne un
  diagnostic tronqué, jamais un chiffre exact de dimensionnement. Un chiffre J0
  exact exige une partition neutre de toutes les ancres et, pour chaque masse
  sautée, un certificat de localité ou une continuation résiduelle. La preuve
  par calottes peut fermer certaines incidences d'ancre ; le reste doit demeurer
  explicitement complet.
- **Q17 : oui pour un `Q4Seed3` singleton, non pour un `FaceBlock` variable.** La
  descente best-first témoin est exacte une fois `a,b,x` fixés. Elle ne certifie
  ni un bloc de seeds variables, ni la génération complète des seeds. Le bloc
  peut être un conteneur de scheduling, jamais une autorité géométrique fondée
  sur ses coins.

## Q15 — garder u16 maintenant, ouvrir binary64 séparément

La spécification normative ne décrit pas une entrée u16 : son §2 interprète les
coordonnées IEEE-754 fournies par l'utilisateur comme des dyadiques exacts et
interdit toute normalisation qui change les distances. Le profil u16 est donc
une exploration spécialisée, pas un remplacement silencieux du contrat
binary64.

Il n'existe pas de théorème universel disant qu'un arrondi ponctuel fixe et non
injectif sur une grille finie préserve le HGP sans information annexe. Le
contre-argument tient déjà sur une boule diamétrale dans un plan de l'espace.
Pour `a=(0,0,0)`, `b=(2,0,0)` et un `epsilon` binary64 représentable inférieur
à la demi-maille locale, les points `z_-=(1,1-epsilon,0)` et
`z_+=(1,1+epsilon,0)` sont respectivement strictement intérieur et strictement
extérieur à la boule de diamètre `ab`. Une quantification assez grossière les
envoie sur le même mot de grille. La même entrée quantifiée ne peut donc être
exactement équivalente aux deux entrées binary64 : la `BallKey` du support `ab`
reste la même, mais `I_B` et le rang diffèrent, tandis que le point quantifié
devient shell. En ajoutant `smax-2` points distincts sûrement intérieurs,
`z_-` fait mourir q2 et `z_+` le laisse accepté. Les égalités, la positivité et
l'ordre de niveaux ont la même fragilité aux frontières.

La version universelle sous l'hypothèse annoncée vient du principe des tiroirs.
Un quantificateur ponctuel fixe vers u16 possède au plus `2^48` sorties ; parmi
plus de `2^48` triplets entiers binary64 d'un cube sûr, deux positions
distinctes `x,y`, distantes d'au moins un, ont la même image. Prendre autour de
`x` une paire dyadique `a=x-e_1/4`, `b=x+e_1/4`, puis `smax-2` autres témoins
dyadiques distincts strictement dans sa boule diamétrale. Deux nuages qui ne
diffèrent que par le remplacement du témoin `x` par `y` ont exactement la même
entrée quantifiée ; q2 porte pourtant `smax-1` intérieurs et meurt dans le
premier, contre `smax-2` et accepté dans le second. Aucun encodage ponctuel fixe
sans side-channel ne peut donc préserver universellement le contrat. Un
encodeur global adaptatif avec métadonnées est un autre objet et exige son
propre contrat.

Ce no-go ne vise pas une représentation globale adaptative certifiée. Il existe
même un pont exact simple : si tous les points binary64 vérifient
`p_i=t+h*q_i`, avec les mêmes `t`, `h>0` exacts et des `q_i` u16, la similarité
préserve incidences, signes, rangs, shells, supports et ordre des niveaux ; les
rayons carrés sont multipliés par `h^2`. Cette similarité doit être enregistrée
comme le demande la spécification. Hors de ce cas, une politique adaptative
doit être spécifiée et certifiée avant de revendiquer une équivalence.

La décision industrielle correcte est donc :

1. continuer la v3 sous `quantized_u16_input_only`, en gardant
   `public_status=not_claimed` ; `exact_on_quantized_coordinates_only` peut au
   plus nommer une capability bornée avant l'ouverture d'une phase formelle ;
2. conserver inchangé le plan de test binary64 normatif ; aucune mesure u16 ne
   qualifie son SLO ni son payload ;
3. ouvrir ultérieurement un profil binary64 distinct, avec décodage dyadique
   exact, filtres GPU unilatéraux et fallback entier/multiprécision ;
4. si une quantification amont approchée est proposée, la déclarer
   approximation, sauf certificat **par entrée** couvrant une base complète de
   décisions, zéros/ties, source, owners et ordre des niveaux. Certifier seulement
   la trace consommée serait circulaire : une omission en amont pourrait avoir
   empêché la décision qu'il fallait comparer.

Le profil u16 à 30 millions peut être utile comme stress de plateaux et de
positions colocalisées. Il ne permet pas d'inférer que le régime binary64 réel
possède les mêmes dégénérescences. La cardinalité seule ne rend d'ailleurs pas
les collisions dominantes : sous occupation uniforme des `2^48` cellules, 30
millions de points donnent environ 1,6 paire en collision en espérance. Les
plateaux d'un LiDAR surfacique dépendent de la distribution et de la politique
de quantification ; ils doivent être mesurés.

## Q16 — la boule de requête est exacte, la liste d'ancres ne l'est pas

Une fois une arête owner `ab` de longueur `D_e` connue, les bornes géométriques
de la question sont utilisables. Les autres sommets d'un support de diamètre
`D_e` sont dans la lentille des deux boules de rayon `D_e`, donc à distance au
plus `sqrt(3)*D_e/2` du milieu. Pour un support positif en dimension trois, Jung
donne un rayon au plus `sqrt(3/8)*D_e` et un décalage du centre au milieu au plus
`D_e/sqrt(8)` ; tout point de la boule fermée, intérieur ou shell, est donc dans
la boule du milieu de rayon
`(sqrt(3)+1)*D_e/(2*sqrt(2))`, strictement inférieur à `D_e`. La requête
`B(m,D_e)` est ainsi un sur-ensemble sûr du census conditionnel à cette ancre ;
le test de puissance exact reste nécessaire pour obtenir `I_B/U_B`.

Cela ne prouve rien sur les paires qui n'ont pas été énumérées. Le maximum des
diamètres **observés** est une statistique conditionnée par `D<=dmax` : une
unique ancre pertinente à `0,8*dmax`, sans aucune sortie près de la frontière
intérieure, laisse passer la garde `max_observe<0,75*dmax` tout en étant omise.
Le refus a posteriori ne peut donc publier que
`truncated_candidate_lower_bound`, jamais `source_complete` ni un exposant de
sortie exact.

La sonde ajoutée ensuite à `acd792d` falsifie déjà ce garde sur une famille
intégrée. `two_lines,n=10` rend code zéro, vingt q2 et
`diam_max/dmax=0,007`; son mode `--verifie` trouve quarante-cinq q2. Même
`--dmax-espacements=64` conserve vingt contre quarante-cinq. Cette fixture doit
rester permanente : elle transforme le no-go informationnel en régression
exécutable.

J0 exact doit porter le reçu suivant :

1. `NeutralPairPartition` couvre chaque paire non ordonnée exactement une fois ;
2. tout parent remplacé est conservé par l'identité parent--enfants, sans trou
   ni double compte ;
3. toute masse non descendue possède un certificat exact propre à la lane, ou
   reste une continuation ;
4. les calottes emploient séparément les seuils de mort `10/9/8` pour q2/q3/q4
   et donnent, endpoint par endpoint, une borne sur le diamètre `D_B=2R` de la
   boule. Le cutoff porte, lui, sur `D_e`. La composition simple sûre est un
   certificat `D_B<r` avec `r<=dmax`, puisque `D_e<=D_B`. Une paire est
   résiduelle tant qu'aucun de ses endpoints ne la ferme. Un point extrême n'est
   généralement pas certifiable par sa propre couverture, mais l'autre endpoint
   peut l'être ;
5. `unresolved_pair_mass=0`, `continuations=0` et les ledgers de conservation
   sont exigés avant de nommer les comptes « taille de l'objet » ;
6. sur petit `n`, les mêmes records complets — support, owner, `I_B`, `U_B` et
   `BallKey` — égalent le brute force, pas seulement leur cardinal.

Le fuseau `W_q` et la recherche locale peuvent réduire ce parcours. Ils ne
remplacent pas le reçu de couverture. Avec `--dmax` non certifié, Claude peut
continuer à mesurer des coûts et des bornes inférieures clairement étiquetés ;
il ne doit pas alimenter le tableau de dimensionnement exact de J0.

## Q17 — domaine exact du best-first axial

À `Q4Seed3=(a,b,x)` fixé, la lecture est correcte. Pour chaque témoin `z`,
`P_z(tau)=A_z-tau*B_z`, avec `A_z` quadratique convexe séparable en `z` et
`B_z` linéaire. Sur une AABB témoin, le minimum de `A` vient du clamp rationnel
du minimiseur, son maximum des huit coins, et les extrema de `B` des coins. Un
nœud dont `B` change de signe est séparé ; lorsque son signe est strict, les
quotients bornent la racine sans division flottante. L'égalité descend. Un
best-first peut donc sélectionner les groupes `First_k/Last_k` exacts sans
matérialiser le produit seed--apex. Les extrema portent sur la boîte continue,
pas directement sur ses points : l'exactitude vient de la descente fail-open
jusqu'aux feuilles et du range-report complet sur égalité.

Trois limites sont impératives :

- le `Q4Seed3` doit être singleton avant d'employer ces bornes. Si `a,b,x`
  varient, `G,W,n,T2` et les comparaisons croisées de racines sont des
  polynômes corrélés non convexes ; les coins d'un `FaceBlock` ne prouvent aucun
  `ALL` ;
- l'axe retire le produit avec le quatrième point, pas le coût de génération de
  tous les `Q4Seed3`. `Lane4` doit construire ses seeds depuis sa propre source
  complète, mesurer `seed_blocks/splits/seeds_singleton` et ne lire aucun
  verdict de `Lane3` ;
- `a369452` refuse désormais `MORT_GAP` et les apex retenus mais profonds ; les
  identités non injectives/disjointes restent le P0 avant la descente
  industrielle.

Un `Q4Seed3Block` reste donc autorisé comme unité de stockage et de scheduling.
Il ne devient une autorité de décision collective qu'avec une extension
d'intervalles/Bernstein reçue ou un replay exhaustif de sa microtuile. À défaut,
il se scinde jusqu'aux seeds ponctuels. Le pire cas du parcours témoin demeure
linéaire par seed ; les visites de nœuds, splits de signe, égalités, opérations
de heap, comparaisons larges, octets et HWM sont des sorties obligatoires de J1.
Le parcours doit aussi conserver les permanents `B=0,A<0`, le shell persistant
`B=0,A=0`, masquer exactement les trois IDs du seed, comparer les bouts
algébriques de `J_f` et garder les groupes égaux complets ou capés. Son pire cas
reste de l'ordre du nombre de seeds multiplié par le nombre de témoins.

## Rappel de seuil, sans cascade entre lanes

Sous `smax=11`, q2 accepte `I<=9` et meurt à dix, q3 accepte `I<=8` et meurt à
neuf, q4 accepte `I<=7` et meurt à huit. `smax-2=9` est donc le seuil strict de
mort q3, jamais une acceptation inclusive. Ces trois verdicts sont calculés par
trois producteurs indépendants et n'ont aucune autorité les uns sur les autres.

GCP non utilisé.
