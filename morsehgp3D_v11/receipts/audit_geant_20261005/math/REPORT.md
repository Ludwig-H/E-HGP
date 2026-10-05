# Audit mathématique indépendant de la v11, 5 octobre 2026

Base figée : `238734f1d03ab32e2a722bf036eb8fc5626dfd44`.
Cadre : `phase=exploration_v11_hors_registre / backend=cpu_reference / profile=quantized_u21_input_only / public_status=not_claimed`.
Worktree propre et détaché : `build/v11-audit-geant-math-20261005`, sans branche. Audit en lecture seule ; aucune source produit modifiée, aucun build/test natif, aucun GCP, aucun fit HDBSCAN et aucune donnée KITTI.

**Verdict.** Aucun nouveau défaut mathématique du produit FULL établi. Le cœur T2/T4/T5 et les lemmes P/W/C/F/G/H sont cohérents à la lecture ; la nouvelle contre-épreuve ci-dessous élargit les tests bornés par une route d'adjacence différente et par les ordres terminaux. Elle ne démontre ni complétude générale du moteur, ni qualification u24, ni temps cible.

## Nouvelle exécution bornée

`check_math.py` utilise le MEB exact de l'étage A, mais écrit son propre graphe : pour chaque paire de K-parties F et F', l'arête est présente si `beta(F union F') <= a` (ou `< a`), sans limiter l'union à K+1 sites. C'est le graphe complet des intersections des régions témoins convexes. Une DSU propre compare ensuite les **ensembles complets de K-parties par composante**, pas seulement les populations de sites, avec les propriétaires du graphe Johnson de `Definition`. Cette route vérifie ainsi un étage de T1 que le README de référence disait auparavant seulement invoqué (`reference/README.md`, « Ce que la référence n'établit pas »). Elle partage les MEB avec A ; elle ne constitue pas un oracle géométrique indépendant des formules de Gram de A.

Chaque coupe ouverte est comparée à un niveau strictement intermédiaire entre deux MEB successifs de toutes les parties non vides : aucune égalité de seuil n'est arrondie. Toutes les K-1 faces de chaque K-partie active sont ensuite retrouvées dans l'ordre inférieur ; elles doivent avoir la même composante, égale à la remontée de la verticale publiée. Cela exerce les verticales à toute la coupe, au-delà du seul témoin de naissance.

Dix nouveaux nuages de 6 à 8 sites : cube privé de deux sommets opposés, cube avec un site de frontière, octaèdre déformé, coquille axiale avec deux intérieurs, deux triangles à plateaux et leur pont, quasi-plan/quasi-ligne et coins aux extrêmes u21 ; deux de ces nuages sont relevés aux extrêmes u24. Coordonnées exactes dans `normal.json`. Tous les ordres K=1..n sont jugés, dont dix ordres terminaux K=n.

Résultats :

| Mesure | Total |
| --- | ---: |
| Nuages | 10 |
| Ordres | 65 |
| Requêtes d'unions arbitraires F union F' | 17 276 |
| Coupes strictes/fermées | 1 453 |
| K-parties attribuées | 6 912 |
| Contrôles de faces verticales | 15 925 |
| Boules W_K rejugées par S1 | 563 |
| Supports rejugés | 635 |
| Dates exactes points, u21 | 304 |
| Propriétaires exacts points, u21 | 304 |

Sur les huit nuages u21, l'étage B égale A sur **tous** les ordres. Pour K<n, les pendaisons `hang_margin_radius` sont confrontées à la routine propre de `reference_radius_rules`, date par somme de radicaux et propriétaire exact ; `m(1)=1` est appliqué. Les deux nuages u24 n'exercent **que** A/S1 : `intgeom.MORTON_BITS=21`, donc aucun transfert à B ni au produit u24.

Exécution normale et `python3 -O`, avec `-B` pour ne pas modifier le worktree : sorties JSON identiques octet pour octet. Aucune instruction `assert` dans ce script. Les SHA-256 de la source de preuve, des sorties et des sources utilisées sont dans `SHA256.json`.

Commandes :

```sh
MHGP11_AUDIT_TREE=/chemin/du/worktree/morsehgp3D_v11 python3 -B check_math.py > normal.json
MHGP11_AUDIT_TREE=/chemin/du/worktree/morsehgp3D_v11 python3 -O -B check_math.py > optimized.json
cmp normal.json optimized.json
```

Deux essais initiaux du harnais ont échoué par une faute du harnais : le rôle naissance de S1 était comparé à 0, puis à 1, alors que S1 le publie comme chaîne `naissance`. Les commandes, codes 1 et messages exacts capturés dans la conversation sont conservés dans `attempts.json`. Les deux versions initiales du script ont été modifiées en place et n'ont pas été sauvegardées : leur contenu complet et leur empreinte sont absents, sans reconstruction. Correction du prédicat explicite, puis exécutions finales conformes. Aucun échec produit n'est déduit de ces tentatives. Un ajout ultérieur du contrôle des verticales a entraîné son propre rejeu normal/-O, lui aussi conforme.

## Relecture produit et contrats

- `src/tower/cells.cpp:76` : classifier la MEB de la **trace de coquille** suffit pour le test T2 ; la trace complète I union A est construite ensuite (`:103`). La MEB de A ne remplace pas celle de I union A dans la descente. Les faces analytiques sont limitées à m=q (`:115`), et t=m est une naissance (`:116`). Les autres traces sont exhaustives avec deux passes (`:130`). Aucun raccourci local par intersection des supports n'est utilisé.
- `src/tower/descent.cpp:195` : la diminution exacte des niveaux protège la descente ; les choix hors Cat_K restent traités par census et MEB. `descent_memo.cpp:78` contrôle la **date initiale** du suffixe, et non seulement son terminal. `forest_plateau.cpp:53` vérifie également `initial_level < lambda` avant d'utiliser une graine préplateau.
- `src/tower/forest_plateau.cpp:69` : publication à la fin du plateau, enfants strictement plus bas (`:85`) et rangs N-aires. La racine canonique choisie par les unions est la plus petite naissance ; aucun arbre binaire intermédiaire n'est publié.
- `src/tower/forest_ancestor_sweep.hpp:44` : toutes les fusions égales sont activées avant une requête fermée. La requête n'admet une graine qu'à un rang où elle est née (`:62`). Cela conserve la frontière ouverte/fermée exigée pour l'attache S3.
- `docs/MATHEMATIQUES.md:486` : P traite les sommets neufs privés et les boules hors fenêtre ; W (`:530`) évite le raccourci faux « enlever les liaisons hors catalogue ». Les nouvelles épreuves jugent bien la définition complète, et pas `window_tree` seul.
- `docs/MATHEMATIQUES.md:653` et `reference/hgp11_ref/supports.py:424` : conserver D2, qui distingue l'ensemble des nœuds de coupe précédente et la population effective d'une composante. Aucune nouvelle garde `beta(F) <= niveau precedent` n'est justifiée.
- Les domaines des poids restent distincts : A/B admettent les copies ; S1 exige les positions distinctes (`reference/hgp11_ref/supports.py:194`) ; le produit FULL doit continuer à refuser les multiplicités au niveau annoncé. Les tests sur A/B ne qualifient pas une tour native pondérée.

S3/S6 ne sont pas commités au pin : pas de `src/supports/`, pas de `WindowAttachment` ni de `build_order` dans l'arbre figé. Les rapports de brouillons des audits antérieurs restent des revues de WIP, pas des livraisons ni des qualifications natives. Cet audit n'a pas importé ces brouillons dans sa preuve.

## Frontière précise à fixer avant L3

Le cas K=n est licite pour FULL et supports : les dix ordres terminaux de la nouvelle suite donnent l'unique naissance B(X), comme attendu. Il faut conserver cette admission pour ces objets.

Pour points/plat, `docs/SORTIES.md:55` indique seulement K<=n, tandis que `docs/HIERARCHIE_POINTS.md:346` fixe m(K)=K+1 à K>=2. À K=n>=2, aucune composante ne peut atteindre m. La trace minimale X={(0,0,0),(2,0,0)}, K=2, m=3, produit actuellement `ExportError: jamais_qualifie` dans `qualified_starts` (`bench/points_hierarchy.py:300`), via `hang_margin_radius` (`points_radius.py:212`). La trace est conservée dans les deux JSON.

**C'est une frontière du futur contrat L3, pas un défaut du moteur FULL, de L0, ni d'une sortie points déjà livrée.** Avant S9, choisir explicitement : refus de requête points/plat pour K=n>=2, avec priorité/raison/porte contractuelle ; ou représentation native des points inactifs et hiérarchie sans entrée. Ne pas silencieusement abaisser m à n : cela change le modèle. La même porte doit couvrir n=1,K=1, où la convention m(1)=1 admet bien le seul site, et n=2,K=1, où les sites entrent à 0.

Le défaut k=1 des bancs plats m=2 est déjà déclaré dans `HIERARCHIE_POINTS.md:350` ; la preuve nouvelle emploie la convention future m(1)=1 et ne qualifie pas ces anciens bancs.

## Limites et suite utile

Aucun coût global ni sous-quadratique, aucune stabilité du carrier, aucun label plat stable et aucune propriété de l'échelle LiDAR ne suivent de ce résultat. Les coupes FULL sont exactes ; leur transfert à points reste limité aux pendaisons décidées, puis la condensation/sélection est un contrat séparé. Les K élevés K10/K12 et les coquilles 24 restent les portes dédiées décrites par les audits antérieurs ; cette suite nouvelle ne remplace ni ces témoins ni leur future exécution native G4.

Pour le développeur : conserver ces dix fixtures comme candidats ciblés, surtout les K=n pour la fermeture du domaine, les coins/quasi-plans pour les représentations numériques et les deux nuages à coquilles avec intérieurs pour les traces strictes. En S3/S6, comparer le vrai FULL et l'attache fermée de plateau ; en S9, graver le traitement explicite K=n et K=1 avant de promouvoir une sortie points.
