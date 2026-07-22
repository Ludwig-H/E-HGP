# Locator positif sparse et descente top-k locale — Phase 10.5a

## Portée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. La porte d'entrée est satisfaite par les incréments 10.1 à 10.4. Le `public_status` reste `not_claimed`.

Ce jalon sépare strictement deux couches :

- **A — socle combinatoire 10.5a** : un dictionnaire positif sparse, relatif à des assertions externes de l'appelant, retrouve une clé connue et résout son handle dans l'état engagé avant l'appel;
- **B — descente géométrique 10.5b** : une recherche top-k locale devra produire de nouvelles facettes strictement descendantes lorsqu'une facette n'est pas encore connue. Son théorème est seulement conditionnel et son implémentation complète reste ouverte.

Le socle A ne construit aucune facette absente, ne décide aucune naissance, continuation ou multifusion d'ordre supérieur et ne transforme jamais une absence en isolation. Il ne matérialise ni coupe Gamma, ni cellule, ni coface globale, ni incidence globale, ni mosaïque de Delaunay d'ordre supérieur.

---

# Partie A — socle combinatoire livré par 10.5a

## A.1 Domaine positif relatif

Pour une facette $F$ de cardinal $1\leq k\leq K\leq10$, sa clé canonique complète est le tuple

$$\kappa(F)=(k,p_0,\ldots,p_{k-1}),\qquad p_0<\cdots<p_{k-1}.$$

Une liaison proposée est un triplet logique

$$b=(\kappa(F),h,w),$$

où $h$ est un `component_handle` stable et $w$ contient seulement un `external_authority_id` et un `replay_token` non nuls. Le locator vérifie la forme et la concordance de ces deux entiers; il ne possède ni l'autorité externe, ni un identifiant externe de snapshot, ni la géométrie permettant de prouver que $F$ appartient à la composante de $h$. L'appelant doit conserver puis rejouer cette autorité avant d'interpréter sémantiquement un hit.

Le locator est **positif** et **relatif** : son domaine est exactement l'ensemble des clés munies d'au moins une liaison acceptée sous cette hypothèse d'appelant. Il n'affirme ni la vérité de ces liaisons, ni que ce domaine contient toutes les facettes actives, toutes les facettes d'une composante ou toutes les gateways nécessaires à la hiérarchie.

## A.2 Empreinte non autoritaire et comparaison complète

Chaque clé porte une empreinte déterministe masquée $d=h(\kappa(F))$. Elle sert seulement à choisir le premier slot d'une table à adressage ouvert. Une réponse positive interne exige ensuite :

1. l'égalité du cardinal;
2. l'égalité, dans l'ordre, de chacun des au plus dix `PointId`;
3. la présence de jetons externes non nuls liés à l'identifiant d'autorité configuré;
4. la résolution du handle dans le DSU engagé avant l'appel.

Ainsi, deux clés de même empreinte restent deux clés distinctes. Une collision d'empreinte ne peut ni créer un hit, ni masquer un conflit de liaisons, ni servir de preuve d'identité. Le stockage conserve toujours la clé complète après l'empreinte.

L'implémentation de référence préalloue $2M+1$ slots pour une capacité configurée de $M$ clés et utilise le sondage linéaire. Le masque nul des tests force toutes les empreintes à entrer en collision et vérifie que seule la comparaison complète décide. Aucune borne moyenne constante ni propriété cryptographique de l'empreinte n'est revendiquée.

## A.3 Handles et unions sans réécriture des clés

Un `component_handle` est un élément stable du DSU, pas l'identifiant mutable de sa racine courante. Il peut devenir un nœud interne après des unions. Pour une époque interne $t$, notons $\mathrm{find}_{t}(h)$ sa racine canonique. Une entrée conserve $h$; après une union ultérieure, la requête résout simplement le même handle dans le nouveau DSU.

Pour deux handles $h_1$ et $h_2$, la compatibilité dans l'époque $t$ signifie

$$h_1\sim_{t}h_2\quad\Longleftrightarrow\quad\mathrm{find}_{t}(h_1)=\mathrm{find}_{t}(h_2).$$

Une union modifie seulement la forêt DSU. Elle ne réécrit ni les clés, ni les témoins, ni chaque liaison déjà publiée. La stabilité porte sur le handle; la racine canonique observée peut légitimement changer d'une époque à la suivante.

## A.4 Époque pré-appel, staging et commit logique

Un appel séquentiel suit le protocole suivant.

1. **Préflight.** Les formes, indices denses, jetons externes et plafonds du lot sont vérifiés avant toute mutation logique.
2. **Localisation.** Toutes les requêtes consultent exclusivement la table et le DSU engagés avant l'appel.
3. **DSU candidat.** Les parents sont copiés, puis toutes les unions explicites sont appliquées à cette copie.
4. **Compatibilité et staging.** Les doublons sont comparés dans le DSU candidat post-unions; les seules clés nouvelles sont conservées dans des arènes privées. Une liaison courante reste invisible aux requêtes du même appel.
5. **Commit.** Si et seulement si tout le lot est accepté et toutes les capacités durables suffisent, parents, unions, clés nouvelles et agrégat de lot sont engagés. Sinon, le contenu logique antérieur reste inchangé.

Une union explicite du lot peut donc rendre compatibles deux liaisons qui ne l'étaient pas dans l'état pré-appel. Sa vérité reste une assertion externe indépendante à rejouer par l'appelant. Après toutes ces unions, une même clé visant encore deux classes distinctes provoque le rejet transactionnel complet.

Cette propriété fournit la visibilité pré-lot nécessaire seulement si l'appelant place effectivement tout le niveau HGP simultané dans un même appel. Le noyau n'est pas une MVCC et n'offre aucune synchronisation entre threads; « atomique » signifie ici tout-ou-rien logique pour un appel séquentiel.

## A.5 Résultats d'une requête

Une requête valide sur $\kappa(F)$ retourne exactement l'une des deux dispositions suivantes :

- `positive` : une clé complète égale a été trouvée, les jetons externes stockés sont bien formés et son handle se résout dans l'état pré-appel;
- `unresolved` : aucune clé complète égale n'est présente dans le domaine positif visible;

`unresolved` signifie seulement « non prouvé par ce dictionnaire sparse ». Il ne signifie ni `isolated`, ni `new_root`, ni `inactive`, ni `absent_from_full_pi0`. Il ne porte donc aucun `component_handle` inventé.

Les clés ou jetons mal formés, budgets insuffisants et conflits de liaisons sont des décisions de **lot**, pas une troisième disposition de lookup.

## A.6 Conflits permanents

Après application des unions explicites d'un lot, deux liaisons acceptées de même clé complète doivent viser des handles compatibles. Si

$$\kappa(F_1)=\kappa(F_2)\quad\text{et}\quad\mathrm{find}_{t^+}(h_1)\neq\mathrm{find}_{t^+}(h_2),$$

alors les assertions de l'appelant placent une même facette dans deux classes distinctes. C'est une contradiction du contrat relatif, jamais un tie-break par identifiant.

La transaction est rejetée avant publication; le test minimal permanent vérifie également qu'une union sans rapport n'est pas engagée. Un doublon compatible est compté mais n'est pas stocké une seconde fois : seul le premier témoin accepté reste attaché à la clé. Le noyau ne fournit donc ni sélection indépendante de l'ordre, ni journal exhaustif des provenances de doublons.

## A.7 Correction du locator

### Théorème A — absence de confusion de clés

Si le locator retourne `positive` pour $F$, alors la clé complète d'une liaison stockée est exactement $\kappa(F)$ et le handle renvoyé est la racine pré-appel de son handle stable.

**Preuve.** Une empreinte ne suffit jamais à produire la réponse. Le hit impose l'égalité du cardinal et de tous les `PointId` avec une clé stockée. La recherche précède toute mutation du lot, puis `find` suit seulement le DSU engagé. Une collision ou une liaison courante ne peut donc produire ce hit. $\square$

### Corollaire A' — sens conditionnel du hit

Si l'appelant rejoue indépendamment le témoin externe et certifie que la liaison stockée place $F$ dans la composante de $h$, alors la composante renvoyée contient $F$ dans l'état pré-appel. Ce corollaire est `conditional_on_caller_external_replay`; le locator seul n'établit pas son hypothèse.

### Théorème B — honnêteté du miss

Si le locator retourne `unresolved`, aucune conclusion négative sur $F$ ne suit.

**Preuve.** Le dictionnaire représente une application partielle dont le domaine est l'ensemble des liaisons positives acceptées, et non l'univers de toutes les facettes. L'absence de $\kappa(F)$ prouve seulement que $F$ n'appartient pas à ce domaine visible. Sans certificat externe de totalité, le complément du domaine contient à la fois des facettes réellement absentes et des facettes valides mais pas encore découvertes. Toute classification plus forte serait donc non fondée. $\square$

### Théorème C — stabilité sous union

Sous l'hypothèse externe que les unions sont valides, leur suite conserve la validité de toute liaison positive sans réécrire sa clé.

**Preuve.** Une union DSU ne retire aucun élément d'une classe; elle remplace deux classes par leur union. Le handle immuable de la liaison reste un élément de la classe résultante et `find` en donne la racine courante. La proposition « $F$ appartient à la composante de $h$ » reste vraie par monotonie des unions. $\square$

### Théorème D — visibilité strictement pré-appel

Toutes les localisations réussies pendant un appel proviennent de l'état engagé avant cet appel.

**Preuve.** Par construction, la boucle de requêtes précède la copie mutée du DSU et ne consulte jamais l'arène de staging. Une liaison ou une union courante ne peut donc influencer aucun lookup du même appel. $\square$

Ces résultats prouvent la correction **interne et relative** du socle. Ils ne prouvent ni l'autorité externe, ni la complétude du domaine, ni la correction d'une action HGP lorsque certains endpoints restent `unresolved`.

## A.8 Vérification structurelle fraîche

`verify_exact_direct_sparse_positive_facet_locator_structure` reçoit séparément les paramètres de construction fiables et une vue non propriétaire de l'état observé. Il rejette les tailles de spans dépassant ces paramètres avant tout scan ou scratch dépendant de l'image. Il vérifie ensuite :

- la couverture sans trou ni recouvrement de l'arène plate de clés;
- la forme canonique, l'empreinte recalculée et la retrouvabilité de chaque clé complète;
- les indices, handles et jetons non nuls des unions durables;
- le DSU final par rejeu de ces unions;
- les partitions et plafonds des compteurs agrégés par lot;
- les faits structurels, la décision d'initialisation et la portée relative.

Il ne rejoue ni l'autorité externe, ni les anciens payloads de requêtes ou de doublons, qui ne sont pas conservés. Les booléens historiques sont donc seulement exigés et leurs agrégats rendus cohérents; ils ne deviennent pas des preuves fraîches de la transaction passée. Le scratch temporaire est $O(L+KL+H)$, sans seconde sortie durable.

## A.9 Bornes réelles du noyau de référence

Notons $M$ la capacité configurée de clés, $L\leq M$ le nombre de clés distinctes engagées, $H$ le nombre de handles, $U$ le nombre d'unions durables, $T$ le nombre de lots engagés, $B$ le nombre de demandes de liaison du lot courant et $Q$ son nombre de requêtes. Comme $K\leq10$, une clé complète occupe au plus dix `PointId` plus un cardinal fixe.

La table est préallouée à $2M+1$ slots. L'état durable occupe donc

$$O(M+KL+H+U+T).$$

Le scratch d'un lot occupe

$$O(Q+KB+H).$$

Le DSU actuel n'emploie ni rang ni compression de chemin, et le hash ouvert n'a pas de repli radix. En notant $U_b$ les unions du lot, une borne pessimiste explicite est

$$O\bigl(H+Q(KM+H)+U_bH+B(K(M+B)+H)\bigr).$$

Le vérificateur structurel peut lui-même atteindre $O(KLM+UH)$, puisqu'il retrouve fraîchement chacune des $L$ clés et rejoue toutes les unions. Ces bornes ne qualifient pas le temps attendu du mélangeur; elles garantissent seulement que la correction ne dépend pas de sa qualité.

Aucun terme ne dépend de $\binom{n}{k}$. Le noyau exclut en particulier :

- une table indexée par toutes les $k$-facettes;
- un vecteur de toutes les cellules ou cofaces;
- la matérialisation de toutes les incidences d'un niveau;
- un historique Gamma dans le chemin produit;
- une mosaïque de Delaunay d'ordre supérieur, même sous un autre nom.

---

# Partie B — couche géométrique top-k-only encore ouverte

## B.1 Objet

Sur un `unresolved`, la prochaine couche pourra tenter une descente géométrique depuis la facette $F$ vers une facette canonique $G$ de niveau strictement inférieur, puis réinterroger le locator. Cette couche ne fait pas partie de la preuve A et ne doit pas être utilisée pour publier une attache avant certification de chaque arc et de son rejeu.

Soit $F\subseteq X$, $\lvert F\rvert=k\leq10$. Une miniball locale exacte fournit son centre $c_F$, son niveau $\beta(F)$, son intérieur local $I_F$ et sa frontière locale $U_F^{\partial}$. Elle fournit séparément un support positif minimal $U_F\subseteq U_F^{\partial}$.

Une requête top-k exacte complète au centre $c_F$ fournit le cutoff $d_k(c_F)$, l'ensemble strict $E_{<}$, le shell $E_{=}$ et un choix canonique

$$G=E_{<}\cup A,\qquad A\subseteq E_{=},\qquad\lvert A\rvert=k-\lvert E_{<}\rvert.$$

La complétude signifie que tous les points strictement sous le cutoff et tous les points exactement au cutoff sont représentés ou couverts par un certificat exact; une liste tronquée de voisins ne satisfait pas cette hypothèse.

## B.2 Lemme top-k-only

> **Statut : `conditional_theorem`.** L'énoncé ci-dessous doit encore être confronté au formalisme existant, implémenté avec un contrat versionné, rejoué fraîchement et falsifié par l'oracle borné avant de devenir une base de décision du chemin produit.

### Énoncé conditionnel

Sous l'exactitude de la miniball locale de $F$ et de la requête top-k complète en $c_F$, on a toujours

$$d_k(c_F)\leq\beta(F),$$

car $F$ est lui-même un choix de $k$ points dont toutes les distances à $c_F$ sont au plus $\beta(F)$.

Les branches admissibles sont les suivantes.

1. **Cutoff strict.** Si $d_k(c_F)<\beta(F)$, alors le choix canonique vérifie $\beta(G)<\beta(F)$.
2. **Cutoff égal régulier.** Supposons $d_k(c_F)=\beta(F)$, $U_F^{\partial}=U_F$ et $E_{=}=U_F$. S'il existe un point de $E_{<}\setminus F$, alors $F$ est inactive en son propre centre et le choix canonique $G$ omet au moins un point essentiel de $U_F$; on a encore $\beta(G)<\beta(F)$.
3. **Actif.** Sous les mêmes égalités régulières, si $E_{<}\setminus F=\varnothing$, alors $G=F$ et $F$ appartient à la famille top-k en son centre.
4. **Non pris en charge.** Si le cutoff égal s'accompagne d'une frontière locale non réduite au support positif, d'un shell top-k différent de cette frontière, d'une requête incomplète ou d'une autorité non rejouable, la décision est `unsupported` ou `unresolved`, jamais un arc strict inventé.

Une observation $d_k(c_F)>\beta(F)$ contredit les entrées exactes et doit échouer fermée.

### Preuve conditionnelle

Dans la branche 1, tous les points de $G$ sont à distance carrée au plus $d_k(c_F)$ de $c_F$. Ainsi

$$\beta(G)\leq\max_{x\in G}\left\Vert x-c_F\right\Vert^2\leq d_k(c_F)<\beta(F).$$

Dans la branche 2, l'égalité $U_F^{\partial}=U_F$ implique que tous les points de $F\setminus U_F$ sont strictement intérieurs à la miniball. Comme $E_{=}=U_F$, l'ensemble $E_{<}$ contient $F\setminus U_F$ ainsi qu'au moins un point extérieur à $F$. Le choix de cardinal $k$ doit donc remplacer autant de points de $U_F$ qu'il ajoute de points strictement intérieurs externes; il omet en particulier un point du support positif.

Le critère de miniball dit que le centre d'une boule minimale appartient à l'enveloppe convexe de ses points de frontière. La positivité et la minimalité de $U_F$ impliquent que $c_F$ n'appartient à l'enveloppe convexe d'aucun sous-ensemble propre de $U_F$. Or les seuls points de $G$ encore sur la sphère source forment un tel sous-ensemble propre; tous les autres sont strictement intérieurs. La boule source contient déjà $G$ mais n'est pas sa boule minimale, d'où $\beta(G)<\beta(F)$.

Tout point de $E_{<}$ appartient obligatoirement à chaque choix top-k au cutoff. La présence d'un tel point extérieur à $F$ interdit donc $F$ dans la famille top-k en $c_F$. Cela prouve son inactivité dans cette branche. Dans la branche 3, $E_{<}=F\setminus U_F$ et le cardinal impose $A=U_F$, donc $G=F$ et la facette est active.

La preuve n'établit rien dans la branche 4 : un support non essentiel ou un extra-shell peut produire un plateau, plusieurs choix de même niveau ou une information insuffisante. La classification `unsupported` est donc nécessaire.

## B.3 Pourquoi aucun vecteur global d'extérieur n'est requis

Le lemme consulte seulement :

- la miniball de la petite facette $F$, de cardinal au plus dix;
- les points strictement sous le cutoff top-k;
- le shell exact au cutoff;
- un certificat que toutes les régions restantes du nuage sont strictement au-delà du cutoff.

Dans la branche stricte, la borne sur $G$ suffit. Dans la branche égale, comparer $E_{<}$ à $F\setminus U_F$ détecte un intérieur externe et comparer $E_{=}$ à $U_F$ ferme la régularité. Les points de distance strictement supérieure à $\beta(F)$ ne participent à aucune de ces implications. Il n'est donc pas nécessaire de matérialiser la partition `closed_ball` globale ni un vecteur contenant tous les points extérieurs.

Cette économie ne supprime pas l'obligation d'exclusion. Un nœud LBVH non développé doit porter une borne exacte prouvant que tous ses points sont strictement au-delà du cutoff. Une égalité force la descente ou un certificat de shell; une frontière non épuisée laisse la requête `unresolved`.

## B.4 Tranche 10.5b recommandée

`spatial::lbvh_top_k` existe déjà et son shell est complet : le parcours ne coupe un nœud que lorsque sa borne inférieure exacte est strictement supérieure au cutoff courant, jamais en cas d'égalité. En revanche, l'API est synchrone, non interruptible et non budgétée; son pire cas, son tableau de voisins évalués et un shell dégénéré restent linéaires en $n$. Le booléen `shell_complete()` reflète cette construction, mais n'est pas un certificat autonome : le consommateur certifié doit rejouer la requête depuis le nuage, le centre, le rang et le LBVH fiables.

La première tranche 10.5b doit rester un saut unique :

1. sonder $F$ dans le locator pré-lot par une API `const` budgétée;
2. sur miss, construire et vérifier fraîchement la miniball locale de $F$;
3. exécuter une tentative LBVH top-k bornée, sans exclusions;
4. construire le choix canonique $G$, puis construire et vérifier fraîchement sa miniball;
5. accepter un arc seulement après la comparaison directe $\beta(G)<\beta(F)$;
6. sonder $G$ dans le même état pré-lot et ne résoudre positivement que si sa clé est déjà liée;
7. retourner sinon `unresolved`, sans insertion ni singleton implicite.

Cette comparaison directe de miniballs suffit à certifier le saut et peut accepter prudemment plus de cas que le lemme régulier B.2. Le lemme reste utile pour expliquer les branches, mais il n'a pas besoin d'être promu en autorité de décision. Le témoin de segment conserve le cutoff au centre source et $\beta(G)$ au centre cible; aucun vecteur global de boule fermée extérieure n'est nécessaire.

La tentative top-k doit vérifier avant chaque opération des plafonds distincts pour les visites de nœuds, expansions internes, évaluations exactes de bornes AABB, distances exactes aux points, taille de frontière, voisins évalués et identifiants du shell. `budget_exhausted` ou `cancelled` ne doit exposer aucun `TopKPartition` scientifique partiel et ne doit muter ni locator ni journal.

Une sonde `const` du locator est également obligatoire. Employer `apply_batch` avec des unions et insertions vides copierait encore les $H$ parents et engagerait un lot; ce n'est ni une lecture ni un chemin acceptable pour 10 M+. La sonde doit budgéter séparément les slots visités et les sauts DSU, puis rendre `unresolved` si elle ne termine pas.

Les autres dettes restent :

- isoler la miniball de facette dans un target étroit, sans l'archive `hierarchy` complète;
- certifier le cutoff, le choix canonique, le strict set complet et l'absence de tout extra-shell sans vecteur global d'extérieur;
- rejouer fraîchement miniball, clé source, requête, choix cible et comparaison de niveaux depuis les autorités;
- distinguer une proposition flottante d'une décision multiprécision certifiée;
- conserver un témoin d'arc rejouable et un budget de fermeture; un arc individuellement vrai ne prouve pas que la descente atteint une liaison connue;
- traiter ou refuser explicitement les plateaux, supports non essentiels et shells dégénérés;
- prouver que les descentes et les gateways positives suffisent à toutes les attaches nécessaires au profil `hgp_reduced`;
- maintenir `unresolved` après épuisement de budget, sans naissance ni isolation implicite.

L'oracle Gamma exhaustif borné peut falsifier ces arcs et leurs cibles sur de petits nuages. Il ne devient ni l'index du locator, ni une solution de secours en production, ni l'architecture de la descente.

---

## Cibles produit et limites honnêtes

Pour le cas interactif autour de 50 000 points et $K\leq10$, le socle possède des clés de largeur fixe et n'alloue rien en fonction du nombre de points extérieurs. Toutefois, un probe peut visiter $O(M)$ slots et suivre $O(H)$ parents; ces propriétés ne suffisent donc pas encore à une réponse sous la seconde, et aucun SLO n'est atteint ni revendiqué par ce jalon.

Pour les nuages de 10 000 000 de points ou davantage, l'absence de terme $\binom{n}{k}$ est acquise, mais la table résidente est provisionnée à la capacité $M$ et chaque lot copie les $H$ parents. À cela s'ajoutent le coût d'une frontière LBVH ambiguë, d'un shell massif et de longues descentes. Le noyau de référence prépare l'interface sparse; il n'est pas qualifié 10 M+.

## Tableau des statuts

| énoncé ou objet | statut |
|---|---|
| identité exacte par clé complète après empreinte | proved_here et validated_host_software |
| sens sémantique d'un hit positif | conditional_on_caller_external_replay |
| stabilité interne des handles sous unions DSU | proved_here et validated_host_software |
| invisibilité des insertions et unions du lot courant pour les lookups | proved_here et validated_host_software |
| miss donnant exclusivement `unresolved` | proved_here et validated_host_software |
| conflit post-unions d'une clé exacte vers deux composantes incompatibles | contradiction_permanente |
| état sans terme combinatoire $\binom{n}{k}$, mais préalloué en $M$ | proved_here |
| vérification structurelle fraîche sans rejeu externe ou historique | validated_host_software |
| implémentation hôte dédiée du socle 10.5a | validated_host_software |
| lemme top-k-only de descente stricte | conditional_theorem |
| LBVH top-k-only budgété avec preuve de complétude et rejeu | proof_obligation |
| fermeture des gateways silencieux non directs | proof_obligation |
| M.1 et exactitude `full_pi0` | proof_obligation |
| exactitude publique de la hiérarchie d'ordre supérieur | not_claimed |

Le jalon reste donc `partial_refinement`, avec `public_status=not_claimed`.
