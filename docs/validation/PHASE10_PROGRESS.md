# Progression Phase 10 — journal Morse et réduction directe

> [!IMPORTANT]
> Phase `10` formellement ouverte, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. Sa porte d'entrée est satisfaite par la fermeture de la Phase 9, qualification G4 comprise, et par le seul jalon local de Phase 5 `compact_k1_forest_certified/local_k1_compact_forest_only`; la Phase 5 globale reste `ready`.

## Incrément 10.1-RCPU livré

`ExactDirectMorseEventJournalResult` consomme la façade terminale directe de Phase 9 sans recopier centres exacts ni ensembles intérieurs. Il ajoute une projection singleton canonique par `PointId`, de support un, niveau carré nul, rang fermé un et naissance à l'ordre un. Pour chaque événement direct de rang fermé $s$, il projette sa naissance à l'ordre $s$ lorsqu'elle appartient à la fenêtre et sa selle à l'ordre $s-1$ lorsqu'elle appartient à la fenêtre.

Les rôles sont triés par ordre, niveau exact, index de projection puis type. Chaque clé `(ordre, niveau exact)` produit un lot contigu qui compte séparément naissances et selles. Aucun ordre artificiel d'exécution entre rôles de même lot n'est encore revendiqué : la résolution par hypergraphe quotient appartient au prochain incrément.

## Incrément 10.2-RCPU livré — graines de bras factorisées

Pour chaque rôle selle issu d'un événement direct $S=I\cup U$ de rang fermé $s$, `ExactDirectSaddleArmSeedJournalResult` émet une famille d'ordre $k=s-1$ et exactement une graine `(événement, u)` pour chaque $u\in U$. La facette $F_u=S\setminus\lbrace u\rbrace$ n'est pas conservée : elle est reconstruite à la demande, triée, dans un tableau fixe de dix `PointId`.

La preuve 10.2.1 établit que le support positif minimal $U$ rend chaque point essentiel. Comme les points de $I$ sont strictement intérieurs, supprimer $u$ force donc $\beta(F_u)<\beta(S)$. Cette conclusion est exacte relativement à la façade terminale de Phase 9 fournie comme autorité externe; aucun calcul miniball ni scan géométrique global par bras n'est répété dans le chemin produit. Le document [BRAS_SELLE_DIRECTS_PHASE10.md](../math/BRAS_SELLE_DIRECTS_PHASE10.md) donne la preuve et les préconditions.

Le budget est contrôlé avant le rejeu du journal source et avant toute allocation de sortie. Si $J$ désigne le nombre de selles et $A$ le nombre de graines, alors $J\leq E$ et $A\leq4J$, donc l'étage ajoute au plus $J+A\leq5E$ entrées. En coexistence avec 10.1, la borne devient :

$$\text{stockage}_{\mathrm{logique}}\leq3n+10E.$$

Le target `morsehgp3d_direct_saddle_arm_seed_journal` ne lie que le journal direct et ses autorités transitives. Le rejeu source contrôle les deux digests du nuage en deux passes séquentielles, puis valide projections, rôles et lots sur place, sans second journal ni tri. La reconstruction publique vérifie les autorités du nuage et un digest local d'événement. Un contrôle source et symboles interdit l'archive historique, miniball, `critical_arm`, Gamma, cellules, cofaces et mosaïque. Le tétraèdre régulier donne 11 familles et 28 graines; la fixture obtuse vérifie que $I$ appartient à chaque bras; les deux selles miroir simultanées conservent six provenances même lorsque deux graines reconstruisent la même facette. Les tests ciblés et statiques passent en moins de 0,2 seconde.

## Incrément 10.3-RCPU livré — forêt directe exacte d'ordre un

À l'ordre un, tout rôle selle a rang fermé deux. Il impose $I=\varnothing$, un support $U=\lbrace u,v\rbrace$ et deux bras singletons. La racine stricte de chaque bras est donc obtenue directement par le DSU courant, sans descente géométrique. Si la boule diamétrale fermée d'une paire contient un troisième site, les deux cordes passant par ce site sont strictement plus courtes; une descente finie montre que les paires directes engendrent exactement les mêmes composantes filtrées que le graphe complet et la forêt EMST. La preuve complète est consignée dans [FORET_DIRECTE_K1_PHASE10.md](../math/FORET_DIRECTE_K1_PHASE10.md).

`ExactDirectK1ForestJournalResult` maintient un DSU global, mais construit un second DSU transitoire sur les seules racines touchées par le niveau courant. Toutes les liaisons `ArmRootBinding` sont calculées sur le snapshot strict avant toute mutation. Chaque composante du quotient devient ensuite soit une multifusion unique pour $q\geq2$, soit une continuation explicite sans nœud pour $q=1$. Les feuilles `RootBirth` sont implicites, les enfants des multifusions sont aplatis et aucun `cut` complet n'est matérialisé par niveau.

Pour $J_1$ selles d'ordre un, le résultat stocke $2J_1$ liaisons, $J_1$ selles, au plus $J_1$ groupes, au plus $2n-2$ enfants et au plus $J_1$ lots. L'ajout vérifie donc :

$$\text{stockage}_{10.3}\leq2n+5J_1-2.$$

Le scratch logique préflighté comprend deux arènes de taille $n$ et au plus $11B_{\max}$ entrées pour le plus gros lot; il reste $O(n+B_{\max})$. Le vérificateur final rejoue la construction et compare chaque record au fil de l'eau sans seconde arène persistante de forêt. Le target `morsehgp3d_direct_k1_forest_journal` ne lie publiquement que 10.2. Les fixtures courtes couvrent la paire binaire, la multifusion ternaire de deux selles égales, la continuation $q=1$, un parent post-lot falsifié, un nœud de continuation inventé, un journal 10.2 falsifié et un budget diminué d'une unité. Le test fonctionnel et le contrôle statique passent ensemble en moins de 0,25 seconde.

## Borne de stockage propre à 10.1 et objets évités

Si $n$ est le nombre de sites et $E$ le nombre d'événements directs terminaux, le journal 10.1 conserve exactement $n+E$ projections, au plus $n+2E$ rôles et au plus $n+2E$ lots. Son audit certifie donc la borne logique suivante :

$$\text{stockage}_{\mathrm{logique}}\leq3n+5E.$$

Cette borne est proportionnelle aux sorties utiles. Le target ne lie pas l'archive historique de hiérarchie et ne construit aucune cellule top-$m$, coface, incidence Gamma, mosaïque d'ordre supérieur, forêt, table de racines ou `GatewayAttach`.

## Échec fermé et validation courte

Une façade non terminale, un nuage dont l'un des deux digests domain-separated diverge, un payload localement incohérent ou un diagnostic extra-shell pertinent retourne une décision explicite et trois vecteurs vides. Le vérificateur reconstruit projections, rôles et lots depuis le nuage et la façade externes; aucun champ du résultat observé ne pilote ce rejeu. Cette certification reste relative à la façade terminale fournie comme autorité externe : ce premier incrément ne rejoue pas lui-même la géométrie de Phase 9.

Le test ciblé strict passe en 0,02 seconde environ. Sur le tétraèdre régulier, les quatre singletons et onze événements directs donnent 15 projections, 26 rôles et sept lots. Les falsifications d'autorité, de rôle et de clé de lot sont rejetées; le carré cocirculaire exerce l'arrêt sur extra-shell.

## Limites et suite

La généalogie H0 est désormais fermée seulement à l'ordre un. Pour $k\geq2$, il reste à descendre ou localiser chaque facette dans l'état strict pré-lot, intégrer les suppressions intérieures qui naissent au niveau égal et conserver les gateways silencieux nécessaires aux événements futurs. Une absence dans le dictionnaire de facettes doit rester `unresolved`; elle ne certifie jamais une composante isolée. Le contre-exemple permanent à cinq points et la preuve M.1 restent les portes scientifiques. Aucun `public_status` n'est publié et Gamma exhaustif demeure l'oracle borné jusqu'à $n\leq14$.

GCP n'a pas été utilisé pour ces incréments CPU. La session G4 consignée dans la revue de porte appartient exclusivement à la qualification de fermeture de la Phase 9.
