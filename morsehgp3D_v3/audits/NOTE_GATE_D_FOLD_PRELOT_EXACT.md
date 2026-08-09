# Gate D — fold horizontal exact sur snapshot strict

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=bounded_exact_horizontal_fold_then_external_versions`,
`public_status=not_claimed`.

> [!CAUTION]
> **État audité du noyau F0-A : rouge.** Le script courant imprime `PASS`, mais
> sa vérité Warshall et son sujet DSU rejettent ensemble une composante
> `N_a--N_a` portée par une `DirectHyperedge`. Ce rejet contredit les §3 et §8
> ci-dessous, qui classent ce cas comme une naissance `q_R=0`. Le détail et la
> fixture géométrique d'arité quatre sont dans la
> [`note des verrous mathématiques prioritaires`](NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md)
> §2.

> [!NOTE]
> Sous la porte régulière forte seulement, chaque `DirectHyperedge` possède
> exactement $\lvert U\rvert\geq2$ facettes strictes. Ce résultat conditionnel
> se valide **par record brut avant projection**; il ne justifie pas une garde
> par composante dans le fold source-agnostique. Une autre hyperarête portant un
> latent peut sinon masquer un record entièrement neuf malformé.

> [!IMPORTANT]
> Après la descente locale vers une facette cœur, la dernière décision globale
> horizontale est un quotient de composantes **par lot exact**. Son état strict
> contient deux sortes d'objets qu'il est interdit de confondre : les racines
> réduites publiques et les carriers encore latents. Le lot se ferme sur leur
> union disjointe, puis seulement il compte les racines. Cette information peut
> vivre dans un journal externe; elle ne peut pas être supprimée.

Cette note traite le fold supérieur $2\leq k<n$. À l'ordre un, les singletons
sont immédiatement des racines publiques et exigent une branche distincte de
type EMST; à $k=n$, aucune coface n'existe. Dans ce domaine, la note spécialise la
[`frontière des globalités résiduelles`](NOTE_GATE_D_GLOBALITES_RESIDUELLES.md)
et reçoit les clefs cœur produites par la
[`descente locale`](NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md).
Elle décrit d'abord un oracle borné indépendant du stockage, puis deux
raffinements d'I/O. Elle ne suppose ni catalogue global produit, ni
$\Gamma_k$ résident, ni mosaïque de Delaunay d'ordre supérieur.

## 1. Objet filtré et trois records

À un ordre $k$ fixé, un **handle** est l'identité canonique complète d'une
$k$-facette. Son identifiant dense éventuel n'est qu'une entrée de dictionnaire
liée à cette clef par un manifeste scellé.

Le fold consomme trois familles de records.

1. `FacetActivate(F,beta_F,provenance)` rend le handle $F$ visible à la coupe
   fermée $\beta_F$. Pour $k\geq2$, cette activation isolée reste latente et ne
   crée aucune racine publique.
2. `DirectHyperedge(Q,beta_Q,facets(Q),provenance)` relie simultanément toutes
   les $k$-facettes de la coface directe $Q$.
3. `ResolvedAttachment(F,a_F,R_F,descent_receipt)` relie la facette cœur égale
   $F$ au carrier strict représenté par la facette cœur $R_F$. Le producteur
   fournit une clef et un certificat, jamais un identifiant de racine comme
   autorité.

Les niveaux sont des rationnels canoniques ou sont comparés exactement par
produits croisés multiprécision. Une valeur `double`, un hachage ou l'égalité de
deux encodages rationnels non normalisés ne définit jamais un lot.

Le premier prototype doit exiger un manifeste terminal de tous les runs. Des
watermarks dynamiques pourront raffiner ce contrat; ils ne sont pas nécessaires
pour juger la sémantique du fold.

## 2. Snapshot strict : racines et latents sont disjoints

Juste avant un niveau $a$, chaque composante active possède une version
canonique. Sa projection typée est :

- $R^{-}(r)$ si la composante porte la racine publique active `root_id=r`;
- $L^{-}(v)$ si elle reste latente, où `v` est l'identité de sa version de
  carrier.

Les espaces de noms $R^{-}$ et $L^{-}$ sont disjoints, même si leurs
représentations entières coïncident. Deux handles latents déjà équivalents avant
$a$ se projettent sur le même $L^{-}(v)$. Deux handles enracinés dans la même
composante se projettent sur le même $R^{-}(r)$.

Une facette activée exactement à $a$ n'appartient pas au snapshot strict. Si un
record égal la touche, elle devient un sommet neuf typé $N_a(F)$, distinct des
latents stricts. Une facette de niveau strictement supérieur à $a$ dans un record
du lot est une contradiction. Le tag d'époque empêche un `find_<a` d'accepter
une activation anticipée.

La distinction n'est pas décorative. Avec deux racines strictes $R_1,R_2$, un
sommet $Z\in L^{-}\cup N_a$ et les deux hyperarêtes
$\lbrace R_1,Z\rbrace$ et $\lbrace Z,R_2\rbrace$, supprimer $Z$ avant la
fermeture laisserait deux
continuations. La fermeture correcte donne une seule composante contenant deux
racines : c'est une multifusion.

## 3. Sémantique exacte d'un lot

Pour chaque classe maximale de records de même niveau rationnel $a$, traitée en
ordre strictement croissant :

1. figer le locator et les versions de la coupe ouverte $<a$;
2. résoudre tous les endpoints stricts dans ce snapshot et créer les sommets
   neufs $N_a$ activés à $a$;
3. quotienter les endpoints en sommets typés
   $R^{-}\sqcup L^{-}\sqcup N_a$;
4. fermer **ensemble** toutes les hyperarêtes directes et attaches du lot;
5. pour chaque composante temporaire qui contient au moins une arête logique,
   compter les `root_id` distincts qu'elle contient;
6. préparer toutes les versions, activations, unions, couvertures, parents et
   provenances;
7. valider le staging entier, puis effectuer un unique commit logique.

Une **arête logique** est un record source présent, même si plusieurs de ses
endpoints se projettent sur un seul sommet typé. Une hyperarête entièrement
redondante reste donc une continuation attestée; elle ne disparaît pas sous
prétexte que sa projection est unaire.

Soit $q_R$ ce nombre de racines strictes distinctes.

- $q_R=0$ : une naissance réduite est autorisée seulement si la composante
  contient une `DirectHyperedge`. Une composante constituée uniquement
  d'attaches est une contradiction : toute attache certifiée doit viser une
  cible $R^{-}$.
- $q_R=1$ : le lot prolonge l'unique racine sans créer de nœud public.
- $q_R\geq2$ : le lot crée une multifusion unique dont les racines strictes sont
  les enfants. Cette composante doit contenir une `DirectHyperedge`; une
  composante multiracine constituée uniquement d'attaches contredit l'unicité de
  la cible attachée à chaque facette neuve.

Une simple `FacetActivate` non incidente à une arête égale ne passe pas par cette
classification : elle reste latente. Inversement, une composante composée
uniquement de latents mais reliée par une ou plusieurs hyperarêtes relève bien de
$q_R=0$.

Tous les carriers de la composante temporaire reçoivent la même version fermée,
y compris pour $q_R=1$. Omettre cette version silencieuse perdrait l'information
nécessaire à un futur `find`.

La couverture ne se déduit pas de $q_R$. Dans le contrat quotienté, le fold doit
aussi conserver l'union exacte des **facettes cœur $D_k$ actives** et des
`PointId`, puis calculer son delta contre l'union des parents stricts. Il ne
prétend pas rétablir les facettes Gamma omises. Une attache issue du théorème
régulier annonce
`added_points=empty`; le fold doit pouvoir le vérifier, pas seulement lui faire
confiance.

## 4. Obligation renforcée des attaches résolues

Le certificat de descente prouve que $R_F$ appartient à la composante stricte du
bras initial. Le certificat combiné de l'attache, avec ses ponts stricts et son
argument de couverture, prouve en plus que cette composante est non triviale et
couvre déjà $F\cup\lbrace z_F\rbrace$.
Par conséquent :

- la facette $F$ doit se projeter sur le sommet neuf $N_{a_F}(F)$ engagé par son
  activation au même niveau;
- `find_<a_F(R_F)>` doit réussir;
- sa disposition doit être enracinée, jamais latente;
- le stamp du résultat doit être celui du snapshot strict du lot;
- suivre un lien de niveau $a_F$ ou postérieur est interdit.

Un latent est légitime pour une facette directe générique. Il est une
contradiction pour la cible certifiée d'une `ResolvedAttachment`. Cette
postcondition sépare un vrai raccord de carrier d'un simple handle connu.

## 5. Journal immuable de versions

Le DSU résident est un premier sujet correct, pas une obligation d'architecture.
Une représentation externalisable associe à chaque composante modifiée une
version immuable $V$.

- Une ancienne version consommée reçoit un unique lien
  `successor=(V,niveau_du_lot)`.
- Chaque handle conserve son `activation_level` exact. Un handle activé dans le
  lot reçoit une liaison vers $V$ au commit.
- Les niveaux des liens le long d'une chaîne sont strictement croissants.
- $V$ porte zéro ou une racine active selon la décision $q_R$.
- Un même lot ne crée aucune chaîne interne de successors : il crée directement
  une version par composante temporaire complète.

Une activation isolée crée au commit sa propre version latente sans successor
entrant. Elle n'est ni oubliée, ni classée par $q_R$ avant qu'une arête logique ne
la touche.

Les liens historiques sont immuables. Avec `V0->V1@a` puis `V1->V2@b`, une
compression destructive en `V0->V2` détruirait les requêtes dont le cutoff se
situe entre $a$ et $b$. Pointer-jumping ajoute des raccourcis versionnés avec leur
domaine de validité, ou produit une table dérivée; il ne réécrit jamais
l'autorité historique.

Alors `find_<a(h)` refuse d'abord tout handle dont `activation_level>=a`, puis
suit exactement les liens de niveau strictement inférieur à $a$.
`find_<=a(h)` accepte l'activation au niveau $a$ et suit aussi les liens de ce
niveau. Les deux requêtes ne sont pas interchangeables.

L'équivalence au DSU se prouve par induction sur les lots. L'hypothèse
d'induction identifie les versions terminales avant $a$ aux composantes de la
coupe ouverte. La fermeture connexe du lot produit exactement les composantes de
la coupe fermée. Les nouvelles versions encodent cette partition et préservent
le tag racine/latent donné par $q_R$. Cette induction exige la terminalité du
lot, l'unicité fonctionnelle des successors, l'absence de mutation avant
validation et la conservation des continuations $q_R=1$; elle n'est pas une
conséquence du seul pointer-jumping.

## 6. Trois étages de falsification

### F0 — sémantique en mémoire

Un oracle borné reconstruit depuis zéro, à chaque niveau, l'hypergraphe complet
aux coupes ouverte et fermée. Pour rester indépendant du sujet, sa vérité emploie
une matrice booléenne d'incidence puis une fermeture de Warshall, jamais le DSU.
Un sujet DSU résident consomme les mêmes records par lots gelés. Le premier
palier **réellement exhaustif et conservé** porte sur au plus cinq sommets
projetés et deux records logiques distincts dans un lot. L'enveloppe de trois
niveaux, six handles stricts, quatre activations courantes et cinq records par
lot reste une cible de génération canonique et de tests ciblés, pas un domaine
qu'un petit script peut honnêtement appeler exhaustif : à un ordre fixé, dix
handles donnent déjà plus de $10^{10}$ multisets bruts d'au plus cinq records
au pire des arités, avant même les états stricts et les historiques. Des
fixtures séparées couvrent les arités
produit jusqu'à onze. L'oracle applique les règles suivantes :

- `activation_level(h)<a` : la classe stricte de `h` devient $R^{-}$ ou
  $L^{-}$ selon sa racine partielle;
- `activation_level(h)=a` : `h` devient $N_a(h)$;
- handle absent ou activé après $a$ : rejet;
- attache `(F,R)` : $F$ doit être $N_a(F)$ et $R$ doit être $R^{-}$;
- fermeture transitive de chaque hyperarête directe et paire d'attache sur la
  matrice typée, puis seulement classification par $q_R$.

La fonction racine de la vérité est constante sur chaque classe stricte et
injective entre les classes enracinées. Les signatures récursives de naissance
et multifusion, et non ses identifiants denses, définissent son égalité.

Après chaque lot, le différentiel compare :

- partition complète des handles actifs;
- disposition latente ou enracinée de chaque composante;
- signatures récursives canoniques des naissances et multifusions;
- racine héritée des continuations;
- ledger canonique de chaque arête logique et de ses provenances, y compris si
  sa projection est unaire;
- unions et deltas exacts de facettes et de points;
- état autoritatif entier inchangé après chaque faute injectée, compteurs
  d'allocation, journal, chaîne de reçus et prochain identifiant compris.

La comparaison doit porter sur les clefs complètes et la structure normalisée,
pas sur les représentants accidentels du DSU ni sur une somme de hachages.
F0 certifie seulement le fold relativement au multiensemble scellé reçu. Un
oracle exhaustif $\Gamma$ séparé reste nécessaire pour juger la complétude de la
source `directes + attaches`.

#### F0-A exécutable — noyau de quotient typé

[`check_gate_d_fold_f0.py`](check_gate_d_fold_f0.py) conserve le premier noyau
indépendant. Sa vérité exécute exactement le pseudo-code suivant; le sujet
emploie une union--recherche distincte.

```text
resolve every handle with activation<a to R-/L-, activation=a to N_a, reject future
prevalidate every attachment as N_a -> R^-, with at most one target per fresh facet
prevalidate every raw direct arity in [2,11]
set M[i,i]=true; connect one pivot to every endpoint of every logical record
Warshall(M)
for every closed component C containing a logical record:
    roots = distinct R^- values in C
    q = size(roots)
    decide birth iff q=0 and C has a direct record; continuation iff q=1; multifusion iff q>=2 and C has a direct record
compare partition, disposition, recursive root and complete logical-record ledger
commit the prepared value only after every check succeeds
```

La campagne courante décide 2 168 lots abstraits, dont 1 703 acceptés et 465
rejetés, sur les sous-ensembles du pool structurel fixe
`{R0,R1,L0,N0,N1}` et zéro, un ou deux records générés. Ce dénombrement est
exhaustif sur ce pool précis, pas sur toutes les répartitions de namespaces ni
sur les nouvelles dimensions provenance, handles source et arité. Onze fixtures
ciblées, huit entrées forgées, onze permutations, une arité onze, dix mutants et
cinq fautes de rollback sont annoncés par l'exécution normale.

Ces compteurs ne rendent pas la porte verte. Le garde partagé « toute composante
avec record doit contenir `R` ou `L` » rejette la naissance directe entre deux
facettes activées au niveau courant. La fixture `carrierless` grave ce rejet et
le mutant `accept_carrierless_group` grave l'acceptation comme faute : l'oracle
et le sujet sont donc corrélés contre le pseudo-code ci-dessus. De plus, plusieurs
obligations ciblées reposent sur `assert`; `python3 -O` les désactive tout en
laissant le script imprimer `Gate_D_F0_kernel=PASS`.

Deux contre-exemples précisent le contrat de test. Premièrement, supprimer une
`DirectHyperedge` déjà unaire après projection ne change ni partition, ni racine,
ni couverture : sans comparaison du ledger logique, cette mutation passe.
Deuxièmement, une faute qui incrémente seulement `next_commit_id` laisse les trois
digests partition--forêt--couverture inchangés mais modifie le prochain résultat;
le rollback doit donc comparer tout l'état autoritatif.

Les accords qui subsistent qualifient seulement des sous-propriétés de **F0-A,
noyau combinatoire d'un lot fourni**. Le script ne valide pas encore sa propre
sémantique de naissance et ne rejoue ni la complétude géométrique de la source,
ni les unions de couverture, ni un historique multilevel complet de successors.
Ces corrections, les runs F1 et le journal externe F2 restent des portes
séparées; le script ne doit jamais être déplacé dans le chemin produit.

### F1 — runs scellés

Le même sujet reçoit des runs triés séparément, puis un `k`-way merge par niveau
exact. La campagne force `run_size=1`, coupe chaque lot à toutes les frontières
de runs et permute les records. Les sorties doivent être identiques à F0. Un run
manquant, un manifeste non terminal ou un budget épuisé annule toute la sortie
scientifique.

### F2 — versions externes

Le DSU est remplacé par le journal de versions. Des tris et self-joins, avec
pointer-jumping ou recherche d'ancêtre pondéré, résolvent les cutoffs. F2 doit
être identique à F0 et F1 après chaque coupe ouverte et fermée, y compris pour
les versions silencieuses $q_R=1$.

Pointer-jumping en $O(\log d)$ rondes pour une profondeur $d$ est une
construction sûre, pas une borne inférieure. Une passe logique reste possible
avec un état externe adressable. En revanche, un stockage purement séquentiel à
mémoire bornée, sans relecture, ne peut pas répondre à une partition historique
arbitraire : il doit conserver l'information ou effectuer des passes.

## 7. Reçu minimal rejouable

Un reçu `ExactPreBatchHorizontalFold` engage au moins :

- profil, ordre, niveau rationnel canonique et politique de coupe;
- digests du nuage, du dictionnaire de handles, des sources, de l'autorité de
  régularité et du manifeste terminal;
- checkpoint strict pré-lot et digest du reçu précédent;
- nombres et digests canoniques des records, endpoints et provenances;
- pour chaque résolution stricte : handle, niveau d'activation, version
  terminale, racine optionnelle, liens suivis et cutoff;
- composantes temporaires, latents, racines distinctes, valeur $q_R$ et décision;
- versions, successors, activations, nœuds, enfants et deltas préparés;
- digests pré/post de la partition, de la forêt et de la couverture;
- identifiant du commit atomique.

Le journal référencé doit rester disponible à un vérificateur frais. Un digest
engage une source; il ne remplace jamais l'égalité exacte des clefs. Toute erreur
avant commit produit zéro mutation et aucun reçu scientifique valide.

## 8. Fixtures et mutations obligatoires

1. Collision numérique `R(7)` contre `L(7)`, puis `R1--L--R2` dans un lot
   unique : les namespaces restent disjoints et le latent réalise une
   multifusion $q_R=2$.
2. Plusieurs hyperarêtes reliées uniquement par $L^{-}$ et $N_a$ : une seule
   naissance $q_R=0$, jamais une racine par facette.
3. Deux handles stricts distincts déjà projetés sur la même racine comptent
   $q_R=1$, pas deux. Trois racines reliées au même niveau par plusieurs runs
   donnent une multifusion
   ternaire, jamais deux fusions binaires.
4. Une continuation $q_R=1$, puis un lot futur qui réutilise sa nouvelle facette :
   oublier le successor doit rendre le second lot rouge.
5. `E5` : attache de `AC` vers sa racine stricte, `added_core_facet=AC`,
   `added_points=empty`, puis `ABC` reste une continuation.
6. Une facette activée exactement à $a$ : sommet $N_a$, invisible à `find_<a`,
   visible à la coupe fermée et jamais confondue avec $L^{-}$.
7. Deux fractions égales encodées différemment, puis deux fractions distinctes
   arrondies au même `double`.
8. Attache forgée vers $L^{-}$, rejetée atomiquement au lieu de créer une
   naissance; hyperarête directe devenue unaire après projection, toujours
   conservée comme record logique.
9. Deux attaches de la même facette neuve vers deux racines strictes distinctes :
   rejet avant fermeture, sans fabriquer une multifusion purement résiduelle.
10. Duplicat identique avec provenances agrégées; duplicat contradictoire rejeté.
11. Successor omis, niveau d'activation omis, cycle de versions, lien de même
   niveau et lien postérieur suivi par une requête stricte.
12. Chaîne `V0->V1@a`, `V1->V2@b`, puis requête entre $a$ et $b$ : une
    compression destructive vers `V2` doit être réfutée.
13. `DirectHyperedge` de onze endpoints à $k=10$, dont plusieurs se projettent
    sur les mêmes $R^{-}$, $L^{-}$ et $N_a$ : cardinal frontière et racines
    **distinctes** sont contrôlés.
14. Échec injecté après chaque étape du staging, avec digests de partition,
    forêt et couverture inchangés.

Les mutations de projection des latents avant fermeture, activation anticipée,
`find_<=a`, union record par record, groupement flottant, découpage par chunk,
comptage des versions plutôt que des racines distinctes et choix d'un
représentant selon l'ordre d'entrée doivent toutes être détectées.

## 9. Coûts et décision

Soient $H$ le nombre de handles, $P$ le nombre total d'occurrences d'endpoints,
$V$ le nombre de versions et $P_a$ la masse du plus grand lot.

- DSU résident : $O(H)$ mémoire et $O(P\alpha(H))$ après tri;
- runs externes : coût d'un tri externe sur $H+P$;
- journal : $O(H+V)$ informations sur disque;
- lot courant : jusqu'à $\Theta(P_a)$ avant segmentation externe.

Ces bornes décrivent seulement le noyau de connectivité. Les identités et DAG de
couverture, provenances, reçus, clefs de dictionnaire, entrées et sorties
scientifiques s'ajoutent séparément.

Le seul $n=50\,000$ ne borne ni $H$, ni $P$, ni $P_a$. Aucun SLO ne découle de
la localité du parent ou de la descente du carrier.

Décision :

- quotient pré-lot sur $R^{-}\sqcup L^{-}\sqcup N_a$ et règle
  $q_R=0,1,\geq2$ : **théorème de composantes élémentaire, sous source
  complète**;
- journal de versions équivalent au DSU : **prouvable par induction sous les
  invariants ci-dessus**;
- source `directes + attaches` équivalente à $H_0$ : **conditionnelle à la porte
  régulière globale et aux autorités des notes amont**;
- fold produit, crash recovery, couverture durable, verticales et contrat 50 k :
  **ouverts**.

Le fold est global dans l'information qu'il conserve, pas nécessairement dans
sa résidence. GCP non utilisé.
