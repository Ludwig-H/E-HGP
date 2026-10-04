# Audit mathématique indépendant depuis les fondations v11

Capsule figée, Git `0f5e8a207f2974e262cd40a8882b97af1da396af`, sources privées avant/après distinctes. Lecture mathématique, calculs Python exacts bornés ; aucun natif, fit, build, GCP, workflow, production ou note d'audit modifiée.

**Résultat nouveau utile :** le critère privé B (cœur du propriétaire) est la rencontre datée de la pendaison H avec la pendaison core. Sous les hypothèses et l'alignement fort de H3, sa date est stable en 3ε. Un LCA/meeting par point remplace son balayage de toutes les coupes. Cela ne restaure pas ses deux triangles ni ne démontre une supériorité statistique. [Preuve autonome et limites](PROOF.md).

**Contrôles :** 12 968 gardes normal/−O concordantes ; 8 nouveaux nuages de 6–7 sites, 32 ordres, 1 082 coupes ouvertes/fermées, populations fortes complètes rapportées à chaque composante, 156 dates B. Les routes A/B du pin sont comparées champ par champ, y compris parents/verticales/cœur/couverture ; un troisième calcul Gram/Γ contrôle directement le catalogue et la géométrie des coupes. La complétude pour tout n reste un théorème conditionnel, pas une conséquence du nombre de tests. [Matrice](COVERAGE.md), [lecteur](check.py).

Rejeu portable : `PYTHONDONTWRITEBYTECODE=1 python3 check.py` et `PYTHONDONTWRITEBYTECODE=1 python3 -O check.py`. Le lecteur ne dépend que de la bibliothèque standard et des copies figées dans la capsule. Il exécute les cibles Python sur ces copies, aucune brique native ni import privé scientifique. Les sorties et commandes/exit codes sont conservés dans `normal.json`, `optimized.json`, `LEDGER.json`.

`SOURCES.json` inventorie la capture initiale ; `SOURCES_AFTER.json` recontrôle les empreintes et conserve l'état privé final. Les huit sources privées n'ont pas changé entre ces captures. Les 282 fichiers Git sont une fermeture documentaire/source élargie, **pas** 282 fichiers intégralement audités : les copies historiques incluses restent du contexte, pas des ports qualifiés. Le root SHA256SUMS inclut tous les fichiers, y compris les inventaires imbriqués ; lui seul est exclu de sa propre liste.

La qualification et les erreurs C++ de protocole/concurrence sont revues séparément par les autres auditeurs. Ce reçu ne réexécute pas leur moteur et ne revendique aucun nouveau chrono ou résultat G4.
