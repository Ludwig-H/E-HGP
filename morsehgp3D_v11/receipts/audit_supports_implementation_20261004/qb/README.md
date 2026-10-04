# Relecture S6 : domaine public de Shape

Capsule figée le 4 octobre 2026. Source WIP de `/workspaces/E-HGP/build/v11-impl-s6`, base Git `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` ; `src/supports/` était non suivi. Les octets lus sont dans `sources/`, pas des blobs prétendument publiés à cette base. Aucun natif, build, fit, workflow ou GCP exécuté ; aucune modification du produit.

Constat matériel : `sources/src/supports/counts.cpp:8` compare `p + q` en u32. La factory publique promet de contrôler le domaine de Shape (`counts.hpp:82–101`), dont p ≤ K−1. Pour `(p,m,q,K)=(4294967295,2,2,1)`, la somme se replie à 1 et la factory accepte. Le chemin habituel `ball_shape` depuis un Catalogue valide possède déjà la borne sur p : ce témoin vise l'appel direct public, sans prétendre à un nuage géométrique ou à une panne native.

Le rejeu scalaire sur l'ABI GCC/Clang x86-64 montre une conséquence, pas seulement une forme incohérente : avec la fermeture régulière d'une paire `N=(0,0,1)`, `ball_counts` publie `{1,1,0,0,0}` au lieu de refuser ; le compte exact C(p+m,K) vaut 4294967297. `support_cofaces(shape,2)` vaut 0 contre C(p+m−2,K+1−2)=1. Aucun index hors table ni allocation massive n'intervient dans ce témoin. Élargir la comparaison à `u64{p}+q > u64{k}+1`, ou borner p avant l'addition, ferme la garde. Ajouter une porte de refus directe pour p=UINT32_MAX et ses voisins.

`check_shape.py` épingle les trois sources, contrôle les expressions reproduites, puis exécute un modèle explicite u32/i32 ; ce n'est ni une exécution C++ ni un interpréteur C++. 529 gardes passent en Python normal et −O avec sorties identiques : 26 entrées invalides voisines de UINT32_MAX acceptées par la garde capturée, 398 cas valides conservés par l'élargissement. Les conversions signées de la conséquence sont modélisées pour l'ABI annoncée. La faiblesse de la factory ne dépend pas de ces conversions.

La relecture bornée de l'énumérateur est favorable : supports minimaux écrits avant fermeture zêta ; toutes les arités 2/3/4 sont parcourues sans filtrer par qmin ou nombre de cofaces ; m≤24 et tailles de scratch/output gardés ; comparaisons de sphères exactes ; refus autorisant explicitement des buffers indéterminés. Les nouvelles fixtures cube (q4 à K1, cofaces nulles), mixte et capacités ont été lues, jamais exécutées. Cette capsule ne requalifie pas les profils u18/u21/u24 ni l'intégration native.

Chronologie : `BEFORE.json` est la capture avant lecture ciblée ; `TESTS_BEFORE.json` est la capture tardive des trois tests avant leur lecture. `AFTER.json` recoupe les 22 sources possédées : toutes identiques. Seul le fichier de déclaration des tests a évolué pendant la revue ; ses octets AFTER sont conservés sous `after_tests/`. `docs/SORTIES.md` absent est enregistré comme état du chantier, pas comme défaut.

Rejeu depuis ce répertoire (stdlib seule) :

```sh
python3 -B -S check_shape.py > normal.json 2> normal.stderr
python3 -B -S -O check_shape.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

`COMMANDS.json` conserve commandes, versions et résultats. `SHA256SUMS` inventorie chaque fichier de la capsule sauf lui-même. Capsule close ; aucune qualification native.
