# Reproduction du défaut d'alias — 27 septembre

Le [rapport et le code](../../audits/b_population_alias_20260927/README.md)
décrivent une anomalie de l'API publique, pas une correction du moteur.

La capture initiale `manifest.json` est conservée **failed** : huit commandes
exécutées, compilation Release/sanitizer réussie, cas Release reproduit,
puis échec de LeakSanitizer sous ptrace. Cet échec d'outil n'est pas imputé
au moteur. Le diagnostic est conservé dans `reproduce_sanitize.stderr`.

L'autorité du rejeu est `outside_sandbox/capture.json` : trois commandes
sur les **mêmes binaires**, code 0 pour Release et ASan/UBSan/LSan, code 1
attendu pour la fixture déclenchant le décalage UBSan de 32 sur u32.
Le premier échec n'est ni réécrit ni rendu vert. Les dépendances hachées
avant la compilation initiale sont encore identiques après le rejeu ;
le lecteur vérifie aussi les binaires et les sorties brutes.

Build épinglé : `/workspaces/E-HGP/build/v9-audit-population-alias-20260927`.
Lecture normal/−O : `replay.py check`, voir le rapport. Aucun GCP utilisé.
