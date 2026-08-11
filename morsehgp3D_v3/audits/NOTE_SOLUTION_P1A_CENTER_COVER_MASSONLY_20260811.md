# Note de solution — falsificateur de masse P15-HOCUDA-P1a (center-cover, mass-only)

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note spécifie le profiler **mass-only** demandé par l'audit courant
(« Ordre d'implémentation », point 4) et par
[`../PROPOSITION.md`](../PROPOSITION.md) (jalon 3) : la tranche
`P15-HOCUDA-P1a` de la roadmap, en version CPU u16 **q4 seulement** de
falsification. Il
n'émet ni ancre, ni paire, ni support : il accumule `pruned_mass` et
`microtile_mass` et ferme `pruned_mass+microtile_mass=C(n,2)` sans
matérialiser les paires. Le statut logiciel appartient à
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

Réponse de l'audit indépendant : le certificat géométrique ci-dessous est
admissible comme spécification de travail. Cette conclusion est conditionnée
par les trois obligations désormais explicites dans la note : subdivision
rationnelle sans trou des 64 patchs, rejeu du témoin contre **chaque point
réel** du nœud d'extrémités plutôt que contre les seuls coins de sa boîte, et
partition bijective des paires dans le juge borné. Aucune implémentation **v3
u16 q4** n'est encore reçue. Un prior art q4/binary64 existe au commit
`95dd8036a2fcb36c8a7b6aeb7c44197d9c9f7e03`; il n'a jamais été compilé ni
exécuté nativement et ne possède aucun reçu G4. Les contrats réutilisables et
les limites de ce comparateur sont inventoriés dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md).

## 1. Objet et théorème de prune

Pour un bloc croisé `(A,B)`, ou un self-bloc diagonal, de la partition
triangulaire implicite
`T(N)=T(L) sqcup (L times R) sqcup T(R)` sur le LBVH Morton partagé
(`prototype/morton_lbvh.hpp`), la lane q4 au seuil huit prune le
bloc entier lorsque :

1. un domaine extérieur `T0` couvre tous les centres admissibles de toutes
   les paires du bloc ;
2. `T0` est partagé en 64 patchs boîtes ;
3. chaque patch **faisable** possède huit `PointId` distincts certifiés
   strictement intérieurs pour TOUT centre du patch et TOUTE paire du bloc.

Alors toute sphère d'un support propre positif q4 dont l'arête maximale
appartient à `A times B` a son centre dans un patch faisable, donc au moins
huit intérieurs stricts distincts : `p+q>=12`, le bloc est H0-inerte à
`K=10` et sa masse `|A| fois |B|` est prunée en un reçu croisé; la masse d'un
self-bloc de taille `m` vaut `m(m-1)/2`. Un patch ou un
range-query ambigu force le partage du bloc ; une microtuile terminale est
comptée sans résolution.

## 2. Domaine extérieur exact

Pour une paire `(a,b)` d'arête maximale d'un support propre, le centre vérifie
`c=M+t`, `t dot d=0`, et le rayon `r=|c-a|` respecte Jung :
`r^2<=3D^2/8`, d'où `||t||^2<=D^2/8`.

Sur le bloc, `D^2<=maxdist^2(A,B)` (maximum de boîte à boîte, exact par
coins). Le domaine extérieur est la boîte

$$T0=\left\lbrack\left\lfloor\frac{A_{lo}+B_{lo}}{2}\right\rfloor-R_t,\ \left\lceil\frac{A_{hi}+B_{hi}}{2}\right\rceil+R_t\right\rbrack^{3},$$

avec `R_t` le plus petit entier satisfaisant
`8*R_t^2>=maxdist^2(A,B)`. Il est obtenu par racine entière et
comparaison croisée, sans division arrondie. Conservateur par construction :
couvrir trop large ne peut créer aucun faux prune.

La subdivision en 64 patchs n'emploie aucun arrondi implicite. Pour chaque
axe, si l'intervalle entier extérieur est `[l_d,h_d]`, on pose les cinq bornes
rationnelles `b_{d,j}=l_d+j(h_d-l_d)/4`, `j=0,...,4`, puis les quatre
intervalles fermés consécutifs. Les produits cartésiens donnent exactement 64
boîtes fermées dont l'union est `T0`; leur chevauchement sur les frontières est
volontaire. Les coordonnées de coin ont un dénominateur divisant quatre. Les
marges affines sont mises à l'échelle quatre et les distances carrées à
l'échelle seize avant évaluation entière; employer quatre pour une distance
carrée serait une erreur. Un centre
sur une frontière peut appartenir à plusieurs patchs, mais le prune exige le
certificat de chacun des patchs faisables concernés.

## 3. Faisabilité soutenue d'un patch

Un patch `T_i` est déclaré **infaisable** seulement sous un certificat exact :

- la borne inférieure de
  `|c-a|^2-|c-b|^2` est strictement positive sur
  `T_i times A times B`, ou sa borne supérieure est strictement négative — le
  patch ne rencontre aucun médiateur d'une paire du bloc ;
- la distance du patch au domaine entier des milieux `(A+B)/2` dépasse la
  borne de déplacement `maxdist^2(A,B)/8` ;
- `dist^2(T_i,A)>maxdist^2(T_i,B)` ou `dist^2(T_i,B)>maxdist^2(T_i,A)` —
  aucun point de `T_i` n'est équidistant d'un `a` et d'un `b` (le rayon vers
  A dépasse toujours celui vers B, ou l'inverse) ;
- `dist^2(T_i,A)>(3/8)*maxdist^2(A,B)` (comparaison entière
  `8*dist^2>3*maxdist^2`) — le
  rayon dépasserait la borne de Jung de toute paire du bloc.

`dist^2` et `maxdist^2` de boîte à boîte sont séparables exacts. Toute
incertitude laisse le patch faisable (fail-open).

## 4. Prédicat témoin exact par patch (coins × clip)

Pour un témoin `z`, un patch `T_i` et la boîte d'extrémités `A` : `z` est
strictement intérieur à toute sphère de centre `c in T_i` passant par un
`a in A` exactement lorsque `|c-z|^2<|c-a|^2` pour tous `c,a`, soit
`f(c,a)=|a|^2-2c dot a-|z|^2+2c dot z>0`.

Pour `c` fixé, `f` est **convexe séparable** en `a` : le minimum par axe est
atteint à `a_d=clip(c_d,[A_{lo,d},A_{hi,d}])` et vaut
`clip^2-2c_d*clip`. Pour `a` minimisé, `f` est **concave (affine par
morceaux) en `c`** : son minimum sur la boîte `T_i` est atteint sur un coin.
Le certificat exact est donc :

$$\min_{c\in\mathrm{corners}(T_i)}\left\lbrack\sum_{d}\min_{a_d\in[A_{lo,d},A_{hi,d}]}\left(a_d^{2}-2c_da_d\right)-\left\Vert z\right\Vert^{2}+2c\mathbin{\cdot}z\right\rbrack>0.$$

Huit coins, trois clips par coin, arithmétique `i64`; après multiplication par
quatre pour les coins rationnels, la borne u16 reste strictement sous `2^40`.
Le côté `A` suffit (le rayon de toute sphère admissible du bloc est `|c-a|`) ;
le test symétrique côté `B` est admissible en défense en profondeur. Toute
égalité refuse le témoin (fail-open).

Le chemin chaud accepte aussi un nœud témoin `W` entier lorsque la même marge
est strictement positive pour toute sa boîte. Les nœuds acceptés forment une
antichaîne de plages Morton disjointes; leur masse totale fournit les huit
`PointId` distincts sans les énumérer. Chaque plage témoin est disjointe des
plages `A` et `B` par **identité de feuille**. Il ne faut pas rejeter un témoin
seulement parce que sa coordonnée appartient spatialement à l'AABB de `A` ou
`B` : cette règle serait sûre, mais inutilement destructrice.

## 5. Machine et ledger

1. Partition triangulaire : `process(N)` = self-bloc récursif ;
   `process(A,B)` croisé : si `|A|*|B|<=microtile`, `microtile_mass+=|A|*|B|`
   et retour (compté, jamais résolu) ; sinon tenter le prune (T0, 64 patchs,
   faisabilité, témoins), sinon partager le plus gros côté.
2. Recherche de témoins : pour chaque patch `T_i`, parcours LBVH borné sur
   `T_i oplus [-R_w,+R_w]`, avec `R_w` le plus petit entier satisfaisant
   `8*R_w^2>=3*maxdist^2(A,B)`. Tout témoin universel d'un patch
   contenant un centre réellement admissible appartient à cette expansion.
   La dilatation de `T0` entière reste sûre mais plus lâche. Une recherche plus étroite reste
   permise comme politique fail-open, jamais comme preuve d'absence. Le budget
   de candidats par bloc est une politique de travail : son épuisement
   conserve le bloc. L'arrêt anticipé n'intervient que lorsque tous les patchs
   faisables ont leurs huit témoins.
3. Ledger : `pruned_mass+microtile_mass=C(n,2)` exact en `i128` pour q4 ;
   aucun tableau de paires, aucune émission.
4. Dans ce probe diagnostique seulement, refus atomique sur budget global
   (`code 3`), jamais une troncature. Le futur chemin produit n'a pas de budget
   configurable : il termine avec backpressure ou échoue sur une ressource
   physique réelle.

## 6. Juge indépendant (n<=32)

Le juge d'autorité ne partage ni le découpage récursif, ni le prédicat `clip`,
ni le LBVH, ni les primitives `mhgp`. Il énumère **tous** les quadruplets,
reconstruit en rationnels le centre circonscrit par le système de Gram, exige
l'indépendance affine et les quatre numérateurs barycentriques strictement
positifs, puis recalcule le census `p` par le signe rationnel de puissance.
Il vérifie que tout support q4 **non inerte** (`p<8`) a son arête maximale
canonique hors de tout bloc pruné. `mhgp::miniball_of` et `sphere_side` peuvent
servir de différentiel supplémentaire, jamais de juge unique.

Les reçus de blocs sont ensuite rejoués témoin par témoin. Pour chaque nœud
témoin accepté, chaque coin rationnel `c` du patch, chaque `PointId` réellement
contenu dans sa plage et **chaque `PointId` réellement contenu dans la plage
A**, le juge recalcule directement en `i128` l'inégalité
`|c-z|^2<|c-a|^2`, après mise au même dénominateur. Il ne teste pas seulement
les coins de la boîte `A` : la fonction est convexe en `a` et son minimum peut
être intérieur à cette boîte. Ce rejeu discret recertifie exactement la
propriété scientifique nécessaire pour toutes les extrémités possibles du
bloc, sans réutiliser le `clip` du sujet.

Enfin, le juge maintient un sort par paire non ordonnée. Il développe les
blocs prunés et les microtuiles seulement à `n<=32`, exige que chaque paire
reçoive exactement un sort et compare cette bijection à `C(n,2)`. La seule
égalité scalaire de masse ne suffit pas : une duplication et une omission de
même cardinal pourraient se compenser.

## 7. Portes exigées

- fixtures : un bloc réellement prunable (cluster dense contre paire
  lointaine) ; un support non inerte dont l'ancre canonique tombe dans un
  bloc candidat — le bloc DOIT se partager ; colocalisés (aucun prune) ;
  extrêmes u16 ; un patch faisable sans témoin (le bloc se partage) ;
- mutants à code 4 : `patch-omitted` (63 patchs contrôlés),
  `feasible-declared-infeasible` (faisabilité large→stricte),
  `corner-skipped` (7 coins), `witness-duplicated` (même `PointId` compté
  deux fois dans un patch), `witness-in-endpoint-range` (témoin pris dans
  `A`), `jung-radius-understated` (rayon q4 trop petit),
  `microtile-truncated` (masse microtuile tronquée),
  `terminal-compensated` (une paire omise et une autre dupliquée à masse
  totale inchangée), `witness-subtrees-overlap` (deux masses partagent une
  feuille) ;
- planchers : `--min-block-prunes`, `--min-pruned-mass`,
  `--min-microtiles`, `--min-splits`, chacun code 3 ;
- équivariance par renversement du nuage ;
- compteurs aux tailles `12 500/25 000/50 000` : blocs visités,
  patch-évaluations, témoins testés, prunes, masses, part terminale. Deux
  pentes consécutives `>1,35`, une masse majoritairement terminale ou un
  rescan par paire classent la route no-go avant G4 (audit, point 5).

GCP non utilisé pour cette note.
