# Premières incidences des facettes directes réutilisées — Phase 10.7-RCPU

## Portée et statut d'ouverture

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement` et `public_status=not_claimed`.

Le jalon 10.7-RCPU compose l'alphabet factorisé des suppressions 10.2--10.4 avec l'oracle local 10.6. Il propose exactement les facettes réutilisées par les selles directes déjà présentes dans la façade terminale, déduplique leurs clés complètes, calcule une seule fois leur première incidence et conserve tous leurs co-minimiseurs sous forme factorisée. Il ne prétend pas que ce sous-univers contient toutes les facettes-portes nécessaires à la réduction HGP.

Les énoncés combinatoires et géométriques ci-dessous sont `proved_here`. Le suivi logiciel est `validated_host_software` après la campagne courte 10.7; cette validation reste strictement relative au sous-univers direct et ne change aucun statut public.

## 1. Porte d'entrée

La porte d'entrée globale de la Phase 10 est satisfaite. La porte locale de 10.7 exige en plus :

- une façade terminale de Phase 9 et les journaux 10.2 et 10.4 fraîchement rejoués sous des autorités concordantes;
- la partition exhaustive des suppressions de chaque selle directe fournie par 10.4;
- l'oracle 10.6 validé pour une facette canonique de cardinal au plus dix;
- un nuage canonique et un LBVH fiables liés à la même autorité;
- des plafonds fiables pour les familles sources, les références brutes, les clés distinctes, les `PointId` des clés, les candidats, les lots, leurs références et le stockage logique, plus un budget 10.6 appliqué indépendamment à chaque clé distincte.

Les étages 10.5a--10.5c ne sont pas des préconditions. Ce jalon ne consulte et ne modifie aucun locator, et il ne localise aucune racine.

## 2. Univers direct proposé

Pour un ordre $k$, soit $\mathcal{E}_k$ l'ensemble des selles directes terminales fournies et soit $\mathcal{E}=\bigcup_{k=1}^{K}\mathcal{E}_k$. Pour $e\in\mathcal{E}$, notons $S_e$ sa coface fermée, $a_e=\beta(S_e)$ son niveau exact et $v$ un point de $S_e$. La facette associée à cette suppression est $F_{e,v}=S_e\setminus\lbrace v\rbrace$.

Les occurrences brutes et les clés distinctes sont définies par :

$$\mathcal{R}_{\mathrm{dir}}=\lbrace(e,v):e\in\mathcal{E},\ v\in S_e\rbrace,\qquad\mathcal{F}_{\mathrm{dir}}=\lbrace F_{e,v}:(e,v)\in\mathcal{R}_{\mathrm{dir}}\rbrace.$$

La première collection conserve les provenances : deux selles distinctes ou deux suppressions peuvent mener à la même clé de facette sans devenir le même fait source. La seconde collection déduplique uniquement par cardinal et liste complète de `PointId` strictement croissants.

Si $J$ est le nombre de selles directes, $R=\lvert\mathcal{R}_{\mathrm{dir}}\rvert$ le nombre d'occurrences et $D=\lvert\mathcal{F}_{\mathrm{dir}}\rvert$ le nombre de clés distinctes, alors 10.4 donne exactement :

$$R=\sum_e\lvert S_e\rvert\leq11J,\qquad D\leq R.$$

Cette borne vient de $\lvert S_e\rvert=k+1\leq11$ pour $K\leq10$. Elle est proportionnelle aux événements directs déjà retenus et ne contient aucun terme $\binom{n}{k}$. Lorsque $J=0$, le résultat complet vide vérifie $R=D=0$ et ne lance aucun appel géométrique.

## 3. Première incidence et tokens factorisés

La famille `ExactDirectSparseGatewayCandidate*` porte ce contrat. Pour chaque $F\in\mathcal{F}_{\mathrm{dir}}$, 10.7 appelle exactement une fois l'oracle 10.6 et définit :

$$\lambda(F)=\min_{x\in X\setminus F}\beta(F\cup\lbrace x\rbrace),\qquad M(F)=\lbrace x\in X\setminus F:\beta(F\cup\lbrace x\rbrace)=\lambda(F)\rbrace.$$

Le journal de candidats de première incidence est :

$$T_{\mathrm{dir}}=\lbrace(F,x,\lambda(F)):F\in\mathcal{F}_{\mathrm{dir}},\ x\in M(F)\rbrace.$$

Chaque `facet_token` conserve une clé $F$ de cardinal au plus dix, son niveau et ses plages contiguës. Chaque `gateway_candidate` conserve le seul `added_point_id` $x$ et le témoin de support positif de cardinal au plus quatre rendu par 10.6. La coface logique $F\cup\lbrace x\rbrace$ peut contenir onze points à $K=10$, mais aucune clé persistante de largeur onze n'est construite.

Les `facet_tokens` sont triés et regroupés canoniquement par `(facet_cardinality, exact_first_incidence_level)`. Toutes les clés d'un lot et tous les co-minimiseurs d'une clé appartiennent à la même publication atomique. Le niveau n'est pas répété comme une autorité indépendante dans chaque candidat : il appartient au record de facette et au descripteur de lot exact.

La sortie scientifique possède exactement cinq arènes plates : `deletion_projections`, `facet_tokens`, `gateway_candidates`, `batches` et `batch_facet_token_indices`. Elles contiennent respectivement les $R$ provenances, les $D$ clés distinctes, les $C$ co-minimiseurs factorisés, les $B$ lots et les $D$ références de tokens dans ces lots. Aucun résultat 10.6 enfant complet n'est conservé.

## 4. Relation au lot direct source

Pour toute provenance $(e,v)$, le point retiré $v$ est un candidat admissible puisque $S_e=F_{e,v}\cup\lbrace v\rbrace$. Par définition du minimum :

$$\lambda(F_{e,v})\leq\beta(F_{e,v}\cup\lbrace v\rbrace)=\beta(S_e)=a_e.$$

Chaque projection source reçoit donc exactement une des deux dispositions suivantes :

- `first_incidence_strictly_below_saddle` si $\lambda(F_{e,v})<a_e$;
- `first_incidence_equal_to_saddle` si $\lambda(F_{e,v})=a_e$.

Une relation $\lambda(F_{e,v})>a_e$ ou une décision `complete_no_coface` est une contradiction permanente : la coface $S_e$ existe déjà. La disposition stricte signale un candidat latent antérieur au lot direct; la disposition égale signale une incidence simultanée. Aucune des deux ne constitue à elle seule un `GatewayAttach` ni une preuve de racine.

## 5. Preuves relatives fermées par le jalon

L'exhaustivité de $\mathcal{F}_{\mathrm{dir}}$ relativement au flux fourni découle de 10.4 : les suppressions du support sont exactement les bras 10.2, les suppressions intérieures sont exactement les graines égales 10.4, et leur union contient les $k+1$ facettes de chaque $S_e$. Leur reconstruction en flux suivie d'une déduplication par clé complète produit donc exactement $\mathcal{F}_{\mathrm{dir}}$.

Pour une clé distincte, une terminaison complète de 10.6 certifie exactement $\lambda(F)$ et la famille entière $M(F)$. Une invocation unique par clé et une projection bijective de chaque sortie vers $T_{\mathrm{dir}}$ donnent donc exactement tous les tokens du sous-univers direct. Les co-minimiseurs égaux ne peuvent être séquentialisés ou tronqués.

Ces conclusions restent relatives à la façade terminale externe et aux journaux 10.2--10.4 fraîchement vérifiés. Elles ne prouvent pas que $\mathcal{F}_{\mathrm{dir}}$ est suffisant pour toutes les composantes de Gamma ou pour la seule forêt `hgp_reduced`.

## 6. Publication atomique et vérification fraîche

Les plafonds globaux de familles parcourues, références, clés distinctes, `PointId` de clés, candidats, lots, références de lots et stockage logique sont contrôlés avant la réserve, la requête ou la publication correspondante. Le nombre d'appels est certifié égal à $D$; le même budget 10.6, avec ses neuf plafonds locaux, est appliqué indépendamment à chaque clé. Une autorité divergente, une reconstruction impossible, un doublon incohérent, une décision 10.6 non complète, un dépassement global ou une relation $\lambda(F)>a_e$ rend le jalon incomplet.

Dans ce cas, aucun niveau, lot, token, projection scientifique ou préfixe de facettes n'est publié. Les diagnostics et compteurs de travail peuvent subsister, mais ils ne sont pas une sortie HGP. Le vérificateur reçoit séparément les autorités et budgets fiables, borne les tableaux observés avant de les parcourir, reconstruit $\mathcal{R}_{\mathrm{dir}}$ et $\mathcal{F}_{\mathrm{dir}}$, puis rejoue 10.6 une fois par clé distincte.

## 7. Bornes et structures évitées

Posons $C=\sum_{F\in\mathcal{F}_{\mathrm{dir}}}\lvert M(F)\rvert$ et $B$ le nombre de lots `(cardinal, niveau)`. Le résultat logique contient $R$ projections, $D$ clés de cardinal au plus $K$, $C$ tokens factorisés et $B\leq D$ descripteurs de lots, donc :

$$\mathrm{stockage}_{\mathrm{logique}}=O(R+KD+C),\qquad C\leq\sum_{F\in\mathcal{F}_{\mathrm{dir}}}(n-\lvert F\rvert)\leq D(n-1).$$

Le scratch conserve la table bornée des seules clés directes, les index nécessaires à la forme canonique et une seule invocation 10.6 à la fois. Il ne persiste pas une frontière LBVH, un shell, une partition, une miniball source complète ou un résultat enfant 10.6 entier par facette.

Le chemin produit interdit explicitement :

- l'univers des $\binom{n}{k}$ facettes et des $\binom{n}{k+1}$ cofaces;
- tout catalogue, coupe, composante ou incidence Gamma global;
- les cellules top-$m$, la mosaïque de Delaunay d'ordre supérieur et un graphe de Gabriel global;
- la matérialisation de cofaces de onze identifiants;
- toute mutation d'un locator, toute DSU de forêt, racine, union, `RootBirth`, `AtomicUnionBatch` ou `GatewayAttach`;
- la réimplémentation de l'oracle exhaustif borné sous un autre nom.

Le pire cas peut encore effectuer $D$ parcours linéaires du LBVH et produire $C=\Theta(Dn)$ tokens. Les rationnels exacts n'ont pas encore de budget de limbs ou d'octets. Le jalon ne qualifie donc ni 50 000 points sous une seconde, ni 10 000 000 points ou davantage, malgré l'absence de structure combinatoire globale.

## 8. Validation hôte réalisée

La promotion à `validated_host_software` a exigé au minimum :

- le compte exact $R=\sum_e\lvert S_e\rvert$, la borne $R\leq11J$, la déduplication complète et une projection unique de chaque provenance;
- exactement un appel 10.6 par clé distincte et l'invariance sous permutation des événements et sous les deux ordres LBVH;
- un différentiel indépendant pour $n\leq14$ contre toutes les cofaces à un point des seules clés de $\mathcal{F}_{\mathrm{dir}}$;
- des facettes dupliquées par plusieurs provenances, des cas $\lambda(F)<a_e$ et $\lambda(F)=a_e$, et le résultat complet vide;
- la fixture permanente à cinq points, où la suppression de $B$ dans la future selle directe $ABC$ propose $AC$, puis publie exactement $D$ et $E$ au niveau $33/2$;
- la frontière $K=10$ sans clé persistante de onze points;
- chaque plafond global et local à sa valeur exacte puis diminué d'une unité, avec sortie scientifique entièrement vide sur échec;
- les falsifications d'autorité, de relation temporelle, de projection, de lot, de token et de stockage observé;
- un contrôle statique et symbolique interdisant toutes les structures globales et mutations citées plus haut.

Le dernier rejeu GCC Release ferme 11 CTests en 3,84 secondes. Il comprend le différentiel indépendant par miniballs explicites pour $n=6\leq14$, le cas $J=R=D=0$, la déduplication d'une facette partagée, les relations stricte et égale, la fixture `AC`, la frontière $K=10$, les deux ordres LBVH, la permutation des entrées brutes vers le même flux canonique, les huit plafonds globaux et les neuf plafonds 10.6 exacts puis moins-un, les autorités étrangères, les falsifications de stockage et de liens croisés, le garde statique et l'audit `nm`.

L'ordre des événements est une partie indexée de l'autorité amont : une permutation directe de ce tableau n'est pas un planning alternatif certifié mais une falsification, et le test la rejette. Le compteur d'appels publié vaut $D$; l'unique site d'appel 10.6 dans la boucle des clés distinctes est imposé par le garde statique. Sous une source 10.4 certifiée, `complete_no_coface` est inaccessible puisque le point retiré fournit déjà une coface; cette branche défensive est conservée et contrôlée statiquement.

Aucun benchmark long ni campagne GCP n'a été requis pour cette promotion hôte.

## 9. Limite scientifique et incrément suivant

La note sur les incidences silencieuses suggère de calculer $\lambda(F)$ pour les facettes réutilisées par un futur lot Gabriel, mais précise que la suffisance de cette sélection reste à démontrer lorsque plusieurs minimiseurs ou plusieurs lots partagent une attache. 10.7 matérialise exactement cette proposition parcimonieuse sans la promouvoir en théorème global.

Un incrément ultérieur pourra localiser les cofaces minimisantes dans des racines gelées, traiter atomiquement chaque lot exact et produire des actions HGP seulement après preuve de complétude. La génération de tous les gateways silencieux non directs, le quotient, les racines, unions, forêts, `GatewayAttach`, M.1, les SLO 50 k et 10 M+ et tout statut public restent ouverts. La Phase 10 demeure `in_progress` avec `exit_gate_satisfied=false`.
