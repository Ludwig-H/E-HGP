# Gate D — fold horizontal exact sur snapshot strict

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=bounded_exact_horizontal_fold_then_external_versions`,
`public_status=not_claimed`.

> [!IMPORTANT]
> Après la descente locale vers une facette cœur, la dernière décision globale
> horizontale est un quotient de composantes **par lot exact**. Son état strict
> contient deux sortes d'objets qu'il est interdit de confondre : les racines
> réduites publiques et les carriers encore latents. Le lot se ferme sur leur
> union disjointe, puis seulement il compte les racines. Cette information peut
> vivre dans un journal externe; elle ne peut pas être supprimée.

Cette note spécialise la
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
latent strict ou neuf $L$ et les deux hyperarêtes $\lbrace R_1,L\rbrace$ et
$\lbrace L,R_2\rbrace$, supprimer $L$ avant la fermeture laisserait deux
continuations. La fermeture correcte donne une seule composante contenant deux
racines : c'est une multifusion.

## 3. Sémantique exacte d'un lot

Pour chaque niveau rationnel maximal $a$ :

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

Soit $q_R$ ce nombre de racines strictes distinctes.

- $q_R=0$ : les arêtes du lot rendent la composante non triviale; elles créent
  une naissance réduite et une nouvelle racine publique.
- $q_R=1$ : le lot prolonge l'unique racine sans créer de nœud public.
- $q_R\geq2$ : le lot crée une multifusion unique dont les racines strictes sont
  les enfants.

Une simple `FacetActivate` non incidente à une arête égale ne passe pas par cette
classification : elle reste latente. Inversement, une composante composée
uniquement de latents mais reliée par une ou plusieurs hyperarêtes relève bien de
$q_R=0$.

Tous les carriers de la composante temporaire reçoivent la même version fermée,
y compris pour $q_R=1$. Omettre cette version silencieuse perdrait l'information
nécessaire à un futur `find`.

La couverture ne se déduit pas de $q_R$. Le fold doit aussi conserver l'union
exacte des facettes et des `PointId`, puis calculer son delta contre l'union des
parents stricts. Une attache issue du théorème régulier annonce
`added_points=empty`; le fold doit pouvoir le vérifier, pas seulement lui faire
confiance.

## 4. Obligation renforcée des attaches résolues

Le certificat de descente prouve que $R_F$ appartient à la composante stricte du
bras initial. Le certificat combiné de l'attache, avec ses ponts stricts et son
argument de couverture, prouve en plus que cette composante est non triviale et
couvre déjà $F\cup\lbrace z_F\rbrace$.
Par conséquent :

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
aux coupes ouverte et fermée. Un sujet DSU résident consomme les mêmes records
par lots gelés. Après chaque lot, le différentiel compare :

- partition complète des handles actifs;
- disposition latente ou enracinée de chaque composante;
- signatures récursives canoniques des naissances et multifusions;
- racine héritée des continuations;
- unions et deltas exacts de facettes et de points;
- état inchangé après chaque faute injectée.

La comparaison doit porter sur les clefs complètes et la structure normalisée,
pas sur les représentants accidentels du DSU ni sur une somme de hachages.

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

1. `R1--L--R2` dans un lot unique : le latent réalise une multifusion $q_R=2$.
2. Plusieurs hyperarêtes reliées uniquement par des latents : une seule naissance
   $q_R=0$, jamais une racine par facette.
3. Trois racines reliées au même niveau par plusieurs runs : une multifusion
   ternaire, jamais deux fusions binaires.
4. Une continuation $q_R=1$, puis un lot futur qui réutilise sa nouvelle facette :
   oublier le successor doit rendre le second lot rouge.
5. `E5` : attache de `AC` vers sa racine stricte, `added_core_facet=AC`,
   `added_points=empty`, puis `ABC` reste une continuation.
6. Une facette activée exactement à $a$ : sommet $N_a$, invisible à `find_<a`,
   visible à la coupe fermée et jamais confondue avec $L^{-}$.
7. Deux fractions égales encodées différemment, puis deux fractions distinctes
   arrondies au même `double`.
8. Duplicat identique avec provenances agrégées; duplicat contradictoire rejeté.
9. Successor omis, niveau d'activation omis, cycle de versions, lien de même
   niveau et lien postérieur suivi par une requête stricte.
10. Échec injecté après chaque étape du staging, avec digests de partition,
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
