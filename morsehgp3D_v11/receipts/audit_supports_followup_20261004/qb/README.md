# Q_b : contrelecture du contrat S6 et de l'énumérateur à porter

Sources moteur figées au commit `57dd21be1fd9ce68935910a78c8fc7174a1a3a5f`, copies avant la lecture ciblée. Documents de conception copiés séparément, avec leurs SHA. Aucun source, worktree développeur, note active ou reçu clos n'est modifié. Aucun build, C++/CUDA, fit, G4 ou grand tableau n'est exécuté.

## Périmètre réellement disponible

`impl_S4.md` et `verif_s4.md` décrivent **io**, dans `build/v11-impl-s4`. Ils ne livrent pas Q_b. Dans la spécification figée, Q_b relève de **S6-module-supports** (§3.3, §9.1 ; `slices_finales.json`). Le module `src/supports` est absent des trois acteurs inspectés : Claude, S2 et S4. Cette capsule relit donc le contrat S6 et son énumérateur de référence `bench/catalogue_euler.hpp`, sans attribuer au produit un module encore absent ou les tests locaux rapportés pour io.

Le contrat révisé est favorable : tous les supports positifs minimaux par inclusion, depuis U complète, toutes arités2..4 ; S* seulement pour reconstruire la sphère ; comparaison exacte du niveau ; pas de réutilisation du drapeau strict du tétraèdre générateur pour d'autres quadruplets ; plafond m24 et refus de l'appel entier au-delà. La sphère partagée conserve son arité de présentation et ses certificats numériques. Les prédicats existants `is_midpoint`, `strictly_acute`+orientation du centre et les quatre signes stricts de `strictly_inside` s'appliquent à tous les tuples de la coquille ; aucune redéfinition du canoniseur S* ni recherche de Q_b non canonique par `find_support` n'est nécessaire.

## Deux portes précises pour le port

**Fermeture zêta et liste minimale séparées.** `catalogue_euler.hpp:137–164` marque Q_b. `closure_counts:110–127` **modifie ces mêmes mots** pour marquer toutes les parties contenant un support. Ces bits ne sont plus Q_b. Sur le cube `{0,2}³`, Q_b contient6 supports (4 diamètres+2 tétraèdres) ; après fermeture,177 parties sont marquées, dont84 de cardinal2..4. Il faut conserver/exporter la liste minimale avant cette transformation, ou la produire dans un stockage distinct. Cette porte vérifie une difficulté réelle du raccord prévu, sans prétendre qu'un `enumerate.cpp` absent commet déjà cette erreur.

**Un compteur de cofaces nul ne supprime pas une primitive.** La boule du cube, p0,qmin2,β3, appartient à W1. Q_b contient les deux tétraèdres même si leurs cofaces d'ordre K+1=2 sont au nombre0. Une coupure `|Q|≤K+1` ou `cofaces(Q)>0` serait incompatible avec Q_b. Le modèle vérifie aussi une coquille entière3D à9 sites, de rayon5, où une même boule a **1 q2,4 q3,13 q4**. Les cas droit et exactement équilatéral distinguent les signes stricts et le test du plan.

Ces deux portes complètent la garde cube déjà close. Aucun défaut de conformité nouveau du moteur actuel n'est établi dans cette lecture.

## Capacités et refus à conserver

Pour W_K, K≤12, p≤11 et m≤24 :

- au plus `C(24,2)+C(24,3)+C(24,4)=12926` supports par coquille ; masques locaux sur24 bits, distincts des SiteIdx globaux ;
- `k_parts≤834451800`, `cofaces(b)≤1476337800`, `cofaces(Q)≤193536720` : les champs **par boule/par support** u32 de la spécification sont cohérents ;
- zêta à m24 :262144 mots u64, soit2097152 octets de scratch **par worker**, à compter avec les sorties retenues ;
- les offsets et nombres globaux de supports restent u64 (borne scalaire55516747242244 avec moins de kNone boules) ; les sommes de métriques ne sont pas les champs locaux. Par exemple un majorant de somme d'incidences par boule vaut398209865430 : ne pas le transporter dans un u32. Une somme globale doit être bornée, additionnée de façon vérifiée, ou portée en u128.

Le futur `ball_supports` public devra vérifier le BallIdx, m≤24, les tailles out/scratch et les capacités **avant les décalages et écritures**. Le helper interne de banc `mark_supports` les reçoit de son appelant. Le plafond de dégénérescence doit produire un refus de toute la hiérarchie, sans tronquer Q_b ni publier un préfixe. Ces obligations d'API/transaction restent à relire lorsque S6 existera.

## Vérification et limites

`check.py` est stdlib seulement. Il reprend **uniquement** la fonction barycentrique indépendante du reçu antérieur (copie/hash inclus, extraction AST) ; il n'exécute pas son ancienne suite. Il confronte cet oracle à la composition mathématique des prédicats de l'énumérateur, puis confronte le modèle de fermeture par mots à l'énumération indépendante des sur-ensembles. Des gardes textuelles contrôlent que les sources copiées portent bien ces trois boucles et ces opérations de zêta.

Quatre petites fixtures (m3,8,9) et les trois versions scalaires de la fixture mixte poussées dans les domaines u18/u21/u24 sont vérifiées. Ces versions changent l'échelle géométrique ; elles ne constituent ni un différentiel natif entre profils, ni une preuve nouvelle des bornes C++/Wide. Le plus grand ensemble de parties effectivement alloué a512 masques (m9). Les calculs m24 sont exclusivement scalaires.

Commandes exactes depuis ce dossier :

```sh
python3 -B -S check.py > normal.json 2> normal.stderr
python3 -B -S -O check.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

**6300 gardes**, normal/−O identiques, stderr vides. `BEFORE.json` conserve aussi l'échec de repérage `sphere_access.hpp` (fichier absent, pas une preuve manquante inventée). `AFTER.json` compare les copies aux octets Git/LIVE et distingue les évolutions éventuelles de documents. Le seul nouveau code est ce modèle privé. Aucun temps, absence de panne native, qualification S6 ou gain n'est revendiqué.
