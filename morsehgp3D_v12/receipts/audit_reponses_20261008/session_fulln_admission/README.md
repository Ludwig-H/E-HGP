# FULLN : relecture des derniers temps CPU/GPU

8 octobre 2026. Source `8a0716e74`, R1 après retrait A6b, avant B3. Exploration v12 hors registre,
FULL Pi0, entrée u21 ; `public_status=not_claimed`. Session résidente, 48 fils hôte ; la voie GPU
utilise CUDA pour le catalogue. Segmentation et préparation des fichiers d'entrée hors murs FULL.

Relecture **concordante sous limites de provenance**, pas fermeture indépendante complète : les pilotes
archivent les JSONL et leur résultat de lecture, sans codes/stderr individuels des sondes. Les codes
rejoués sont inférés conditionnellement de ce résultat et du pilote épinglé ; ils ne deviennent pas des
codes archivés. Les empreintes des deux ELF sont prises avant les campagnes ; le manifeste final du
worker couvre une autre sonde, pas ces sous-builds. Aucun hash final des sondes mesurées n'est réinventé.

| Trame | Sites | CPU K5 | GPU K5 | GPU K10 |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 39 885 | 354,359 (358,203) | 80,310 (97,824) | 495,103 (495,783) |
| ng01 | 35 551 | 298,896 (301,963) | 66,592 (83,081) | 366,141 (367,166) |
| ng02 | 45 845 | 361,175 (365,106) | 83,320 (88,298) | 422,194 (422,532) |

Millisecondes : médiane des passes chaudes réunies, puis maximum brut entre parenthèses. GPU K5 :
5 processus × 10 passes, première écartée, soit 45 valeurs par trame. CPU K5 et GPU K10 : 3 × 5,
soit 12 valeurs par trame. Ce ne sont pas les médianes des médianes de processus ; les deux sont
conservées dans `results.json`. Identité FUL1 CPU/GPU à K5 et stabilité des répétitions concordantes.

Cohorte de 37 trames GPU K5 : 5 processus, deux cycles de 37 ; second cycle retenu, 185 valeurs.
Médiane des 37 médianes : **142,410 ms** ; pire médiane **287,187 ms** ; maximum brut **288,221 ms**.
**14/37** trames ont leurs cinq valeurs retenues sous 100 ms. Le verdict FULL recalculé reste
**non tenu**. Pas de CPU sur ces 37 trames, ni de CPU K10 ng00–02 dans cette campagne. Les alias
v12set08/000000 et 000100 ont des empreintes d'entrée différentes de ng00/ng01 : aucune identité
FULL inter-alias n'est imposée. Le détail indépendant des étages est dans le reçu voisin `fulln_temps`.

MES-C : 159 cas de métadonnées (132 réels, 15 synthétiques sains, 12 difficiles), 56 processus,
3 608 FULL achevés et 2 392 passes retenues ; 48 processus réussis et 8 refus déclarés. OLS
indépendante en fractions, par configuration et famille : CPU K5/48, coût fixe extrapolé
**14,278 ms**, pente **9,697 µs/site** ; C1 et C2 non tenus. Le seuil C2 reste celui annoncé,
3,7272 µs/site ; il n'est pas recalculé des nouveaux temps FULL. C3 non tenu à K5 : sphères 3 000 et
10 000 sites refusées `unsupported_degeneracy/wide_leaf`, CPU/GPU. Ces quatre refus se répètent
à K10, soit huit au total. Aucun dépassement de délai.
Sur les 132 petits nuages réels, médiane des latences chaudes par nuage K5 : CPU48 **23,845 ms**,
GPU48 **7,315 ms**. Le manifeste de cohorte est réutilisé avec son pin ; son lien aux octets du tar
de données reste déclaré, sans nouvelle lecture du tar ni de payload.

Fermetures : 488 sources runtime identiques à Git, archive résultats 1 913 411 octets,
156 fichiers manifestés ; les trois commandes rendent zéro, 747 CTests déclarés réussis,
aucun saut signalé. Cela ne qualifie pas les portes hors socle ou les dépendances v11 non configurées.
Arrêt certifié RUNNING→TERMINATED, aucune erreur ni alerte du contrôleur. Toutes les statistiques
FULL publiées sont retrouvées exactement ; toutes les valeurs MES-C et ses critères concordent.
Aucun programme natif ni contrôleur n'a été exécuté ; aucun appel GCP, aucune lecture de
coordonnées ou d'identifiants d'entrée.

Rejeu de ce dossier, sur les preuves locales conservées :

```sh
python3 -B -S check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.fulln
python3 -B -S -O check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.fulln
```

Le lecteur vérifie les sources et archives sans extraire de données d'entrée. Il réutilise explicitement
le lecteur indépendant `mes_c_contrelecture/reader.py` du commit épinglé, adapté au schéma recouvert
et à la cohorte W4/W48, et le lecteur FULL livré du même commit. La note présente des observations
rejouées avec les limites ci-dessus ; elle ne constitue ni une adoption B3 ni une nouvelle qualification.
