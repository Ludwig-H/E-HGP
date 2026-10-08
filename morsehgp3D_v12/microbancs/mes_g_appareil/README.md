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

## Session `gapp` (8 octobre) et règle de l'étape 1

`REGLE_G_APPAREIL` (10:04 UTC, avant toute mesure) a **rejeté** la suite sur les seules propositions : census 0,091 à
0,107 et sondes 0,046 à 0,053, mais propositions DWelzl en binaire64 0,738 à 0,758 pour un seuil de 0,50
([reçu](../../receipts/g4_gapp_20261008/README.md)). Le binaire64 de ce GPU va à 1/64 du binaire32.

## Étape 2 : trois mécanismes de proposition, issues comptées

Conception, preuve de l'issue et amendement proposé : [note de l'étape 2](../../receipts/developpement_20261008/etude_g_appareil_etape2.md) ; verdict de l'étape 1 : [session G-APP](../../receipts/g4_gapp_20261008/README.md).

Le poste « propositions » joue désormais, sur les mêmes parties, trois mécanismes, chacun sur l'hôte et sur
l'appareil : `p64` (DWelzl binaire64 du produit, rejeu de `gapp`), `l4` (voie entière du bras L4 de T2-d-B : paire la
plus éloignée et boule diamétrale, triangle aigu ; puis DWelzl binaire64 amorcé sur la paire la plus éloignée exacte,
joué sur l'appareil en deux noyaux, voie entière puis file compactée par warp) et `l4f32` (même voie entière, puis
DWelzl binaire32). Les classes DWelzl de L4 sont une copie textuelle du texte du bras, générée par
[`generer_l4_hd.py`](generer_l4_hd.py) (empreintes `92495aa6` du produit et `a82de524` du bras vérifiées).
L'**issue** de chaque proposition (boule de la table : S\* dans F et F dans P_b ; sinon sphère certifiée et support
canonique parmi les sites de F) est calculée par les fonctions exactes du produit (`lem_t1`, `certify_part`,
`exact_support`, `find_support`) et comparée à celle de `p64` ; le **mécanisme** (LEM-T1 sans arithmétique,
certificat, repli exact) est compté. Un mutant de la voie entière (« L4 sans test diamétral ») ne change aucune issue
(le repli exact rattrape tout) : il se voit à son taux de replis, que la règle borne à 0,1 %.

`REGLE_G_APPAREIL_2` (11:09 UTC, en tête de [`pilote_g_appareil.py`](pilote_g_appareil.py), avant toute mesure de `l4`
et `l4f32`) juge deux conceptions, la première préférée : **D2**, `l4f32` sur l'appareil et `p64` sur l'hôte, qui exige
d'amender `CONTRAT_TOUR.md` § 8 (compteurs des plus petites boules par issue, mécanisme physique) : total des trois
postes ≤ 0,10 et propositions ≤ 0,20 ; **D1**, politique `l4` partagée par les deux exécuteurs, sans amendement :
total ≤ 0,15, propositions ≤ 0,50, neutralité de l'hôte ≤ 1,05. Identité, replis ≤ 0,1 %, A/A, isolation, mutants :
voir l'en-tête.

## Essais locaux (codespace, sans GPU, 8 fils, machine chargée ; les temps ne jugent rien)

Étape 1 : identité complète du noyau en source unique joué sur l'hôte sur les trois trames (252 152, 290 357 et
646 621 requêtes de census ; 3,4, 5,5 et 10,0 millions de représentants ; 0,85, 1,25 et 2,48 millions de propositions) ;
mutant « côté nul » tué (code 1) ; refus propre sans appareil (code 2).

Étape 2 : issues de `l4` et `l4f32` égales à celles de `p64` sur toutes les parties des trois trames, aucun repli ;
voie entière concluant 55,1 / 54,5 / 53,1 % des parties ; mécanismes de `l4` égaux à ceux de `p64`, ceux de `l4f32`
différents sur 3 parties de la trame maximale (LEM-T1 direct au lieu d'un certificat) ; mutant « L4 sans test
diamétral » à 50,7 % de replis sur ng00 (tué par la règle) ; trois binaires appareil construits pour sm_120 (voie
entière 40 registres, DWelzl binaire64 72, binaire32 56) ; information K10 sur ng00 conforme (5,37 M parties, issues
identiques, 5 replis exacts de `l4f32`) ; sur l'hôte, `l4` coûte comme `p64` et `l4f32` 1,5 à 1,9 fois
plus (replis internes de DWelzl en binaire32), d'où une conception D2 propre à l'appareil.
