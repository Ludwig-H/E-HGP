# Fenêtre de rang et composition horizontale : contre-lecture courante

4 septembre 2026, conclusions actualisées le 5 septembre. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Conclusion bornée.** Le raccord de contact du cœur du § 3 de la [composition horizontale](../docs/PREUVE_HORIZONTALE_COMPOSITION.md) est justifié sous les prémisses S et les contrôles locaux annoncés. Une régularité géométrique globale n'est pas nécessaire. La fixture traverse effectivement la frontière de rang onze : elle vérifie le catalogue par supports indépendants jusqu'à 24 points, puis le vrai pipeline et ses deltas contre Gamma à K=2 et K=10 sur quatre exécutions à 13 points. Aucun contre-exemple n'a été trouvé. La [preuve S1 complémentaire](S1_COURANT.md) ferme conditionnellement le parcours du générateur. Les preuves des [lanes et Cramer](ARITHMETIQUE_LANES_COURANTE.md), des [produits larges et réductions](ARITHMETIQUE_LARGE_COURANTE.md) et des autres primitives sont désormais raccordées aux exécutions qualifiées par le [certificat horizontal CPU E](CERTIFICAT_HORIZONTAL_COURANT.md). Celui-ci ferme la composition réduite dans son domaine explicite ; il conserve l'attribution historique propre des fixtures ci-dessous.

## 1. Preuve du raccord de contact du cœur

Écrivons $q(B)$ pour le cardinal **minimal** d'un support positif de B, $p(B)$ pour son nombre d'intérieurs stricts, et $e(B)$ pour son nombre total de points de shell. La fenêtre utilise $p+q$, pas $p+e$.

Les facettes du cœur obtenues en retirant un sommet essentiel d'une coface directe sont visitées par `Builder::run`. Le contrôle local de la miniball puis la requête globale `intruders` excluent toute extra-shell. La collecte s'arrête à deux intrus, mais la requête de bord continue; seuls les sous-arbres strictement intérieurs sont sautés. Retirer un point intérieur d'une coface directe conserve sa miniball régulière, déjà qualifiée sous S. Ainsi, après succès, chaque facette du cœur possède une miniball globalement régulière. Sources : [silent_incidence.hpp, ligne 195](../src/forest/silent_incidence.hpp#L195) et [ligne 283](../src/forest/silent_incidence.hpp#L283).

Soit un bloc irrégulier hors fenêtre B, de niveau a, et une facette du cœur F contenue dans son saturé $S_B=X\cap B$. Si $\beta(F)=a$, l'unicité de la miniball donnerait $B_F=B$, contradiction. Sinon, F est un sommet du graphe strict du théorème d'inertie 4.2. Le passage à une incidence antérieure repose sur la borne suivante :

$$\lvert S_B\rvert\geq p(B)+q(B)\geq r_{\max}+1\geq K+2.$$

Le graphe strict couvre tous ces points. Une seule facette de cardinal K ne suffit donc pas à sa couverture : il possède au moins deux sommets. Sa connexité donne un voisin G à **chaque** F stricte. Par définition de son arête, $Q=F\cup G$ contient K+1 points et vérifie $\beta(Q)<a$. Donc $\lambda(F)<a$. Le bloc omis ne cache aucune première incidence du cœur au niveau a. Ce raisonnement utilise le [théorème transverse 4.2](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md), sans convertir l'inertie en régularité.

Le raccord avec les maillons retenus hors cœur est également explicite. Chaque maillon ajouté a terminé sa requête globale de shell avant publication. Un contact égal avec un bloc irrégulier lui imposerait la même miniball, ce qui est impossible. Un contact strict passe par l'apex antérieur, auquel son suffixe décroissant le relie effectivement dans le sous-flot. Pour une coface directe, la facette de contact appartient au cœur et l'invariant d'activation s'applique. L'[ancrage par suffixes et la bijection par inclusion des facettes](REPONSE_AUDITEUR_COMPOSITION.md) évitent de supposer un resolver top-K ou d'identifier des composantes par leurs seuls points, qui peuvent se recouvrir.

## 2. Le catalogue direct se juge sans énumérer toutes les cofaces

Soit Q une coface Gabriel au sens faible, sans intrus strict étranger. Elle contient tous les intérieurs de sa miniball B, ainsi qu'un support positif $U_Q\subseteq Q$. Sans régularité, le support global de cardinal minimal n'appartient pas nécessairement à Q, mais son cardinal est au plus celui de $U_Q$. Ainsi :

$$p(B)+q(B)\leq p(B)+\lvert U_Q\rvert\leq\lvert Q\rvert.$$

Toute coface directe demandée appartient donc à une boule de la fenêtre $p+q\leq r_{\max}$. Sous S et après refus de toutes les extra-shells pertinentes, $E(B)=U(B)$ et nécessairement $Q=I(B)\cup U(B)$. Réciproquement, cet ensemble possède B comme miniball et n'omet aucun intrus strict. L'expansion régulière par $K=p+q-1$ est donc une bijection, sous ces prémisses. Une égalité de catalogue n'est pas réclamée sur un plateau accepté sous une autre politique.

La [fixture](math_window_repro_20260904.cpp) énumère seulement les supports positifs de tailles 2, 3 et 4, résout leurs centres par Gram/Cramer OBig640, puis calcule leurs intérieurs et shells par puissances exactes. Deux supports donnant le même shell définissent sa même miniball unique; elle conserve leur cardinal minimal. Elle compare ce catalogue indépendant aux boules effectivement générées, préfiltrées et censées, puis compare l'expansion de chaque ordre K=1 à 10. Pour les cas à 13 points, elle contrôle également que les callbacks du vrai pipeline reçoivent exactement ce catalogue direct à chaque ordre. Le juge n'utilise aucun prédicat géométrique produit pour construire ses décisions de référence.

Cette procédure d'audit coûte une énumération de supports d'ordre quatre; elle n'est pas proposée comme architecture produit. Elle qualifie S sur chaque entrée examinée, sans prouver le quantificateur universel de S1.

## 3. Frontière onze/douze, refus et mutant ciblé

Les deux nuages ciblés ont les points suivants, complétés par neuf ou dix points strictement intérieurs déterministes :

$$A=(0,1000,1000),\quad B=(2000,1000,1000),\quad Y=(1000,2000,1000),\quad c=(1000,1000,1000),\quad\beta(AB)=1000000.$$

Le support minimal est AB, le shell est ABY. Les intérieurs proviennent du générateur entier explicite de la fixture, graine 78234729, dans le cube de coordonnées 800 à 1200. Leurs trois coordonnées sont conservées dans la recette déterministe, sans tirage dépendant d'une bibliothèque.

| Cas | Résultat exact observé |
|---|---|
| Neuf intérieurs, n=12, p+q=11 | 138 boules pertinentes; l'extra-shell est reçue, puis `unsupported_degeneracy`, zéro callback |
| Dix intérieurs, n=13, p+q=12 | 179 boules pertinentes et trois omises; une extra-shell uniquement hors fenêtre; pipeline complet, dix callbacks |
| Même cas hors fenêtre, K=2 et K=10 | 728 coupes Gamma, 302 coupes de deltas, une coface silencieuse et 88 contacts de cœur stricts vérifiés; aucun échec |
| Mutant officiel `depth-threshold-minus-one` | Sur n=13, graine 132741, exactement six boules de rang onze disparaissent : cinq q3 et une q4; toutes les autres demeurent conformes; code 4 |
| Mutation d'audit `--mutant=shell-rank` | Supprime exactement la boule AB/Y à neuf intérieurs en remplaçant le critère p+q par p+e; le refus pertinent disparaît et dix callbacks sont publiés; cette perte ciblée du refus est détectée, code 4 |

La dernière mutation intervient uniquement à la couture déclarée `prefilter_census_override`, depuis la fixture d'audit. Elle ne modifie pas le produit et n'ajoute aucun nom à son registre de mutants. Elle démontre le besoin de conserver le **refus du plateau pertinent**; elle ne prétend pas avoir trouvé une divergence H0 sur ce nuage corrompu.

Les cas aléatoires complètent ces dents : n=13, graines 132741 et 712391, vérifient chacun le vrai pipeline et ses deltas à K=2 et K=10. La graine 132741 est rejouée avec ordre physique inversé, PointId conservés, deux fils demandés et stockage CSR. Ce rejeu ne certifie pas à lui seul qu'une population aussi petite a créé deux travailleurs effectifs.

À n=16, graine 391749, 464 boules pertinentes sont retrouvées et 51 sont omises. À n=24, graine 777931, les 1079 pertinentes et 522 omises concordent avec l'oracle; les boules exactement au rang onze sont respectivement 6 de support q2, 59 de support q3 et 46 de support q4. Sur ces deux tailles, le contrôle porte sur le catalogue et son expansion, pas sur Gamma exhaustif ou les deltas.

## 4. Un raccord autonome supplémentaire pour S1 : existence d'un seed q4 aigu

Considérons un tétraèdre de support strictement positif, et AB une de ses arêtes maximales. Au moins un des deux autres sommets X est strictement extérieur à la boule diamétrale AB. Sinon cette boule contiendrait le tétraèdre. Sa circumboule, qui est sa miniball par positivité stricte, aurait alors un rayon au plus égal à AB/2. Comme elle contient A et B, son rayon serait exactement AB/2 et son centre serait le milieu de AB, incompatible avec un centre strictement intérieur au tétraèdre.

Pour ce X, le triangle ABX est aigu en X par l'extériorité stricte à la boule diamétrale. Il est aussi strictement aigu en A et B, car AB est maximale :

$$2\langle B-A,X-A\rangle=\lVert A-B\rVert^2+\lVert A-X\rVert^2-\lVert B-X\rVert^2\geq\lVert A-X\rVert^2>0.$$

L'inégalité symétrique vaut en B. Toute arête maximale possède donc au moins un seed incident strictement aigu, y compris en présence d'égalités de longueurs. Un choix canonique d'arête maximale puis du plus petit PointId parmi ses seeds aigus conserve un représentant mathématique unique, **si ces seeds sont effectivement tous accessibles**.

Cette preuve établit fraîchement l'existence annoncée dans [q4.hpp, ligne 13](../src/lanes/q4.hpp#L13), sans hériter d'un reçu v4. Elle ne démontre ni la présence du seed dans le cover, ni sa survie aux filtres, ni le parcours effectif de son représentant. Ces clauses sont traitées dans la [preuve S1 complémentaire](S1_COURANT.md).

## 5. Sources, reproduction et portée

La compilation utilise GCC 13.3, C++20, `-O3 -Wall -Wextra -Wpedantic -Werror`, sans suppression d'avertissement. Les neuf invocations maintenues ont les codes attendus : sept codes 0 et deux codes 4, aucun débordement OBig observé. Le [reçu brut](receipts_20260904/math_window_repro.json) contient sorties, codes, hashes de fixture et de binaire. Le [snapshot de 101 fichiers](receipts_20260904/math_window_source_snapshot.json) sépare les octets exécutés des [textes de composition relus](receipts_20260904/math_window_review_sources.json). L'oracle réutilise les fonctions OBig du juge existant; ce n'est pas une troisième arithmétique indépendante.

```bash
mkdir -p morsehgp3D_v7/audits/.work_math2/tmp
TMPDIR="$PWD/morsehgp3D_v7/audits/.work_math2/tmp" g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -DMHGP7_TESTING=1 '-DMHGP7_AUDIT_GATE_SOURCE=".work_math2/source/morsehgp3D_v7/tests/silent_incidence_gate.cpp"' -pthread morsehgp3D_v7/audits/math_window_repro_20260904.cpp -o morsehgp3D_v7/audits/.work_math2/math_window
morsehgp3D_v7/audits/.work_math2/math_window --boundary-inside
morsehgp3D_v7/audits/.work_math2/math_window --boundary-outside
morsehgp3D_v7/audits/.work_math2/math_window --mutant=depth-threshold-minus-one
morsehgp3D_v7/audits/.work_math2/math_window --mutant=shell-rank
morsehgp3D_v7/audits/.work_math2/math_window --random 13 132741
morsehgp3D_v7/audits/.work_math2/math_window --random 13 132741 reverse
morsehgp3D_v7/audits/.work_math2/math_window --random 13 712391
morsehgp3D_v7/audits/.work_math2/math_window --random 16 391749
morsehgp3D_v7/audits/.work_math2/math_window --random 24 777931
```

Les deux commandes `--mutant` doivent rendre 4. Sans le snapshot temporaire, compiler sans `MHGP7_AUDIT_GATE_SOURCE` utilise les sources courantes; leurs hashes doivent être contrôlés avant réattribution du résultat.

La [preuve des lanes](ARITHMETIQUE_LANES_COURANTE.md) confirme chaque intermédiaire q2/q3/q4, les signes de Cramer et la conversion du centre q3 pour une vraie forme non colinéaire. La [preuve des produits larges et réductions](ARITHMETIQUE_LARGE_COURANTE.md) ferme les colonnes, les capacités U192/U320, le PGCD, les casts et les divisions dans leurs domaines nommés. Les petites portes C++ causales sont intégrées et leurs [reçus](AUDIT_QUALIFICATION_20260905.md) sont contre-vérifiés ; elles ne restent pas des demandes ouvertes. Le centre q3 supérieur à $2^{40}$ et le premier bit de `U320.w[4]` possèdent désormais des fixtures de référence précises ; leurs domaines géométrique et numérique doivent rester distincts.

Pour AxisBounds, le [reçu indépendant](receipts_iteration3/axis_execution.json) et son [snapshot](receipts_iteration3/axis_source.json) apportent une qualification exécutée : six portes réussies, dont cinq mutants rendus en code 4 avec divergence identifiée. Le nominal vérifie 1 212 boîtes et 45 requêtes de profondeur. Le CLI est reconstruit depuis cette copie ; cette exécution ciblée ne rejoue ni la fixture de fenêtre ci-dessus ni la suite complète. Les autorités de chaque résultat restent leurs propres octets et reçus.

La [preuve S1 complémentaire](S1_COURANT.md) compose les clauses du générateur jusqu’au RLE sous contrats explicites. Les invariants d’index et de parcours sont [raccordés](AUDIT_RACCORD_INDEX_FRONT_20260905.md) ; les bornes du front sont complétées par les [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md). Le [domaine CPU](DOMAINE_CPU_COURANT.md) nomme les préconditions et la portée des exécutions. Leur [raccord compilé](receipts_front_compiled_20260905/README.md) et le [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) ferment désormais les obligations de primitives et d'assemblage horizontal sur E. Les sources F concurrentes ne sont pas assimilées à cette qualification. Cette note conserve ses preuves et reçus de frontière de fenêtre ; la verticale, les poids de rendu, les identités publiques du quotient, la reprise et les coûts industriels restent des livrables distincts. Aucun code produit modifié. **GCP non utilisé.**
