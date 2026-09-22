# Constantes préparées du census q2 : identité et durée du cache

Le nouveau `Q2PreparedBounds` conserve les extrema exacts et les frontières du census sur le snapshot audité. La préparation de 48 octets peut suivre les divisions de Z sans changement ; après une division de B, elle doit être reconstruite pour conserver exactement le même travail discret. Le code capturé respecte cette distinction. Cette note qualifie la transformation locale, sans mesure de temps ni qualification de la tour FULL.

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=implementation_v8_p0`, `public_status=not_claimed`.

L'identité était déjà proposée dans [le contrat du census](../../morsehgp3D_v8/docs/P0_CENSUS_Q2_PARTAGE.md#qualification-et-décision), à partir des [extrema exacts de l'autre auditeur](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches). L'apport présent est son contrôle indépendant sur le vrai en-tête, le contre-exemple de réemploi après division de B et la comparaison intégrale des exécutions.

## Preuve et bornes entières

Pour un axe et une extrémité $e$ de B, posons $C=a+e$ et $D=(e-a)^2$. Alors $4(z-a)(e-z)=D-(2z-C)^2$. L'expression est affine en b : pour ses deux extrema sur B×Z, il suffit de tester les deux extrémités de B, puis d'optimiser en z. Son minimum utilise l'extrémité de 2Z la plus éloignée de C ; son maximum utilise le point de 2Z le plus proche de C. Les trois coordonnées sont indépendantes, donc leurs extrema s'additionnent.

Les deux carrés $(2z_{\min}-C)^2$ et $(2z_{\max}-C)^2$ suffisent : le carré de la distance minimale vaut zéro si C appartient à 2Z, sinon l'un de ces deux carrés. Cela inclut les centres demi-entiers et les égalités aux frontières. Les valeurs obtenues sont les extrema sur les **boîtes continues** ; elles restent des bornes certifiées sur les sites u16 qu'elles contiennent, sans prétendre être les extrema atteints par ces seuls sites.

Avec $M=65535$, on a $C\leq 2M=131070$ et $D\leq M^2=4294836225$ : les deux constantes tiennent séparément dans un `uint32_t`. Le produit définissant D est effectué après promotion signée. Lors de la requête, elles sont repromues en `i64` avant les différences ; $|2z-C|\leq 2M$ et chaque carré est au plus $4M^2$. Enfin $-12M^2\leq 4H\leq 3M^2$, soit de −51 538 034 700 à 12 884 508 675. Une somme globale de distances carrées ne tiendrait pas toujours en u32 ; le code n'effectue pas ce stockage.

Le census conserve `minimum4 > 0` pour créditer un bloc intérieur et `maximum4 <= 0` pour écarter ses intérieurs. Le maximum nul reste une frontière, avec une collecte de coquille distincte. Le constructeur public contrôle B ; `bounds` contrôle Z. Le raccourci privé `bounds_unchecked` est utilisé uniquement sur les boîtes construites dans l'index validé. La valeur inerte privée des tâches singleton n'est jamais interrogée : ces tâches gardent le chemin `pair_bounds`.

## Une préparation appartient à (a,B)

Les constantes ne dépendent ni de Z, ni du compte acquis, ni du curseur des témoins. Leur copie par valeur ne porte aucune vue de points ou durée de vie empruntée. Elles restent identiques lors d'une division de Z et d'un déplacement de son curseur. Un changement d'ancre ou de boîte B exige en revanche une nouvelle préparation pour retrouver les mêmes extrema.

La fixture entièrement sur l'axe x utilise a=1000, B=[60000,60004], son enfant B′=[60002,60004] et Z={60001}, les deux autres coordonnées étant nulles. Les intervalles exacts de 4H sont respectivement [−236004,708012] et [236004,708012]. La préparation du parent laisse la requête incertaine ; celle de l'enfant certifie tous ses témoins intérieurs. Réutiliser les constantes du parent reste conservateur pour l'enfant, mais peut modifier les visites. Dans le code capturé, chaque appel récursif sur B′ reconstruit ses constantes ; le curseur et le compte sont transmis séparément.

La [comparaison intégrale](Q2_PREPARED_CENSUS_CHECKS.json), effectuée par le second juge complémentaire, confirme cette distinction : le vrai code conserve les payloads canoniques, les trois cardinalités et les 26 compteurs de chacun des 932 appels sur 466 fixtures, entre l'ancien census, le nouveau et son exécution UBSan. Le mutant transmettant les constantes du parent à ses enfants conserve les sorties de l'oracle, mais change le travail de 81 appels ; le premier passe de 302 à 306 visites. Ce mutant réfute l'égalité du travail, sans constituer un contre-exemple géométrique. Le mutant préparant avec une autre ancre est rejeté.

## Porte des fonctions et coût explicite

Le [juge C++](q2_prepared_bounds_probe.cpp) et son [runner](q2_prepared_bounds_checks.py) comparent le vrai en-tête aux textes exacts de `shared_bounds` et `pair_bounds` extraits du census `f4815cd4`, puis à une évaluation indépendante de $(z-a)(b-z)$ sur une grille rationnelle. Le [reçu](Q2_PREPARED_BOUNDS_CHECKS.json) embarque les sources compilées, l'ancien fichier complet, les hashes des fonctions extraites et le contexte du raccord census ; aucun accès Git ou ancien répertoire temporaire n'est nécessaire au rejeu.

Les runs GCC stricts `-O2` et `-O1` avec UBSan passent : 15 924 requêtes, 1 118 préparations, 3 375 cas de grille et 49 875 évaluations rationnelles, 7 939 cas d'extrémités u16 et 3 393 comparaisons B singleton avec les bornes de paire. Les cas 3D supplémentaires sont reproductibles par graine fixe. Le juge contrôle aussi la copie de la préparation, les deux rejets de boîtes inversées et la fixture de cache ci-dessus. Cinq vrais mutants de l'en-tête sont rejetés avec sortie 1 : centre stocké sur 16 bits, distance carrée stockée sur 16 bits, sommet intérieur omis, mauvais extremum pour le minimum et contrôle public de Z supprimé. Le succès vaut sortie 0 ; un appel sans `--selftest` vaut sortie 2. Le runner reste effectif sous Python `-O`.

Le modèle d'opérations suivant compte les produits de deux valeurs non constantes présents dans les expressions sources, en excluant les multiplications par 2 ou 4 et le travail du juge :

| Pour P préparations et V requêtes de bornes | Ancien `shared_bounds` | Nouveau code |
| --- | ---: | ---: |
| Produits des expressions sources | 24V | 6P + 12V |
| Pour les 1 118 préparations et 15 924 requêtes du juge | 382 176 | 197 796 |

Chaque préparation paie six carrés de différence ; chaque requête paie douze carrés d'extrémité, réutilisés pour les deux extrema. Cette comptabilité **ne mesure ni les instructions générées, ni le temps** : le compilateur peut déjà supprimer des expressions redondantes, et préparations, lectures, comparaisons, parcours et sorties conservent leur coût. Elle ne réduit pas le nombre de tâches et n'établit aucune borne globale meilleure. Le passage ultérieur doit donc porter sur le coût total apparié, comme prévu par le développeur.

Sources épinglées : en-tête `7bb46b4b7af3beede9bc2fc8926bafda9c900eb573671583206a6d94ffac5d21`, nouveau census `b1ca5edd575f39bc7995bcf09dca0ccc9bb6838469fe3761f9bf1a2191187c74`, ancien census `3c513cc474c3d3a249779032f5cd03dac47198cf4b25d7698855bd118e0e593a`. Le reçu distingue cette capture du worktree éventuellement modifié ensuite. Rejeu depuis la racine :

```bash
python3 audits/morsehgp3D_v8_complementaire/q2_prepared_bounds_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_PREPARED_BOUNDS_CHECKS.json
python3 -O audits/morsehgp3D_v8_complementaire/q2_prepared_bounds_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_PREPARED_BOUNDS_CHECKS.json
```

L'intégration se rejoue séparément avec [son runner](q2_prepared_census_checks.py). Aucun test lourd ni chronométrage n'est ajouté ici. GCP non utilisé.
