# Contre-audit B — premier reçu G4 de la tour v9

22 septembre 2026. Lecture indépendante du reçu
`morsehgp3D_v9/receipts/g4_tower_r1_20260922/` ajouté par `ad2d0ebb` ;
aucun accès GCP ni nouveau calcul. **Huit cas FULL CPU sur G4 ont réellement
achevé**, sans cas partiel ; aucun contrat de temps ou GPU n'est acquis.

## Provenance et état exécuté

Les 175 entrées de `SHA256SUMS` passent. `PACKAGE.json` épingle le snapshot
`e766319e…`, manifeste `afae8980…`, protocole et sources du commit
**`e28296bb`**, trois entrées sans sol à 1 mm avec SHA-256. Ses champs
`status=prepared_not_executed`, `GCP_used=false`, `FULL_executed=false`
décrivent **le paquet avant lancement**, pas le résultat de session. Les
reçus ultérieurs `vm/receipt.json` et `host/receipt.json` disent
`status=completed`, `FULL_executed=true`, `GPU_executed=false` ; le worker
a terminé les indices 0–7 avec huit sorties de sonde
`complete_relative`/code 0. Sources VM avant/après identiques ; manifeste
des dépendances compilées `dece5ca2…`, binaire stable SHA-256 `9f010b05…`,
capture reçue. Le contrôle hôte cible la même génération GCE, puis
`host/guarded_stop.redacted.stdout` constate `TERMINATED` et
`targeted_shutdown_certified=true`. Cela constitue une preuve documentaire
de la session, non une lecture GCP actuelle ni une facture.

Le probe **effectivement capturé** est `mhgp9_tower_probe_v1`, sans clé
top-level `ledger`, et satisfait le schéma strict du worker épinglé à
`e28296bb`. Le `ledger` et l'optimisation MEB « première paire maximale »
arrivent **après ce snapshot** : ni les nouveaux compteurs ni le nouveau
temps FULL ne peuvent être attribués à ce reçu G4. Tout prochain paquet
doit épingler ensemble probe, worker, selftest et validation du schéma ;
un probe avec `ledger` face à l'ancien `TOP_KEYS` serait refusé après calcul.

## Mesures du reçu

Trames 08/000000, 000100 et 000200, sans sol, `quantized_u18_input_only`,
s8, une répétition ; VM SPOT `g4-standard-48`, EPYC 9B45, 24 cœurs/48 fils,
~185,5 M kB de RAM. Temps ci-dessous : `chain_total`, q3/q4 et FULL en
secondes ; RSS en **KiB tels que publiés par GNU time**, sans conversion
ambiguë en « Go ».

| Cas | Trame | K | W/static | Total | q3/q4 | FULL | CPU·s | RSS KiB |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 000000 | 5 | 48/0 | 18,81 | 11,12 | 6,74 | 545,1 | 1 130 624 |
| 1 | 000100 | 5 | 48/0 | 15,05 | 9,11 | 5,30 | 439,3 | 956 456 |
| 2 | 000200 | 5 | 48/0 | 29,25 | 21,33 | 6,89 | 960,1 | 1 210 768 |
| 3 | 000000 | 10 | 48/0 | 111,68 | 32,72 | 75,90 | 1 663,4 | 4 883 764 |
| 4 | 000100 | 10 | 48/0 | 82,31 | 24,40 | 55,67 | 1 237,7 | 3 899 876 |
| 5 | 000200 | 10 | 48/0 | 125,44 | 58,03 | 64,28 | 2 788,2 | 4 910 660 |
| 6 | 000000 | 5 | 24/0 | 21,92 | 13,88 | 7,03 | 344,0 | 1 136 244 |
| 7 | 000000 | 10 | 48/48 | 70,00 | 32,76 | 34,14 | 1 680,8 | 4 759 720 |

Les condensés de tour des cas 0/6 et 3/7 concordent entre options de fils ;
les six cas de base concordent avec les condensés locaux des mêmes entrées.
Le README annonce ×11,6 pour q3/q4 sur 000000 K5 :
`128,48 s / 11,12 s = 11,56`, **arithmétiquement juste**. Ce n'est
**pas** une efficacité de parallélisation W8→W48 : l'hôte local était un
EPYC 7763 à quatre cœurs physiques, le G4 est un EPYC 9B45 à 24 cœurs,
avec charges et environnement différents. À G4 seulement, W24→W48 sur
000000 K5 fait 13,88→11,12 s pour q3/q4 ; une paire, pas une courbe.

Un calcul de capacité sur les six cas W48/`static=0` donne une indication
plus directe du parallélisme déjà utilisé. Si le FULL non statique consomme
un fil pendant son intervalle, le reste de la chaîne occupe en moyenne
`(chain_cpu_s−tower_s)/(chain_total_s−tower_s)` = **42,6 à 44,6 CPU
logiques sur 48**. Ces six ratios sont calculés sur les bruts de la sonde,
pas sur les temps arrondis du tableau. Ce n'est pas un profil par phase :
q2, fusion et census sont inclus, et la charge utile réelle des cœurs SMT
reste distincte des CPU·s. Mais l'amont n'apparaît pas massivement en
attente de fils supplémentaires sur cette VM ; pour K5 il faut surtout
réduire ses opérations/copies ou exploiter une autre architecture (GPU).
À K10, le FULL en un fil reste séparément un verrou : la variante statique
réduit son mur de 75,90 à 34,14 s sur 000000, sans le faire passer à 1 s.
Cette comparaison n'est **pas** une efficacité de parallélisation pure :
les deux modes ont mêmes catalogue, ordres et condensé, mais pas le même
`tower_work`. Le cas séquentiel compte 12 003 966 appels MEB et
1 065 359 881 tests de puissance, contre 11 309 383 et 1 000 198 900 en
statique ; `resolver_cache_hits` passe de 12 284 408 à zéro. Il faut donc
isoler travail algorithmique, cache et occupation par worker avant
d'attribuer le facteur 2,22 aux seuls 48 fils.

Le worker rapporte 486,6 s utiles pour les huit cas (474,5 s cumulés de
chaîne). Les commandes hôte vont de la demande de démarrage à 23:26:36 UTC
à l'arrêt vérifié à 23:38:25 UTC, soit ~11 min 49 s de fenêtre de contrôle ;
la durée **facturable exacte et le montant** ne sont pas dans le reçu.
Coupe-circuits : SPOT, `maxRunDuration=3600 s`, arrêt invité à 40 min,
budget worker 1500 s et plafond 600 s par cas ; arrêt ciblé effectivement
observé. Les bornes ne doivent pas être prises pour une consommation.

## Lecture et priorités

Même sur ce profil favorable sans sol/grille, la tour entière prend
15–29 s à K5 et 82–125 s à K10 en voie temporelle ; la seule variante
statique K10 mesurée prend 70 s. Le repli 1 s et la cible 1 s K10, puis
100 ms, restent ouverts. K5 est d'abord limité par q3/q4 ; à K10 q3/q4
**et** FULL comptent, alors que q2 et recensus sont bien plus courts.
Priorité : réduire le travail total cover/atlas/graine-cellule, puis borner
catalogue/résidence et paralléliser la résolution FULL à lots de niveaux
atomiques, en mesurant le nouveau ledger et le MEB avec un reçu apparié.
Trois trames d'une seule séquence, une répétition, sans sol et u18 ne
qualifient ni la trame brute entière, ni le float32 original, ni le GPU,
ni une loi sous-quadratique globale.
