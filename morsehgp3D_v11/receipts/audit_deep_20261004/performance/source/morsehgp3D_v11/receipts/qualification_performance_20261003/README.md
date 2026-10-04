# Qualification et mesures FULL G4 — 3 octobre 2026

Moteur **c40f40798375a0fc37917499401f16876cccbd2a**, CPU de G4, u21.
**4073/4073 portes**, **326 mutants tués** (324 juges, deux refus de compilation
attendus), zéro signal/délai. **81/81 prises appariées**, sorties FULL complètes
identiques octet pour octet ; 94/94 portes ciblées du binaire reconstruit avant chrono.
[Validation et sessions](validation.json), [contrelecture](review/captures.normal.json),
[analyse](review/analysis.md), [métriques dérivées](review/metrics.normal.json).

## Résultats

Médianes de trois prises FULL K=1..5/u21/W48, en ms :

| Trame entière sans sol / sites | Baseline v11 895 /2047 | c40 /2047 | c40 /16379 | Intervalle min–max /16379 |
|---|---:|---:|---:|---:|
| 08/000000 /39885 | 1309,857 | 843,962 | **489,099** | 468,730–493,124 |
| 08/000100 /35551 | 1070,932 | 678,702 | **345,066** | 333,573–354,942 |
| 08/000200 /45845 | 1419,802 | 894,237 | **432,397** | 415,678–444,127 |

Réduction16379 face à baseline :62,66 /67,78 /69,55 %, gain×2,68–3,28.
Aucune des81 prises ne passe 200  ms ni 100  ms. Domaine seul>200  ms à chaque prise
ng00/ng02 ; génération catalogue 131–176  ms puis descentes régulières 74–131  ms
sont les prochains postes à traiter. Médianes de phases indépendantes, non additives.
CPU FULL 13,537 /10,661 /12,501 s ; pics réservés346,0 /298,7 /368,3 MiB, hors RSS.
Processus natif1,046 /0,832 /1,032 s (dump/destruction inclus), Python séparé.

## Capture et périmètre

| Dossier dans captures/ | État conservé |
|---|---|
| failed_initial | source 672 :4072/4073, mutant catalogue invalide pour compilation |
| qualification_650 | source 650 :4073/4073,326 mutants, historique clos |
| failed_paired_plan | source 650 :94/94 natifs, label absent, aucun chrono |
| failed_paired_inventory | source 650 :94/94 natifs, inventaire LIST refusé, aucun chrono ; override historique exact conservé |
| qualification | sourcec40 :4073/4073,326 mutants, qualification définitive |
| paired | sourcec40 :81/81,94/94 ciblés, aucune matrice runtime substituée |

Même paquetc40 et cible **devpod-gpu-exploration/us-central1-c/ehgp-v7-3b1d496aed430749ea7e049f**.
G4standard48, SPOT/STOP, maxRun4200s et garde invitée55min. Arrêts ciblés certifiés
pour toutes les générations. Les deux définitives sont 08:57:18.161−07:00 et 09:18:25.458−07:00
le3octobre. GCC18/21/24, ASan24, TSan21, poison21 ; ASan18 num/index/tower344/344,
sans catalogue/FENV ; Clang absent. Les binaires du banc sont reconstruits avec
leurs propres provenance et portes ; aucune identité avec la première compilation.

Calendrier : 3 trames×W1/W8/W48×3 producteurs×3 prises, ordre des producteurs tournant.
Baseline **v11**895680ff866fbe41c450c87b2498ebff2ac7408b/2047, c40/2047 et c40/16379.
Même compilateur/flags/u21, XYZ et IDs hachés de reuse1 ; coordonnées préparées1mm
à étendueu18, poids unitaires. Chaque processus et propriétaire est neuf ; caches OS non vidés.
16379 combine graphe/populations/concurrence et retire mémo4 : pas d'ablation isolée.

FULL=index+domaine+forêts ; sol/grille, IO/Cloud/Pool, dumps et Python séparés.
Mur collecteur530,322s, construction baseline et Python inclus. Chaque dump est rehaché
et comparé octet pour octet avant retrait ; trois décodages initiaux,78 réemplois de résumé.
Les dumps255–329 Mo et binaires ne sont pas archivés : le lecteur recoupe les relevés,
pas un rehachage indépendant de fichiers absents. Pas81 oracles indépendants.
CPU seulement ; K10, GPU, points/tête, plusieurs séquences et multi-millions non qualifiés.
Le différentiel canonique v10/v11 entier reste ouvert : ces81 prises comparent deux v11.

## Relecture autonome

Depuis la racine du dépôt, avec `CAP=morsehgp3D_v11/receipts/qualification_performance_20261003` :

```bash
python3 morsehgp3D_v11/bench/verify_full_captures.py --qualification "$CAP/captures/qualification" --paired "$CAP/captures/paired" --source-package "$CAP/captures/sources/c40f40798/package.tar.gz" --out /tmp/v11-full-review.json
python3 "$CAP/derive_metrics.py" --archive "$CAP/captures/paired/results/results.tar.gz" --out /tmp/v11-full-metrics.json
python3 morsehgp3D_v11/tests/support/full_capture_reader_test.py
```

Rejouer avec `python3 -O` : deux rapports identiques, code0,55selftests par mode.
[Guide du lecteur](../../bench/FULL_CAPTURE_REVIEW.md). Les stdout/stderr sont dans review/ ;
SHA256SUMS couvre tous les fichiers, inventaires imbriqués inclus, sauf lui-même.
Chaque archive de résultats a son propre manifeste exhaustif vérifié.

Trois paquets source Git exacts sont partagés sous captures/sources/ (672,650,c40),
sans KITTI ni binaire ; export-ignore ciblé après sessions évite leur copie récursive.
La baseline 895 (428 blobs Git,743847 octets, SHA d9f8765c7284887d54bc248223fac3d00b9602aedbba1b8566e4727bdfbc8eef)
reste incluse dans les paquets futurs ; son manifeste détaille chaque blob, sans anciennes portes héritées.
Plans gardés dans bench/plans :1870 s de commandes qualification,1920s comparatif,
maxRun4200 ; modèles à instancier après qualification close au même pin.
protocol_validation.json conserve la validation initiale du protocole ; les72 contrôles
c40 sont rejoués dans la matrice G4. protocol_review/ conserve le replay factice81/81,
avec dépendances de harnais locales non fournies : trace de préparation, pas nouvelle
qualification native/LiDAR. Les premiers refus de préparation restent distincts du moteur.
