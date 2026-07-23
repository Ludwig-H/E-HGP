# Résolution temporelle sparse des gateways — Phase 10.13-TRES

## Statut et portée

Le jalon 10.13-TRES conserve `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, `partial_refinement` et `public_status=not_claimed`. Sa porte d'entrée locale est satisfaite par les sorties 10.7, 10.9, 10.10 et le scellement 10.12-AUTH. Le builder compose leurs formes déjà certifiées; seul le fresh verifier rejoue 10.9, AUTH/CLOCK et le sweep historique avant de certifier le résultat observé.

La sortie répond à une question précise : pour chaque facette de suppression proposée par 10.9, quel était son état dans le locator au préfixe strictement pré-lot engagé pour son lot source ? Elle ne décide ni le quotient, ni la forêt, ni un `GatewayAttach`.

## Clé temporelle exacte

Soit une projection de suppression $j$, son lot source $s_j$, son token localisé $t_j$ et le préfixe pré-lot $p_{s_j}$ certifié par AUTH/CLOCK. La clé de résolution est exactement :

$$\kappa_j=(p_{s_j},t_j).$$

Deux projections partagent une résolution si et seulement si leurs deux coordonnées coïncident. Dédupliquer par le seul token serait faux : une même facette peut être latente au préfixe $p$, puis positive au préfixe $p'>p$. Un commit vide suffit aussi à distinguer les deux états temporels, même si aucune clé ni union n'est ajoutée.

Si $P$ désigne le nombre de projections et $Q$ le nombre de couples distincts, alors :

$$Q\leq P\leq 11C,$$

où $C$ est le nombre de candidats factorisés 10.7. Le facteur onze vient uniquement des suppressions transitoires d'une coface logique de cardinal au plus onze; aucune clé persistante de onze points n'est créée.

## Construction

Le builder effectue les opérations suivantes :

1. Il contrôle les formes de 10.7, 10.9, du locator final et du certificat AUTH scellé, puis borne les populations et les octets avant chaque allocation correspondante.
2. Il construit un scratch de triplets `(préfixe, token, projection)` et applique un heapsort déterministe borné selon cet ordre lexicographique.
3. Il déduplique seulement les deux premières coordonnées, émet une requête 10.10 par couple distinct et conserve la clé de facette seulement dans ce scratch transitoire.
4. Il appelle exactement un sweep monotone 10.10, puis publie atomiquement deux arènes plates : une référence dense par projection et une résolution par couple distinct.

Une résolution publiée contient l'indice du token 10.9, le préfixe, le nombre de projections qui la référencent et, pour un hit historique, la racine DSU historique et le témoin original. Elle ne copie aucun `PointId` ni aucune clé de facette et ne revendique aucune durabilité après crash.

## Invariants de monotonie

Le locator est append-only. Une liaison déjà visible n'est jamais supprimée et son témoin original reste stable; seules les unions peuvent changer sa racine. Il en résulte :

- un hit historique implique un hit final avec le même témoin, mais pas nécessairement la même racine;
- un token latent dans le snapshot final était latent à tout préfixe antérieur;
- un token positif final peut avoir été latent au préfixe pré-lot;
- si le préfixe demandé est le préfixe final, disposition, racine et témoin coïncident avec le token 10.9 final.

Ces propriétés sont contrôlées avant publication. Une contradiction renvoie un échec atomique et les deux arènes scientifiques restent vides. Une latence demeure `latent_unresolved`; elle ne signifie jamais composante isolée et ne crée aucun singleton.

Le prédicat auto-contenu vérifie en temps linéaire les indices denses, l'ordre strict des couples, les dispositions et payloads, les compteurs, la plage de chaque référence et la somme des nombres de références. Le fresh verifier reconstruit ensuite le résultat attendu et exige son égalité récursive avec le résultat observé.

## Coûts et structures évitées

Le travail propre, hors les vérificateurs amont et l'unique sweep 10.10, est :

$$O(S+P\log P+KP+KQ),\qquad K\leq10.$$

Le stockage scientifique publié est $O(P+Q)$ et le scratch propre est $O(P+Q)$, auquel s'ajoute le scratch DSU unique de 10.10. Le heapsort borné favorise ici une preuve simple et déterministe; la voie 10 M+ pourra le remplacer par un radix sort déterministe sur les deux indices sans modifier le schéma des deux arènes.

Le jalon ne construit ni snapshot par lot, ni DSU par préfixe, ni catalogue global de facettes ou cofaces, ni Gamma, ni cellule, ni mosaïque de Delaunay d'ordre supérieur. Il ne matérialise pas davantage quotient, racines finales de quotient, forêt ou attaches. Le coût reste proportionnel aux propositions directes effectivement produites.

## Autorités résiduelles

Le fresh verifier rejoue l'autorité temporelle en mémoire et peut donc publier `external_clock_authority_replayed=true`. Il conserve cependant trois prémisses explicites :

- la discipline scientifique stricte pré-lot de l'orchestrateur;
- le gel synchronisé du locator pendant le calcul et le rejeu;
- l'autorité externe qui donne leur sens scientifique aux témoins de liaison du locator.

Ainsi `external_binding_authority_replayed=false`, `crash_durable=false` en amont et aucun statut exact public ne découle de 10.13. La complétude des gateways silencieux hors du flux direct, l'intégration atomique au quotient, M.1, le SLO 50 k sous la seconde et la voie 10 M+ restent ouverts.

## Validation courte

Une seule fixture de quatre points capture tous les lots source au préfixe zéro, insère ensuite une facette sélectionnée, scelle AUTH, puis reconstruit 10.9 et 10.13. Elle vérifie que la facette positive dans le snapshot final demeure latente dans chaque état strictement pré-lot, que $Q$ est exactement le nombre de couples `(préfixe, token)` distincts, que seules deux arènes sont publiées et que le fresh verifier compose bien localisation, AUTH et sweep. Aucun benchmark long n'est requis pour cette preuve fonctionnelle.
