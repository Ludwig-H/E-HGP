# Stratégie q3/q4 sous-quadratique à proposer à Claude — 30 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Périmètre relu : tout `morsehgp3D_v5/`, notamment les sources WSPD, index,
pipeline q3/q4, filtres CPU/device, oracles, probes, reçus d'échelle, documents
mathématiques et audits actifs, puis le commit source `HEAD=eba24b9a` qui ferme
le build/parsing et ajoute l'instrumentation q4. Le worktree d'audit reste
concurrent. Ses modifications non commitées ne sont jamais substituées au pin ;
aucun claim de résultat, de complexité ou de fraîcheur ne leur est attribué.

Mise à jour sur la base relue `1e4e0845` : le source q4 est toujours celui du commit
`e2ac9da2`, et la contre-relecture WSPD/facteurs/moteur plan ci-dessous porte
sur ce nouvel état. La porte de masse `chord_positive` est reçue comme
régression de compteur, mais pas comme fixture de correction : le `continue`
positif précède encore `chord.dead(h4)` et le device garde l'ancien branchement.
Ce verrou reste donc ouvert malgré les trois CTests verts rejoués localement.
Claude a par ailleurs retiré dans `d723b68a` son interprétation des exposants
mono-graine de la canopée bornée ; les mesures à trois graines confirment un
mur de compteurs sur `terrain`, mais ne bornent aucun terme de l'algorithme
proposé ici.

Cette stratégie optimise uniquement la source horizontale actuelle sous
`forest_semantics=verified_events_only`. Elle ne fournit ni les incidences
silencieuses Gamma, ni la reconstruction correspondante, ni une raison
d'accepter `require_exact=true`. Le P0 Gabriel/Gamma et l'ordre de fermeture
d'`ETAT_COURANT.md` restent prioritaires.

## Verdict

La direction `A x B`, citron, `h_core`, `h_a`, `h_b`, puis `h_c` est bonne,
mais elle ne suffit pas seule. Le candidat architectural le plus complet relu
est :

```text
WSPD binaire A x B, pilotée par des cellules, à prouver à s=8
  -> mort du rectangle et requêtes h_a/h_b sans auto-produits
  -> parcours hiérarchique des carriers et certificat h_c
  -> ancres réellement résiduelles
       petit fanout : route historique exacte
       grand fanout : arrangement des faibles profondeurs dans le plan médiateur
  -> quotient par centre, puis census exact requis par le contrat
```

Le même moteur plan sert aux deux lanes, sans confondre leurs seuils ni leurs
sources :

- q3 teste le centre désigné porté par chaque droite de carrier ;
- q4 énumère seulement les intersections de droites de profondeur strictement
  inférieure à 8.

Cette mutualisation est le seul candidat relu qui pourrait retirer
structurellement les deux produits tardifs `seed x cover` en q3 et `C x D` en
q4, sous réserve de recevoir son constructeur exact. `h_c` est le bon
préconditionneur pour ne pas matérialiser les fibres mortes ; le sous-complexe
de faibles profondeurs est le terminal proposé pour les fibres vivantes à
grand fanout.

Le statut présent reste **NO-GO pour un claim sous-quadratique global**. La
cible recevable est une borne conditionnelle, sensible aux incidences et à la
sortie. Certaines familles pourraient devenir quasi linéaires si tous les
termes du grand-livre sont bornés ; une famille dont la masse résiduelle
d'ancres ou de covers reste quadratique ne sera pas sauvée par une WSPD
linéaire.

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

Le profiler q4 candidat ne doit pas encore guider l'architecture. Le code
scalaire, shaped et device saute actuellement certains sites certifiés `P>0`
avant `ChordPieces::update`, alors que `P-mu*B` peut devenir négatif sur un
morceau extérieur. Le théorème et l'ancien probe tous-sites ne mesurent donc
pas le chemin produit. Les timers `boucle_seeds`, cœur/corde et complétions
sont en outre emboîtés : ils ne doivent pas être additionnés.

Enfin, le plateau cocyclique historique v3/v4 à 384 points et 2 322 560
supports pour une même `BallKey` interdit de cacher le coût de l'expansion
demandée. Ce différentiel n'est pas encore une fixture v5 requalifiée et ne
prouve pas, à lui seul, une borne inférieure quadratique en `BallKey`
distinctes. Les audits doivent distinguer sortie canonique, incidences de
coquille et expansion explicite des supports. Sa source différentielle est
`morsehgp3D_v4/audits/lectures_20260817/proposition.md` § 5.6.

## Étage 0 — rendre le front binaire prouvable

La WSPD binaire reste le bon front : une WSPD symétrique ternaire réintroduit
l'obstruction cercle--axe et n'est pas la généralisation recherchée. En
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

La seule multiplicité sept ne prouve pas encore le facteur 49. Il faut recevoir
un lemme de simulation : les arêtes sortantes de chaque composante vont vers
au plus huit octants distincts ; les états à cellules égales ou imbriquées ne
terminent pas par `SepQ` ; la contraction simule exactement l'expansion
multiway ; et l'exact-once des graines LCA empêche de produire deux fois un
même couple. On peut alors injecter chaque terminal binaire dans
`(terminal_octree,rank_left,rank_right)` et borner une fibre canonisée par
`7*7=49`. Sous ce lemme écrit et reçu, le squelette de preuve vise
`R_shadow<=49*R_oct<=49*C_3(8)*m` pour `m` positions uniques. Le facteur 49 est
un majorant du déroulage Patricia, **pas** la constante WSPD totale : le
charging `C_3(8)` doit encore traiter les boîtes entières de largeur `2^k-1`,
les cellules virtuelles des arêtes comprimées, leur disjonction et leur
distance bornée. Il est interdit d'inventer une valeur numérique pour cette
constante.

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
Morton suivant, quotient octree de fanout huit, fibre terminale au plus 49,
mapping terminal ombre vers coupe réelle, `R_emis<=R_cut<=R_shadow`, ledger
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

Chaque crédit conserve ses IDs ou une partition de provenance. Les témoins du
cœur, de `h_a`, de `h_b`, de la grille et de `h_c` ne s'additionnent pas si
leurs domaines peuvent se recouvrir. Un maximum sûr ne doit pas être remplacé
par une somme sans preuve de disjonction.

## Étage 2 — le rôle exact de `h_c`

La proposition de Claude sur `A x B x C` est la bonne étape suivante, à
condition que `C` soit parcouru hiérarchiquement. Calculer `h_c(c)` après avoir
énuméré toutes les faces ne change que la constante.

Les handles bornés à 32 et les patches de centres donnent d'abord un certificat
par carrier feuille. Pour chaque patch faisable, on combine le crédit extérieur
partagé, le crédit propre au patch et le crédit propre au carrier avec une
partition de provenance explicite. La forme condensée ci-dessous est celle de
q4 ; q3 emploie son seuil et ses patches propres :

```text
b_i,j(c) = max(rect_core4, g_not_i,j + max(g_i,j, h_c,j(c)))
tau_i(c) = max over feasible j of max(0, h4 - b_i,j(c))
face closed iff h_a(a) + h_b(b) >= tau_i(c)
```

Un masque faisable vide est une absence, jamais `tau=0`. La somme locale
`64*sum_i(m_i^2) <= 2048*sum_i(m_i)` rend un premier shadow plat raisonnable,
mais ne borne ni le nombre global de handles ni les fibres résiduelles.

Cette formule ne ferme pas automatiquement un nœud interne `H`. Il faut un
minorant de témoins uniforme sur tous les carriers valides de `H`, avec leurs
masques faisables, ou une majoration certifiée
`tau_i(H)>=max_{c in H} tau_i(c)` obtenue sans visiter les feuilles. Le nœud est
fermé seulement si `h_a(a)+h_b(b)>=tau_i(H)`. Tout descendant non classé force
le fate `MIXED`; il est interdit d'inférer la fermeture du seul carrier ayant
fourni le meilleur crédit.

Ordre de calcul recommandé :

- construire d'abord `tau` avec `h_c=0` ;
- ne payer `Phi32` que sur le résidu ;
- fermer les nœuds de carriers entiers ;
- descendre seulement les nœuds mixtes ;
- matérialiser un carrier feuille seulement s'il reste réellement vivant.

`h_c` ferme un **rôle de seed/fibre**, pas l'identité du point. Un carrier tué
peut encore être témoin, complétion q4 ou membre du census de coquille. Les
bitsets de rôles et le ledger doivent donc survivre à la fermeture.

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
import de l'oracle. Avec `G_grid` la résolution et
`K_conf=sum_query |conf[cell(query)]|`, le coût publié est
`O(G_grid*m_e+k_e+K_conf)` après rasterisation, ou
`O(G_grid^2*m_e+k_e+K_conf)` au premier brouillon. À `G_grid` fixé, `K_conf`
peut encore valoir `Theta(m_e*k_e)` : ce shadow mesure le verrou, il ne le
ferme pas.

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

Le même `PlanConflictGrid` fournit un shadow q4 limité : rasteriser les lignes
incidentes aux cellules, former seulement les paires de classes non parallèles,
garder l'intersection dans sa cellule canonique puis appliquer Cramer et les
portes existantes. Son coût critique est
`P_grid=sum_C binom(line_incidence_C,2)`. Un faisceau peut rendre ce terme
quadratique. Le premier shadow q4 est donc strictement counter-only et n'émet
rien. Un futur candidat produit devra faire un preflight complet, puis choisir
atomiquement la route grille ou le fallback avant toute émission. Il mesure si
la grille suffit sur un régime, sans être confondu avec le constructeur optimal
visé.

Un centre concurrent ne peut émettre une `BallKey` qu'après avoir prouvé qu'il
existe une paire incidente distincte qui satisfait rang, rôles de support,
lentille, owner, exact-once et bien-centrage. Tester toutes les paires d'un
groupe de multiplicité `t` réintroduirait `t^2`. Il faut une porte d'existence
canonique sous-quadratique, ou un quotient/refus transactionnel explicite. Le
coût `i_e` des incidences et de cette porte est ajouté au grand-livre. Si le
contrat exige l'expansion des paires de supports, ce coût est nommé comme
sortie et ne peut pas être caché.

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

Le minimum exact sur corde, avec au plus huit racines d'entrée et huit de
sortie, est un excellent oracle et un bon terminal pour une face déjà
matérialisée. Il coûte encore `O(F_e*m_e)` sur `F_e` faces et ne mutualise rien
entre elles : ce n'est pas la route sous-quadratique finale. De même, la
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
  carriers ;
- `E` les ancres résiduelles **étiquetées par lane** ; une même géométrie q3/q4
  compte deux fois tant qu'un partage de tape n'est pas reçu ;
- `m_e` les demi-plans construits pour l'ancre `e` ;
- `k_e` ses centres q3 désignés ;
- `z_e` ses centres q4 canoniques peu profonds, comptés par ancre avant la
  déduplication globale ;
- `i_e` les incidences de groupes concurrents réellement traitées ;
- `C_emit` les enregistrements candidats remis au tri/RLE ;
- `V_census` et `V_forest` les visites du census et de la réduction demandée ;
- `S` la taille de sortie effectivement demandée, expansion comprise.

Si le constructeur stratifié `A_e` est reçu, la cible du pipeline couvert par
ce schéma est :

$$T_{pipeline}=O\left(n\log n+R+V_{wspd}+V_{block}+\lvert E\rvert+\sum_{e\in E}\left(A_e(m_e,h_3)+k_e\log m_e+z_e+i_e\right)+C_{emit}\log C_{emit}+V_{census}+V_{forest}+S\right).$$

Le terme de tri peut être remplacé seulement par un constructeur canonique
linéaire prouvé. `S` ne paie ni les visites d'index du census, ni celles de la
forêt. La sémantique de `V_forest` doit en outre nommer le sous-flot Gabriel
actuel ou le contrat Gamma futur.

`A_e` désigne ici le coût cumulé des deux tapes de lane. Si leurs masques et
sources empêchent un partage reçu, il faut sommer `A_e,3` et `A_e,4`, pas
facturer artificiellement une seule construction.

Avec `s=8`, `h3=9` et `h4=8`, `R=O(n)` est une **précondition encore à
prouver**, pas un résultat v5. Le pipeline est sous-quadratique sur une classe
d'entrées seulement si `V_wspd`, `V_block`, les ancres, les
incidences/logarithmes, le tri, le census, la forêt et la sortie demandée sont
tous `o(n^2)`. Une formulation plus utile introduit la charge moyenne
contrôlable `Lambda=(|E|+sum_e(m_e+k_e))/n` : `Lambda` polylogarithmique vise le
quasi-linéaire seulement si les autres termes le sont aussi, tandis que
`Lambda=O(n^(1-epsilon))` donne une contribution sous-quadratique à facteurs
logarithmiques près.

Cette hypothèse d'incidence doit être vérifiable ou au moins réfutable sur
l'entrée ; « dimension intrinsèque faible », « WSPD linéaire » ou « candidats
linéaires » ne la remplace pas. `scanline` illustre précisément l'échec : le
front et les candidats peuvent paraître presque linéaires alors que la masse
d'ancres q4 mesurée approche `n^1.98`.

Le produit complet possède déjà une obstruction quadratique reçue en
q2/Gabriel. Aucune borne inférieure `Omega(n^2)` n'est encore établie pour les
seules `BallKey` q3 ou q4 distinctes ; la présente conclusion porte donc sur
des lanes et des régimes nommés, jamais sur un théorème universel inventé.

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

1. Corriger la corde q4 tous-sites dans scalaire, shaped et device ; graver
   `chord-positive-site` et le mutant `chord-skip-positive`.
2. Ajouter la porte exacte `2*P(D)^2<=J*B(D)^2` comme rejet distinct, sans lui
   attribuer un gain d'exposant.
3. Publier des timers non chevauchants ou explicitement inclusifs, puis fermer
   `seeds = cells + core + chord + faces_D` sans soustraction saturée.

### P1 — recevoir le front et retirer les auto-produits

1. Exposer le cube `Q` existant, partager split/tie-break par diamètre de `Q`,
   puis implémenter le shadow `SepQ` et le terminal `SepQ || SepTight`.
2. Recevoir le quotient octree, les fibres de taille au plus 49, le ledger
   global de `R_cut` et le coarsening, à `s=8`, `postsep=0`.
3. Ajouter le garde cardinal puis remplacer `corner_histograms` par les
   requêtes `ALL/NONE/MIXED` saturées et la porte `all_dead` avant handles.
4. Émettre le résidu dans l'ordre historique et publier `V_R+C_R+P_R`; aucun
   claim si les visites mixtes gardent l'auto-produit.

### P2 — mesurer le vrai résidu `h_c`

1. Construire le shadow `tau(c)` en deux passes, puis le parcours de nœuds de
   carriers avec `tau(H)` uniforme ; tout descendant inconnu reste `MIXED`.
2. Mesurer les fibres fermées **avant matérialisation**, ancres résiduelles,
   somme des covers et histogramme de `m_e`.
3. Abandonner la voie « WSPD généralisée seule » sur tout régime où ces termes
   gardent une pente proche de 2.

### P3 — construire le moteur de faibles profondeurs

1. Commencer par un oracle demi-plans exhaustif pour `n<=14` et les deux
   seuils stricts. Graver sites axiaux constants, faces 1D/0D, droites
   parallèles, classes confondues, concurrence multiple, témoin sur frontière,
   groupes `witness_only` et source `cover3` suivie du census.
2. Livrer d'abord `PlanConflictGrid` en shadow q3 : affine, classification de
   cellule et propriétaire rationnel W256 contre le scan exhaustif.
3. Ajouter le shadow q4 counter-only et publier `K_conf` puis `P_grid`, y
   compris le nombre de budgets qui auraient été dépassés. Un raccord produit
   ultérieur devra faire son preflight et son fallback avant toute émission.
4. Construire ensuite le sous-complexe orienté de faibles profondeurs ; un
   mutant qui accepte un centre sans paire incidente admissible doit tomber
   sans énumération quadratique.
5. Comparer la correction de corde et la petite route au flux brut historique.
   Pour la route quotientée, comparer le set et les niveaux post-RLE des
   `BallKey`, le census, les fates, événements, forêt et digests ; conserver un
   ledger distinct de la multiplicité brute supprimée.
6. Garder la route historique sous un `m0` constant et router les ancres lourdes
   vers l'arrangement.

## Compteurs et portes go/no-go

Chaque run doit publier au minimum :

- WSPD : états, terminaux ombre/réels, visites de témoins, charge maximale et
  HWM ;
- facteurs : requêtes bulk/mixed, visites, saturations, couples `PENDING` et
  rectangles fermés avant handles ;
- `h_c` : nœuds/patches morts, vivants, mixtes, appels `Phi32`, fibres évitées
  et résidu matérialisé ;
- ancres : distribution de `m_e`, `k_e`, somme `m_e`, somme `m_e*log(m_e)` et
  queue par octaves ;
- q3 : points localisés, profondeurs exactes, `K_conf`, conflits maximum et
  quantiles, fallback et supports/`BallKey` ;
- q4 : `faces_D`, essais `D`, groupes de droites, sommets par profondeur,
  `P_grid`, incidences de groupes, multiplicité maximale, centres émis et
  census ;
- candidats remis au tri, comparaisons/RLE, visites du census et de la forêt ;
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
2. Où `h_c` ferme-t-il un nœud de carriers avant toute matérialisation de
   `A x B x C` ?
3. Quel est, après `h_core+h_a+h_b+h_c`, le nombre d'ancres résiduelles et la
   somme exacte de leurs covers par régime ?
4. Le premier moteur q4 conserve-t-il `cover3` pour la parité historique, ou
   ouvre-t-il explicitement une requalification `cover4` ?
5. La sortie demandée est-elle le quotient par `BallKey`, le shell complet ou
   l'expansion des supports ?
6. La fixture de corde à deux permutations, shaped puis device, est-elle reçue
   en plus du plancher de masse avant d'interpréter le nouveau profiler ?

Ces réponses déterminent la structure à coder. Elles ne bloquent pas le shadow
`h_c`, mais elles bloquent tout claim de sous-quadraticité et toute conclusion
tirée des taux de la corde actuelle.
