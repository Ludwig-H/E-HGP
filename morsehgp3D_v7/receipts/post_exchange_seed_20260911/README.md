# Semis complets après échange — preuves CPU privées

Proposition d'optimisation statique, sans application active par cette sous-tâche.
Cadre : `exploration_v7_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé. Aucun contrat de latence acquis.

L'entrée exacte I union U après échange fournit la même MEB terminale sans
la recalculer. Une clé partielle ne suffit pas ; le rayon doit strictement
décroître. La table existante est locale à K et à l'index, immuable pendant
les workers ; ancres et DSU temporels restent hors du helper géométrique.

Le [lecteur autonome](verify.py) est à lancer avec `python3 -B verify.py`,
puis `python3 -B -O verify.py`. Il ne compile ni n'exécute le moteur : il
revérifie sources, captures, sorties physiques, comptages et causes des mutants.
Les sources historiques, scripts, patch, flux bruts et commandes exactes sont
conservés octet pour octet via [storage_map.json](storage_map.json). Le chemin
logique `README_PROOF.md` porte le détail mathématique et les réserves.
Les anciens Markdown sont stockés comme `.source` pour ne pas créer de faux
liens vivants. [extract.py](extract.py) reconstruit explicitement les chemins
dans un nouveau répertoire ; aucune commande historique n'est rejouée.

## Qualification conservée

| Capture | Portée |
| --- | --- |
| observation O2 | MEB encore payée, BallId/clé/niveau vérifiés, ancien travail identique |
| raccourci O2 et SAN a73 | 12 bras 0/1/2 × cache/sans cache/statique1/statique4, sorties physiques égales |
| micro mono 200/400/800/1000 | tours K1..10, mêmes sorties ; environ 5,7 % des MEB évitées |
| causal r1 puis r2 | mutant clé partielle survivant aux seules signatures Gamma, résultats négatifs conservés |
| causal r3 | trois mutations compilées et réfutées, dont la clé partielle par observation de la vraie MEB |
| clean O2 et SAN propre | 14 commandes chacun ; quatre physiques appariées, propre gate 34 nuages/150 ordres |

Le mutant de rayon `<=` est réfuté dans un état fautif non géométrique
explicitement injecté : aucun contre-exemple valide à la décroissance stricte
n'est revendiqué. Le mutant clé partielle peut rejoindre la bonne composante
malgré une mauvaise boule ; l'oracle Gamma ne suffit donc pas à qualifier
le terminal géométrique. L'omission du dernier échange échoue sur le travail.

Avec U uniques, S semis initiaux, D échanges et H hits après échange :
Q=D, H=T≤min(D,U−S), M nouveau=M ancien−H. Les anchor_hits évités ne sont pas
facturés : M=anchor_hits+intruder_queries et anchor_hits+T=U−S sur succès.
Les parents, contributions, populations et verticales sont comparés champ par
champ ; les capacités dépendant du Worker instrumenté sont distinguées.

## Limites explicites

La réduction de travail micro n'est pas une preuve de gain de latence : ordre
fixe 0/1/2, une répétition, hôte partagé et baseline conservée pour comparaison.
La résidence reste O(A+R+n+workers*depth), sans garantie universelle
sous-quadratique ni garantie de sortie FULL petite. Les corpus géométriques
restent bornés ; leur accord ne certifie pas la complétude WSPD universelle.
Ni 8k+, ni 50k sous 1 seconde/100 ms, ni plusieurs dizaines de millions sur G4
ne sont qualifiés par ce paquet.

Les dépendances système/compilateur/Boost ne sont pas distribuées ; les
commandes et dépendances `.d` capturées sont conservées, sans prétendre fournir
une chaîne binaire entièrement reproductible ou un vendor épinglé avant chaque
compilation. Les ELF sont exclus et leurs empreintes conservées séparément.
Les SAN exécutés par ROOT gardent ASan/UBSan et la détection des fuites activés.
