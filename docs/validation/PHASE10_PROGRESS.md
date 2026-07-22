# Progression Phase 10 — journal Morse et réduction directe

> [!IMPORTANT]
> Incrément préparatoire de Phase `10`, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique publique interne `partial_refinement`. Son ouverture formelle attend encore la qualification G4 de Phase 9; le second prérequis est déjà satisfait par le seul jalon local de Phase 5 `compact_k1_forest_certified/local_k1_compact_forest_only`, tandis que la Phase 5 globale reste `ready`.

## Incrément 10.1-RCPU livré

`ExactDirectMorseEventJournalResult` consomme la façade terminale directe de Phase 9 sans recopier centres exacts ni ensembles intérieurs. Il ajoute une projection singleton canonique par `PointId`, de support un, niveau carré nul, rang fermé un et naissance à l'ordre un. Pour chaque événement direct de rang fermé $s$, il projette sa naissance à l'ordre $s$ lorsqu'elle appartient à la fenêtre et sa selle à l'ordre $s-1$ lorsqu'elle appartient à la fenêtre.

Les rôles sont triés par ordre, niveau exact, index de projection puis type. Chaque clé `(ordre, niveau exact)` produit un lot contigu qui compte séparément naissances et selles. Aucun ordre artificiel d'exécution entre rôles de même lot n'est encore revendiqué : la résolution par hypergraphe quotient appartient au prochain incrément.

## Borne de stockage et objets évités

Si $n$ est le nombre de sites et $E$ le nombre d'événements directs terminaux, le journal conserve exactement $n+E$ projections, au plus $n+2E$ rôles et au plus $n+2E$ lots. Son audit certifie donc la borne logique suivante :

$$\text{stockage}_{\mathrm{logique}}\leq3n+5E.$$

Cette borne est proportionnelle aux sorties utiles. Le target ne lie pas l'archive historique de hiérarchie et ne construit aucune cellule top-$m$, coface, incidence Gamma, mosaïque d'ordre supérieur, forêt, table de racines ou `GatewayAttach`.

## Échec fermé et validation courte

Une façade non terminale, un nuage dont l'un des deux digests domain-separated diverge, un payload localement incohérent ou un diagnostic extra-shell pertinent retourne une décision explicite et trois vecteurs vides. Le vérificateur reconstruit projections, rôles et lots depuis le nuage et la façade externes; aucun champ du résultat observé ne pilote ce rejeu.

Le test ciblé strict passe en 0,02 seconde environ. Sur le tétraèdre régulier, les quatre singletons et onze événements directs donnent 15 projections, 26 rôles et sept lots. Les falsifications d'autorité, de rôle et de clé de lot sont rejetées; le carré cocirculaire exerce l'arrêt sur extra-shell.

## Limites et suite

Ce journal ne constitue pas encore une généalogie H0. Il reste à construire les bras de selle, résoudre les multifusions de même niveau, attacher les racines du snapshot strict pré-lot et conserver les gateways silencieux nécessaires aux événements futurs. Le contre-exemple permanent à cinq points et la preuve M.1 restent les portes scientifiques. Aucun `public_status` n'est publié et Gamma exhaustif demeure l'oracle borné jusqu'à $n\leq14$.

GCP n'a pas été utilisé pour cet incrément CPU.
