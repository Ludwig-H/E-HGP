# Stratégie q3/q4 sortie-sensible proposée à Claude — 30 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Périmètre relu : tout `morsehgp3D_v5/`, notamment les sources WSPD, index,
pipeline q3/q4, filtres CPU/device, oracles, probes, reçus d'échelle, documents
mathématiques et audits actifs. La dernière tête amont relue est `92605016`;
son dernier pin fonctionnel reste `fc53472f`. Le présent audit ne modifie
aucune source fonctionnelle. Ce pin conserve le crédit sectoriel, la corde
corrigée et leur raccord à la sonde, puis propage le paramètre de hauteur au
champ scanline. `a78d0338` versionne le reçu `scanline_relief` et une
interprétation documentaire sans modifier ces sources ; `92605016` ajoute la
contre-relecture scanline/`linked_arcs` dans l'audit de dialogue, toujours sans
porte `linked_arcs_u16` versionnée. Le kernel reste reçu en lecture seulement :
aucun `nvcc` n'est disponible et la porte hôte n'exerce pas le device.

Mise à jour du 30 août : les corrections CPU de corde et la propagation du
crédit dans les lots sont cohérentes en lecture et les quatre portes ciblées
q3/q4 `uniform`/tout-hôte passent après rebuild. Le nouveau combinateur prend
bien le maximum secteur par secteur et sa fixture croisée tue maintenant
`sector-credit-global`; l'intégration depuis les histogrammes reste ouverte.
Le kernel est corrigé en lecture, sans `nvcc`, sans fixture exacte à cinq
points et sans exécution device. Le reçu `canopee_q4` a terminé avec l'ancien
probe ; le reçu versionné `terrain_deux_echelles` a aussi terminé sur la sonde
de `9f504e52` avec 27/27 codes zéro, mais reste une comparaison de distributions
sans latent commun et non un reçu causal. Le reçu versionné
`scanline_relief`, construit depuis `fc53472f`, a terminé avec 18/18 codes zéro
et identités internes fermées. Il reste `diagnostic_unpaired` : une répétition,
pas d'alternance, pas de tape/lineage/digest d'entrée, et seulement
`scanline_single_pass`. Ces fermetures locales ne bornent aucun terme de la
stratégie ci-dessous.

Cette stratégie optimise uniquement la source horizontale actuelle sous
`forest_semantics=verified_events_only`. Elle ne fournit ni les incidences
silencieuses Gamma, ni la reconstruction correspondante, ni une raison
d'accepter `require_exact=true`. Le P0 Gabriel/Gamma et l'ordre de fermeture
d'`ETAT_COURANT.md` restent prioritaires.

## Verdict

La direction `A x B`, citron, `h_core`, `h_a`, `h_b`, puis `h_c` est bonne,
mais elle ne suffit pas seule. La bonne « généralisation » est **asymétrique** :
`A x B` reste le propriétaire exact-once de l'arête longue, tandis que `C` est
une fibre de carriers parcourue conditionnellement. Une décomposition ternaire
symétrique fortement séparée réintroduirait l'obstruction cercle--axe déjà
prouvée. Le candidat architectural le plus complet relu est :

```text
WSPD binaire A x B, pilotée par des cellules, à prouver à s=8
  -> mort du rectangle et requêtes h_a/h_b sans auto-produits
  -> parcours hiérarchique de C et certificat h_c, sans matérialiser A x B x C
  -> supports réellement résiduels
       petit fanout : BallKey puis requête globale exacte déjà disponible
       grand fanout : zone de faibles profondeurs dans le plan médiateur
  -> q4 : intersections orientées seed aigu vivant x complétion admissible
          seulement dans les zones non certifiées profondes
  -> quotient par BallKey, puis census exact requis par le contrat
```

Le même moteur plan sert aux deux lanes, sans confondre leurs seuils ni leurs
sources :

- q3 teste le centre désigné porté par chaque droite de carrier ;
- q4 énumère les intersections de faible profondeur avec au moins une droite
  jouant le rôle de seed aigu survivant ; l'autre peut être une complétion non
  aiguë.

Cette mutualisation est le candidat **local par ancre** le plus avancé dans la
relecture pour retirer les deux produits tardifs `seed x cover` en q3 et
`C x D` en q4, sous réserve de recevoir son constructeur exact. Le lift global
streamé, décrit plus bas, reste une autre voie capable de traiter plusieurs
ancres ensemble et ne doit pas être exclu prématurément. `h_c` est le bon
préconditionneur pour ne pas matérialiser les fibres mortes ;
`ball_depth_at_least` est le terminal exact le moins risqué pour les clés déjà
formées ; le sous-complexe de faibles profondeurs reste le terminal proposé
pour les fibres vivantes à grand fanout.

Le statut présent reste **NO-GO pour un claim sous-quadratique global**. Il
existe même, pour le catalogue explicite de `BallKey`, une famille quadratique
q3 et q4 dans le modèle exact, détaillée ci-dessous. La cible recevable est
donc `temps = préparation + coût
shallow + B + S_shell + S_expand`, sous-quadratique seulement sur les entrées
où sorties et incidences le sont. Certaines familles pourraient devenir quasi linéaires
si tous les termes du grand-livre sont bornés ; une famille dont la masse
résiduelle d'ancres, de covers ou de `BallKey` reste quadratique ne sera pas
sauvée par une WSPD linéaire.

## Ce que la relecture complète change

Le pipeline actuel contient au moins cinq coûts indépendants que le nombre de
rectangles WSPD ne borne pas :

1. `corner_histograms` forme encore les auto-produits de chaque facteur ;
2. les ancres de `A x B` sont développées même lorsque leurs crédits peuvent
   les fermer avant handles ;
3. les carriers et covers sont revisités par ancre ;
4. q3 scanne les témoins pour chaque seed ;
5. q4 scanne les témoins par face, puis énumère les complétions `D`.

Fusionner les trois descentes WSPD, paralléliser ou ajouter une grille plate
réduit des constantes. Cela ne retire aucun de ces produits. Le diagnostic des
audits précédents doit donc être resserré : les `58 %` de visites WSPD évitées
dans les deux transcriptions internes de la sonde sont un diagnostic
d'ingénierie, pas encore une comparaison au chemin produit ni une stratégie
d'exposant.

Deux chargements empêchent maintenant de laisser ce constat au stade de simple
prudence. D'abord, `rect_cover_handles` contient `A union B` pour les
coefficients produit 3 et 4 : pour `z` dans `A` et `b` dans `B`, le point `z+b`
appartient à la boîte des sommes et la distance de `2z` à cette boîte est au
plus `|z-b|`, donc au plus le diamètre croisé. Par conséquent, une WSPD
linéaire ne borne pas la masse de ses handles. La construction candidate
`x_i=L^i`, avec `L` suffisamment grand, produit un radix en peigne et des
terminaux à facteur singleton. Si le ledger prouve en plus la partition
exact-once des paires, alors
`sum_R(|A_R|+|B_R|)=sum_R |A_R|*|B_R|+R=binom(n,2)+R`. Cette construction
demande une précision croissante ; sous u16 elle devient une fixture finie de
dépendance à la profondeur, pas une asymptotique infinie. La fixture doit
recevoir le singleton et l'exact-once, pas les supposer. Un éventuel charging
`O(n*w)` en la profondeur `w` reste à prouver.

Plus grave, le travail **hors facteurs par ancre** peut déjà être quadratique à
une seule échelle. Dans le modèle exact, prendre `a=0`, `k` points `B` sur une
petite calotte de la sphère de rayon `R` autour de l'axe x, et `M` points `Z`
près de `z0=(3R/8,3*sqrt(3)*R/8,0)`. Pour `b=(R,0,0)`, on a
`|z0|^2=9R^2/16`, `|z0-b|^2=13R^2/16`,
`|2z0-b|^2=7R^2/4`, mais `H=z0 dot (b-z0)=-3R^2/16`. Les `Z` sont donc dans
la lentille et le cover coefficient 3 de chaque ancre, tout en restant hors de
`W2`, donc hors du cœur, de `W3/W4` et des secteurs. Comme les `b` ont même
norme, un autre `b'` donne `b' dot b-R^2<=0`; avec `A={a}`, ni `h_a` ni `h_b`
ne ferme l'ancre. Les marges strictes survivent à une réalisation rationnelle
puis entière assez fine. Ainsi `sum_e m_e>=k*M=Omega(n^2)` après les portes
actuelles. Sous u16, il faut graver la plus grande réalisation en plage et la
qualifier comme contre-fixture bornée, sans transformer le domaine fini en
théorème asymptotique.

Il existe aussi une vraie barrière de **sortie pour un catalogue explicite**,
indépendamment de cette implémentation. La construction `A3(n,Delta)`
d'Edelsbrunner et Pach possède
`N=2n+2` points, `2n(n+1)` triangles de Delaunay et `n^2` tétraèdres. Leur
lemme 3.5 prouve que tous ces simplexes sont critiques : le circumcentre est
dans l'intérieur du support, la sphère est strictement vide et aucun autre
point n'est sur sa coquille. Chaque triangle satisfait donc exactement les
conditions q3 avec profondeur zéro et fournit une `BallKey` distincte ; chaque
tétraèdre fait de même en q4. L'owner exact-once choisit une représentation, il
ne réduit pas ces `Theta(N^2)` clés. Le pipeline courant appelle le fold dense,
et son `ForestResult` conserve explicitement `facet_keys` et
`final_canon_fid`; si la porte produit reçoit les événements critiques
attendus, cette sortie dense est elle aussi quadratique, même si toutes les
composantes finales fusionnent. Ce constat ne borne pas toute future API
exacte implicite ou reconstructible, ni le fold vivant encore non raccordé,
ni Gamma. La fixture oracle+pipeline doit encore relier les comptes
géométriques à chaque étage produit. Source primaire :
[Maximum Betti Numbers of Čech Complexes](https://pub.ista.ac.at/~edels/Papers/2024-01-MaxBettiCech.pdf),
§ 3.1 et lemme 3.5.

Cette borne est formelle dans le modèle géométrique exact à précision
croissante. Un replay préparatoire, non encore reçu ni indépendant de la route
de calcul qui l'a produit, donne aussi une contre-fixture **entière u16** à
graver sous le nom `linked_arcs_u16`. Pour `n` dans `{2,4,8,16}`, poser
`N=2n+2`, `R=60000`, `o=30000`, `theta=0.04`,
`t_i=-theta+2*theta*i/n`, puis matérialiser en littéraux les arrondis de
`A_i=(R*cos(t_i),o+R*sin(t_i),o)` et
`B_i=(R*(1-cos(t_i)),o,o+R*sin(t_i))`. Toutes les coordonnées sont dans
`[0,65535]`. L'énumération exhaustive avec les formes exactes v5 donne :

```text
x = [59952,59963,59973,59981,59988,59993,59997,59999,60000,59999,59997,59993,59988,59981,59973,59963,59952]
u = [27601,27900,28200,28500,28800,29100,29400,29700,30000,30300,30600,30900,31200,31500,31800,32100,32399]
A_i = (x_i,u_i,30000), B_i = (60000-x_i,30000,u_i)
```

La ligne complète vaut `n=16`; prendre les indices de pas 2, 4 ou 8 donne
respectivement `n=8,4,2` sans aucun appel à libm. Pour la reproductibilité, les
`PointId` suivent d'abord l'ordre littéral
`A_0,...,A_n,B_0,...,B_n`. Une permutation physique à `PointId` fixes doit
conserver le même owner ; un réétiquetage peut changer le tie-break, mais doit
rester équivariant, choisir exactement un owner et conserver le même ensemble
de `BallKey`. Ces deux cas appartiennent à la porte.

| `n` | `N` | clés q3 | clés q4 |
|---:|---:|---:|---:|
| 2 | 6 | 12 | 4 |
| 4 | 10 | 40 | 16 |
| 8 | 18 | 144 | 64 |
| 16 | 34 | 544 | 256 |

Les identités sont exactement `q3=2n(n+1)` et `q4=n^2`. À `n=16`, la plus
petite marge brute d'acuité entière vaut `58928`; les plus petites valeurs
brutes positives des formes de non-support valent `9505372644204968192` en q3
et `2588950695868800` en q4. La première dépasse `INT64_MAX` : calcul,
assertions et impression de ces marges doivent donc rester au moins en `i128`
ou `OBig`, sans conversion signée étroite. Ces grandeurs précèdent la réduction
primitive de la `BallKey` : seuls leur signe et leur non-nullité sont
contractuels. Tous les tétraèdres retenus passent le test strict de centre
intérieur.

La porte permanente doit embarquer les coordonnées ou un digest littéral —
jamais dépendre de `cos` à l'exécution. Son oracle indépendant parcourt tous les
triples/quadruples, exige rang, centre strictement intérieur, aucun non-support
intérieur ni sur coquille, réduit chaque `BallKey` par son gcd, puis compte les
clés distinctes. La route produit vérifie séparément profondeur zéro, coquille
égale au support et owner exact-once ; ce dernier ne découle pas de
l'énumération géométrique.

Le domaine u16 est fini : quatre tailles ne constituent donc **pas** une borne
asymptotique. Elles donnent une contre-fixture de profil où `S/n^2` reste
constant jusqu'à `N=34` et une porte de non-régression forte. L'impossibilité
asymptotique `Omega(N^2)` est celle de la famille exacte à précision croissante.
Elle impose une API sortie-sensible à toute généralisation de précision qui
matérialise ce catalogue ou cette forêt explicite ; une représentation
implicite/reconstructible relève d'un autre contrat. Dans le profil u16
courant, la même discipline reste la cible prudente, pas un théorème
asymptotique inventé.

Une seconde sortie doit rester séparée du nombre de clés. Une `BallKey` de
profondeur zéro peut avoir une coquille complète `U_B` de taille `Theta(n)`.
Noter donc `B` le nombre de clés distinctes et
`S_shell=sum_key |U_B|`, puis facturer à part l'expansion éventuelle des
supports. Dans le lift, q4 groupe exactement tous les hyperplans concourant au
même sommet ; q3 doit grouper les triples qui donnent la même droite puis le
même minimum radial. Une perturbation symbolique est interdite : elle changerait
la partition coquille/intérieur qui fait partie du contrat. Le constructeur
shallow doit streamer ces groupes plats, pas les exploser en sous-ensembles
avant de connaître la sortie demandée.

La conséquence architecturale est nette : prouver `R=O(n)` ne prouvera jamais
le coût aval. Tout chemin qui reconstruit un cover par ancre conserve
explicitement `H_rect=sum_R handle_mass(R)`, puis surtout
`H_scan=sum_R anchors_surv(R)*handle_mass(R)` avant compaction et
`M_anchor=sum_e m_e` après compaction dans son grand-livre. Une borne globale
sous-quadratique demanderait soit une hypothèse de régime vérifiable sur ces
sommes, soit une nouvelle primitive qui traite plusieurs ancres ensemble ; le
`PlanConflictGrid` par ancre ne fournit pas cette mutualisation.

### Repli exact déjà disponible : requête globale par `BallKey`

La primitive `pipeline/census.hpp::ball_depth_at_least` descend déjà l'index
global avec des bornes exactes de puissance sur les boîtes, crédite un nœud
entièrement intérieur et s'arrête au seuil 8/9. Elle rend un verdict de seuil
exact ; après arrêt positif, son paramètre `count` n'est pas une profondeur
totale exacte et ne doit pas être journalisé comme telle. En q3, le générateur rescane
aujourd'hui un cover qui contient tous les intérieurs, puis le pipeline répète
cette décision globale après RLE. En q4, le scan `cover3` n'est qu'un minorant,
avant la même décision globale. Le premier shadow à coder n'est donc pas un
nouvel arrangement : former la `BallKey` du support survivant, interroger cette
primitive reçue, et comparer fate, nombre de nœuds, feuilles et range-add aux
tests de sites de la route actuelle.

Ce pivot supprime la dépendance `ancre x témoins` du **moteur de décision** sur
les cas où la requête visite peu de nœuds, notamment la contre-famille de
calotte ci-dessus. Il ne supprime ni la découverte des ancres, ni celle des
supports q3, ni l'énumération `C x D` q4. Le radix courant peut encore visiter
`Theta(n)` nœuds par clé ; aucune borne sous-quadratique n'est reçue pour lui.
Pour éviter de refaire la requête sur une même sphère, un prototype streamé
trie chaque chunk en runs mais ne décide encore rien. Il fusionne/RLE ensuite
**globalement** tous les runs, réunit une clé présente dans plusieurs chunks,
retrouve son arité minimale et son représentant de niveau canonique, puis
appelle une seule fois la requête. Décider avant cette fusion serait faux si
deux occurrences portent des arités, donc des seuils, différents. Le ledger
publie `supports_raw`, `keys_chunk_unique`, `keys_global_unique`, visites de
profondeur, range-add, sorties et replis ; aucune émission ne précède le choix
transactionnel.

Si ce shadow ne suffit pas sur les fibres lourdes, le moteur de niveaux peu
profonds devient justifié. Sa formulation conceptuellement directe est le lift
des points en hyperplans : la profondeur d'une boule est un niveau
d'arrangement, q3 sélectionne le minimum radial sur l'intersection de trois
hyperplans et q4 les sommets de quatre hyperplans. Le produit ne doit toutefois
construire que la zone shallow et streamer ses points critiques, jamais la
mosaïque de Delaunay ou l'arrangement global. `PlanConflictGrid` reste le
préflight mesurable de cette idée, pas encore son constructeur prouvé.

Le profiler q4 candidat ne doit pas encore guider l'architecture. Au pin
`b8082040`, les trois transcriptions forment le morceau avant le branchement
`P>0` et les routes CPU constatent la mort avant le `continue` ; c'est la bonne
correction en lecture. Un replay shaped sur nuage généré discrimine les deux
ordres et son mutant ; la fixture exacte cinq points et la compilation CUDA
restent absentes. La sonde de `9f504e52` transmet enfin `EndpointCredit` et la
campagne terminale emploie bien ce pin. Cela ne la transforme pas en autorité
produit : elle est compilée avec `MHGP5_TESTING=1`, contourne `run_pipeline`,
le census et la forêt, et ses timers `boucle_seeds`, cœur/corde et complétions
sont emboîtés ; ils ne doivent pas être additionnés.

Enfin, le plateau cocyclique historique v3/v4 à 384 points et 2 322 560
supports pour une même `BallKey` interdit de cacher le coût de l'expansion
demandée. Ce différentiel n'est pas encore une fixture v5 requalifiée et ne
prouve pas, à lui seul, une borne inférieure quadratique en `BallKey`
distinctes. Les audits doivent distinguer sortie canonique, incidences de
coquille et expansion explicite des supports. Sa source différentielle est
`morsehgp3D_v4/audits/lectures_20260817/proposition.md` § 5.6.

## Étage 0 — rendre le front binaire prouvable

La WSPD binaire reste un bon front d'owner et de certificats : une WSPD
symétrique ternaire réintroduit l'obstruction cercle--axe et n'est pas la
généralisation recherchée. Elle n'est pas encore démontrée comme le meilleur
moteur global d'énumération ; le lift shallow peut traiter plusieurs ancres
ensemble. En
revanche, la borne `O(s^3 n)` annoncée par `src/wspd/wavefront.hpp` n'est pas
encore prouvée pour la variante réellement codée ;
`docs/MATHEMATIQUES.md` la laisse correctement ouverte.

### Couture Morton à fermer

Pour un préfixe de longueur utile `used=3*l+r`, avec `r` dans `0,1,2`, la
cellule exacte fixe `l` bits de chaque axe, puis le bit suivant de `z` si
`r>=1`, puis celui de `y` si `r=2`. Son aspect est au plus 2.

`cell_of_prefix` calcule aujourd'hui seulement `level=used/3`. Les champs
`clo/chi` décrivent donc le cube octree englobant `Q`, pas la cellule exacte de
préfixe `P`. La décision recommandée est de **conserver `Q`** pour le premier
incrément. Il existe déjà, ne change ni le layout ni les derniers bits. Le
lemme candidat est le suivant : pour un cube non singleton, les préfixes
internes réalisés de résidus 0, 1 et 2 portant le même `Q` forment une
composante connexe de taille au plus `1+2+4=7`. Leur contraction doit produire
un octree occupé compressé, éventuellement à racine comprimée, de fanout au
plus huit. Définir explicitement `Q(leaf)={point}` dans `packing_box_of` ; une
feuille est alors son cube singleton de niveau 16 et ne s'ajoute pas aux sept
représentants internes de son parent.

La seule multiplicité sept ne donne encore **aucune** constante terminale. Il
faut d'abord construire le quotient octree indépendamment, prouver la
connexité de chaque microarbre `Q`, séparer les feuilles singleton, établir le
fanout huit, puis simuler exactement split et tie-break. L'exact-once des
graines LCA doit ensuite injecter chaque terminal binaire dans un terminal du
quotient avec une fibre uniformément bornée. Le produit heuristique `7*7` est
un candidat de preuve, pas un invariant reçu.

Après seulement, un charging WSPD propre à `s=8` doit traiter les boîtes
entières de largeur `2^k-1`, les cellules virtuelles des arêtes comprimées,
leur disjonction et leur distance. La cible est
`R_shadow=O(R_oct)=O(m)` pour `m` positions uniques ; aucune constante
numérique, ni de déroulage ni de charging, n'est aujourd'hui reçue.

Le front ombre emploie seulement la séparation des cellules et scinde le
facteur de plus grand diamètre de cellule, avec tie-break canonique. Le front
réel effectue les mêmes scissions mais termine sur :

```text
SepCell(A,B) || SepTight(A,B)
```

Chaque terminal réel doit alors être l'ancêtre d'un ensemble non vide de
terminaux ombre. Le ledger exact-once est conservé à chaque remplacement
`A x B = A0 x B disjoint_union A1 x B`, et le front réel est un coarsening du
front de packing. Cette projection exige **le même split par diamètre de `Q` et
le même tie-break** dans le shadow et le réel. Elle donne alors
`R_cut<=R_shadow`; aucune monotonie de `SepTight` n'est nécessaire, avec
`R_cut=rectangles_emis+rectangles_tues_core` et `R_emis<=R_cut`. Les produits
correspondants forment une partition de masse, lane par lane. Le
`PostsepLedger` actuel ne voit pas les morts du cœur de la descente principale :
il faut donc un nouveau ledger global depuis les graines jusqu'à cette coupe,
et non renommer le ledger local existant.

L'incrément minimal à Claude est donc : exposer `packing_box_of`, renommer la
fausse « cellule exacte », partager une politique pure `SepQ || SepTight`, puis
ajouter un shadow test-only `SepQ`. Ne pas commencer par `P`, et ne pas annoncer
`O(n)` avant que le quotient octree indépendant, l'injection des terminaux et
le charging soient écrits et reçus.

Ce coarsening ne vise ici que `postsep_refine_levels=0`. Tout raffinement
post-séparation modifie l'arbre de rectangles et demande une projection, un
ledger et une borne propres.

### `s>=8` ne constitue pas un profil de coût

Le plancher mathématique produit reste 8, mais un claim de coût doit, pour
l'instant, figer **`s=8`**. À `s=INT64_MAX`, tout facteur de diamètre positif
échoue à la séparation et la source produit exactement une paire par couple de
positions, donc un front quadratique. La primitive arithmétique large peut
rester testée hors profil de coût.

Fixtures minimales : `used mod 3 = 0,1,2`, inclusion `tight subset Q`,
multiplicité interne du cube au plus sept, les huit feuilles singleton d'un
cube `2x2x2`, diamètre non croissant puis strictement décroissant au triplet
Morton suivant, quotient octree de fanout huit, histogramme des fibres
terminales, mapping terminal ombre vers coupe réelle, relation conditionnelle
`R_emis<=R_cut<=R_shadow`, ledger
global exact sous permutations et mutants de scission par boîte serrée, `AND`
au lieu de `OR`, feuille rattachée au parent et tie-break divergent. Exiger une
décroissance stricte à chaque enfant réfuterait à tort `Q`, dont le diamètre
peut stagner pendant les bits résiduels.

## Étage 1 — fermer `A x B` sans le développer

Le cœur du citron et les crédits d'extrémités doivent être utilisés ensemble.
Ils répondent à des domaines de témoins distincts et ferment une ancre lorsque
leur somme certifiée atteint le seuil de la lane. Un cœur faible ne doit donc
jamais court-circuiter `h_a` et `h_b`.

Le premier raccourci est gratuit : si
`(|A|-1)+(|B|-1)<need`, aucun couple ne peut mourir par `h_a+h_b`, donc les
deux histogrammes sont entièrement sautés ; un facteur singleton reçoit zéro.
Sinon, une descente one-sided exacte est possible sans enrichir `RadixNode`.
Pour l'endpoint `s`, la boîte partenaire `T` et un nœud témoin `Z`, les
64 couples de coins de `T x Z` donnent un classifieur `ALL` **exact sur le
continu**. La raison est double : à `t` fixé, le fuseau ouvert est convexe en
`z`; à `z` fixé, les partenaires admissibles forment un cône convexe en `t`.
L'évaluation peut donc appeler `universal_over_corners(q,s,T,zc)` pour chaque
coin distinct `zc` de `Z`.

Le crédit `ALL` du premier shadow doit conserver l'unité historique :
`corner_histograms` incrémente une fois par **position unique** `upos`, pas par
`PointId` du bucket. Créditer un sous-arbre par `node_weight()` ou
`range_weight()` changerait donc le compte ; employer
`range.last-range.first+1`, moins la feuille `s` si nécessaire. Une éventuelle
requalification par multiplicité est un autre contrat, avec oracle et digests
propres. Ajouter un mutant `endpoint-credit-use-weight` et une fixture à bucket
dupliqué empêche ce glissement silencieux.

`NONE` doit rester conservateur. Pour un coin partenaire `t`, calculer
`M=hmax4_boxes(point_box(s),point_box(t),Z)=4*max_Z(H)`. Pour chaque composante
`j` de `d cross (z-s)`, former son intervalle exact `I_j` sur `Z`, puis
`Xlb=sum_j dist(0,I_j)^2` ; sommer des extrema incompatibles ou prendre un
maximum serait faux. Si `M<=0`, ou si `beta_q*M*M<=16*Xlb` avec `beta_3=3` et
`beta_4=2`, aucun point de `Z` ne témoigne pour ce `t`, donc aucun n'est
universel sur `T`. Dans tous les autres cas le nœud est `MIXED` et il faut
descendre ; à la feuille, `universal_over_corners` reste l'autorité exacte.
Voir tous les couples de coins hors du fuseau ne prouve jamais `NONE` : deux
ensembles discrets peuvent avoir la même AABB, l'un sans témoin et l'autre avec
un témoin au centre.

La couture qui change le coût est la suivante :

1. remplacer les doubles boucles de `corner_histograms` par des requêtes
   one-sided saturées au seuil, donc au plus 9 ;
2. créditer en bloc les sous-arbres entièrement témoins, rejeter ceux qui ne
   peuvent pas témoigner et ne descendre que les nœuds `MIXED` ;
3. construire, pour chaque côté, au plus une famille constante de bitsets
   cumulatifs — un par classe de seuil, jamais un par point `a` — et un index
   des seuls mots non nuls ;
4. appliquer `rect_hist_all_dead` avant handles, requête diamétrale, acuité et
   cover ;
5. dans un rectangle partiellement vivant, n'énumérer que les couples marqués
   résiduels, jamais balayer tout le bitset dense.

Avec `h3=9` et `h4=8`, le nombre de classes de seuil est constant. Le coût visé
devient linéaire en `|A|+|B|`, visites de nœuds mixtes et couples réellement
`PENDING`, au lieu de `|A|^2+|B|^2+|A||B|` par défaut. Il peut encore être
quadratique si tout reste mixte : le nombre de visites et la masse `PENDING`
doivent être publiés comme critères de réfutation.

La borne honnête par rectangle est
`O(V_R+C_R+|A|+|B|+P_R)`, où `V_R` somme tous les nœuds classifiés,
`C_R<=64*V_R` les couples de coins évalués et `P_R` les ancres survivantes de
ce seul filtre. Une AABB seule autorise encore
`V_R=Theta(|A|^2+|B|^2)` : cette primitive est un prototype exact et mesurable,
pas encore une preuve d'exposant. Employer la boucle directe sur les facteurs
minuscules, l'arbre seulement au-dessus d'un seuil fixé, et publier
`all_nodes`, `none_nodes`, `mixed_nodes`, `corner_pair_evals`, `leaf_tests`,
`range_add_unique`, `V_R` et `direct_pairs`.

La fusion q3/q4 est sûre avec des masques : `ALL4` crédite les deux lanes,
`ALL3` seulement q3, `NONE3` ferme les deux et `NONE4` seulement q4. Une lane
créditée est retirée avant la descente. La feuille `z=s` reste exclue et ne peut
jamais devenir `ALL` (`H=0`) ; une garde, une assertion et un mutant doivent
graver cet invariant. Les comptes sont saturés séparément à
`need3=h3-hcore3` et `need4=h4-hcore4`, et le shadow compare
`min(corner_histograms,need_q)` par lane. Aux maxima du profil `smax=11`, les
`b` sont rangés dans au plus neuf classes de crédit, puis émis par bitsets
cumulatifs ou merge **dans l'ordre Morton historique** ; concaténer les classes
casserait la parité brute.

Chaque crédit conserve ses indices `upos` ou une partition de provenance. Les témoins du
cœur, de `h_a`, de `h_b`, de la grille et de `h_c` ne s'additionnent pas si
leurs domaines peuvent se recouvrir. Un maximum sûr ne doit pas être remplacé
par une somme sans preuve de disjonction.

## Étage 2 — le rôle exact de `h_c`

La proposition de Claude sur `A x B x C` est la bonne étape suivante, à
condition que `C` soit parcouru hiérarchiquement. Calculer `h_c(c)` après avoir
énuméré toutes les faces ne change que la constante.

### Composer réellement `h_core`, `h_a`, `h_b` et `h_c`

Le patch `EndpointCredit` courant ne transporte que `h_a+h_b`. `ar.core` sert
à la porte d'histogramme, mais disparaît lorsque l'ancre survivante atteint les
secteurs. C'est sûr, car le scan pur peut retrouver ces témoins, mais cela ne
réalise pas la cascade demandée : un témoin du cœur peut être universel sans
être reconnu par chaque polygone sectoriel.

Les domaines donnent une couture simple. Les témoins de `h_a+h_b` vivent dans
`A union B`; ceux du cœur vivent hors de cette union. En revanche, `h_core` et
un `h_c_ext` calculé sur tous les sites extérieurs à `A union B` peuvent
désigner les mêmes positions. Sans provenance supplémentaire, le minorant sûr est :

```text
E_ab = h_a(a) + h_b(b)
lower_bound(c) = E_ab + max(h_core, h_c_ext(c))
```

Le prototype plus fort doit utiliser le helper déjà présent
`collect_universal_ids`. Pour chaque `AliveRect` vivant et chaque lane, il
demande `cap=ar.core`, reproduit des indices `upos` **uniques**, puis vérifie
qu'ils sont hors `A union B` et certifiés pour cette lane et ce rectangle. Si
`r_core<=h_core` indices sont recertifiés, il construit le tape résiduel hors
`A union B union U_core`, où `U_core` désigne cette petite liste et non la
fibre de carriers `C`. Sous le profil sans positions dupliquées, le minorant
graduel devient :

```text
E_ab = h_a(a) + h_b(b)
lower_bound(carrier) = E_ab + r_core + max(h_core - r_core, h_c_residual_after_Ucore(carrier))
```

La même liste constante améliore les secteurs : conserver le compte pur
`cnt[k]`, former `cnt_res_after_Ucore[k]` en excluant `A`, `B` et `U_core`, puis tester
`min_k max(cnt[k],E_ab+r_core+max(h_core-r_core,cnt_res_after_Ucore[k]))>=h_q`. Le compte pur
empêche toute régression ; le résiduel rend exactement `U_core` disjoint. Ce
n'est pas la profondeur totale exacte : les témoins non universels de
`A union B` sont volontairement abandonnés et les `h_core-r_core` témoins non identifiés peuvent
recouvrir le résidu. `r_core=0` redonne le fallback sûr ; `r_core=h_core` redonne la somme
entièrement disjointe. Une recertification partielle, notamment après héritage
post-séparation, ne doit donc pas jeter tout le crédit. Publier `core_requested`,
`core_recertified` et la distribution de `r_core`.

Cette collecte se fait par rectangle, jamais par ancre. À `h3=9` et `h4=8`,
elle stocke au plus huit puis sept indices. Si le profil accepte un jour des
multiplicités, une liste de positions uniques ne suffira plus : il faudra
porter les poids ou revenir au maximum sûr.

Le premier contrat produit peut être plus simple que le cas partiel. Sous les
positions distinctes actuelles, `collect_universal_ids(cap=h_core)` doit
retrouver exactement `h_core` indices, y compris après héritage postsep. La V1
accepte donc un `CoreCredit` seulement si la factory recertifie
`count==h_core`, unicité, exclusion de `A union B`, lane et rectangle ; sinon
elle incrémente un invariant et retombe sur
`E_ab+max(h_core,residual_lb)`, sans retirer d'indice du tape. Le cas
`0<r_core<h_core` reste une formule sûre et une fixture utile, mais pas un état
nominal à propager tant qu'il n'apporte rien.

Une API minimale empêche les additions accidentelles : `ResidualTape` porte
`lane`, source de scan, digest des exclusions, `tape_id` et les `upos` uniques
hors `A union B union U_core`; `AnchorCredit::compose(residual_lb)` est la
seule opération et rend
`min(h_q,E_ab+r_core+max(h_core-r_core,residual_lb))`. `CoreCredit` est tagué
par lane et `RectKey`. Une grille est de même clé
`(AnchorKey,lane,rho2_den,tape_id,G)`. Partager la géométrie entre lanes est
permis, partager silencieusement leur `base_depth` ne l'est pas : leurs tapes,
seuils et domaines diffèrent.

`h_c` et le `base_C` de `PlanConflictGrid` lisent précisément ce même tape
résiduel. Ce sont deux minorants du **même** compte : on compose
`residual_lb=max(h_c,base_C)`, jamais `h_c+base_C`. En q3, une requête exacte de
grille au centre remplace même `h_c_q3_point`; elle ne s'y ajoute pas. La
fixture minimale prend le même témoin avec `h_c=base_C=1` au seuil 2 et doit
laisser vivre.

### Représentation de la fibre `C`

Ne pas réutiliser un `NodeRef` spatial brut comme certificat de carriers. Pour
une ancre fixée, la transformation `c -> centre q3 / droite q4` contient la
forme q3 et des quotients ; les coins de la boîte XYZ de `C` ne bornent donc
pas automatiquement les centres de tous ses descendants. Le premier incrément
sûr crée, seulement après survie de l'ancre, un `CarrierRecord` léger : indice
`upos`, `Q3Form` brute et `BallKey` réduite, coefficients exacts de la droite
dans le plan médiateur, certificat entier `Jb` de l'intervalle de corde,
masques q3/q4 et rôles. Les extrémités irrationnelles restent comparées par les
primitives carrées existantes ; elles ne sont pas arrondies ni stockées en
flottant. Le record ne contient aucun `D`.

Cette géométrie ne porte aucune profondeur implicite. Un `CarrierDepth` séparé
référence `lane` et `tape_id`, puis contient exactement l'un des contrats
`q3_point` ou `q4_chord`. Une profondeur calculée sur la tape q3 ne peut ainsi
pas être relue par q4 parce que les coordonnées géométriques du carrier sont
partagées.

Le shadow calcule d'abord `h_c_q3_point` et `h_c_q4_chord` record par record
contre l'oracle exhaustif. La référence d'un `CarrierBlock B` est
`min_{c in B} depth_res(center_c)` en q3 et
`min_{c in B} min_{mu in I_c} depth_res(c,mu)` en q4. Ce n'est jamais le
maximum des feuilles ; compter seulement les témoins communs à tous les
descendants donne un minorant plus faible, pas cette valeur exacte. Un bloc ne
devient donc utile que s'il porte un minorant prouvé pour **tous** ses
descendants, avec classification `ALL/NONE/MIXED` dans l'espace de paramètres
des droites ; `MIXED` descend et toute borne inconnue est fail-open. Un arbre
équilibré sur ces paramètres peut être essayé, mais l'octree XYZ n'est pas une
autorité implicite. Comme la V1 construit déjà les `CarrierRecord`, elle mesure
les faces et essais `D` évités **après records** ; annoncer des records évités
exigerait un pré-record distinct. `D` n'est énuméré qu'après survie du rôle de
seed et de ses portions admissibles.

### Minimum exact sur toute la corde

Ce certificat est propre à la fibre q4. En q3, le carrier `c` désigne un seul
centre, celui de la face `abc` : `h_c_q3` compte le résidu à ce point
(`mu=0` dans la paramétrisation ci-dessous). En q4, le même carrier doit être
fermé pour **tous** les centres que pourrait créer une complétion `D` ; le bon
crédit uniforme est donc le minimum sur toute sa corde admissible. Conserver
deux champs `h_c_q3_point` et `h_c_q4_chord` évite de payer un certificat trop
faible en q3 ou d'employer fautivement un compte ponctuel en q4.

Pour une ancre `(a,b)` et un carrier aigu `c`, reprendre ses quantités q3
`g_c>0`, `n_c=(b-a) cross (c-a)` et `J_c>0`. Les centres admissibles de la
corde s'écrivent `v_c(mu)=v3_c+mu*n_c/(2*g_c)` pour
`mu` dans `[-sqrt(J_c/2),+sqrt(J_c/2)]`. Pour un site `z`, poser
`P_cz=q3_power(f3_c,z)` et `B_cz=n_c dot (z-a)`. L'identité exacte est :

```text
g_c * F_z(v_c(mu)) = 4 * (mu*B_cz - P_cz)
z intérieur strict  iff  P_cz - mu*B_cz < 0
```

Ainsi `h_c_residual(c)` est le minimum, sur **toute** la corde, du nombre de
sites résiduels satisfaisant cette inégalité. Il s'agit de
`min_mu #{sites actifs à mu}`, non de `#{sites actifs pour tout mu}` : les
témoins peuvent se relayer. Outre `F1(mu)=mu,F2(mu)=-mu`, graver sur `[-1,1]`
`F1(mu)=mu+1,F2(mu)=1-mu` : la profondeur minimale stricte vaut 1 alors
qu'aucun site n'est strictement actif sur toute la corde. Le sweep 1D est
exact : `B>0`
entre juste après la racine `P/B`, `B<0` sort à cette racine, `B=0,P<0` est
actif partout, et `B=0,P=0` reste coquille. À une racine multiple, retirer
d'abord toutes les sorties, évaluer le point de racine où les incidents valent
zéro, puis ajouter les entrées. Les deux extrémités fermées, les racines
confondues et la frontière `2*P^2=J*B^2` sont évaluées exactement en arithmétique
large. Le carrier lui-même est coquille partout : l'exclure et l'asserter.

Le `PlanConflictGrid` peut mutualiser ce sweep seulement si sa partition est
complète sur **le même tape résiduel**. Pour chaque cellule fermée `C`, exposer
`base_C=#{z:min_C F_z>0}` et
`conf_C={z:min_C F_z<=0<max_C F_z}`. Sur la portion de corde dans `C`, la
profondeur vaut alors `base_C` plus le compte exact des conflits actifs. Une
face est fermée seulement si toutes ses portions et tous leurs points frontière
sont profonds. Le `CellGrid` actuel ne suffit pas : son `cnt` vient de `cover`,
il n'expose pas les conflits et sa boîte flottante n'énumère pas les cellules
canoniques traversées.

Avec `K_c` la somme des listes de conflits lues par le carrier, le premier
shadow trie les événements et vise
`O(G^2+m_e*G+sum_c(G+K_c*log(K_c)))`. La saturation du seuil à huit ne dispense
pas de connaître l'ordre des racines ; remplacer `log(K_c)` par `log(h4)` exige
un algorithme top-seuil et une preuve distincts. Un même site rasterisé dans
plusieurs cellules est réévalué, jamais recompté dans une portion ; une
optimisation par stamps peut le dédupliquer tout en conservant deux sites
distincts sur la même racine. Ce coût peut rester quadratique. Publier somme,
maximum et quantiles de `K_c`, les sites uniques après stamp, cellules
profondes/peu profondes, faces fermées et essais `D` évités.

`h_c` ferme un **rôle de seed/fibre**, pas l'identité du point. Un carrier tué
reste dans `witness_tape`, `base_depth`, les conflits, le census et ses autres
rôles ; en q4 seule son appartenance à `A_C` disparaît, jamais son rôle de
complétion dans `U_C`. Sinon le préfiltre de profondeur devient
auto-réalisateur ou perd des q4 valides. Un point shallow sur la corde ne
garantit par ailleurs aucun `D` admissible : il impose le fallback, jamais une
émission.

Fixtures minimales : `F1(mu)=mu`, `F2(mu)=-mu`, `h=1` sur `[-1,1]` — le point
commun a profondeur zéro —, racine sur une arête de cellule, racine à une
extrémité, site dupliqué entre cellules, deux sites distincts sur la même
racine, les trois cas `B=0`, recouvrement cœur/`h_c`, recouvrement `A`/grille,
et une sortie située dans une cellule profonde avant une portion shallow.
Ajouter `F1(mu)=mu+1,F2(mu)=1-mu`, qui sépare minimum des comptes et témoins
communs ; un `U_core` connu laissé dans le tape ; le même témoin donnant
`h_c=base_C=1` au seuil 2 ; deux feuilles de bloc `[1,0]`, dont le minimum vaut
0 ; et une profondeur 1 portée par des témoins différents dans deux feuilles,
où l'intersection des témoins vaut pourtant 0. La fixture lane
`a=(1000,1000,1000)`, `b=(2000,1000,1000)`,
`z=(1010,1016,1000)` appartient à `W3` mais pas à `W4` et interdit de partager
un tape ou un crédit sans masque. La sortie avant une portion shallow interdit
de fusionner des runs en oubliant leurs événements.

Un nœud interne de carriers ne ferme qu'avec un minorant uniforme sur tous ses
descendants obtenu sans visiter les feuilles. Tout descendant non classé force
`MIXED`; il est interdit d'utiliser le meilleur carrier observé comme crédit du
nœud entier.

## Étage 3 — un moteur plan commun, avec deux contrats de profondeur

Les autorités à requalifier sont
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md)
§§ 4.4–4.6 et
[`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md)
§§ 70–72. Elles cadrent la proposition ; elles ne livrent pas le constructeur
exact v5.

Fixons une ancre `e=(a,b)`. Les centres des sphères passant par `a` et `b`
vivent dans le plan médiateur de `ab`. Chaque site `z` non axial y définit :

- une droite `ell_z` sur laquelle `z` appartient à la coquille ;
- un demi-plan ouvert `H_z` dans lequel `z` est strictement intérieur.

Les extrémités `a,b` sont exclues du tape. Un autre site situé sur l'axe de
`ab` donne au contraire un signe constant sur tout le plan : il témoigne
partout s'il est strictement dans la boule diamétrale de `ab`, nulle part sinon
et reste un site de coquille universel au cas d'égalité pour le census.
Ces témoins constants sont comptés une fois dans `c0_e`; si `c0_e>=h_q`,
l'ancre est morte, sinon le moteur travaille au seuil résiduel `h_q-c0_e`. Il
ne faut pas inventer une droite dégénérée.

Sur le disque utile de Jung, une droite non axiale peut aussi ne jamais entrer
dans le domaine. Son demi-plan est alors intérieur-universel ou
extérieur-universel sur ce disque : le premier cas rejoint `c0_e`, le second
est écarté, et une tangence reste une strate de frontière. Cette classification
doit être exacte ; elle évite de charger l'arrangement de lignes sans événement
utile.

La profondeur **relativement à la source du tape** est `c0_e` plus le nombre de
demi-plans ouverts qui contiennent le centre. Les évaluations restent entières
et strictes ; une droite incidente ne compte jamais son propre site comme
témoin intérieur. Chaque position distincte porte en outre ses rôles
`support_eligible` et `witness_only` : toutes les entrées contribuent à la
profondeur, mais seules les premières peuvent définir un seed q3 ou un sommet
q4 à émettre. Avec `cover3`, cette profondeur n'est qu'un minorant certifié de
la profondeur globale et le census final reste obligatoire.

Nommer donc deux contrats : `delta_e^(3)` pour la profondeur historique sur
`cover3`, et `delta_e^full` pour la profondeur globale qui porte le rang fermé.
Ils ne sont pas interchangeables ; `mhgp5_q4_cover_fixture` doit rester la
fixture de séparation des deux sources.

### q3 : interroger les centres désignés

Après les portes bon marché de rang, non-colinéarité, owner et acuité, chaque
carrier admissible `c` donne un point précis de `ell_c`, le centre de la
circonférence passant par `a,b,c`. Ce point est sur la droite du carrier, mais
sa strate minimale peut être 1D ou 0D si plusieurs classes sont concurrentes
ou cosphériques. Une point-location dans une cellule 2D adjacente compterait à
tort un site incident. La requête doit rendre la profondeur stricte de cette
strate minimale, classe confondue comprise. Si elle atteint 9, le seed est
mort ; sinon les portes exactes restantes continuent. `k_e` compte tous les
carriers effectivement interrogés.

Le raccord algorithmique reste ouvert. Il faut prouver un constructeur du
sous-complexe stratifié de profondeur inférieure à `h3`, dégénérescences du
profil comprises, de coût cible
`A_e(m_e,h3)=O(m_e*log(m_e)+m_e*h3)`. Sous cette précondition, les requêtes q3
coûtent `O(A_e+k_e*log(m_e))`. Si `k_e=O(m_e)`, le produit `seed x cover` serait
alors remplacé par `O(m_e*log(m_e))` à seuil constant. La complexité
combinatoire des niveaux ne fournit pas, à elle seule, ce constructeur exact.

Le premier incrément codable est un `PlanConflictGrid` **shadow exact par
rapport aux `witness_tape` et `q3_query_tape` reçus**. Pour
`r_z=2z-a-b`, `q_z=|r_z|^2-|b-a|^2` et un centre décalé `v`, chaque témoin
définit l'affine `F_z(v)=4*r_z dot v-q_z`; la profondeur stricte est le nombre
de `F_z(v)>0`. Sur une cellule fermée, `min(F)>0` crédite `base_depth`,
`max(F)<=0` ne crédite rien, et le reste va dans la liste de conflits. Le centre
q3 rationnel est localisé dans une cellule canonique par comparaisons entières
256 bits, puis seuls les conflits sont réévalués exactement. La parité à graver
est `4*r_z dot N-2*g3*q_z=-8*q3_power`, avec `g3=f3.g`. Les formes axiales
`A=B=0` sont retirées avant rasterisation : `q_z<0` crédite toutes les cellules,
`q_z=0` est coquille universelle sans profondeur, et `q_z>0` ne crédite rien.

Le localisateur flottant actuel ne convient pas : il certifie un sur-ensemble
de cellules fermées, pas un propriétaire canonique. L'oracle
`tests/cell_grid_oracle.cpp` contient déjà les primitives W256 et leurs bornes ;
elles doivent devenir une primitive produit testée indépendamment, jamais un
import de l'oracle. Avec `G` la résolution, poser
`I_conf=sum_C c_C` et `K_conf=sum_C q_C*c_C`, où `c_C` est le nombre de
conflits et `q_C` le nombre de **centres de requête uniques** possédés par la
cellule. Le coût exact du shadow quotienté est
`O(G^2+m_e*G+k_unique+K_conf)` et `I_conf=O(m_e*G)` ; l'expansion éventuelle des
supports reste un terme de sortie séparé. À `G` fixé, une famille adaptée peut
encore rendre `K_conf=Theta(m_e*k_unique)` : ce shadow mesure le verrou, il ne
le ferme pas.

Une résolution `G` proche de `sqrt(k_unique)` est seulement un modèle moyen. En
notant `N_G` le nombre **exact** de cellules canoniques de la grille et
`rho_qc=N_G*K_conf/(k_unique*I_conf)` lorsque
`k_unique*I_conf>0`, l'identité
`K_conf=rho_qc*k_unique*I_conf/N_G` montre exactement l'hypothèse cachée ; dans
le cas nul, publier une sentinelle plutôt que diviser. Si
`I_conf=Theta(m_e*G)` et `rho_qc=O(1)`, alors `G≈sqrt(k_unique)` donne le modèle
`O(k_unique+m_e*sqrt(k_unique))`. Pour toute résolution fixée, ou toute suite
bornée choisie d'avance, une famille peut concentrer `k_unique` centres
distincts dans une cellule traversée par `m_e` conflits. Une concurrence exacte
peut au contraire se quotienter en un seul centre et ne constitue pas à elle
seule ce pire cas. Un faisceau presque parallèle adapté au dernier maillage est
une contre-fixture candidate pour cette concentration, pas encore un fait reçu
tant que carriers et requêtes couplés ne sont pas gravés. Un décalage aléatoire
ne donne aucune garantie sans analyse probabiliste propre.

Le prototype doit donc essayer une petite suite prédéfinie de résolutions en
puissances de deux et facturer explicitement
`sum_j(G_j^2+m_e*G_j+k_unique+K_conf,j)` jusqu'à l'arrêt du preflight. Cette
somme n'est pas géométrique en général : `K_conf,j` peut rester grand ou
non monotone à chaque résolution. Il retient transactionnellement la première
grille sous budget et mesure
`N_G,m_e,k_raw,k_unique,I_conf,K_conf,c_max,q_max,rho_qc`, temps et octets. Il
ne fixe jamais `G=sqrt(k_unique)` comme preuve ni ne choisit sa résolution après
avoir partiellement émis.

La voie d'implémentation la plus courte ne duplique pas la grille actuelle.
En mode strict nominal et pour **le même tape avec les mêmes exclusions**,
`CellGrid::count_site_t` calcule déjà, par seuils monotones de lignes, les
cellules où les quatre sommets satisfont l'affine : son `cnt` est exactement le
futur `base_depth`. Le `cover` actuel et un futur `scan_sites()` ne sont pas le
même tape ; réutiliser la primitive ne permet pas de recopier aveuglément son
tableau. Pour préserver exactement la profondeur historique, le premier
shadow reconstruit donc base et listes **après** la compaction, sur
`scan_sites()`. L'autre option — rester sur `cover` — exige un lemme séparé
prouvant que les sites filtrés ne contribuent à aucune requête admissible ; elle
n'autorise toujours pas l'égalité des tableaux sur tout le losange.

Refactorer cette primitive en générateur de bornes et callbacks. Deux sweeps
sont nécessaires par rangée, l'un pour `F>0`, l'autre pour `F>=0`, afin
d'émettre séparément `depth_conflict` lorsque `min(F)<=0<max(F)` et
`line_incidence` lorsque `min(F)<=0<=max(F)`. Une droite **non axiale** touche
`O(G_grid)` cellules ; la variation monotone des bornes donne donc un coût
`O(G_grid^2+m_e*G_grid+sorties)` avec l'initialisation de la grille. Le cas
`du=0,dv!=0` émet une rangée entière ou vide. Le cas `du=dv=0` est axial :
intérieur global, coquille globale ou extérieur global en `O(1)` ; matérialiser
la coquille dans `G_grid^2` listes serait une faute de coût et de multiplicité
q4. Ajouter `conflict_cells_emitted` et `incidence_cells_emitted`, puis comparer
cellule par cellule au balayage exhaustif du tape choisi. Séparément, appeler le
helper refactoré sur `cover` et exiger que l'ancien `CellGrid::cnt` reste
bit-identique. Les fixtures comprennent diagonale par sommets, ligne sur arête,
coin tangent et les deux strictesses. Le cas `max(F)=0` appartient seulement
aux incidences, jamais aux conflits de profondeur.

La littérature distingue précisément ce qui est acquis. Har-Peled et Sharir,
[Depth contours in arrangements of halfplanes](https://www.math.tau.ac.il/~michas/k_depth.pdf),
lemme 2.5, bornent par `m*(k+1)` les sommets de profondeur au plus `k` pour des
orientations mixtes **en position générale**. Les coïncidences, concurrences,
verticales et frontières strictes v5 restent donc à étendre. Everett, Robert et
van Kreveld,
[An optimal algorithm for computing (≤k)-levels, with applications](https://doi.org/10.1142/S0218195996000186),
donnent le constructeur `O(m log m+m*k)` pour les niveaux classiques, mais pas
pour le niveau pondéré `+1/-1` induit par nos orientations mixtes. Aronov et
Har-Peled,
[On Approximating the Depth and Related Problems](https://sarielhp.org/p/04/depth/depth.pdf)
§ 4.2, rappellent en le citant l'incrémental randomisé pour des régions
pseudodisques x-monotones bornées par des courbes fermées. L'adaptation exacte
aux demi-plans non bornés, aux lignes confondues et aux strates strictes v5
reste à prouver. Une shallow cutting seule ne produit pas ce sous-complexe.

### q4 : énumérer les sommets peu profonds

Un centre passant par `a,b,c,d` est une intersection `ell_c intersect ell_d`.
Le candidat consiste à énumérer les sommets distincts dont la profondeur
stricte sur la face 0D est inférieure à 8, puis à appliquer disque de Jung,
diamètre et les portes propres aux supports. En position générale, le nombre
de sommets de profondeur inférieure à `h` est `O(m_e*h)` ; la cible
algorithmique locale conditionnelle est :

```text
O(m_e log m_e + h4*m_e + z_e)
```

où `z_e` compte les centres canoniques produits **par ancre**, avant une
éventuelle déduplication inter-ancres, et non toutes les paires de droites. Ce
résultat combinatoire doit encore être étendu aux profondeurs strictes
stratifiées et transformé en constructeur exact de cette complexité ; il ne
constitue pas aujourd'hui une mesure v5.

Les classes de droites confondues sont groupées : deux supports d'une même
classe ne définissent aucun centre isolé et leur paire dépendante est rejetée.
Deux classes parallèles n'ont pas d'intersection. Plusieurs classes distinctes
peuvent en revanche être concurrentes en un centre ; tous leurs événements
sont appliqués en bloc, sans perturbation symbolique qui transformerait une
coquille en intérieur.

Le même `PlanConflictGrid` fournit un shadow q4 limité, mais il doit respecter
les **rôles orientés** du produit. Un q4 bien centré d'owner `ab` possède au
moins une face incidente aiguë, jamais nécessairement deux. Le produit choisit
donc un seed aigu `c`, puis une complétion `d` qui peut être non aiguë. Pairer
seulement deux carriers aigus perdrait la contre-fixture déjà gravée de
`docs/MATHEMATIQUES.md` § 6.4.

Pour chaque cellule, noter `U_C` les classes de **droites support uniques**
éligibles unairement comme complétions relativement à l'ancre `ab` : la
provenance appartient au support source de `D`, passe les portes propres à
l'ancre et indépendantes de tout seed, n'est pas axiale, et sa droite clippée
au disque utile rencontre la cellule. Les tests qui dépendent d'un couple —
distance `|c-d|`, owner, exact-once et bien-centrage — viennent seulement après
l'appariement ; les inclure dans `U_C` rendrait sa définition circulaire et
pourrait casser `A_C subset U_C`. Cette dernière contient une classe s'il
existe au moins un seed aigu survivant de cette classe dont la corde ou un
fragment exact rencontre la cellule et dont `L_cC<h4`. C'est un OR par classe,
pas un certificat pour chaque provenance. Plusieurs segments ou points sur une
même droite restent dans `SeedProv[L,C]` et `CompletionProv[L,C]`; ils ne
créent pas plusieurs classes géométriques. `h_c` retire seulement une
provenance de seed : le même point et la même droite restent dans `U_C`, dans
le tape de témoins et dans l'autre lane.

Le minimum `h_c_q4_chord` ne livre qu'un booléen « corde entière morte ou non ».
Il ne livre pas automatiquement **un segment shallow** : la profondeur peut
alterner sur plusieurs sous-intervalles. La V1 sûre rasterise donc toute la
corde de Jung d'un seed survivant. Une version plus précise doit faire sortir
du sweep l'union exacte de tous les sous-intervalles où le minorant reste sous
8, avec frontières strictes, nombre de fragments et coût de rasterisation au
ledger ; oublier un fragment serait un faux rejet.

Le préfiltre cellulaire compose les bornes du même tape par maximum. Pour un
seed `c` recertifié et une cellule touchée par sa corde, poser :

```text
L_cC = h_a + h_b + r_core + max(h_core - r_core, h_c(c), base_C)
A_C contains line(c) if some live seed c of that class covers C and L_cC < h4
S_h = {C : A_C is nonempty}
```

Une cellule/seed avec `L_cC>=h4` est profonde partout et ce rôle seed peut être
retiré. Dans le cas `r_core=h_core`, tester d'abord en entier signé
`h_a+h_b+h_core>=h4`; si vrai tout rôle est fermé, sinon seulement former
`r_e=h4-(h_a+h_b+h_core)>0` et comparer
`max(h_c(c),base_C)<r_e`. Cette garde interdit l'underflow d'un seuil `u64`.
À `r_core=0`, conserver `max(h_core,h_c(c),base_C)` évite de compter deux fois
un cœur inconnu. Si un vrai centre shallow appartient à la cellule, son rôle
seed reste dans `A_C` : le filtre est fail-open.

La somme exacte des **co-incidences cellulaires non orientées** ayant au moins
un rôle seed est :

```text
P_role = sum_C [binom(|U_C|,2) - binom(|U_C minus A_C|,2)]
       = sum_C [|A_C|*(|U_C|-|A_C|) + binom(|A_C|,2)]
```

Le produit orienté `|A_C|*|U_C|` n'est qu'un majorant : il inclut la classe
propre et double les paires dont les deux lignes portent un seed. `P_role` est
lui-même un majorant fail-open avant rejet des parallèles, vérification que
l'intersection tombe dans la cellule et appartenance au fragment exact. Ce
n'est ni le nombre de paires distinctes globales ni celui des centres, car une
paire peut traverser plusieurs cellules. Le `P_grid` grossier
`sum_C binom(|U_C|,2)` le majore seulement si les deux compteurs emploient
exactement les mêmes classes de droites et les mêmes incidences ; des classes
de segments différentes annulent cette comparaison.

Le contrat géométrique comprend extrémités exactes des cordes et fragments,
statut ouvert/fermé de chaque frontière stricte, rasterisation des arêtes et
coins, puis cellule propriétaire canonique. Une intersection sur un bord ne
doit être ni perdue ni facturée comme deux centres. Tant que ces règles et la
fixture « exactement une face `abv` aiguë » ne passent pas contre l'oracle,
`P_role` reste counter-only. Publier directement sa somme entière ainsi que
`seed_lines_before`, `h_c_dead`, `a_max`, `u_max`, sommes d'incidences seed et
complétion, fragments, concurrences et budgets dépassés ; une moyenne ou une
variance sur un unique `l_C` n'a plus la bonne sémantique.

Un faisceau peut garder `P_role` quadratique. Réduire les rôles seed par
`h_c`, raréfier `S_h`, borner les co-incidences ou exploiter le parallélisme
sont des mécanismes possibles, ni nécessaires ni suffisants isolément. Le
critère reçu est que `P_role` **et tous les autres termes payés** soient
sous-quadratiques sur le régime annoncé. Le shadow fait son preflight complet,
choisit atomiquement grille ou fallback avant toute émission et ne se confond
pas avec le futur constructeur de niveaux shallow.

À un centre concurrent, toutes les provenances de toutes les classes de lignes
incidentes/coïncidentes sont sur la coquille. Elles sont d'abord dédupliquées
par `upos` : plusieurs fragments, cellules ou rôles d'un même point ne mettent
ce témoin à zéro qu'une fois. La profondeur stricte de la strate 0D s'évalue en
bloc — retirer les sorties, mettre tous ces `upos` à zéro, évaluer, puis ajouter
les entrées — jamais depuis une cellule adjacente. Le
centre ne peut émettre une `BallKey` qu'après avoir trouvé deux classes de
droites distinctes et des provenances avec au moins un seed aigu qui satisfont
rang, lentille, owner, exact-once et bien-centrage. L'exact-once choisit le plus
petit `PointId` entre les deux seules faces `abc` et `abd` incidentes à l'arête
owner qui sont aiguës ; une orientation non canonique qui échoue ne condamne
pas la paire.

Un premier couple concret peut certifier l'existence ; s'il échoue alors que
les classes portent d'autres provenances, on ne peut pas jeter le centre sans
une porte supplémentaire ou le fallback exact. Si `t` désigne le nombre total
de provenances, et non de classes, tester les produits de leurs listes peut
réintroduire `t^2`; avant quotient exact-once, ce coût orienté est facturé
comme `sum_C sum_{L != M}|SeedProv[L,C]|*|CompletionProv[M,C]|`. Le cas
`L=M` a déterminant nul et ne définit aucun centre isolé. Lorsque les deux
classes portent des seeds, les deux orientations restent candidates jusqu'aux
tests canoniques, puis une seule occurrence validée est conservée. Il faut une
porte d'existence canonique sous-quadratique, ou un quotient/refus
transactionnel explicite. Le coût `i_e` des incidences et de cette porte est
ajouté au grand-livre. Si le contrat exige l'expansion des paires de supports,
ce coût est nommé comme sortie et ne peut pas être caché.

Le plafond de coquille courant donne néanmoins une fast path constructive.
L'ancre et le centre déterminent déjà une `BallKey` provisoire, sans choisir une
paire de provenances. Grouper d'abord globalement ces tentatives avec les
occurrences q2/q3 déjà valides. Le seuil `h_q=smax-q+1` et le cap intérieur
`smax-q` sont toujours choisis depuis l'arité minimale des occurrences
**validées** ; une q4 provisoire ne peut jamais imposer `h4` à une clé q2/q3.
Une exploration q4-only peut employer le seuil 8 pour tuer cette seule
tentative ou récupérer sa coquille, mais son verdict ne modifie pas les autres
arités du groupe.

Dans une coquille acceptée par `ball_census(shell_cap<=12)`, conserver toutes
les ancres et listes de provenances pendant le RLE. Intersecter d'abord chaque
liste avec la coquille `U_B`, puis la dédupliquer par `upos`; les incidences de
fragments/cellules d'un même point sont combinées par OR. On peut alors rejouer
au plus 144 couples **par ancre**, ou valider globalement au plus
`binom(12,4)=495` quadruplets de coquille ; sans cette intersection et cette
déduplication, seule la seconde borne reste sûre. Garder une seule ancre
représentante serait incomplet. Chaque couple doit recertifier, pour la même
ancre et le même tape, un seed pris dans `SeedProv[L,C]`, une complétion prise
dans `CompletionProv[M,C]` avec `L != M`, puis distance, lentille, owner,
exact-once et centre strict. Après le premier support valide, recalculer
l'arité minimale valide et appliquer son vrai seuil/cap. Si un couple concret
valide était déjà connu, un overflow devient le `resource_exhausted`
transactionnel prévu. Si une occurrence q2/q3 de la clé est déjà validée, son
propre contrat d'arité et de census gouverne le groupe : l'overflow ne peut ni
être ignoré à cause du statut provisoire de q4, ni invalider cette occurrence.
En revanche, pour une clé **q4-only provisoire**, sans aucune occurrence q2/q3
validée, un overflow avant d'avoir certifié un support q4 valide ne suffit pas
à refuser : le centre pourrait n'avoir aucune paire admissible. Ce cas doit
retomber sur une porte d'existence exacte ou le chemin historique et reste un
verrou de la borne globale.

Le flux est donc `tentatives -> RLE global -> census/cache -> validation ->
C_emit`; aucune clé seulement provisoire n'entre dans le catalogue ou
`digest_balls`. Une occurrence q2 ou q3 valide de la même clé conserve son
propre résultat même si toutes les provenances q4 échouent.

### Partage q3/q4 sans assimilation abusive

Le moteur, les coefficients de droites, les groupes de dégénérescence et une
partie du tri peuvent être partagés. Les comptes ne le sont pas implicitement :
`W4` est inclus dans `W3`, mais `h4=8`, `h3=9` et les sources historiques de
scan diffèrent. Chaque entrée du tape porte donc ses masques de lane et chaque
zone garde son seuil. Pour la première requalification q4, conserver
`scan_source=cover3` afin de reproduire le flux historique ; un cover 4 plus
complet est un autre contrat et peut changer `digest_balls`.

Concrètement, ne jamais fusionner trois populations : `witness_tape` est le
`scan_sites()` qui compte la profondeur, `q3_query_tape` contient les carriers
à interroger, et `q4_support_tape` porte les rôles carrier/complétion. `h_c`
ferme un rôle de fibre ; il ne retire pas le même point du tape de témoins.

## Route adaptative qui conserve la borne

Un arrangement exact a un coût fixe et ne gagnera pas sur toutes les petites
ancres. Employer un seuil de fanout constant `m0`, fixé avant les mesures :

- `m_e<=m0` : route historique corrigée ; en q4, le minimum exact sur corde
  peut supprimer la boucle D d'une face ;
- `m_e>m0` : moteur de faibles profondeurs obligatoire.

La petite route q3 coûte au pire `O(m0*m_e)`. La route historique q4 peut
encore payer `C x D x profondeur`, donc `O(m_e^3)` ; sous le seuil elle vaut
`O(m0^2*m_e)`. Comme `m0` est constant, aucune des deux ne change l'exposant
global. Le seuil ne doit pas être choisi après coup par famille.

Le minimum exact sur corde qui trie **toutes** les racines est un excellent
oracle et un bon terminal pour une face déjà matérialisée. Une version
top-seuil pourrait ne conserver que les `h4` entrées et sorties extrêmes, mais
elle exige encore un lemme d'ordre ; l'implémenter avant cette preuve serait un
faux écrêtage. Même reçue, elle coûte `O(F_e*m_e)` sur `F_e` faces et ne
mutualise rien entre elles : ce n'est pas la route sous-quadratique finale. De même, la
condition nécessaire `2*P(D)^2<=J*B(D)^2` évite Cramer et le centre, mais elle
arrive après l'énumération de `D`.

Son compte constant doit inclure tous les témoins actifs sur toute la corde :
`B=0,P<0`, mais aussi une entrée `B>0` dont la racine est avant `alpha` et une
sortie `B<0` dont la racine est après `beta`. Les deux cas opposés hors corde ne
témoignent jamais. Omettre cette classification rendrait même l'oracle local
faux.

## Borne cible et sens précis de « sous-quadratique »

Notons :

- `R` le nombre d'états/terminaux WSPD ;
- `V_wspd` les visites de l'index par les compteurs de témoins pendant la
  descente WSPD ;
- `V_block` les visites réellement payées par les requêtes de facteur et de
  blocs de carriers, hors expansion des plages de points ;
- `H_rect=sum_R handle_mass(R)` la masse offerte par les covers de rectangles,
  une fois par rectangle, ventilée entre `A union B` et son extérieur ;
- `H_scan=sum_e cover_points_tested(e)` la masse réellement reparcourue par
  ancre avant compaction ; sur le produit courant, c'est essentiellement
  `sum_R anchors_surv(R)*handle_mass(R)`, et non `H_rect` ;
- `E` les ancres résiduelles **étiquetées par lane** ; une même géométrie q3/q4
  compte deux fois tant qu'un partage de tape n'est pas reçu ;
- `m_e=|scan_sites_e|` les demi-plans restant après compaction pour l'ancre
  `e`, et `M_anchor=sum_e m_e`, distinct de `H_scan` ;
- `V_hc` les visites, événements et tris de racines réellement payés pour
  `CoreCredit`, les records et `h_c`, et `V_frag` l'émission/rasterisation de
  tous les fragments q4 ;
- `a_e` les classes de lignes portant un seed aigu vivant et `u_e` les classes
  éligibles comme complétions q4 ;
- `k_e` ses centres q3 désignés ;
- `z_e` ses centres q4 canoniques peu profonds, comptés par ancre avant la
  déduplication globale ;
- `i_e` les incidences de groupes concurrents réellement traitées ;
- `Q_try` les occurrences géométriques provisoires avant validation de support,
  `B_try` leurs clés uniques après fusion globale, `V_census_try` les visites de
  census correspondantes, `S_shell_try` leur masse de coquille matérialisée et
  `V_support_try` les couples de provenances/fallbacks testés ;
- `C_emit` les enregistrements candidats remis au tri/RLE ;
- `B` les `BallKey` distinctes conservées après RLE ;
- `S_shell=sum_key |U_B|` la masse des coquilles complètes matérialisées par le
  census ;
- `S_expand` l'expansion supplémentaire des supports/incidences demandée par
  l'API ;
- `V_census` et `V_forest` les visites/travaux du census et de la réduction
  demandée ;
- `S_forest` les enregistrements ou octets de forêt effectivement publiés ;

Si le constructeur stratifié `A_e` est reçu, la cible du pipeline couvert par
ce schéma est :

$$T_{pipeline}=O\left(n\log n+R+V_{wspd}+V_{block}+H_{rect}+H_{scan}+M_{anchor}+V_{hc}+V_{frag}+\lvert E\rvert+\sum_{e\in E}\left(A_e(m_e,a_e,u_e,h_3,h_4)+k_e\log m_e+z_e+i_e\right)+Q_{try}\log Q_{try}+B_{try}+V_{census,try}+S_{shell,try}+V_{support,try}+C_{emit}\log C_{emit}+B+V_{census}+S_{shell}+V_{forest}+S_{forest}+S_{expand}\right).$$

Le terme de tri peut être remplacé seulement par un constructeur canonique
linéaire prouvé. `S_shell` ne paie ni les visites d'index du census, ni celles
de la forêt ; `S_expand` ne les absorbe pas non plus. `S_forest` interdit de
cacher les nœuds, attaches, deltas ou digests publiés derrière un simple compte
de visites. La sémantique de `V_forest/S_forest` doit en outre nommer le
sous-flot Gabriel actuel ou le contrat Gamma futur. `H_rect` ne remplace
`H_scan` qu'après une mutualisation réellement implémentée par rectangle. Les
termes `*_try` empêchent qu'une masse quadratique de centres shallow sans aucun
support admissible disparaisse parce qu'elle n'atteint jamais `C_emit`.

`A_e` désigne ici le coût cumulé du moteur stratifié, rôles q4 compris. Si les
masques et sources des lanes empêchent un partage reçu, il faut sommer
`A_e,3` et `A_e,4`, pas facturer artificiellement une seule construction.

Avec `s=8`, `h3=9` et `h4=8`, `R=O(n)` est une **précondition encore à
prouver**, pas un résultat v5. Le pipeline est sous-quadratique sur une classe
d'entrées seulement si `V_wspd`, `V_block`, `H_rect`, `H_scan`, `M_anchor`,
`V_hc`, `V_frag`, les ancres, les incidences/logarithmes, le tri, le census, la
forêt et la sortie demandée sont tous `o(n^2)`, y compris `Q_try`, `B_try`,
`V_census_try`, `S_shell_try`, `V_support_try`, `B`, `S_shell`, `S_forest` et
`S_expand`. Une formulation plus utile introduit la charge moyenne
contrôlable `Lambda=(|E|+sum_e(m_e+a_e+u_e+k_e))/n` : `Lambda` polylogarithmique vise le
quasi-linéaire seulement si les autres termes le sont aussi, tandis que
`Lambda=O(n^(1-epsilon))` donne une contribution sous-quadratique à facteurs
logarithmiques près.

Cette hypothèse d'incidence doit être vérifiable ou au moins réfutable sur
l'entrée ; « dimension intrinsèque faible », « WSPD linéaire » ou « candidats
linéaires » ne la remplace pas. `scanline` illustre précisément l'échec : le
front et les candidats peuvent paraître presque linéaires alors que la masse
d'ancres q4 mesurée approche `n^1.98`.

Le produit complet possède déjà une obstruction quadratique reçue en
q2/Gabriel. La construction critique liée citée plus haut donne en outre
`Omega(n^2)` `BallKey` distinctes dans chacune des lanes q3 et q4 du modèle
exact. Sa transposition au profil u16 doit être une fixture v5 entière avant de
devenir une preuve contractuelle du profil, mais elle suffit déjà à interdire
un objectif d'API universel qui ne facture pas la sortie.

## Choix par régime

| Régime | Première porte | Route résiduelle | Condition de viabilité |
|---|---|---|---|
| volumique homogène | cœur puis facteurs | petite route, arrangement sur queue lourde | somme des covers et sorties quasi linéaire |
| amas séparés | `h_a/h_b`, puis `h_c` inter-amas | arrangement intra-amas lourd | visites mixtes et charge locale sous-quadratiques |
| surface/terrain borné | patches et `h_c` | arrangement q3/q4 prioritaire | canopée physiquement bornée et incidences mesurées |
| terrain historique | mêmes portes | banc adversarial obligatoire | aucun claim tant que l'échelle de canopée croît avec `n` |
| scanline/filiforme exact | rang/déterminant, facteurs, carriers hiérarchiques | arrangement seulement après réduction des ancres | NO-GO si ancres résiduelles ou somme des covers restent proches de `n^2` |
| cosphérique/cocyclique | détection de groupe | quotient par centre et census | jamais développer les paires sans facturer la sortie |
| adversarial peu profond | toutes les portes sûres | budget transactionnel | sortie/charge dense annoncée, jamais faux succès |

Pour un nuage exactement de rang affine inférieur au besoin de la lane, la
porte de déterminant peut fermer en bloc. Une simple faible épaisseur ne permet
aucune approximation : elle doit passer les certificats exacts ou être traitée
comme un régime général.

Sur les reçus actuels, les candidats q3 restent proches de linéaires, mais les
seeds `terrain` ont des pentes locales supérieures à 2 sur la dernière
doublure. En q4, les essais `D` par face croissent fortement sur `terrain` et
`scanline`, beaucoup moins sur `uniform`. Cela motive l'adaptation par fanout,
pas un choix unique de micro-kernel.

## Ordre concret proposé à Claude

### P0 — fiabiliser ce qui mesure

1. Conserver le replay shaped d'ordre et son plancher `20000` comme régression
   d'intégration, puis graver la fixture exacte cinq points dans le helper et
   `process_anchor_q4`, et compiler/rejouer le kernel ; garder CUDA non reçu
   sans `nvcc`. Les `426` désaccords sont ceux de ce reversal précis, pas le
   compte universel des seeds historiquement manqués.
2. Ajouter la porte exacte `2*P(D)^2<=J*B(D)^2` comme rejet distinct, sans lui
   attribuer un gain d'exposant.
3. Publier des timers non chevauchants ou explicitement inclusifs, puis fermer
   `seeds = cells + core + chord + faces_D` sans soustraction saturée.
4. Graver `linked_arcs_u16` contre l'oracle exhaustif et le pipeline : comptes
   q3/q4 littéraux, profondeur zéro, shell égal au support, clés post-RLE et
   owner exact-once. Cette porte fixe le contrat sortie-sensible avant toute
   promesse de coût.

### P1 — recevoir le front et retirer les auto-produits

1. Exposer le cube `Q` existant, partager split/tie-break par diamètre de `Q`,
   puis implémenter le shadow `SepQ` et le terminal `SepQ || SepTight`.
2. Recevoir le quotient octree, ses fibres avec leur cardinal réellement
   mesuré, le ledger global de `R_cut` et le coarsening, à `s=8`, `postsep=0`.
   Ne fixer une borne constante de fibre qu'après preuve de la simulation et de
   l'injection exact-once ; le nombre `49` n'est pas encore un contrat.
3. Ajouter le garde cardinal puis remplacer `corner_histograms` par les
   requêtes `ALL/NONE/MIXED` saturées et la porte `all_dead` avant handles.
4. Émettre le résidu dans l'ordre historique et publier `V_R+C_R+P_R`; aucun
   claim si les visites mixtes gardent l'auto-produit.

### P2 — mesurer le vrai résidu `h_c`

1. Construire `CoreCredit` et `ResidualTape` : collecter avec `cap=ar.core`,
   accepter nominalement seulement `r_core=h_core`, sinon invariant et fallback
   sans exclusion. Transmettre la seule opération
   `compose(residual)=h_a+h_b+r_core+max(h_core-r_core,residual)` aux secteurs,
   `h_c` et à la grille. Graver aussi les cas partiels comme fixtures de sûreté
   et l'héritage postsep.
2. Recevoir d'abord `h_c_q3_point` scalaire et un shadow grille q3 sur le même
   tape, avec `max` entre leurs bornes. Construire ensuite l'oracle du minimum
   q4 sur toute la corde et comparer chaque face au scan exhaustif,
   racines/frontières et échange des quantificateurs compris.
3. Garder q4 counter-only et record par record jusqu'à parité. Un
   `CarrierBlock` vient seulement ensuite, avec le minimum uniforme sur ses
   descendants ; tout inconnu reste `MIXED` et aucun point n'est retiré du tape
   de témoins ou du rôle de complétion.
4. Mesurer les fibres fermées **avant matérialisation**, `H_rect`, `H_scan`,
   `M_anchor`, `V_hc`, `V_frag`, distribution des `m_e` et incidences `K_c`.
   Abandonner la voie « WSPD généralisée seule » dès qu'un de ces termes garde
   une pente proche de 2.

### P3 — construire le moteur de faibles profondeurs

1. Livrer d'abord le shadow le plus court : trier les `BallKey` en runs par
   chunks, les fusionner/RLE globalement avec arité minimale et représentant
   canonique, puis appeler `ball_depth_at_least`, comparer chaque fate au scan
   historique et publier visites/range-add/replis. Ne poursuivre l'arrangement
   que sur les régimes où cette route exacte laisse un verrou mesuré.
2. Commencer alors par un oracle demi-plans exhaustif pour `n<=14` et les deux
   seuils stricts. Graver sites axiaux constants, faces 1D/0D, droites
   parallèles, classes confondues, concurrence multiple, témoin sur frontière,
   groupes `witness_only` et source `cover3` suivie du census.
3. Livrer `PlanConflictGrid` en shadow q3 : affine, classification de
   cellule et propriétaire rationnel W256 contre le scan exhaustif. Essayer une
   grille de résolutions prédéfinies, puis publier `I_conf`, `K_conf` et
   `rho_qc`; `G≈sqrt(k_unique)` reste une hypothèse mesurée.
4. Ajouter le shadow q4 counter-only orienté : lignes de complétion `U_C`,
   lignes de seeds aigus `A_C`, toute la corde des seeds survivants ou tous ses
   fragments exacts, `P_role` et budgets dépassés. Graver le cas où une seule
   des deux faces incidentes est aiguë. Un raccord produit ultérieur devra
   faire son preflight et son fallback avant toute émission.
5. Construire ensuite le sous-complexe orienté de faibles profondeurs ; un
   mutant qui accepte un centre sans paire incidente admissible doit tomber
   sans énumération quadratique. Graver séparément un groupe concurrent de
   multiplicité `t` : la porte doit trouver un support admissible canonique ou
   refuser transactionnellement sans tester les `t^2` paires.
6. Comparer la correction de corde et la petite route au flux brut historique.
   Pour la route quotientée, comparer le set et les niveaux post-RLE des
   `BallKey`, le census, les fates, événements, forêt et digests ; conserver un
   ledger distinct de la multiplicité brute supprimée.
7. Garder la route historique sous un `m0` constant et router les ancres lourdes
   vers l'arrangement.

## Compteurs et portes go/no-go

Chaque run doit publier au minimum :

- WSPD : états, terminaux ombre/réels, visites de témoins, charge maximale,
  `H_rect`, `H_scan`, masse dans/hors facteurs, multiplicité rectangle--site et
  HWM ;
- facteurs : requêtes bulk/mixed, visites, saturations, couples `PENDING` et
  rectangles fermés avant handles ;
- `h_c` : indices `upos` cœur attendus/recertifiés, replis de provenance,
  nœuds/patches morts, vivants, mixtes, `V_hc`, `V_frag`, incidences `K_c`,
  fibres évitées et résidu matérialisé ;
- ancres : distribution de `m_e`, `k_e`, somme `m_e`, somme `m_e*log(m_e)` et
  queue par octaves ;
- q3 : points localisés, verdicts de seuil exacts, profondeurs complètes
  seulement lorsque le census les a réellement calculées, `I_conf`, `K_conf`,
  `rho_qc`, conflits/requêtes maximum et quantiles, fallback et
  supports/`BallKey` ;
- q4 : `faces_D`, essais `D`, groupes de droites, sommets par profondeur,
  cellules shallow, seeds avant/après `h_c`, lignes de complétion, fragments,
  incidences `A_C/U_C`, `P_role`, multiplicité maximale, `Q_try`, `B_try`,
  `V_census_try`, `S_shell_try`, `V_support_try`, centres émis et census ;
- candidats remis au tri, comparaisons/RLE, `B`, `S_shell`, expansion des
  supports, visites du census et de la forêt ;
- temps exclusifs, HWM, sortie canonique et expansion demandée.

Porte sémantique : égalité exhaustive des candidats bruts seulement lorsque la
représentation et la source ne changent pas. Le quotient par centre compare les
`BallKey` et niveaux post-RLE, puis census, événements, fates, forêt et digests,
avec multiplicité brute auditée séparément. Toutes les frontières strictes et
mutants ciblés doivent tomber.

Porte de coût pour une classe annoncée : cinq tailles, graines 3/4/5, aucune
censure silencieuse, et borne supérieure de pente strictement inférieure à 2
pour **chaque** terme payé, pas seulement le mur total. La mesure valide le
domaine de l'hypothèse ; elle ne remplace ni la preuve WSPD ni la preuve du
constructeur de niveaux.

NO-GO si la somme cumulée des mots de bitsets denses, des réénumérations de
carriers ou des arrangements complets n'a pas de borne sous-quadratique sur le
régime annoncé, si les paires d'un plateau sont développées sans contrat de
sortie, ou si un terme payé est omis du grand-livre. Un balayage dense unique
est linéaire, et un arrangement complet sous `m0` constant reste compatible
avec la petite route ; c'est leur répétition non bornée qui est interdite.

## Questions à Claude avant de raccorder

1. Le front `Q` partage-t-il maintenant strictement split et tie-break entre
   shadow, WSPD produit et `alive_rectangles` ?
2. Où les indices `upos` de `h_core` sont-ils recertifiés puis exclus du tape
   résiduel avant de composer les crédits, et quel est le repli postsep ?
3. Quel est, après ces quatre crédits, le nombre d'ancres résiduelles,
   `H_scan`, `M_anchor`, `K_conf` q3 et `P_role` q4 par régime ?
4. Le premier moteur q4 conserve-t-il `cover3` pour la parité historique, ou
   ouvre-t-il explicitement une requalification `cover4` ?
5. La sortie demandée est-elle le quotient par `BallKey`, le shell complet ou
   l'expansion des supports ?
6. La fixture de corde à deux permutations, shaped puis device, est-elle reçue
   en plus du plancher de masse avant d'interpréter le nouveau profiler ?

Ces réponses déterminent la structure à coder. Elles ne bloquent pas le shadow
`h_c`, mais elles bloquent tout claim de sous-quadraticité et toute conclusion
tirée des taux de la corde actuelle.
