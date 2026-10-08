# Cache2 — admission indépendante et fermeture ELF

Audit Codex, 8 octobre 2026. **105 journaux / 1 362 passes conformes**, résumés
reconstruits et règle appariée reproduite : le cache de 8 Gio est **adopté**
sur les trois trames décisives. L'empreinte ELF finale existe cette fois et
égale celle d'ouverture ; le source exécuté la relève **après les six Sessions
informatives**. Aucun moteur, build, contrôleur, appel GCP ou lecture de
coordonnées XYZ/IDs par cet audit.

## Configuration et fermeture

Source exécuté `bdfca8fb198e6711626c6506b816a2a656315c83`, pilote
`c795dae5…`, lecteur FULL renforcé `15437e5f…`. Le plan `e2427911…` commande
u21, K5, catalogue appareil, 48 fils, dix tours de dix passes sur ng00/01/02.
Les bras sont **ref=`--cache=0`, aa=`--cache=0`, cache=défaut 8 Gio**. Les
options explicites des témoins évitent de confondre le défaut nouvellement
activé avec une référence sans cache. Aucun bras CPU, K10 ou W1 dans ce lot.

Le [reçu de provenance](../cache2_provenance/README.md) porte l'archive, les
sources compilables, les commandes et l'arrêt. Les sept déclarations d'entrées
(XYZ/IDs des trois ng et archive des 37 trames) égalent celles de M ; le présent
lecteur utilise les mêmes métadonnées, sans rouvrir les payloads. Les effectifs
d'entrée attendus sont 39 885 / 35 551 / 45 845, et les 37 noms/effectifs sont
fermés dans l'ordre de la Session. Les tailles seules ne prouvent pas
indépendamment l'absence de doublons géométriques.

L'ELF d'ouverture **et** de fermeture déclaré vaut exactement :

```text
a72884f5a4d25078c8192f2dabc698ed6490567931b4ffca8d2509b0c402d903
```

Dans `main()` du pilote épinglé, `play_session_info` finit avant l'affectation
`sonde_fin_sha256=file_sha(probe)`, elle-même avant `judge` et l'écriture du
rapport. Le lecteur exige deux chaînes hexadécimales identiques ; le juge
livré les exige aussi. C'est une fermeture relevée par le pilote qualifié par
ses sources et son rapport, pas un hash final inventé ni le rehachage local
d'un exécutable retourné.

Le champ d'environnement « après » reste relevé **avant** ces Sessions. Il
ferme l'observation après la campagne décisive, pas celle après l'information ;
deux photographies d'occupation ne prouvent pas une isolation continue entre
elles. L'égalité ELF finale ne remplace pas cette distinction.

## Admission et statistiques

| Bloc | Journaux | Passes | Chaudes décisives ou secondes visites |
|---|---:|---:|---:|
| Identité : 3 trames × 3 bras | 9 | 18 | identité sur les deux passes |
| Décision : 3 trames × 3 bras × 10 tours | 90 | 900 | 810 |
| Information : 3 bras × 2 Sessions de 37 trames | 6 | 444 | 222 |

Le rejeu ferme cohorte/configurations/rotation des bras, séquences et types,
identités FULL entre bras, tous les résumés de prises, diagnostics temporels
et capacités budgétées. Les journaux et rapport sont rehachés après lecture.
Comme pour M, les codes et hashes individuels des **seules Sessions informatives**
ne sont pas publiés par le pilote : leur lecture avec code 0 reste conditionnelle
à son chemin de succès et à son résumé complet sans refus. Les 99 prises
d'identité/décision portent chacune leur code et leur hash.

Les statistiques sont recalculées indépendamment : médiane des passes 2..10
par processus, rapports appariés par tour, moyenne géométrique et 10 000
rééchantillonnages (graine `20261008`, quantiles 250/9749). Même règle, aucun
seuil déplacé, aucun arrondi pour décider. Rejeu indépendant, juge livré et
rapport concordent **sans écart d'ULP**.

| Trame | GM cache / ref | IC95 | Mur ref (ms) | Mur cache (ms) |
|---|---:|---:|---:|---:|
| ng00 | 0,922088896 | [0,916858556 ; 0,927269322] | 94,743048 | 87,163653 |
| ng01 | 0,925366062 | [0,920246412 ; 0,931022974] | 77,779745 | 71,796757 |
| ng02 | 0,912035997 | [0,906159383 ; 0,917334720] | 96,413295 | 88,117145 |

Les murs sont les médianes des dix médianes de processus : leur rapport n'est
pas le GM utilisé pour décider. A/A donne **0,995138877 / 1,007903351 /
0,995259712**, dans ±1,5 %. La règle A/A porte sur ces **GM**, pas sur leurs
IC : la borne basse de l'IC ng00, 0,984870713, ne constitue donc pas un veto
dans ce protocole. Les trois bornes hautes des IC cache sont strictement sous 1.

## Sessions informatives et ressources

Pour chaque trame : médiane de ses deux secondes visites, une par processus ;
puis médiane et maximum des 37 valeurs. Ce bloc reste informatif, sans test
d'adoption sur les 37 trames.

| Bras | Médiane des 37 (ms) | Maximum des 37 (ms) |
|---|---:|---:|
| Référence sans cache | 160,232419 | 318,927719 |
| A/A sans cache | 161,298079 | 324,978574 |
| Cache 8 Gio | 147,772250 | 297,526334 |

Les [résultats exacts](results.json) conservent les 37 valeurs, ressources par
prise/Session et fenêtres d'étages. Les durées CPU de la campagne avec GPU
sont celles du processus hôte ; elles ne sont pas des mesures de FULL CPU.
Les pics du budget actif des Sessions valent 3 306 507 216 octets sans cache
et 3 368 338 953 avec cache. Les RSS maximum des deux processus cache valent
4 095 815 680 et 4 095 406 080 octets. Le budget actif partagé hôte/appareil
exclut les blocs hôte inactifs du cache ; le RSS hôte cumulé n'est ni ce budget,
ni la VRAM, ni une mesure après libération. Capacités appareil et épinglées
sont contrôlées contre le pic partagé ; son pic appareil séparé vaut zéro
par convention. Aucun gain nouveau CPU/K10 ou FULL massif déduit.

## Réserve documentaire et reproduction

[documentation.patch](documentation.patch), proposé sur le README livré en
`389b5e453`, remplace « avec toutes les preuves » par la fermeture exacte
observée et corrige C3 « mesuré sans effet » : les deux comparaisons de petits
nuages ont été **refusées par A/A**, ce qui ne démontre pas une absence d'effet.
Voir [l'admission C3](../session_c3_apparies/README.md). Le patch est vérifié
par `git apply --check` et appliqué uniquement en copie temporaire.

`check.py` est un port explicite du [lecteur M](../session_m_apparie/check.py) :
trois bras, options modifiées et fermeture ELF désormais exigée. Il importe
les sources Python déjà livrées à `bdf…` sans leur appliquer de correction.
Une première préparation avait conservé la rotation sur quatre bras du lot M ;
cette hypothèse a été remplacée par le nombre de bras avant admission. Aucun
journal ni rapport de campagne n'a été changé. Les deux modes donnent
exactement [results.json](results.json) :

```sh
python check.py --repo DEPOT --folder DOSSIER_RETURNED_CACHE2 \
  --metadata INPUT_METADATA_SESSION_M_JSON --plan PLAN_CACHE2_JSON --check
python -O check.py --repo DEPOT --folder DOSSIER_RETURNED_CACHE2 \
  --metadata INPUT_METADATA_SESSION_M_JSON --plan PLAN_CACHE2_JSON --check
```

[capture.json](capture.json) épingle les quatre sources, le plan, les métadonnées,
le rapport, l'inventaire JSONL et la pré/postimage documentaire. Provenance
extérieure séparée ; aucun fichier produit, ancien reçu ou registre modifié.
