# Durées de vie mémoire de la résolution temporelle — Phase 10.15-LIFE

## Statut et portée

Le jalon 10.15-LIFE est une optimisation interne du builder 10.13-TRES. Il conserve exactement `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, `partial_refinement` et `public_status=not_claimed`. Il ne modifie aucun schéma, type public, champ, compteur, budget, décision, portée, ordre canonique, prédicat de certification ou résultat scientifique.

Le changement porte uniquement sur l'instant où le scratch de tri est détruit. Les deux arènes scientifiques restent `projection_to_resolution` et `temporal_resolutions`; le sweep 10.10, ses autorités et toutes les prémisses résiduelles de 10.13 restent inchangés.

## Notation et tailles LP64

Soient $P$ le nombre de projections 10.9, $Q$ le nombre de couples distincts `(préfixe, token)`, $H$ le nombre de handles du locator et $C$ le nombre de candidats 10.7. Les seules bornes générales sont $Q\leq P\leq11C$; le nombre de points $n$ ne borne pas à lui seul $P$.

Les tailles suivantes ont été contrôlées sous ABI LP64 :

| objet | rôle | octets par entrée |
|---|---|---:|
| `PrefixTokenProjection` | triplet de tri `(prefix, token, projection)` | 24 |
| `ExactDirectSparseGatewayProjectionToTemporalResolution` | mapping scientifique final | 16 |
| `ExactDirectSparsePositiveFacetPrefixQuery` | requête scratch 10.10 avec clé fixe | 104 |
| `ExactDirectSparsePositiveFacetPrefixResolution` | résolution scratch du sweep | 48 |
| `ExactDirectSparseGatewayTemporalResolution` | résolution scientifique finale | 72 |
| `ExactDirectSparseComponentHandle` | parent DSU du sweep | 8 |

Ces nombres décrivent les payloads de vecteurs. Les objets de contrôle, le bookkeeping de l'allocateur et l'arrondi en pages restent des constantes ou des surcoûts d'allocation distincts.

## Ancien pic

Avant 10.15, le vecteur `sorted` de $24P$ octets restait vivant après le sweep et pendant la construction des $Q$ résolutions finales. Deux phases pouvaient déterminer le pic propre du builder :

$$M_{\mathrm{ancien}}=\max(40P+224Q,\ 40P+152Q+8H).$$

Le premier terme correspond à `sorted`, au mapping final, aux requêtes, aux réponses du sweep et aux résolutions finales simultanément vivants. Le second correspond au sweep pendant lequel les parents DSU s'ajoutent à `sorted`, au mapping, aux requêtes et aux réponses historiques. La sortie retournée ne vaut pourtant que :

$$M_{\mathrm{sortie}}=16P+72Q.$$

## Nouvelle ordonnance des allocations

Après le heapsort et le comptage des $Q$ groupes, une seule passe sur `sorted` construit désormais simultanément :

- les $Q$ requêtes monotones;
- le mapping dense des $P$ projections;
- les $Q$ records finaux avec indice dense, préfixe, token et multiplicité de projections.

Toutes les données nécessaires après le tri vivent alors dans ces arènes de taille $P$ ou $Q$. Un `swap` avec un vecteur vide détruit réellement l'allocation de `sorted` avant l'appel au sweep. `clear()` seul serait insuffisant, car il conserverait la capacité $P$.

Le sweep 10.10 reçoit exactement les mêmes requêtes. À son retour, la passe d'enrichissement complète en place chaque record déjà construit avec sa disposition historique, sa racine et son témoin éventuels. Elle ne réordonne ni les requêtes, ni les résolutions, ni le mapping.

Le nouveau pic propre est :

$$M_{\mathrm{nouveau}}=\max(40P+176Q,\ 16P+224Q+8H).$$

Le premier terme est atteint avant la libération du tri, lorsque les résolutions finales préconstruites coexistent encore avec `sorted`. Le second décrit le sweep après cette libération. Le stockage asymptotique reste $O(P+Q+H)$, mais les triplets de tri ne coexistent plus avec les allocations propres au sweep.

## Invariance scientifique

La transformation préserve le résultat record par record :

1. le même heapsort borné produit le même ordre `(préfixe, token, projection)`;
2. les mêmes ruptures de couples créent les mêmes indices de résolution;
3. chaque projection reçoit le même indice et chaque multiplicité est le cardinal du même groupe contigu;
4. les requêtes 10.10 sont identiques et le sweep est appelé une seule fois avec les mêmes autorités et budgets;
5. l'enrichissement lit la réponse historique de même indice et applique les mêmes contradictions append-only;
6. les deux vecteurs publiés, leurs compteurs, besoins, décisions et faits de portée restent identiques.

Les arènes préconstruites restent locales jusqu'à la publication atomique finale. Une exception, un budget épuisé ou une contradiction les détruit sans payload scientifique partiel. La libération du scratch ne change donc ni la sémantique d'échec, ni les garanties de gel du locator.

## Ordres de grandeur

Dans le scénario illustratif $P=Q=H=10\,000\,000$, l'ancien pic propre était de 2,64 Go et le nouveau de 2,48 Go, soit 160 Mo économisés hors surcoûts d'allocateur. À $P=Q=H=50\,000$, les valeurs correspondantes sont 13,2 Mo et 12,4 Mo.

Ces valeurs ne doivent pas être présentées comme des mesures par nombre de points. L'hypothèse $P=n$ n'est pas garantie. Même sous l'illustration $C=n$ et $P=11C$, la mémoire reste de l'ordre de plusieurs dizaines de gigaoctets à dix millions de points. La voie 10 M+ doit donc budgéter directement $P$, $Q$ et $H$, puis poursuivre les réductions de scratch et le streaming.

## Limite du fresh verifier

Le fresh verifier conserve encore le résultat observé, le résultat reconstruit et `BuildArtifacts`, qui retient les requêtes et le résultat du sweep avant un second rejeu 10.10. Ce pic n'est pas fermé par 10.15-LIFE. Le prochain chantier mémoire doit rendre cette vérification streaming ou comparer les résolutions au fil du rejeu, sans seconde arène complète, tout en gardant le même trust boundary.

## Structures toujours évitées

10.15 ne réintroduit aucune structure globale. Le chemin produit ne construit toujours ni Gamma exhaustif, ni catalogue global de facettes ou de cofaces, ni snapshot ou DSU par lot, ni cellule top-$m$, ni clé persistante de coface à onze points, ni mosaïque de Delaunay d'ordre supérieur, ni quotient, forêt ou `GatewayAttach`. Le coût intermédiaire reste proportionnel aux projections et résolutions effectivement produites.

## Validation courte

La fixture fonctionnelle existante de 10.13 et 10.14 suffit : elle repasse le builder, le fresh verifier, les cas positifs et latents, le pont latent et la mutation hostile déjà présente. Elle doit constater le même payload et les mêmes compteurs après la nouvelle ordonnance. Aucun nouveau benchmark long, produit cartésien de budgets, sanitizer, GPU ou campagne GCP n'est requis pour ce jalon de durée de vie.
