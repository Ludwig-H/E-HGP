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

## Verdict

La direction `A x B`, citron, `h_core`, `h_a`, `h_b`, puis `h_c` est bonne,
mais elle ne suffit pas seule. La meilleure architecture trouvée est :

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
audits précédents doit donc être resserré : `58 %` de visites WSPD répétées est
un gain d'ingénierie, pas une stratégie d'exposant.

Le profiler q4 candidat ne doit pas encore guider l'architecture. Le code
scalaire, shaped et device saute actuellement certains sites certifiés `P>0`
avant `ChordPieces::update`, alors que `P-mu*B` peut devenir négatif sur un
morceau extérieur. Le théorème et l'ancien probe tous-sites ne mesurent donc
pas le chemin produit. Les timers `boucle_seeds`, cœur/corde et complétions
sont en outre emboîtés : ils ne doivent pas être additionnés.

Enfin, le plateau cocyclique à 384 points et 2 322 560 supports pour une même
`BallKey` interdit de cacher le coût de l'expansion demandée. Il ne prouve pas,
à lui seul, une borne inférieure quadratique en `BallKey` distinctes. Les
audits doivent distinguer sortie canonique, incidences de coquille et expansion
explicite des supports.

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
préfixe `P`. Deux réparations sont possibles :

- préférence : stocker `P` exactement, y compris les un ou deux bits
  résiduels ;
- repli : conserver `Q`, mais prouver `tight(v) subset P(v) subset Q(v)`, la
  monotonie parent--enfant et le fait qu'au plus sept préfixes binaires
  internes partagent un même cube avant le triplet Morton suivant.

Le front ombre emploie seulement la séparation des cellules et scinde le
facteur de plus grand diamètre de cellule, avec tie-break canonique. Le front
réel effectue les mêmes scissions mais termine sur :

```text
SepCell(A,B) || SepTight(A,B)
```

Chaque terminal réel doit alors être l'ancêtre d'un ensemble non vide de
terminaux ombre. Le ledger exact-once est conservé à chaque remplacement
`A x B = A0 x B disjoint_union A1 x B`, et le front réel est un coarsening du
front de packing. Il reste à écrire le charging par nœud et échelle ; une pente
empirique ne le remplace pas.

### `s>=8` ne constitue pas un profil de coût

Le plancher mathématique produit reste 8, mais un claim de coût doit, pour
l'instant, figer **`s=8`**. À `s=INT64_MAX`, tout facteur de diamètre positif
échoue à la séparation et la source produit exactement une paire par couple de
positions, donc un front quadratique. La primitive arithmétique large peut
rester testée hors profil de coût.

Fixtures minimales : `used mod 3 = 0,1,2`, inclusion `tight/P/Q`, multiplicité
du cube au plus sept, diamètre non croissant puis strictement décroissant au
triplet Morton suivant, mapping terminal ombre vers terminal réel,
`R_real<=R_shadow`, ledger exact sous permutations et mutant de scission par
boîte serrée. Exiger une décroissance stricte à chaque enfant réfuterait à tort
le repli `Q`, dont le diamètre peut stagner pendant les bits résiduels.

## Étage 1 — fermer `A x B` sans le développer

Le cœur du citron et les crédits d'extrémités doivent être utilisés ensemble.
Ils répondent à des domaines de témoins distincts et ferment une ancre lorsque
leur somme certifiée atteint le seuil de la lane. Un cœur faible ne doit donc
jamais court-circuiter `h_a` et `h_b`.

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

Fixons une ancre `e=(a,b)`. Les centres des sphères passant par `a` et `b`
vivent dans le plan médiateur de `ab`. Chaque site `z` non axial y définit :

- une droite `ell_z` sur laquelle `z` appartient à la coquille ;
- un demi-plan ouvert `H_z` dans lequel `z` est strictement intérieur.

Les extrémités `a,b` sont exclues du tape. Un autre site situé sur l'axe de
`ab` donne au contraire un signe constant sur tout le plan : il témoigne
partout s'il est strictement dans la boule diamétrale de `ab`, nulle part sinon.
Ces témoins constants sont comptés une fois dans `c0_e`; si `c0_e>=h_q`,
l'ancre est morte, sinon le moteur travaille au seuil résiduel `h_q-c0_e`. Il
ne faut pas inventer une droite dégénérée.

La profondeur **relativement à la source du tape** est `c0_e` plus le nombre de
demi-plans ouverts qui contiennent le centre. Les évaluations restent entières
et strictes ; une droite incidente ne compte jamais son propre site comme
témoin intérieur. Chaque position distincte porte en outre ses rôles
`support_eligible` et `witness_only` : toutes les entrées contribuent à la
profondeur, mais seules les premières peuvent définir un seed q3 ou un sommet
q4 à émettre. Avec `cover3`, cette profondeur n'est qu'un minorant certifié de
la profondeur globale et le census final reste obligatoire.

### q3 : interroger les centres désignés

Après les portes bon marché de rang, non-colinéarité, owner et acuité, chaque
carrier admissible `c` donne un point précis de `ell_c`, le centre de la
circonférence passant par `a,b,c`. Ce point est sur une face 1D du
sous-complexe : une point-location dans une cellule 2D adjacente compterait à
tort un site incident. La requête doit rendre la profondeur stricte de la face
elle-même. Si elle atteint 9, le seed est mort ; sinon les portes exactes
restantes continuent. `k_e` compte tous les carriers effectivement interrogés.

Le raccord algorithmique reste ouvert. Il faut prouver un constructeur du
sous-complexe stratifié de profondeur inférieure à `h3`, dégénérescences du
profil comprises, de coût cible
`A_e(m_e,h3)=O(m_e*log(m_e)+m_e*h3)`. Sous cette précondition, les requêtes q3
coûtent `O(A_e+k_e*log(m_e))`. Si `k_e=O(m_e)`, le produit `seed x cover` serait
alors remplacé par `O(m_e*log(m_e))` à seuil constant. La complexité
combinatoire des niveaux ne fournit pas, à elle seule, ce constructeur exact.

Une structure de cuttings donnant des requêtes sous-linéaires peut servir de
prototype q3 intermédiaire. Elle ne doit pas devenir l'architecture finale si
elle ne sert pas aussi q4 ou si son prétraitement caché construit l'arrangement
complet.

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
- `E` les ancres résiduelles ;
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
contrôlable `Lambda=(sum_e(m_e+k_e))/n` : `Lambda` polylogarithmique vise le
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

1. Choisir `P` exact ou `Q` avec facteur sept, puis implémenter le shadow
   `SepCell` et le terminal `SepCell || SepTight`.
2. Figer le profil de coût à `s=8`.
3. Remplacer `corner_histograms` par requêtes saturées, bitsets de résidu et
   porte `all_dead` avant handles.

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
2. Ajouter q3 comme requêtes de points désignés et q4 comme sommets peu
   profonds, avec groupes de concurrence. Un mutant qui accepte un centre sans
   paire incidente admissible doit tomber sans énumération quadratique.
3. Comparer la correction de corde et la petite route au flux brut historique.
   Pour la route quotientée, comparer le set et les niveaux post-RLE des
   `BallKey`, le census, les fates, événements, forêt et digests ; conserver un
   ledger distinct de la multiplicité brute supprimée.
4. Garder la route historique sous un `m0` constant et router les ancres lourdes
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
- q3 : points localisés, profondeurs exactes et supports/`BallKey` ;
- q4 : `faces_D`, essais `D`, groupes de droites, sommets par profondeur,
  incidences de groupes, multiplicité maximale, centres émis et census ;
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

1. Veut-il stocker la cellule de préfixe exacte `P`, ou conserver le cube `Q`
   et payer/prover le facteur sept ?
2. Où `h_c` ferme-t-il un nœud de carriers avant toute matérialisation de
   `A x B x C` ?
3. Quel est, après `h_core+h_a+h_b+h_c`, le nombre d'ancres résiduelles et la
   somme exacte de leurs covers par régime ?
4. Le premier moteur q4 conserve-t-il `cover3` pour la parité historique, ou
   ouvre-t-il explicitement une requalification `cover4` ?
5. La sortie demandée est-elle le quotient par `BallKey`, le shell complet ou
   l'expansion des supports ?
6. La correction de corde et les compteurs non emboîtés sont-ils reçus avant
   d'interpréter le nouveau profiler ?

Ces réponses déterminent la structure à coder. Elles ne bloquent pas le shadow
`h_c`, mais elles bloquent tout claim de sous-quadraticité et toute conclusion
tirée des taux de la corde actuelle.
