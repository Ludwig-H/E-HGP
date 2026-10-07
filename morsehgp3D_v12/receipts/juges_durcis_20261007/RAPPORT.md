# Juges et pilotes des microbancs M2, M3, M4 durcis : rapport

7 octobre 2026, 11 h 50 à 13 h 05 UTC (heures lues avec `date -u`). Développeur v12, constats `CST-0018`, `CST-0213`,
`CST-0214`, `CST-0215` de l'auditeur Codex (reçu `audit_socle_microbancs_20261007`, pin `95247cf4b`). Copie de travail :
`microbancs/` (sources des microbancs identiques au pin audité et à `abe9df451`), changements dans
[`CHANGEMENTS.md`](CHANGEMENTS.md). Rien n'est écrit dans le dépôt ; aucune commande git qui écrit.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 (banc compilé sm_120, non joué)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé (aucune commande gcloud ; le plan G4 ci-dessous est validé hors ligne par la fonction du contrôleur)
```

## 1. Bilan

**Aucune adoption ne peut plus sortir d'un banc sans toutes ses preuves, présentes, fraîches et rattachées.** Les quatre
faux verdicts M2 de l'auditeur (identité hôte en échec, JSON périmés d'un autre vidage, aucun cas qui décide, durées
`NaN`), ses six vidages invalides, ses processus M3/M4 muets, son témoin de réplication et son carré sans verticales
rendent désormais « refusé » ou le code de refus attendu, là où ils rendaient « adopté » ou le code 0 ; ses contrôles
(refus d'arène et de banc, témoin complet adopté) gardent leur verdict.
Les règles d'adoption, les seuils et les statistiques sont inchangés : rejugées par le nouveau juge, les 45 prises réelles
de la session G4 A redonnent à l'octet (Python 3.10) les verdicts et intervalles publiés ; les sorties réelles de la
session B sont toutes admises par les nouveaux validateurs, sans faux refus. 22 mutants causaux des gardes ajoutées sont
tous tués par leur porte.

## 2. Tests joués et leurs codes

Constructions (`-Wall -Wextra -Wpedantic -Werror`, GCC 13.3 local ; `libmhgp11.a` `050532a9…`, la bibliothèque des
microbancs) : hôte M2 (six cibles, dont `mhgp12_dump_admission_selftest`) : 0 avertissement ; banc CUDA M2 (nvcc 12.9,
`sm_120`) : compilé, mêmes registres qu'avant (166 ; 178/168/128 ; 194/168/128), aucun avertissement hôte en
`-Wall -Wextra` ; M3/M4 (cinq cibles) : 0 avertissement. Portes Python jouées sous `python3 -S -O` avec **Python 3.10.21**
(celui de la VM est 3.10.12), et aussi 3.12.1.

| Porte | Commande | Résultat | Code |
| --- | --- | --- | ---: |
| admission native M2 | `mhgp12_dump_admission_selftest <dossier>` | vidage valide admis ; 46 mutants à empreinte juste refusés, chacun avec sa raison (dont les six de l'auditeur) | 0 |
| admission rejouée par les outils | `tests/test_admission_m2.py --binaires <hôte> --vidages-reels <vidages>` | `leaf_identity`, `mes_s`, `arena_selftest` : code 2 et zéro ligne de résultat sur les 46 mutants ; valide puis mutant dans une commande : code 2, aucun noyau ; neuf vidages réels admis | 0 |
| juge et pilote M2 | `tests/test_juge_m2.py` | 30 cas : 25 de bout en bout (témoin complet adopté ; 22 preuves manquantes refusées ; 2 mesures rejetées), 3 du juge pur, 2 sur le reçu G4 réel | 0 |
| preuves natives M4 | `tests/test_m4_preuves.py --binaire … --mutant …` | 10 cas sur le carré de l'auditeur | 0 |
| pilote M3/M4 | `tests/test_pilote.py --binaires <construction>` | 32 cas : 8 sur les sorties réelles G4, 22 injections et juge, 2 avec les binaires réels | 0 |
| mutants des juges | `outils/mutants_juges.py` | 22 mutants, 22 tués | 0 |
| bout en bout M3/M4 local | `pilote.py --cas ng00:5,u8000:5 --processus 5 tout` | toutes les preuves présentes ; 10 prises de résolution valides ; § 5 | 0 |
| bout en bout M2 local, sans GPU | `g4_leaf_bench.py --no-cuda --dumps <9 vidages réels>` | preuves hôte présentes ; refusé faute de GPU ; § 5 | 0 |
| règles de documentation | règles de `tools/check_docs.py` rejouées sur les trois README de la copie, liens résolus à leur place dans le dépôt | conforme | 0 |

## 3. Injections de l'auditeur, rejouées

### 3.1 MES-M2 : juge (`preuves/README.md`) et lecteur (`feuille/REPORT.md`)

| Injection de l'auditeur | Au pin `95247cf4b` | Maintenant | Mécanisme |
| --- | --- | --- | --- |
| identité hôte code 1, `identity=false` | adopté, choix `j3` | **refusé**, aucun choix | identité hôte exigée : code 0, une ligne par cas et forme, couverture, empreinte |
| JSON périmés d'un autre vidage (K10/16), banc de code 0 sans écriture | adopté | **refusé** (`fichier de prise absent`) | cible effacée avant chaque prise ; jeton de session ; chemin, empreinte et comptes du vidage |
| `--configs 5:16` : aucun cas qui décide | adopté, rapport `null` | **refusé** | contrat écrit ; cas qui décident exigés ; tout cas attendu avec ses N processus |
| durées `NaN` (juge) | adopté, rapport `NaN` | **refusé** (juge pur et prise) | durées finies strictement positives exigées |
| auto-test d'arène en échec ; banc de code 2 (contrôles) | refusé | refusé | inchangé |
| témoin complet (contrôle) | adopté | adopté, choix `j3` | inchangé |
| vidage `site_out_of_cloud` | admis par le lecteur | **refusé** (site hors du nuage) | `validate()` |
| `wrapped_job_begin` (`begin = 2^64-1`) | admis | **refusé** (pavage de la liste des sites) | `validate()` |
| `short_record_population` | admis | **refusé** (incidences au-delà de la plage) | `validate()` |
| `zero_k` | admis ; identité hôte code 0 | **refusé**, identité code 2, zéro ligne | en-tête : K dans 1..12 |
| `profile_mismatch` (u24 lu par un binaire u21) | admis ; identité code 0 | **refusé**, code 2 | en-tête : profil du binaire |
| `coordinate_out_of_profile` | admis ; identité code 0 | **refusé**, code 2 | coordonnées dans $[0,2^{B})$, boîtes dans $[0,2^{B}]$ |

Autres preuves manquantes jouées par la porte, toutes « refusé » : sanitizer introuvable, sanitizer en défaut, sanitizers
ou identité non joués, sans CUDA, isolation absente au début ou après le banc, moins de cinq processus, trames hors
contrat, jeton différent, empreinte différente, témoin faux, admission refusée (et alors aucun noyau joué), vidage d'un
autre cas, binaire modifié pendant la session, porte d'admission en échec. Mesures : identité du banc en défaut et borne
haute au-delà du tiers donnent « rejeté » (les preuves étant présentes).

### 3.2 MES-M3 et MES-M4 : pilote (`preuves/README.md`) et verticales (`tour/README.md`)

| Injection de l'auditeur | Au pin `95247cf4b` | Maintenant |
| --- | --- | --- |
| cinq processus M3 muets (code 0) | code 0, `routes_identiques_entre_processus = true` | **code 3**, routes jamais déclarées identiques, bloc non conforme |
| processus M4 muet (code 0) et mutant muet (code 1) | code 0, mutant « tué » | **code 3**, mutant **non** tué (aucune réponse géométrique) |
| portes jugées au seul code | conformes, mutants « tués » | portes muettes : `ECHEC` et `MUTANT_SURVIVANT` (code 1) |
| `--processus 5`, une seule commande de résolution ; seconde campagne qui écrase la première | 1 commande, rapport 0,1 remplacé par 0,2 | **5 commandes** par campagne, **deux campagnes conservées** (0,1 et 0,2), 10 journaux distincts, vidages des prises effacés |
| carré K1..4 : `FLOWER` correcte / fausse / absente (binaire M4) | 0 / 1 / **0** (zéro naissance jugée) | 0 (six jugées) / 1 / **3** (ligne `refus`) |
| succès sans verticales jugées (ancienne sortie, simulée dans le pilote) | conforme | **code 3** : 3 ordres par processus refusés (`LEM-T6`) |

Autres cas joués : prise de résolution aux vidages réécrits différents, à un ordre manquant, aux graines différentes ou
au temps non fini : prise refusée, campagne refusée ; binaire remplacé après `construire` : refus ; vidage sans `FLOWER` :
refusé par l'inventaire, MES-M4 jamais lancé ; vidage modifié entre `vider` et MES-M4 : refus sans lancement ; MES-M3 réel
sur le carré (zéro partie) : refus pour preuve vide. Sur le binaire M4 **d'origine**, une boule de cellule hors du
catalogue fait planter le banc (`SIGABRT`) ; AddressSanitizer situe l'écriture hors tableau à
`prev.ball_jtop_node[cells[j].ball]` (`mes_m4.cpp:779` au pin). Le binaire durci la refuse avant tout calcul (code 3).

## 4. Tests positifs sur des sorties réelles

**Reçu G4 `g4_t0a_20261007` (MES-M2).** Les 45 prises (9 cas × 5 processus) rejugées par le juge durci donnent les mêmes
verdicts, rapports et intervalles **à l'octet** sous Python 3.10 (`j3`, `j3_r168`, `j3_r128` adoptées, `coherent*`
rejetées ; choix `j3_r168`, moyenne géométrique 0,176389, pire borne haute 0,202689). Sous Python 3.12, la somme
compensée de `sum()` change le dernier bit des intervalles : mêmes verdicts, écarts inférieurs à $10^{-12}$ en relatif.
Ces prises anciennes ne sont pas des preuves d'une **nouvelle** session : sans jeton, l'admission les refuse.

**Reçu G4 `g4_t0b_20261007` (MES-M3, MES-M4).** Avec les inventaires et empreintes publiés (aucun vidage relu) : vidages
(ordres, graines identiques au journal v11, sortie ok), quatre portes, MES-M3 (5 processus à K5, 3 à K10), variante
« mère puis Welzl » (1 024 098 mères écartées par $S\subseteq F$ ; mutant tué par autant d'écarts de sphère), MES-M4
(LEM-T6 sur toutes les naissances des ordres 2..K : 857 891 par processus pour ng00 K5, 4 374 345 pour ng00 K10) et
mutant M4 : **tous admis**. La prise unique de résolution par cas K10 (0,542, 0,541, 0,547) donne au juge de MES-M3
« **refusé** » (1 prise valide sur 5 exigées) : `CST-0213` sur données réelles. Rejouer cinq fois la même prise (rejeu,
pas une mesure) donne « adopté » avec les mêmes moyennes : seule la réplication manquait.

**Vidages réels locaux**, identiques à l'octet à ceux des sessions G4 (neuf vidages M2 : sha256 du rapport de la session
A ; 96 fichiers M3/M4 : sha256 du rapport de la session B) : admission M2 des neuf vidages (5 s) ; identité hôte
inchangée sur ng00 K5/24 et K10/24 (j3 et cohérente, code 0) ; MES-M4 durci sur ng00 K5 et ng01 K10 : code 0, toutes les
naissances des ordres 2..K jugées (89 159 à 758 514 par ordre). Seuls des comptes et des empreintes sont publiés.

## 5. Bouts en bout locaux

**Pilote M3/M4, `tout`, ng00 K5 (réel) et u8000 K5 (uniforme), 5 processus**, 12:50:52 à 12:58:12 UTC, code **0** :
quatre portes conformes (mutants tués par `WIT-T1-CARRE` et par leurs écarts) ; vidages `MHGP11FUL1` identiques aux
empreintes de `MESURE.md` § 4 (ng00 K5 `3a2bfb4f…`, u8000 K5 `f87dbb19…`), onze fichiers inventoriés par cas, toutes
sections présentes ; **résolution : une campagne par cas, 5 prises valides sur 5**, chacune ayant réécrit les vidages à
l'identique de la référence avant de les effacer, statistique intra-processus « minimum de 3 passes » ; MES-M3 : 5
processus, identité et routes identiques ; variante : 1 024 098 mères écartées par $S\subseteq F$, mutant tué par
1 024 098 écarts de sphère ; MES-M4 : 5 processus conformes, `LEM-T6` sur toutes les naissances, mutant tué ; sources,
binaires et 42 dépendances compilées inchangés. Le verdict d'adoption de MES-M3 est « refusé » : les cas qui décident
(ng00–02 à K10) n'ont pas été joués en local, comme attendu. Les cas K5 sont publiés sans décision : rapports par
processus 1,031, 0,934, 0,887, 0,911, 0,951 sur ng00 (moyenne géométrique 0,942, IC 95 % [0,906 ; 0,986]) et 0,936 à
0,857 sur u8000 (0,894 [0,872 ; 0,916]). L'écart de 0,89 à 1,03 entre processus d'un même codespace chargé montre
pourquoi une prise unique ne peut pas porter un seuil ; ces temps locaux ne décident rien.

Un premier passage du même pilote (12:37 à 12:43) avait rendu le code **3** : j'avais modifié `README.md` et `pilote.py`
pendant l'invocation, et le contrôle de fin (sources hachées au début et à la fin) l'a refusée. Démonstration non
prévue, mais réelle, de la garde de provenance.

**Pilote M2 sans GPU (`--no-cuda`) sur les neuf vidages réels**, 12:59:18 à 13:01:40 UTC, code 0 (rapport écrit) :
porte d'admission (46 mutants refusés), admission C++ des neuf vidages avant tout noyau, identité hôte de code 0 (18
lignes : 9 cas × 2 formes, couverture et empreintes vérifiées), auto-test d'arène (aucun mutant vivant, vidage cité),
MES-S (9 lignes), 12 dépendances compilées relevées. Verdict « refusé » pour toutes les formes, pour les seules raisons
attendues hors G4 : banc CUDA non joué, isolation GPU non certifiable, cas qui décident sans prise.

Les sorties légères de ces passages sont dans `essais/` (publiées par `outils/publier.py`, vidages exclus).

## 6. Plan G4 (à lancer par le développeur principal, session gardée)

Fichier [`plan_g4_t0c_m3_m4.json`](plan_g4_t0c_m3_m4.json), sur le modèle de la session B (`--processus`, `{src}`,
`{build}`, `{data}`, `{out}`), validé hors ligne par `validate_plan` de `gcp-migration/v12_session.py` (neuf commandes ;
aucun appel GCP). Données : les 14 fichiers de la session B (`lidar_ng0*`, archive `v11_src_ac081a06f.tar.gz`).

| Commande | Étapes | Durée attendue sur G4 (d'après la session B) |
| --- | --- | ---: |
| `source_v11` | sources v11 épinglées, `libmhgp11.a` | 5 s |
| `m34_k5` | `pilote.py --cas ng00:5,ng01:5,ng02:5 --processus 5 tout` (dont 15 prises de résolution) | 7 à 8 min |
| `m34_k5_publier` | `publier.py --exclude-suffix .bin --exclude-suffix .ful1` | < 1 s |
| `m34_k10_base` | `--cas ng00:10,ng01:10,ng02:10 --processus 5 construire portes vider m4 rapport` | 3 min |
| `m34_k10_m3` | même sortie, `--processus 3 construire m3 rapport` | 9 à 10 min |
| `m34_k10_resolution_ng00` (puis `ng01`, `ng02`) | `--cas ngXX:10 --chrono-k10 1 --processus 5 construire resolution rapport` | 6,5 / 4,7 / 5,3 min |
| `m34_k10_publier` | `publier.py --exclude-suffix .bin --exclude-suffix .ful1` | < 1 s |

Total attendu : environ 37 minutes ; chaque commande sous 30 minutes. La dernière commande de résolution rend le verdict
de MES-M3 sur les trois cas (`rapport_mes_m3_m4.json`, `verdicts.mes_m3`, preuves citées) ; MES-M4 est jugé conforme ou
non avec ses preuves (`verdicts.mes_m4`), ses temps publiés et comparés aux seuils. Lancement (après intégration au dépôt) :

```bash
# 1. valider sans rien lancer (lectures GCP seulement)
python3 gcp-migration/v12_session.py --snapshot "$PWD" --plan plan_g4_t0c_m3_m4.json --data <dossier des 14 fichiers> \
  --session-dir /workspaces/.ehgp-sessions/v12.<date>.t0c --max-run-seconds 4800 \
  --zone us-central1-c --instance ehgp-v7-3b1d496aed430749ea7e049f
# 2. lancer : la même commande, plus --execute --wait ; arrêt certifié TERMINATED à vérifier au reçu
```

Le pilote M2 garde la même interface : la commande `m2` du plan de la session A rejoue M2 sous le juge durci.

## 7. Ce qui reste

1. **Rejouer sur G4** : le juge M2 durci n'a jamais vu de GPU (banc compilé, non joué) ; MES-M3 reste adoptée à titre
   provisoire jusqu'à la campagne répliquée ; MES-M4 à requalifier avec `LEM-T6` exigé. Les constats `CST-0213`,
   `CST-0214`, `CST-0215` passent au mieux « en cours » (code et portes locales) jusqu'au rejeu G4 et à la contre-lecture ;
   `CST-0018` voit ses trois pilotes durcis.
2. **Statistique de MES-M4 non écrite** : le pilote publie les médianes entre processus du minimum de R passes et les
   compare aux seuils (noyau 10 / 35 ms, contraction 3 ms), sans verdict d'adoption automatique ; écrire cette statistique
   (par exemple celle de MES-M2) est une décision de règle, hors de ce lot.
3. **Passes de résolution** : le plan garde une passe par processus à K10 (comme la session B) ; `MESURE.md` § 5 cite
   dix passes à chaud pour les étages, sans que la règle de MES-M3 fixe ce nombre. À trancher avant la campagne.
4. **Python de la VM** : les nombres du bootstrap sont ceux de Python 3.10 ; un interpréteur ≥ 3.12 change le dernier bit
   (somme compensée), jamais un verdict.
5. **Dépendances compilées** : relevées par les fichiers `.d` du compilateur (34 fichiers locaux pour le banc CUDA, 42 pour
   M3/M4), rehachées en fin de session ; les sources de `libmhgp11.a` restent couvertes par l'épingle de l'archive et
   l'empreinte de la bibliothèque. Si le générateur de la VM n'écrit pas de `.d`, le relevé est vide (noté, non refusé).
6. **Reçu de session** : `outils/recu_session.py` n'est pas modifié ; il devrait citer `evidence` (M2) et
   `verdicts.*.preuves` (M3/M4) dans le reçu publiable.
7. `--chrono-vidage` vaut `non` par défaut : l'étape `vider` seule ne mesure plus la résolution (prise informative sur
   demande) ; les prises qui décident viennent de l'étape `resolution`.
8. Intégration : copier `microbancs/` dans `morsehgp3D_v12/microbancs/`, joindre ce rapport à un reçu, mettre à jour
   `audits/CONSTATS.md` (`CST-0018`, `0213`, `0214`, `0215`) et la ligne de `PLAN.md` sur MES-M3 après G4.

Aucune donnée SemanticKITTI dans ce dossier : comptes et empreintes seulement. Les vidages locaux de l'essai ng00 (dérivés
de KITTI) ont été effacés après usage ; ceux de u8000 sont synthétiques.
