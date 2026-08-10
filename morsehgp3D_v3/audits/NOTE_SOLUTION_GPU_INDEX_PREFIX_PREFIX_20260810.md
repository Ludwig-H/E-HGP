# Solution GPU — index exact préfixe--préfixe sans faces

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=candidat_gpu_g4`,
`profile=hgp_reduced_K10`, `mode=validated_hybrid_fallback`,
`public_status=not_claimed`.

## Résultat directement exploitable

Le fallback du fold n'a pas besoin d'indexer les $k$-faces ni de scanner les
postings complets de tous les points d'un générateur. Un ordre total **commun et
immuable** des `PointId` permet d'indexer et d'interroger seulement un préfixe
court de chaque saturé. Le filtre ne perd aucun couple ayant une intersection
d'au moins $k$ points; une intersection courte des deux listes de membres
recertifie ensuite chaque candidat.

Pour un ensemble fini $S$ de rang $r\geq k$, noter $P_k(S)$ ses $r-k+1$
premiers éléments dans un même ordre total $\prec$ sur les points.

**Théorème préfixe--préfixe.** Pour tous $M,N$ de rang au moins $k$ :

$$\lvert M\cap N\rvert\geq k\Longrightarrow P_k(M)\cap P_k(N)\neq\varnothing.$$

**Preuve.** Soit $x$ le plus petit élément de $M\cap N$ pour $\prec$. Il
existe au moins $k-1$ éléments communs après $x$. Dans $M$, il reste donc au
moins $k-1$ positions après $x$, si bien que $x$ a au plus $\lvert M\rvert-k$
prédécesseurs et appartient à $P_k(M)$. Le même raisonnement vaut dans $N$.

La longueur est optimale au pire cas. Prendre deux ensembles dont les $k$
éléments communs viennent après deux préfixes privés disjoints de taille $r-k$ :
des préfixes de longueur $r-k$ deviennent disjoints alors que l'intersection
complète vaut $k$. Deux énumérations exhaustives indépendantes sur des univers
de tailles 7 et 8 ont vérifié respectivement 28 672 et 131 072 implications,
sans contre-exemple; la preuve reste l'autorité.

## Index et kernel proposés

Pour chaque ordre $k$, construire la CSR :

$$I_x^{(k)}=\left\lbrace N:x\in P_k(N)\right\rbrace.$$

Une requête fallback $M$ concatène les listes $I_x^{(k)}$ pour
$x\in P_k(M)$, trie ou hache les `GeneratorId`, retire les doublons, puis
vérifie directement $\lvert M\cap N\rvert\geq k$ sur les membres triés. Pour
le contrat courant de rang au plus 11, cette dernière intersection tient dans
un warp et n'exige ni sous-simplexe, ni mosaïque, ni table de faces.

Comme $I_x^{(k)}$ est inclus dans le posting complet du point $x$, la masse lue
est monotoniquement inférieure ou égale au cover $t=1$ déjà spécifié. La masse
persistante vaut exactement :

$$L_k=\sum_{N:\lvert N\rvert\geq k}(\lvert N\rvert-k+1).$$

Pour un générateur de rang $r$ présent jusqu'à $m=\min(K,r)$, sa contribution
aux $K$ index vaut :

$$L(r,K)=m(r+1)-\frac{m(m+1)}{2}.$$

Ainsi $r=11,K=10$ donne 65 entrées, contre 110 entrées si les onze membres
étaient réindexés aux dix ordres et 2 046 signatures non triviales si les faces
étaient matérialisées. Ces nombres bornent le stockage de l'index, pas le nombre
de hits : les corrélations des postings restent à mesurer avant admission.

Le choix de $\prec$ ne touche pas à l'exactitude. La première version peut
prendre `PointId`, ce qui autorise un flux en une passe. Une version rare-first
peut prendre `(degre_global,PointId)`, à condition de calculer les degrés sur le
catalogue final et de figer cet ordre pour tous les lots d'un même ordre $k$.
Un degré recalculé par requête, par lot ou par worker invalide le théorème.

## Lots exacts et masque hybride

À un niveau exact, le lot entier est stagé dans l'index avant la première
requête; les générateurs futurs restent invisibles. `ActivationId` est un ordre
canonique dans le lot, indépendant de l'ordonnancement GPU.

Pour un `query_mask` de fallback, une paire recertifiée $(M,N)$ est conservée
si et seulement si `N` n'est pas une requête du lot courant, ou si
`ActivationId(N)<ActivationId(M)`. Le self est exclu séparément. Cette règle :

- possède une paire fallback--fallback exactement une fois;
- conserve toute paire fallback--fast, même si le fast est postérieur;
- laisse les seules paires fast--fast au certificat du chemin principal.

Le `GeneratorId` réel et les $k$ points témoins sont conservés jusqu'à la
recertification. Projeter d'abord un posting vers une racine DSU est faux : des
membres appartenant séparément à une racine ne certifient pas qu'un même
générateur partage les $k$ points. La projection vers les racines strictes
gelées, puis les composantes staging, vient seulement après le test du couple
réel.

L'unité de travail device recommandée est un intervalle de requêtes et un slab
de `GeneratorId` candidats. Aucun posting n'est coupé sans transporter le
compteur de la clef frontière. Les sorties d'un slab sont soit entièrement
committées, soit entièrement rejouées; les capacités sont préflightées depuis
`prefix_hits`, jamais découvertes après une écriture hors budget.

## Variante à seuil $t$

Pour $1\leq t\leq k$, prendre les $\lvert S\rvert-k+t$ premiers éléments de
$S$. Les $t$ plus petits éléments de toute intersection de taille au moins
$k$ appartiennent aux deux préfixes. On peut donc compter les hits communs et
ne recertifier que les candidats de multiplicité au moins $t$.

Le mode $t=1$ minimise la longueur des préfixes et constitue la baseline. Un
$t>1$ augmente forcément les entrées et les hits bruts, mais peut réduire les
faux candidats; seuls `prefix_hits`, `unique_candidates` et
`recertified_true` permettent de choisir. Aucun choix à partir des seuls degrés
ne borne les corrélations.

## Place dans le fold hybride

Ce filtre n'a pas vocation à devenir le chemin universel. L'ordre recommandé
est :

1. certificat exact `principal_support` et au plus quatre lookups de carriers;
2. préfixe--préfixe pour le seul `query_mask` restant;
3. recertification de l'intersection réelle;
4. fermeture atomique du graphe staging;
5. refus ou oracle borné si la provenance de source et le certificat
   fast--fast requis manquent.

Il retire donc le mur des faces du fallback, mais ne remplace ni le
`ValidatedHybridSidecar`, ni la complétude de la source, ni la réduction de la
DSU. Sa valeur industrielle vient de la combinaison « fast majoritaire,
fallback rare, index préfixé », pas d'une prétention que toute CSR est petite.

## Reçu et portes permanentes

Publier par ordre et par lot : `prefix_index_entries`, `prefix_queries`,
`prefix_hits`, `unique_candidates`, `recertified_true`, faux candidats,
longueur maximale d'un posting, high-water des slabs, principal/fallback,
arêtes fast--fast certifiées, commits, rollbacks et digests de l'ordre global,
du catalogue et du nuage.

Portes minimales :

1. exhaustif de petits ensembles contre le join quadratique pour tout
   $1\leq k\leq K$;
2. éléments communs placés aux dernières positions, qui tuent le mutant
   `prefix_length-1`;
3. ordres `PointId` et rare-first, puis permutations d'allocation, avec même
   ensemble d'arêtes recertifiées;
4. lot ex aequo où la dernière requête relie deux chaînes de nouvelles
   activations;
5. paire fallback précoce--fast tardive, perdue par le faux filtre `N<M`;
6. trois points issus séparément de trois générateurs déjà dans la même racine,
   qui tuent la compression des postings par DSU avant recertification;
7. slab coupant un run de candidat, overflow tardif, rollback total et rejeu
   identique;
8. mutants `ordre_par_requete`, `future_visible`, `skip_exact_intersection`,
   `drop_last_posting` et `project_root_first`.

## Prochain palier conseillé à Claude

Implémenter d'abord cet index sur le catalogue CPU déjà qualifié et comparer les
arêtes, partitions, records et marqueurs aux folds `G2`, postings global et
`face-owner` pour $n\leq400$. Mesurer ensuite uniquement le kernel sparse sur le
catalogue $n=2400$ déjà disponible. Cette expérience tranche la masse du vrai
fallback et la réduction mémoire avant de payer une nouvelle génération 50 k.

GCP non utilisé.
