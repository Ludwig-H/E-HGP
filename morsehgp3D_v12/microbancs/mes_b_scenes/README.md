# Microbanc MES-B : la tour FULL de la v12 sur des scènes LiDAR réelles entières

8 octobre 2026. Mesure du régime (b) de la décision D7 (scènes de plusieurs millions de sites), **hors produit**,
publiée telle quelle avec des verdicts écrits d'avance ; aucune règle d'adoption.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue, voie hybride) ; cpu_reference (G, T, M, V, R ; bras CPU identifié)
objet=full_pi0 (tour FULL K1..K, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Captations telles quelles

Consigne de l'utilisateur du 8 octobre : garder au plus les captations telles quelles, sans sous-échantillonnage. Les
cas sont donc les **scènes entières** des paquets de [`bench/data`](../../bench/data) ([`DONNEES.md`](../../docs/DONNEES.md)
§ 3) :

- **IGN LiDAR HD** : dalles entières de 1 km² ;
- **ETH3D** : scans entiers ;
- **FOR-instance** : placettes entières ;
- **Boreas** : fenêtres de 1, 10 et 50 trames consécutives, accumulées entières.

Il n'y a ni découpe ni décimation ; les découpes de 1 à 8 millions de sites de `MES-E` ne servent pas ici. Deux écarts
sont déclarés :

- les retours d'une même position au millimètre sont un seul site (variante `.distinct` du paquet ; 0 à 3,7 % des
  retours selon les jeux ; le moteur refuse les doublons par défaut, décision D8) ;
- le sol est retiré dans les variantes `sans_sol` (classe du producteur, ou Patchwork++ épinglé pour Boreas).

L'« itinéraire » Boreas (une trame sur 200) n'est pas une captation telle quelle : il reste hors de cette mesure.

## Ce que fait le pilote

[`pilote_b.py`](pilote_b.py) (bibliothèque standard, Python 3.10 nu) :

1. relève l'environnement (nvcc, cmake, GPU, hôte) et exige un GPU vide avant et après ;
2. construit `mhgp12_full_probe` (Release, profil 21, CUDA), garde le journal de construction, l'empreinte SHA-256
   de la sonde et un extrait du `CMakeCache` ;
3. joue chaque cas `NOM:K:VOIE:PASSES` dans un processus neuf.

Pour chaque cas, la sonde [`bench/full_probe.cpp`](../../bench/full_probe.cpp) mesure le même mur que `MES-FULL`
(de l'entrée quantifiée en mémoire à la tour complète en mémoire), sur la Session recouverte, voie par défaut de la
sonde depuis la bascule du 8 octobre (`--sequentiel` joue l'ancienne voie et la transmet à la sonde). Elle publie par
passe :

- les étages P, C (dont transferts), G jusqu'au dernier calcul de G, puis la queue (avec `--sequentiel` : G, T, M, V,
  R) ;
- le temps CPU du processus pendant le mur ;
- le pic du budget de l'hôte et le pic de mémoire résidente ;
- la mémoire de l'appareil gardée par le contexte, et le pic de son budget propre (`--budget-appareil`, nouveau : la
  carte et l'hôte sont deux ressources, un manque de l'une rend `memory_budget` sans faux refus de l'autre) ;
- la mémoire du budget de l'hôte par étage (`memoire_octets`) : pour P, C et la tour (avec `--sequentiel` : P, C, G,
  raccord et TMVR), l'usage à la fin de l'étage et le pic pendant l'étage ; le plus haut de ces pics est `pic_octets`,
  ce que le lecteur vérifie.

Le pilote échantillonne aussi `memory.used` de nvidia-smi toutes les 250 ms pendant le cas ; l'échantillon peut manquer
un pic bref. Capacité gardée, pic des réservations, RSS et `memory.used` sont quatre mesures différentes.

Un refus de la sonde (code 2 : `memory_budget`, `wide_leaf`, ...) est un **résultat** publié, le point de rupture. Une
mort par signal, une expiration ou un invariant violé sont des **échecs** du cas, publiés. Une sortie hors schéma ou un
appareil indisponible font **manquer un contrôle** : le verdict d'ensemble est alors refusé.

**Délai.** Un cas dont la prévision dépasse le temps restant n'est pas lancé (« non joué : délai »). La prévision
vaut 20 s, plus le débit du dernier cas comparable fois les sites et les passes, fois 1,3. L'ordre des cas est donc
un ordre de priorité.

**Empreinte FUL1.** Elle hache en un seul flux toute la tour sérialisée, centres exacts compris : environ 34 µs par
site sur le codespace (7,4 s pour 216 000 sites). Elle n'est calculée que jusqu'à `--empreinte-max-sites`. Au-delà,
l'identité entre passes et entre voies n'est pas contrôlée et le rapport le dit.

## Verdicts écrits d'avance

Les objectifs du régime (b) sont ceux de [`MESURE.md`](../../docs/MESURE.md), hypothèses du 7 octobre. Le mur chaud est
celui de la dernière passe jouée si au moins deux le sont, sinon celui de la première, marqué froid.

| Critère | Règle |
| --- | --- |
| B1 | K5, voie appareil : chaque scène lancée aboutit et tient au plus 2 s par million de sites ; un échec compte à toute taille, un refus seulement sous 10 millions de sites (au-delà, refus toléré par les objectifs et publié) |
| B2 | K5, voie appareil : aucune scène de moins de 10 millions de sites refusée ni en échec |
| B3 | K5, voie appareil : pente des moindres carrés de log(mur) contre log(sites) au plus 1,1 sur chaque série emboîtée de captations entières (`--series`) jouée en entier |
| B4 | K10 : chaque scène lancée aboutit et tient au plus 10 s par million de sites |

Chaque critère est « tenu », « non tenu » ou « non évalué ». Le verdict d'ensemble est « tenu » si B1 et B2 sont
évalués et tous les critères évalués tenus, « non tenu » sinon, « refusé » si un contrôle manque. Les séries de la
session L sont Boreas n1 ⊂ n10 ⊂ n50, sans sol et avec sol : une même trajectoire, 1, 10 puis 50 trames consécutives.

## Usage

```bash
python3 pilote_b.py --src <depot> --travail <construction> --donnees <paquet> --sortie <dossier> \
    --cas boreas_202011261358_f4500_n10_sans_sol:5:appareil:2,ign_paris_0651_6863_sans_sol:5:appareil:2 \
    --series a,b,c --fils 48 --budget-gio 160 --budget-appareil-gio 88 --delai-global 2700 \
    --empreinte-max-sites 1600000
python3 pilote_b.py --essai --sonde <mhgp12_full_probe> ...   # essai local, voie CPU, verdict « essai »
```

Sorties : `rapport_b.json`, `tableaux_b.md`, `brut/` (sorties de chaque cas), `construction.log`.

**Portes.** La lecture stricte des sorties est celle du lecteur partagé avec `MES-FULL`,
[`microbancs/outils/lecteur_full.py`](../outils/lecteur_full.py). Sa porte
[`test_lecteur_full.py`](../outils/test_lecteur_full.py) (Python 3.10 nu, aussi sous `-O`) vérifie :

- une sortie conforme admise ; vingt-trois mutations du schéma refusées, dont cinq de la mémoire par étage et le mur
  nul de la [contre-lecture de livraison](../../receipts/audit_reponses_20261008/mes_b_livraison/README.md) ; les
  cinq corruptions de la [prélecture de l'auditeur](../../receipts/audit_reponses_20261008/mes_b_prelecture/README.md)
  refusées ;
- les refus et échecs publiés comme résultats ;
- les Sessions à plusieurs trames, le budget de l'appareil attendu et l'empreinte demandée ou non ;
- le schéma de la Session recouverte : une sortie conforme admise, seize incohérences refusées (schéma, raccord,
  partition, fenêtres, ouverture de G, mémoire, recouvrement, fins par ordre), et les deux schémas jamais mêlés.

[`mutants_lecteur_full.py`](../outils/mutants_lecteur_full.py) tue vingt-deux mutants du lecteur ; un mutant
équivalent est écarté, et le fichier dit pourquoi. La porte du pilote, [`test_pilote_b.py`](test_pilote_b.py), vérifie
les verdicts B1 à B4 aux seuils, les empreintes entre passes et entre voies, ce que le pilote demande au lecteur, les
étiquettes uniques et le pilote complet sur une sonde simulée (délai, seuil d'empreinte, schéma recouvert par défaut,
`--sequentiel` transmis et lu, schéma croisé illisible, colonnes des tableaux) ;
[`mutants_pilote_b.py`](mutants_pilote_b.py) tue dix-sept mutants du pilote.

## Premier essai local (8 octobre, indicatif)

Le codespace (6 fils, voie CPU) traite Lyon sans sol découpé à 1 000 141 sites (première passe, sans empreinte) en
99,0 s, dont C 76,7 s, G 13,9 s, T 3,8 s, R 2,5 s, avec un pic de 9,4 Go, soit 9,4 Ko par site.

Une trame Boreas entière sans sol (146 316 sites) prend 22,6 s à chaud, dont C 19,5 s : 154 µs par site, plus cher
par site que l'aérien. La trame avec sol (215 665 sites) prend 25,9 s à froid. Les temps de G4 font foi.

Mémoire par étage sur cette trame (voie CPU, Ko par site, usage à la fin de l'étage / pic pendant l'étage) : à K5,
P 0,06 / 0,06, C 3,15 / **11,29**, G 5,17 / 5,89, TMVR 7,47 / 7,89 ; à K10, C 14,5 / **48,2**, puis 33 Ko par site à la
fin. Le pic de toute la passe est celui de la construction du catalogue, 3,3 à 3,6 fois sa taille finale : c'est le
premier levier pour faire tenir les captations de plusieurs dizaines de millions de sites.
