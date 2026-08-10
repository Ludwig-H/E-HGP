# Solution constructive — join par postings, transcript saturé et porte G4 50 k

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, backend CPU de vérité, profil
u16, `public_status=not_claimed`. Cette note ne développe aucun backend. Elle
transforme le mur observé du fold saturé en algorithme, reçus et critères de
décision falsifiables.

## Verdict utile

Le fold `O(K*G^2)` est maintenant une bonne vérité bornée, pas une architecture
50 k. Le prochain objet à qualifier est un join exact et output-sensitive sur
les générateurs saturés. Une G4 n'apporte rien au binaire courant, qui est
CPU-only. Elle devient utile seulement après un kernel réel du join, un
différentiel contre le fold de vérité et un manifeste mémoire calculé avant le
démarrage.

## 1. Théorème de réduction à recevoir

Soit `Sigma` la famille complète des saturés fermés, dédupliqués par boule et
triés par niveau exact. Pour chaque point `x`, soit `P_x` la liste des
identifiants de générateurs qui le contiennent. L'émission d'une occurrence par
paire non ordonnée dans chaque `P_x`, suivie d'une réduction globale par paire,
donne exactement :

$$w(M,N)=\lvert M\cap N\rvert.$$

À l'ordre `k`, l'arête `M--N` existe exactement lorsque `w(M,N)>=k`. Elle
devient active au niveau maximum des deux générateurs. Par S.4, les composantes
du DSU ainsi obtenu sont celles de Gamma, sans matérialiser les `k`-facettes,
les graphes de Johnson ni la mosaïque de Delaunay d'ordre supérieur.

Le poids peut être plafonné à `K` **après** le reçu des occurrences; les listes
de membres et postings ne peuvent jamais être tronquées à `K+1` ou `smax`.

## 2. Algorithme par lots exacts

Avant un lot de niveau `a`, les postings anciennes sont `P_x^-`. Pour les
nouveaux générateurs du lot `B`, construire séparément `B_x`.

1. Ancien--nouveau : pour chaque `M` de `B`, chaque `x` de `M` et chaque `N` de
   `P_x^-`, émettre la clef canonique `(min(M,N),max(M,N))`.
2. Nouveau--nouveau : pour chaque `x`, émettre toutes les paires non ordonnées
   de `B_x`.
3. Trier et réduire les clefs; la multiplicité réduite est `w(M,N)`.
4. Pour tout `k<=min(K,w)`, unir les deux générateurs dans `DSU_k`.
5. Valider le reçu entier du lot, puis seulement publier DSU, transcript et
   postings. Toute faute annule le lot complet.

Les identités de masse minimales sont :

$$R_{\mathrm{old,new}}=\sum_{M\in B}\sum_{x\in M}\lvert P_x^-\rvert.$$

$$R_{\mathrm{new,new}}=\sum_x\binom{\lvert B_x\rvert}{2}.$$

$$R_{\mathrm{old,new}}+R_{\mathrm{new,new}}=\sum_{(M,N)}w(M,N).$$

Sur toute la famille, le volume brut est :

$$P_{\mathrm{post}}=\sum_x\binom{d_x}{2}=\sum_{M<N}\lvert M\cap N\rvert,$$

où `d_x=|P_x|`. Ce nombre doit être calculé en entier large avant toute
allocation. Il peut être quadratique en `G`; l'algorithme est output-sensitive,
pas magiquement sous-quadratique.

## 3. Forme GPU reprenable

Une fois la famille connue, construire un CSR global
`PointId -> GeneratorId`, trié par identité canonique. Une tâche GPU est un
domaine disjoint `(point_id, triangular_pair_begin, triangular_pair_end)` dans
la posting d'un point. Elle produit un run de clefs de paires; aucun
échantillonnage ni cap de posting n'est permis.

Le pipeline exact est : compte des degrés, calcul de `P_post`, émission par
chunks, tri de chaque run, merge global, réduction des poids, tri par niveau
exact d'activation, puis lots DSU atomiques. Si les rationnels ne sont pas
comparés sur device, un tri CPU exact peut assigner un `level_id` canonique
avant le join; ce rang doit rester lié au niveau rationnel dans le reçu.

La somme des longueurs logiques de runs doit valoir exactement `P_post`. Une
posting lourde se découpe en intervalles de son triangle; elle n'est jamais
échantillonnée. Le spill disque peut rester exact, mais n'est alors plus un
candidat crédible au SLO sous une seconde.

## 4. Retirer aussi le scan global par lot

`keep_partitions=false` retire la matérialisation des partitions, mais le delta
live parcourt encore tous les actifs pour figer la coupe stricte, puis reconstruit
et trie toutes leurs racines à chaque lot. Ce mur `niveaux*G*log(G)` survivrait
au remplacement du join.

La classification peut rester locale aux composantes touchées :

- chaque racine DSU porte son identifiant public strict, sa couverture et un
  marqueur d'époque;
- au premier contact dans le lot, enregistrer une seule fois l'identifiant
  strict et la taille de couverture pré-lot;
- fusionner les petits conteneurs de couverture dans les grands;
- après toutes les unions, classifier seulement les racines touchées par le
  nombre d'identifiants stricts distincts et le delta de couverture;
- les racines non touchées restent inchangées et ne sont ni parcourues ni
  triées.

Cette métadonnée doit être staging-local jusqu'au commit du lot. Elle conserve
exactement la coupe stricte tout en rendant le coût proportionnel au travail du
lot.

## 5. Transcript Morse sans jeter les silencieux

Un générateur qui ne change ni partition ni couverture n'est pas une
continuation Morse. Il reste pourtant obligatoire dans les postings. À l'ordre
deux, les générateurs `A={1,2,3}`, `B={2,3,4}` sont déjà connectés;
`S={1,3,4}` est silencieux, mais le futur `N={1,4,5}` ne rejoint la composante
que par `S`.

Il faut donc deux couches :

- ledger d'activation de tous les générateurs, silencieux compris, avec niveau,
  identité, digest de membres et reçu d'overlap;
- delta public de composante, calculé après fermeture du lot à partir des
  racines strictes publiques, latentes et de `coverage_delta`.

Types orthogonaux recommandés : `birth`, `continuation`, `multifusion`, puis
`silent_generator_batch`, `coverage_growth`, `latent_assimilation` et
`structural_growth`. Un compteur nommé `*_batches` compte un niveau; s'il
s'incrémente par composante touchée, il doit s'appeler `*_component_events`.

Dans une sous-famille, tous ces types portent le suffixe
`relative_to_certified_subfamily`; un lot silencieux relativement à une source
partielle peut cesser de l'être lorsque la source devient complète.

## 6. Fixtures minimales qui tuent les raccourcis

### Ancien--nouveau et nouveau--nouveau

À `k=2`, activer d'abord `A={0,1,2}`, puis au même niveau
`B={1,2,3}` et `C={2,3,4}`. On a `|A inter B|=2`, `|B inter C|=2` et
`|A inter C|=1`. Oublier les paires nouveau--nouveau laisse `C` isolé;
décaler le seuil d'une unité change aussi le verdict.

### Silencieux indispensable

Utiliser `A`, `B`, `S`, `N` de la section précédente. Supprimer `S` parce que
son activation est silencieuse perd l'attache future de `N`.

### Multifusion

À `k=2`, activer deux composantes portées par `A={0,1}` et `B={4,5}`, puis
`S={0,1,4,5}`. Le lot de `S` doit produire une multifusion de deux racines
strictes, pas une continuation dépendante de l'ordre des unions.

Ajouter les mutants permanents : dernière posting omise, `w>k` au lieu de
`w>=k`, membership tronqué à `K+1`, générateur silencieux collecté, lot de
niveau égal committé séquentiellement et occurrence nouveau--nouveau oubliée.

## 7. Reçu minimal du benchmark CPU

Comparer le join postings et le fold `G^2` sur **les mêmes catalogues
rejoués**, afin que la source ne masque pas le join. Trois tailles clairsemées,
par exemple `G=512,1024,2048`, plus une famille dense hostile suffisent pour le
premier profil descriptif.

Publier par lot et par ordre : `G`, somme et maximum des tailles, niveaux,
degrés de postings, `P_post`, paires uniques, histogramme de `min(K,w)`, arêtes
acceptées, unions tentées/réussies/redondantes, racines touchées, octets et
high-water. `join_comparisons` doit distinguer appels de prédicat et
comparaisons effectives d'identifiants; `join_unions` ne doit pas être confondu
avec le nombre d'arêtes acceptées.

La porte scientifique exige l'égalité exacte des niveaux, coupes strictes et
fermées, couvertures, deltas du transcript et applications verticales, plus les
identités combinatoires ci-dessus. Les permutations des générateurs et de
l'intérieur de chaque lot doivent produire le même reçu canonique.

## 8. Admission d'une session G4

Avant toute VM, calculer le pic prédit en incluant les deux buffers de tri et
son workspace :

$$B_{\mathrm{peak}}=B_0+B_{\mathrm{occ,in}}+B_{\mathrm{occ,out}}+B_{\mathrm{sort}}+B_{\mathrm{pairs}}+B_{\mathrm{DSU}}+B_{\mathrm{output}}.$$

Critères proposés :

- NO-GO si `P_post`, un offset ou une longueur device déborde le contrat;
- NO-GO avant démarrage si le pic conservateur dépasse 70 % de la VRAM;
- arrêt du run si le pic réel atteint 80 %;
- GO diagnostic seulement si chaque chunk est borné, reprenable et si le statut
  est explicitement exact, `partial_refinement` ou synthétique;
- GO qualification seulement après différentiel natif, source complète,
  watermark fermé, join complet, transcript/verticales et replay.

À 50 k, le pipeline actuel reste `partial_refinement` puisque `kMaxRank=32`
rend `smax>=n` impossible. Un flux synthétique complet peut qualifier le kernel
du join, jamais la complétude du pipeline scientifique.

## 9. Prochaines actions ordonnées

1. Graver l'oracle tiers déjà positif sur 470 nuages et 34 003 générateurs.
2. Recevoir `Gamma == tour exhaustive == fold G^2` sur les fixtures nommées.
3. Écrire puis différencier le join postings CPU avec ses identités de masse.
4. Supprimer le scan global des actifs par lot et publier les high-water.
5. Mesurer CPU par catalogue rejoué, puis produire le manifeste 50 k.
6. Seulement alors écrire/qualifier le kernel CUDA et envisager une G4 SPOT
   gardée.

Cette route conserve l'invariant d'architecture : aucune mosaïque globale,
aucune expansion des sous-simplexes, et aucun statut exact sans preuve de
complétude.

GCP non utilisé pour cette note.
