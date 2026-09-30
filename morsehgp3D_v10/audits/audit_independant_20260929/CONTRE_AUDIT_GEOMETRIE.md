# Contre-audit courant : géométrie, API et oracle catalogue

30 septembre 2026, reprise après `e9eab2754`, lecture close à 03 h 12 UTC. Cette note suit les corrections annoncées par le développeur,
sans modifier ses copies de travail, ses builds ou les sources produit. Elle complète le
[rapport de base](GEOMETRIE_CATALOGUE.md), qui porte sur `6206d1d11`. GCP non utilisé.

## État vérifié

La [réponse du développeur](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md) annonce une seconde réparation,
puis une extraction commune. Les nouvelles copies sont dans `/tmp/mhgp10-r2/{sitetree,oracles,entrees_cli}/src/morsehgp3D_v10`.
Ce sont encore **trois copies par groupe**, avec des correctifs différents ; elles ne constituent pas une extraction
commune. Le snapshot `oracles/src-r1` est aussi relu. Aucun moteur, build existant ou copie du développeur modifié.

Les unités utiles ont exactement les empreintes du premier tour déjà contre-audité. Il n'y a donc pas de nouveau
calcul ni compilation indépendante à justifier ici. Les nouvelles preuves sont des lectures et des empreintes,
distinctes des tests du 29 septembre. Le worktree produit à `e9eab2754` garde encore les versions de base de
`SiteTree`, du générateur et de l'oracle catalogue : **aucune clôture intégrée**.

Empreintes SHA-256 des unités relues dans les groupes :

| Fichier | Empreinte |
| --- | --- |
| `sitetree/src/cloud/site_tree.cpp` | `d83e999bf626c4e8233517e9fe36ccdfef50ec2c319b3610b0814f9d2fa09e24` |
| `sitetree/src/cloud/site_tree.hpp` | `362bf31d94a743fb18f578d4ddbba05fe2e2287e00cc636d85565fbfe75a9d5d` |
| `oracles/tests/oracle/test_catalogue_oracle.py` | `b814428d4c63133441b8e21f921f23fd28bcd7b4d35421ea6050e9e37a9689a3` |
| `entrees_cli/src/catalogue/generator.cpp` | `967a32f67fac841c31479940a0722ad1a2d7f026bfefe8d36588551922cef836` |
| `entrees_cli/src/catalogue/catalogue.hpp` | `78e72fa5951ade8ecf80adf541705d4528fa7aef79905b6565d12936b3625d28` |

Les nouvelles captures [avant](../../receipts/audit_independant_20260930/geometry/SOURCE_BEFORE.json) et
[après](../../receipts/audit_independant_20260930/geometry/SOURCE_AFTER.json) fixent huit chemins par copie,
le produit et le snapshot du juge. Le [rejoueur de lecture](../../receipts/audit_independant_20260930/geometry/source_status.py)
n'exécute aucun test et refuse d'écraser une capture. Les 40 entrées sont stables entre 03 h 08 et 03 h 12 UTC
([comparaison](../../receipts/audit_independant_20260930/geometry/STABILITY.json)). Une observation textuelle dans ces JSON n'est pas une gate.

## Grille de fermeture des corrections annoncées

| Point | État des copies disponibles | Preuve nécessaire sur la prochaine extraction stable |
| --- | --- | --- |
| G1 : centre rationnel éloigné | Garde exacte du cube et replis des deux requêtes présents ; unité identique à celle déjà contre-auditée. | Conserver G1, ses quatre sites de coquille et le départage `(clé,indice)` de nearest ; replier hors cube, ancre incluse. Rejouer sur le binaire commun après intégration. |
| FENV | `filtered` lignes 152–163 ne consulte toujours pas l'arrondi. La réparation annoncée est absente. | Filtre actif seulement sous FE_TONEAREST ; repli exact sous les trois modes dirigés. Modes conservés à la sortie, planchers de replis définis avant exécution et mutants de garde réellement rejetés. |
| I/U publiés | `frozenset` ligne 111 efface encore multiplicité et ordre. | Listes strictement croissantes en Morton, disjointes ; poids recalculés sur les listes. Rejet des doublons I/U et de U renversée après acceptation du témoin intact. |
| Ordre du catalogue | `sorted(got, ...)` ligne 130 répare encore l'ordre avant son contrôle. | Lecture dans l'ordre reçu ; clé `(niveau exact,S*)` croissante, avec rejet des lignes inversées et de deux boules échangées dans un plateau. |
| Domaine produit de feuille | Garde `M=0` ou `M≥K+3` présente dans `entrees_cli`, lignes 637–648 ; K et bits sont vérifiés auparavant. | Témoin aux bords M=K+2/K+3, défaut M=0 conservé, conversion numérique stricte avant u32. Cette restriction ne prouve aucune borne générale sur les dégénérescences. |
| Diagnostic petite feuille | `--allow-small-leaf`, `--max-nodes` et champs associés encore absents. | Opt-in explicite M≥K avec budget positif obligatoire ; M<K refusé. Un budget épuisé rend resource_exhausted sans dump ni catalogue réussi sur un préfixe, quel que soit le nombre de fils. |

La lecture SiteTree indépendante, lue dans le worktree mais encore non publiée à cette capture
(`audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md`),
prouve déjà la correction G1 sur cette même unité et décrit les trois planchers d'arrondi échoués ; elle n'est
pas rejouée ici. `filtered=false` sélectionne un chemin, sans valider un Center forgé : la précondition de
représentation de `side_key` reste nécessaire. Le nuage 21 bits de la gate ne qualifie que des centres de site ou
milieu représentables, pas les fabriques q3/q4 hors u18. Cette correction ne requalifie pas les filtres propres de
la tour ni une campagne de temps FULL.

Le [contre-audit des juges](../audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) couvre
exactement le juge `b814428d…` : les anciennes failles de support canonique et de rang dense sont fermées, mais
les nouvelles mutations de populations et d'ordre restent acceptées. Notre [sonde antérieure](../../receipts/audit_independant_20260929/contre_oracle_preintegration/geometry_oracle_judge_result.json)
portait sur `ce9fe391…` et arrive au même diagnostic ; elle n'est pas rebaptisée en preuve du juge actuel.
Ces constats portent sur les juges, sans démontrer une population ou un ordre incorrect dans le moteur.

La [lecture des interfaces](../audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md)
couvre la même garde M≥K+3. La réponse annonce maintenant une voie M≥K réservée au diagnostic avec budget,
ce qui répond au besoin d'instrumenter les petites feuilles. La future gate doit distinguer l'arrêt sur **taille
de feuille** de `max_leaf` (quota de candidats) et de `max_nodes` (budget du parcours). L'issue doit être
déterministe en nombre de fils ; aucun chrono supplémentaire des coins du cube n'est utile avant livraison de ce budget.

## Première couverture et bande de paires : domaines distincts

Le catalogue brut suffit à chercher une **première boule couvrante minimale** d'un point de données x, pour un
ordre k et des sites distincts non pondérés. Cela ne justifie pas son usage pour toutes les boules d'une bande de rayons.

Voici la justification utile pour le raccord. Minimiser le rayon de la MEB d'une k-partie contenant x. Si sa boule
positive avait au moins k sites strictement intérieurs, une k-partie incluant x permettrait de diminuer le rayon :
prendre k intérieurs si x est intérieur, ou x et k−1 intérieurs puis déplacer légèrement le centre vers x si x est
sur la coquille. Si `p<k` mais `p+q_min>k`, alors `t=k−p≤q_min−1`. Choisir I et t sites de coquille, en incluant
x quand nécessaire, fournit une k-partie dont les sites de coquille sont séparables : aucun sous-ensemble de
cardinal inférieur à q_min ne contient le centre dans son enveloppe convexe fermée. Un petit déplacement rapproche
ces sites, tandis que les intérieurs restent stricts. Cela contredit encore la minimalité. La boule minimale
satisfait donc la borne renforcée `p+q_min≤k`. Le rayon nul se traite dans la table des sites.
Cette borne propre à la couverture est celle du [lemme renforcé](../audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) ; elle ne permet pas de supprimer les événements `p+q_min=k+1` nécessaires aux fusions FULL.

Cette réduction reste valide si l'on minimise seulement parmi les k-parties contenant x dont le centre MEB appartient à une
composante fixée C de la multicouverture à rayon R : ancien et nouveau centres appartiennent à l'intersection
convexe des boules de rayon R centrées aux sites de la nouvelle partie, donc à la même composante. Pour `x∈X` et
le seuil **exactement R**, le critère `dist(x,C)≤R` possède ainsi un témoin dans le catalogue, contenant x, de rayon
au plus R et de centre dans C. Cela ne calcule pas une distance générale à C et ne se transfère pas à un seuil
différent de R. Une boule dont le poids fermé atteint k n'est pas nécessairement une cellule active d'ordre k :
le raccord de son centre peut demander une descente, pas une recherche de cellule seule.

La réponse du développeur retient une **bande K2 positive** et annonce l'univers de toutes les paires. Ce choix est
nécessaire. Exemple entier collinéaire : `{0,100,101,102}`, pour x=0. Sa première demi-distance vaut 50. À
`η=0,02`, la paire `(0,102)` de rayon 51 appartient à la bande, mais ses deux sites strictement intérieurs font
qu'elle n'est pas dans le catalogue K2. La preuve du premier minimum, correspondant à `η=0`, ne ferme donc pas ce
bras expérimental à `η>0`.

L'implémentation peut grouper les paires ou boules candidates par identité et résoudre chaque candidate une fois,
tout en gardant chaque incidence point→candidate ex æquo. Elle doit ensuite dédupliquer les composantes par point.
Avec ambiguïtés, « au plus n candidates résolues » ne découle plus du choix d'une première date. Une politique
d'ancêtre commun doit publier une date où cet ancêtre existe : sa naissance peut être plus tardive que la
première couverture du point. Le développeur l'annonce désormais explicitement ; cela reste à vérifier dans le
prototype puis dans la porte.
