# Réservoir : raccords de qualification — 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Source : `59509bbc816646f7bda0c77a3b948f57c79a8b9c`.

Deux corrections proposées pour la qualification de cette tranche :

- [Bornes des masques](gates/README.md) : la porte IO utilise comme refus le nouveau bit valide `131072`, tandis que le lecteur de campagne refuse encore les modes de rejeu `180219` et `212987`. Aligner la borne admise sur `262143` et le témoin hors domaine sur `262144`.
- [Juge GPU](judge/README.md) : un registre absent est accepté dans la comparaison des voies ; les passes instrumentées ne vérifient pas leur registre ni leur statut métier. Le rejeu remplace seulement les appels de sous-processus par des réponses simulées ; il constate le faux `conforme`, puis son refus après correction, en normal et `-O`.

La lecture du moteur est favorable : liens et curseurs séparés pour les deux populations de blocs, emplacements finaux déterministes, `fits=false` irréversible et rejeu intégral après épuisement. Par genre, les prises atomiques sont bornées par `spare + count < kNoChunk`, donc aucun bouclage du curseur u32 dans le domaine admis. Les fragments d'une feuille non résolue ou qui déborde ne sont jamais publiés comme sortie complète.

Cette lecture ne qualifie pas l'exécution native, la concurrence CUDA, l'épuisement d'un réservoir actif ni un gain de temps. La porte actuelle compare réservoir suffisant et réservoir désactivé ; ce dernier n'est pas l'épuisement après allocations partielles. Aucun défaut moteur n'est démontré ici. Aucune compilation, exécution native, mesure ou action GCP de l'auditeur.

Chaque sous-capsule fixe ses sources, son correctif et son rejeu. Le manifeste racine couvre leurs fichiers et manifestes sans modifier les reçus antérieurs.
