# Outils de données, cache de la VM et juges des microbancs : rapport

7 octobre 2026. Développeur v12 ; constats de l'auditeur Codex `CST-0216`, `0217`, `0218`, `0220`, `0222`, `0223` et
résidus de `CST-0018` (juges de M2, M4, M5 et M6). Base : HEAD `df9140b5d` (copie vierge dans `_base/`, copie de
travail dans `morsehgp3D_v12/`, correctif `patch_outils.diff`). Rien n'est écrit sous `/workspaces/E-HGP` ; aucune
commande git qui écrit (lecture seule : `git archive`, `git diff`, `git show`).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 (bancs compilés, non joués)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

**Redémarrage du codespace (16 h 05 UTC).** Un premier passage (14 h 04 – 15 h 00 UTC, base `2f7b41380`) a été
effacé avec `/tmp`. Il est repris à 16 h 17 UTC sur le HEAD `df9140b5d` : entre les deux bases, seuls
`bench/index_io_test.py` et `microbancs/outils/recu_session.py` ont changé dans les zones touchées, et ce lot ne les
modifie pas ; les corrections du premier passage sont donc réappliquées à l'identique, puis rejouées. Les mesures
du premier passage qui dépendaient de données effacées (vidages réels refaits localement, données préparées) sont
signalées comme telles là où elles sont citées.

État : terminé (17 h 21 UTC).

## 1. Données et cache (`CST-0216`, `0217`, `0218`, `0220`)

| Porte | Commande | Résultat | Code |
| --- | --- | --- | ---: |
| outils de données | `python3 -S -O bench/donnees_test.py` (Python 3.12.1 du codespace, numpy 2.5.3) | 37 cas : 21 du vérificateur, 11 du pilote, 5 des découpes | 0 |
| même porte, Python 3.10.21 nu | les cas du vérificateur passent ; ceux qui lancent `crop_scenes.py` ou `verify_inputs.py --measure` s'arrêtent sur l'import de numpy, comme prévu (outils de préparation du codespace, jamais joués sur la VM ; la vérification sans `--measure` reste en bibliothèque standard) | 1 |
| même porte, outils d'origine | copie de `_base/` | 3 écarts : manifeste vide admis, `outils` inconnu (`scripts/` absent), colonne tranchée | 1 |
| rejeu à blanc | `bash bench/data/replay_all.sh outils`, depuis `/tmp` | 25 fichiers épinglés conformes | 0 |
| cache | `python3 -S -O bench/data_cache_test.py` (3.10.21 et 3.12.1) | 8 cas | 0 |
| même porte, cache d'origine | copie de `_base/` | 3 écarts : éviction puis refus, essai après le délai, lien via ancêtre symbolique | 1 |
| autotests existants du cache | `gcp-migration/v12_selftest.py -k test_data_cache` (disposition jetable) | 2 tests | 0 |

**Injections de l'auditeur rejouées.** Vérificateur : `cases=[]` → 2 (était 0), `sha256=null` → 2 (était 0), empreinte
fausse → 1 (inchangé). Pilote : étape `verify` depuis un autre répertoire courant → les outils sont trouvés (était 2,
« outil introuvable ») ; `ROOT` absent → 2. Découpe : quatre sites `(0,0,z)`, `--sizes 2` → aucune découpe (le carré de
rayon 0 contient les quatre sites, donc toute la scène ; était : IDs 0 et 1 gardés, rayon nul annoncé). Cache : lien
dur de six octets, espace libre modélisé nul → refus **avant** éviction, objet gardé (était : objet évincé puis refus) ;
délai dépassé, zéro relance → aucun essai (était : un essai) ; `--link` sous un ancêtre symbolique des résultats → 2
(était 0, lien posé).

**Découpes à rejouer (`CST-0218`).** Mesuré en lecture seule, avant le redémarrage (14 h 59 UTC), sur les données
préparées alors présentes (`v12_donnees/data/`, depuis effacées avec `/tmp`) : sur les **69** découpes publiées, les
**69** tronquaient leur anneau frontière ; la règle exacte leur ajoute de 2 à 6 287 sites (IGN 5 à 317, ETH3D 109 à
3 586, FOR-instance 65 à 6 287, Boreas 2 à 308) ; les rayons recalculés égalent les rayons publiés ; aucune ne couvre
sa scène. Changeront donc : les fichiers `<découpe>.u32le`, `.ids.u32le` et `.mult.u32le` (scènes avec doublons) des
69 découpes, leurs entrées `crops` dans les quatre manifestes multi-millions (`count`, empreintes, `extent_mm`,
`bits_needed`, `returns`, `crop`), les paquets `g4_ign_lidarhd`, `g4_eth3d`, `g4_forinstance`, `g4_boreas`
(`bundle_manifest.json`, `SHA256SUMS.txt`) et les tables de découpes du § 3 de `DONNEES.md`. Ne changent pas : les 24
scènes entières, SemanticKITTI (`manifest.json`, `v12set`, `g4_kitti_v12set`) et les petits nuages (`g4_small`). Les
données n'ont pas été régénérées : à faire par l'étape `crops` puis `bundles` du pilote.

**Admission stricte sur les manifestes réels** (premier passage, données alors présentes) : les 13 manifestes
préparés (six d'ensemble, `manifest_v12set.json`, six paquets) sont admis par l'admission stricte ; `g4_small`,
`g4_kitti_v12set`, `data/small` et `g4_forinstance` vérifiés en entier par le nouveau `verify_inputs.py` (Python
3.10.21 `-S`) : code 0, zéro écart.

## 2. MES-M5 : lecteur, émission des tâches, juge (`CST-0222`, `CST-0223`, `CST-0018`)

Réappliqué à l'identique après le redémarrage et rejoué de 16 h 17 à 16 h 33 UTC.

| Porte | Résultat | Code |
| --- | --- | ---: |
| `mhgp12_traversal_format_selftest` (lecteur strict) | 21 cas : 4 valides admis, les 4 références sémantiques de l'auditeur refusées (tests G1 impossibles, somme qui déborde, feuille non terminale, racine hors enveloppe), ses 6 corruptions déjà refusées le restent, `2^40` nœuds annoncés refusés avant allocation, 5 mutants des nouvelles règles refusés, `checked_add` exacte à la borne de `u64` | 0 |
| portes unitaires `--unit` (`CST-0222`) | huit comptes de 1 à `2^32 - 1` : nombre de tâches émis et scanné égal à l'attendu écrit en dur, `emit_tasks_u64` vrai ; aucun échec | 0 |
| vidages réels refaits localement (13 : `ng00`–`ng02` × 5:16, 5:24, 10:24, trois uniformes de 8 000, 16 000 et 32 000 sites, découpe de 4 000 sites) depuis les trames préparées du codespace (`build/v12-data-20261007`, lecture seule, aucun téléchargement) | 13/13 empreintes sha256 égales à celles publiées par la session C ; tous admis par le lecteur strict | 0 |
| identité hôte locale (`--mutants all --nodes --unit`, 13 vidages et 6 fixtures, 4 fils, 131 s) | 19/19 lignes identiques, mutants tués | 0 |
| construction CUDA locale (`sm_120`, `-Werror`) | 0 avertissement, mêmes registres par noyau qu'avant | 0 |
| `g4_traversal_bench.py --selftest-judge` | 31 scénarios du juge pur | 0 |
| `tests/test_juge_m5.py` (`-S -O`, Python 3.10.21 et 3.12.1) | 30 cas, 0 écart | 0 |

**Injections de l'auditeur rejouées** (reçu `audit_b_m5_20261007/juge_format`, `runner_probe.py`), par le vrai `main`
dans une copie jetable : grille réduite à un cas et un processus, cinq fixtures manquantes, identité hôte de code 2,
fixtures appareil de code 7, une seule passe v11, médiane GPU contraire aux durées brutes : **« refusé »** (étaient
« adopté ») ; contrôle lent et honnête : « rejeté » ; vidage erroné : « refusé ». Autres preuves manquantes, périmées ou
incohérentes (jeton, prise périmée, sanitizer sans prise ou en échec, isolation au début ou après les tours, porte du
lecteur, témoin `CST-0222`, binaire ou source modifié, manifeste des fixtures périmé, K faux, réservation chronométrée,
code 0 avec identité en défaut, sans CUDA, sans sanitizer, passes v11 hors contrat) : « refusé » ; identité appareil en
défaut : « rejeté ».

**Session C rejugée** (`--rejudge`, reçu `g4_t0c_20261007`) : verdict publié **« adopté »** retrouvé, 60 tours, pire
borne haute 0,105646, statistiques identiques à l'octet sous Python 3.10.21 (celui de la VM : 3.10.12) ; cinq preuves
qui n'existaient pas avant le durcissement sont déclarées non rejouables (jeton, porte du lecteur, isolation avant et
après les tours, rehachage final des sources), jamais supposées. Exigées comme preuves d'une **nouvelle** session, ces
prises anciennes sont refusées (129 refus).

## 3. MES-M2 : résidus de `CST-0018` (reçu `audit_cd_corrections_20261007/m2`)

| Porte | Résultat | Code |
| --- | --- | ---: |
| `tests/test_juge_m2.py` (`-S -O`, Python 3.10.21 et 3.12.1) | 40 cas, 0 écart | 0 |

Injections de l'auditeur rejouées par le vrai `main` : prises Compute Sanitizer **sans formes ni feuilles** →
« refusé » ; prise d'échauffement de **code 1 aux identités vraies** → « refusé » ; formes d'identité vraie aux
**compteurs de divergence non nuls et en débordement** → « refusé » (l'auditeur obtenait « adopté » pour les trois) ;
médiane déclarée contraire aux durées brutes → « rejeté » (le juge lit les durées brutes). Chaque garde seule : prise
Compute Sanitizer sans formes, à la couverture fausse, sanitizer de code 1 aux identités vraies, témoin faux dans les
seules prises du banc ou dans la seule prise d'échauffement → « refusé ».

**Session A rejugée** (reçu `g4_t0a_20261007`, 45 prises de 9 cas) : verdicts, rapports et intervalles publiés
retrouvés **à l'octet** (choix `j3_r168`, moyenne géométrique 0,176389, pire borne haute 0,202689) ; règles de
cohérence des compteurs appliquées aux 333 formes réelles (45 prises et trois prises Compute Sanitizer) : 0
contradiction ; couverture des prises Compute Sanitizer égale à `--sanitizer-leaves` et formes égales à la commande ;
ces prises anciennes refusées comme preuves d'une nouvelle session (sans jeton). Statistique et règle inchangées.

## 4. MES-M3 et MES-M4 : mutant rattaché à son cas (reçu `audit_cd_corrections_20261007/m34`)

| Porte | Résultat | Code |
| --- | --- | ---: |
| `tests/test_pilote.py --recu-g4 g4_t0b --recu-g4-d g4_t0d --binaires <construction locale>` (`-S -O`, Python 3.10.21 et 3.12.1) | 41 cas, 0 écart, binaires réels compris | 0 |

Binaires réels construits localement (`mhgp12_vidage`, `mhgp12_mes_m3`, `mhgp12_mes_m4` et leurs mutants, liés à
`libmhgp11.a` `050532a9…` de la v11 gelée, 0 avertissement). **Témoin de l'auditeur** (mutant simulé qui rend l'entrée
d'une autre trame à K1, un seul ordre, aucun compte, fin de code 1) : code **3**, ni tué ni complet, juge de MES-M4
« refusé » (avant : « tué et complet ») ; de même avec le **vrai** M4 normal sur le carré. Chaque garde seule : sortie
complète d'une autre trame, ordre 4 absent, naissances différentes du vidage → code 3. Mutant complet du carré : tué
(code 0) ; mutant complet sans écart : survit (code 1). Vrai M4 et vrai mutant sur le carré : conforme, six naissances
jugées par `LEM-T6`, mutant tué et complet.

**Session D rejugée** (reçu `g4_t0d_20261007`) : lot K10 → MES-M3 **« adopté »**, MES-M4 « conforme » ; lot K5 →
MES-M3 « refusé », MES-M4 « conforme » : verdicts publiés retrouvés ; statistiques publiées
de MES-M3 identiques à l'octet sous Python 3.10.21 ; moyennes de l'auditeur retrouvées à 9 décimales (ng00 0,545047666,
ng01 0,538794259, ng02 0,547584358) ; journaux réels des deux mutants M4 (`ng00`, K10 et K5) tués et complets pour le
validateur rattaché au cas ; 15 processus M4 normaux admis par lot. Le même rapport falsifié (mutant tué mais
incomplet, ou tué avec un refus) est refusé par le juge de MES-M4. Session B (partie A) : toutes ses sorties réelles
admises par les validateurs.

## 5. MES-M6 : pilote (`CST-0018`, complément de pilote du reçu `audit_session_t1_20261007/m6`)

| Porte | Résultat | Code |
| --- | --- | ---: |
| `tests/test_juge_m6.py` (`-S -O`, Python 3.10.21 et 3.12.1) | 44 cas, 0 écart | 0 |
| même porte, pilote d'origine | témoin de l'auditeur : `mes_m6_ok processus=9`, code 0 ; les autres parties échouent | 1 |
| `run_m6.py --rejuger` sur les sorties réelles de la session A | `mes_m6_ok` : 9 prises, 585 lignes, médianes égales à celles de l'auditeur | 0 |

**Témoin exact de l'auditeur** (`check.py` : nvcc simulé, toute capture remplacée par un processus de code 0 sans
sortie, vrai `main`) : **code 3**, `mes_m6_echec`, aucune prise jouée (binaire absent après la compilation) ; avant :
`mes_m6_ok`, code 0, neuf fichiers vides. Avec un binaire produit : neuf prises vides refusées, code 3. Corruptions d'une
seule prise (ligne absente, en double, en trop, remplacée par un doublon, non JSON, mode faux, effectif faux,
quantiles désordonnés, valeur infinie, négative ou `nan` du banc, champ en trop, premier usage renommé, prise
tronquée), code non nul à sortie complète, appareil différent, GPU occupé au début, avant ou après une prise, `nvidia-smi`
illisible, compilation en échec, binaire absent ou modifié pendant les prises, source modifiée : code 3. Sorties
périmées d'un passage antérieur : effacées avant tout. Contexte de la prise encore listé au seul premier relevé
d'après (fin de processus) : admis au second relevé, le relevé seul étant répété.

**Session A rejugée** (`002_m6`, code publié 0) : `mes_m6_ok` retrouvé, 9 prises, 585 lignes, médianes entre
processus égales à celles de l'auditeur (`audit_session_t1_20261007/m6/result.json`) ; déclarés non rejouables, car
absents du rapport v1 : effectif et nombre de processus (pris de la commande par défaut, `--reps 2000`,
`--processes 3`), empreintes du binaire, des sources et des prises, isolation par prise (la sortie de `nvidia-smi` au
début y est publiée sans son code). Relue avec `--reps 1000`, prise altérée, prise ou rapport absent : code 3. Le
pilote ne rend toujours aucun verdict d'adoption (M6 est un banc publié) et ne transforme pas ses quantiles en temps
FULL.

## 6. Mutants des juges (`microbancs/outils/mutants_juges.py`)

Commande (Python 3.10.21, `-S -O`, 17 h 09 – 17 h 19 UTC) : `mutants_juges.py --recu-g4-m2 <g4_t0a>/…/004_m2_publier/files/m2
--recu-g4-tour <g4_t0b> --recu-g4-tour-d <g4_t0d> --recu-g4-m5 <g4_t0c>/…/001_m5/files/m5 --recu-g4-m6
<g4_t0a>/…/002_m6/files/m6 --binaires-tour <construction locale de mes_m3_m4_tour>`.

Résultat : **7 témoins non mutés conformes** (portes m2, tour, m4, m5, m5_format, m5_unit, m6, toutes de code 0) et
**66 mutants, 66 tués, aucun vivant** (code 0). Par porte : m2 16, tour 14, m4 2, m5 13, m5_format 4, m5_unit 1,
m6 16. Les 22 mutants d'origine sont tous encore tués (`pilote_mutant_tue_par_code` remis à la nouvelle signature) ;
44 sont nouveaux (18 de MES-M5, 6 de MES-M2, 4 de MES-M4, 16 de MES-M6). Chacun des nouveaux est tué par le cas qui
le vise (par exemple `m2_compteurs_de_forme_ignores` par l'injection des compteurs contradictoires,
`pilote_mutant_d_une_autre_trame` par le journal complet d'une autre trame, `pilote_juge_m4_mutant_incomplet_admis`
par le rapport réel de la session D falsifié, `m6_binaire_non_exige` par le témoin exact de l'auditeur,
`m6_rejuge_empreintes_ignorees` par une prise v2 modifiée après coup).

**Garde doublée trouvée en route.** Un premier passage complet (17 h 01 – 17 h 08 UTC, 65 mutants) laissait vivant
`m2_temoin_faux_admis` : la nouvelle exigence d'un témoin identique à l'échauffement refusait déjà le scénario qui le
visait (témoin faux dans toutes les prises). Le scénario est scindé (témoin faux dans les seules prises du banc, puis
dans la seule prise d'échauffement), chaque garde a son mutant, et les deux sont tués. Les témoins non mutés, nouveaux
dans l'outil, empêchent l'inverse : un reçu absent ou un binaire manquant tuait jusqu'ici tous les mutants d'une porte
par vacuité.

Sans mutant, et dit : la somme contrôlée des tests G1 dans le lecteur de MES-M5 (la faire déborder sans violer la borne
par nœud exige plus de `10^8` nœuds) ; seule `checked_add` est jouée, à la borne de `u64`.

## 7. Correctif

`patch_outils.diff` (sha256 `e6d60e1d10f4e6e1e7d56b888dd95a44845f29077e8032f96a6803aea8eb1b48`, 29 fichiers : 24
modifiés, 5 nouveaux ; chemins `a/morsehgp3D_v12/…` et `b/morsehgp3D_v12/…`), produit par `git diff --no-index`
entre la copie vierge du HEAD `df9140b5d` et la copie de travail, sans `__pycache__` ni chemin local. Vérifié sur une
extraction neuve du HEAD (`git archive df9140b5d morsehgp3D_v12`) : `git apply --check -p1` et `git apply -p1` de
code 0, arbre obtenu identique à la copie de travail ; `patch -p1 --dry-run` de code 0. Application : à la racine du
dépôt, `git apply patch_outils.diff`.

Contrôles de la copie finale : `tools/check_style.py --root <copie>/morsehgp3D_v12` → `style_ok fichiers=215`
(Python 3.12.1 et 3.10.21 `-S`) ; règles de `tools/check_docs.py` rejouées sur les sept Markdown modifiés ou écrits
(liens vers les reçus résolus dans le dépôt) : 0 erreur ; 101 scripts Python lisibles par Python 3.10, aucune
instruction `assert` ; aucun `__pycache__`.

## 8. Ce qui reste

- **Données à rejouer** (`CST-0218`) : l'outil est corrigé, les données ne sont pas régénérées. Rejouer
  `ROOT=<v12_donnees> PY=<python avec numpy> bash morsehgp3D_v12/bench/data/replay_all.sh crops verify bundles`,
  republier les quatre manifestes multi-millions et les quatre paquets touchés, et remplacer les tables de découpes du
  § 3 de `DONNEES.md` ; les empreintes qui changent sont listées au § 1.
- **G4** : rien n'a été joué sur GPU ni sur G4 (GCP non utilisé). Les pilotes durcis de MES-M2, MES-M3/M4, MES-M5 et
  MES-M6 sont à rejouer sur G4 avant toute nouvelle adoption ; les sessions A, C et D ont été **relues**, pas
  rejouées, et ce que leurs rapports anciens ne contiennent pas est déclaré non rejouable, jamais supposé.
- **MES-M2, couverture partielle** : sous `--leaves`, le banc ne publie pas d'autre référence que la sienne pour un
  préfixe de feuilles ; le juge exige donc des comptes de référence bornés par ceux du vidage (pas égaux), et
  l'égalité forme/référence quand la forme est résolue.
- **Observation sans numéro du reçu `audit_session_t1_20261007/donnees`** : `prepare_outputs` (`v12data/common.py`)
  promet le plus petit ID source par site et garde le premier rang d'entrée. Non traitée ici (hors des constats
  demandés ; la corriger changerait les fichiers `.ids` des scènes à doublons dont les IDs ne croissent pas) : déclarer
  la précondition ou prendre le minimum, puis rejouer.
- **Registre** : `audits/CONSTATS.md` n'est pas touché (canal de l'auditeur). À mettre à jour par l'intégrateur dans
  le commit du correctif : `CST-0216`, `0217`, `0220`, `0222`, `0223` corrigés ; `CST-0218` corrigé dans l'outil,
  données à rejouer ; `CST-0018` : résidus de MES-M2, MES-M4, MES-M5 et MES-M6 fermés en local, à rejouer sur G4. Les
  contrôles de l'auditeur épinglés à `4147c546` refuseront les nouvelles sources (« différent du pin ») : attendu.
- **Nettoyage** : les vidages dérivés de SemanticKITTI refaits localement (1,1 Go) sont effacés ; aucune donnée
  SemanticKITTI dans le correctif ni dans ces documents (comptes et empreintes seulement).
