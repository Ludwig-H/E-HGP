# MES-G-APP : l'étage G sur l'appareil de G4, trois postes archétypes

8 octobre 2026. Microbanc **hors produit** de l'étude de faisabilité « étage G (résolution de la tour) sur le GPU de
G4 » ([note d'étude](../../receipts/developpement_20261008/etude_g_appareil.md)). Il ne modifie ni ne
copie le résolveur : il lie la bibliothèque du produit (`libmhgp12.a` de la construction par défaut) et joue, sur les
mêmes entrées, chaque poste par le produit sur l'hôte et par un noyau en source unique `__host__ __device__`
([`noyau_g.hpp`](noyau_g.hpp)) sur l'appareil, avec identité exigée à l'octet.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit, témoin) ; cuda_g4 (noyaux du microbanc)
objet=full_pi0 (étage G de la tour K1..5 : requêtes et résultats)
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Poste | Part du profil de G à un fil (ng00 K5, après T2-d-B) | Entrée | Sur l'appareil |
| --- | --- | --- | --- |
| `census` | census saturé 15,2 % + complet 7,9 % | toutes les requêtes de census que G émet, **interceptées** dans le produit (`-Wl,--wrap` sur `CensusWorkspace::query`, symbole vérifié par `nm`) : boule certifiée, seuil, témoins, résultat du produit | un fil par requête, parcours préfixe sans pile (pointeurs de sortie de l'index), garde et puissance en `i128` (`GuardedSphere`, voies native et certifiée ; les autres « non résolues ») |
| `sondes` | trace 19,5 % + sonde 14,2 % | tous les représentants des ordres 2..K (cellules, masques de traces, populations du catalogue) ; table LEM-POP refaite au format de l'appareil | un fil par cellule : trace, empreinte splitmix64, répertoire, dichotomie |
| `propositions` | proposition 10,4 % | les traces dont la première sonde échoue (premier pas de chaque chaîne) | `DWelzl` du produit, copie textuelle annotée `__host__ __device__`, binaire64 sans contraction (`-fmad=false`) |

Identité contrôlée : census (genre, p, m, I, U et neuf compteurs du travail : nœuds, sites testés, blocs intérieurs et
extérieurs, témoins, boîtes disjointes et partielles, sites hors du pavé, voies) contre le résultat du produit ;
sondes (naissance par représentant) contre `PopulationTable::find`, et réussites égales à `first_probe_hits` de G ;
propositions (bits de la boule et support) contre le `DWelzl` du produit. La récolte est contrôlée contre les compteurs
de G (`census_saturated + census_complete`, `census_sites`, `census_nodes`).

## Construction et jeu

Le pilote construit tout lui-même (`nvcc` sm_120 et `g++`, mêmes avertissements bloquants que le produit) :

```bash
python3 pilote_g_appareil.py tout --src <racine> --produit <build par défaut du produit> --travail <dossier> \
    --donnees <dossier des trames> --sortie <sortie> --fils 48 --processus 5 --passes 5      # G4
python3 pilote_g_appareil.py tout ... --sans-appareil --fils 8 --processus 1 --passes 2      # essai local
python3 -S -O pilote_g_appareil.py auto-test                                                 # juge seul
python3 -S -O test_pilote_g_appareil.py                                                      # porte locale du juge
```

Trames attendues dans `--donnees` (aucune n'entre dans le dépôt) : `lidar_ng00`, `kitti_ng_02_001606` (médiane de
`v12set`, 64 740 sites), `kitti_ng_08_002119` (maximum de `v12set`, 99 099 sites), chacune `.u32le` et `.ids.u32le`.

## Règle

`REGLE_G_APPAREIL`, écrite le 8 octobre 2026 à 10:04 UTC avant toute mesure G4, est dans l'en-tête de
[`pilote_g_appareil.py`](pilote_g_appareil.py) : rapport appareil / hôte (noyau seul contre le lot du produit à 48
fils), moyenne géométrique sur 5 processus par trame, IC 95 % par bootstrap ; seuils de la borne haute : census 0,20,
sondes 0,20, propositions 0,50, sur chacune des trois trames ; identité complète ; mutant « côté nul » tué ; A/A dans
[0,90 ; 1,10]. **Adopté** veut dire : l'étude justifie d'ouvrir la tranche « G sur l'appareil » ; rien n'entre dans le
produit.

## Essai local (codespace, sans GPU, 8 fils, machine chargée)

Identité complète du noyau en source unique joué sur l'hôte sur les trois trames (252 152, 290 357 et 646 621
requêtes de census ; 3,4, 5,5 et 10,0 millions de représentants ; 0,85, 1,25 et 2,48 millions de propositions) ;
mutant tué (232 162 écarts sur ng00, code 1) ; variante appareil construite pour sm_120 (census 96 registres, sondes
42, propositions 72 et 608 octets de pile) et refus propre sans appareil (code 2). Les temps locaux ne jugent rien.
