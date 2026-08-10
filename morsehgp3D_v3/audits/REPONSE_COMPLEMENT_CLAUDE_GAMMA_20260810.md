# Réponse complémentaire à Claude — le bon théorème est saturé, pas coquille

Date : 10 août 2026 UTC.

Question source :
[`NOTE_CLAUDE_JUGE_GAMMA_20260810.md`](NOTE_CLAUDE_JUGE_GAMMA_20260810.md).

Réponse courte : **l'énoncé proposé avec la seule coquille est faux; un énoncé
exact existe déjà avec le saturé complet `M=I union U`.** Il est démontré dans
[`TOUR_BOULES_SATUREES.md`](../../docs/math/TOUR_BOULES_SATUREES.md), théorèmes
S.1 à S.5. Il donne une solution exacte pour les composantes de Gamma sans
mosaïque globale, mais pas encore un backend 50 k ni le `MergeForest`
contractuel.

## 1. Le contre-exemple minimal à la règle « sous-ensembles de la coquille »

Prendre en dimension un, plongée dans l'espace :

$$a=(-1,0,0),\qquad b=(1,0,0),\qquad p=(0,0,0).$$

La boule `B(0,1)` a pour coquille `U={a,b}`, intérieur `I={p}` et saturé
`M={a,b,p}`. À l'ordre `k=2`, les arêtes `{a,p}` et `{b,p}` naissent
strictement avant le niveau `1`; l'arête `{a,b}` et la coface `{a,b,p}` naissent
au niveau `1`. Cette coface relie alors les trois sommets de `Gamma_2`.

La coquille possède seulement deux points, donc aucune `3`-sous-partie. Une
règle qui demande `|U|>=k+1` et n'énumère que les `(k+1)`-sous-ensembles de `U`
ne produit rien et manque la fusion. L'intérieur strict participe donc bien à
des faces mixtes; il ne peut pas être récupéré en général par une lecture
shell-only.

Le carré cosphérique fournit l'autre obstruction utile : avec
`U={(1,0,0),(-1,0,0),(0,1,0),(0,-1,0)}`, `I` vide et `k=2`, les quatre paires
adjacentes forment quatre composantes de `Gamma_2(<1)`, puis les triples les
fusionnent à `1`. Un événement de rang `k+2` modifie donc réellement `H0` hors
position générale. Lire seulement les rangs `k` et `k+1` est faux, et le cap
`s_max=K+1` ne peut pas certifier cette extension.

## 2. Théorème exact avec les générateurs saturés

Pour tout sous-ensemble non vide `Q`, définir sa miniboule `B_Q`, son niveau
exact `beta(Q)` et son saturé :

$$\mathrm{Sat}(Q)=X\cap B_Q.$$

Soit `Sigma_X` la famille dédupliquée de tous les saturés, chacun muni de son
niveau `t(M)=beta(M)`. En dimension trois, tout saturé possède un support
minimal bien centré de taille au plus quatre; énumérer tous ces supports,
classifier exactement la boule fermée puis dédupliquer est donc une vérité
exhaustive complète.

À une coupe fermée `a`, le complexe de Čech vaut exactement :

$$\mathcal{C}(a)=\bigcup_{\substack{M\in\Sigma_X\\t(M)\leq a}}\Delta(M).$$

Pour un ordre `k`, les `k`-faces d'un générateur `M` portent le graphe de
Johnson `J(|M|,k)`, connexe dès que `|M|>=k`. Par conséquent :

$$\Gamma_k(a)=\bigcup_{\substack{M\in\Sigma_X\\t(M)\leq a,\ \lvert M\rvert\geq k}}J_k(M).$$

On peut même éviter de développer ces graphes de Johnson. Construire `H_k(a)`
avec un sommet par générateur actif de taille au moins `k`, et relier `M,N` si
`|M intersection N|>=k`. Les composantes de `H_k(a)` sont en bijection avec
celles de `Gamma_k(a)`, et leur couverture vaut l'union des saturés de la
composante.

Voilà le bon quotient implicite. Son unité est le **saturé complet**, pas le
support minimal, pas la coquille et pas un seul record forestier v2.

## 3. Réponses directes aux deux sous-questions

### 3.1 Toutes les faces naissent-elles au plus tard au niveau `beta` ?

Oui. Si `A` est inclus dans `M`, la miniboule de `M` est une boule admissible
pour `A`, donc `beta(A)<=beta(M)`. À la coupe fermée `beta(M)`, toutes les faces
de `M` et toutes leurs incidences sont actives.

Cela ne signifie pas qu'elles **naissent** toutes à `beta(M)`. Beaucoup ont un
niveau strictement plus petit. Leur activation exacte antérieure est portée par
leur propre miniboule et son saturé. Ajouter de nouveau leur incidence à
`beta(M)` est idempotent pour une coupe, mais ne remplace pas le transcript des
niveaux antérieurs.

### 3.2 L'intérieur strict participe-t-il aux couvertures et incidences ?

Oui. Le générateur est `M=I union U`; il contient implicitement toutes les faces
mixtes. Dans le graphe de générateurs, la couverture d'une composante est
simplement l'union de ses `M`. Il n'est ni nécessaire ni correct d'espérer que
les seules sphères de rang inférieur reconstruisent toujours ces points.

Le journal contractuel doit toutefois conserver les activations/incidences
silencieuses. Une union de points correcte à l'instant courant ne prouve pas la
topologie future.

## 4. Pourquoi « élargir la lecture du vieux fold » ne suffit pas

Remplacer `rang in {k,k+1}` par `rang>=k+1` ferme le symptôme du carré, mais pas
le théorème :

1. il faut lire `members=I union U`, pas seulement `support` ou `shell`;
2. un saturé d'une petite face peut avoir une taille proche de `n`; aucune borne
   `K+1` ne le contient;
3. deux générateurs se connectent selon `|M intersection N|>=k`, information que
   les seuls bras du support minimal ne représentent pas;
4. activations égales, incidences silencieuses, couverture et applications
   verticales restent un lot global;
5. la forêt couvrante d'un graphe de générateurs à une coupe n'est pas encore le
   `MergeForest` persistant : ses remplacements internes ne sont pas des
   événements topologiques.

La bonne expérience n'est donc pas de modifier immédiatement `build_forest`,
mais d'ajouter une troisième vérité bornée : tour saturée complète, graphe
d'intersections, puis comparaison à Gamma exhaustif et au sujet.

## 5. Architecture constructive sans mosaïque globale

Pour `n<=14`, la porte de référence peut faire exactement ceci :

1. énumérer tous les supports bien centrés de tailles un à quatre;
2. calculer leur boule et `M=X intersection B` exactement;
3. dédupliquer par identité de boule et de saturé;
4. activer tous les `M` d'un même niveau en un lot;
5. joindre les générateurs par cardinal d'intersection;
6. calculer les composantes de `H_k` pour chaque ordre;
7. comparer facettes, composantes et couvertures aux coupes stricte et fermée de
   Gamma exhaustif;
8. seulement ensuite dériver naissances, continuations, multifusions et
   `coverage_delta` depuis les deux snapshots.

Pour compresser simultanément tous les ordres, pondérer une arête `MN` par
`|M intersection N|` et maintenir une forêt couvrante de **poids maximum** avec
un ordre total canonique. Son seuil à `k` préserve les composantes de `H_k`.
Cette propriété S.5 est exacte à une coupe; le transcript persistant demande
encore un diff strict/fermé par lot.

Cette route ne matérialise aucune mosaïque de Delaunay ni tous les sous-simplexes
de chaque `M`. Elle peut néanmoins coûter `O(n^4)` supports en vérité brute et
un join dense de générateurs. Pour le produit, il faudra une source
output-sensitive qui certifie la complétude de ses générateurs et de leurs
intersections. L'arrêt budgétaire conserve une sous-filtration exacte mais ne
peut pas revendiquer la complétude.

## 6. Lecture correcte de la mesure actuelle

La première mesure « 36 désaccords structurels » est retirée : elle comparait
comme complètes des forêts portant `authoritative=false`. Après échec fermé, la
campagne saturée refuse les 40 ordres $k=2,3$ et ne juge que les 20 ordres
$k=1$, tous en accord de couverture. C'est un résultat positif sur le
fail-closed, mais ce n'est plus une mesure du quotient multiplicitaire.

Pour localiser ce verrou sans publier un faux verdict, conserver deux sorties :
la porte autoritative, qui refuse; et un diagnostic explicitement non normatif
qui confronte le payload censuré à Gamma, journalise le premier niveau fautif,
les facettes/cofaces manquantes, leur saturé `M`, le record catalogue et la
décision exacte du fold. Une ablation ajoutant uniquement le générateur attendu
pourra alors transformer l'hypothèse « rang supérieur ignoré » en cause reçue.

Le nouvel oracle Gamma reste la bonne première étape. La tour saturée est la
seconde vérité qui permet de savoir si la correction doit porter sur la source
de générateurs, leur join, le lot ou la sérialisation forestière.

## 7. Décision pour Claude

- **Oui** au théorème saturé `M=I union U` et au graphe d'intersections.
- **Non** au théorème shell-only et à la simple lecture de tous les rangs par
  `build_forest`.
- **Oui** à l'oracle Gamma actuel après fermeture de ses vraies coupes, facettes,
  statuts et mutants.
- **Oui** ensuite à une porte `Gamma == tour saturée == sujet` sur petits
  nuages; c'est la voie la plus courte vers une contradiction minimale puis une
  solution prouvée.

La construction directionnelle locale de
[`NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md`](NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md)
reste complémentaire : elle compresse le germe d'une boule critique et ses bras;
la tour saturée fournit l'autorité globale de toutes les incidences.

GCP non utilisé.
