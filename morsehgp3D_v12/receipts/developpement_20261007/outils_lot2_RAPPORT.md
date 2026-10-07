# Juges M5 et M6, diagnostic `stats` de M5, tables de découpes : rapport (lot 2)

7 octobre 2026. Développeur v12 ; suite des constats `CST-0018` (reçu
`receipts/audit_juges_emst_20261007/`, volets `juges` et `m5`) et `CST-0218` (données régénérées hors dépôt).
Base : HEAD `98ca07556` (copie vierge `_base/`, copie de travail `morsehgp3D_v12/`, par `git archive`, sans
`receipts/`). Rien n'est écrit sous `/workspaces/E-HGP` ; aucune commande git qui écrit ; au plus 3 fils.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 (bancs compilés, non joués)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

État : terminé (19 h 41 UTC).

## 1. MES-M5 : preuves incohérentes ou incomplètes, diagnostic `stats` (`CST-0018`)

Commencé à 19 h 06 UTC.

**Juge et pilote.** Une validation commune et typée (`comparison_problem`) s'applique aux lignes d'identité hôte, aux
prises GPU des tours, aux fixtures sur l'appareil et aux prises Compute Sanitizer, dans la session comme dans la
relecture (`--rejudge`) : identité booléenne, statut, grand livre aux cinq comptes, feuilles et feuilles de référence,
empreintes canoniques hexadécimales, compteurs `missing`, `extra`, `list_mismatch`, `meta_mismatch`, première
différence ; feuilles de référence égales à celles du vidage ; une identité déclarée vraie exige le statut et le grand
livre **du vidage** (en-tête, ou sortie publiée de l'outil de vidage à la relecture), autant de feuilles que la
référence, aucun écart et deux empreintes égales — la définition de `leaves_equal` de `compare.hpp`. L'empreinte de
référence d'une prise du banc doit égaler celle que l'outil d'identité hôte a lue dans le même vidage. Chaque série
`total_ms`, `resident_ms`, `wall_ms` d'une prise (sanitizer compris) compte exactement les répétitions de sa commande,
en valeurs finies positives.

| Porte | Résultat | Code |
| --- | --- | ---: |
| `tests/test_juge_m5.py` (`-S -O`, Python 3.10.21) | 38 cas, 0 écart (30 anciens, 8 nouveaux) | 0 |
| même porte, pilote du HEAD `98ca07556` | arrêt au premier écart : `auditeur_feuille_manquante_identite_vraie` adopté | 1 |
| `g4_traversal_bench.py --rejudge` sur la session C (`g4_t0c_20261007`) | « adopté », 60 tours, statistiques identiques à l'octet, aucun refus nouveau : toutes les sorties réelles portent les champs exigés et les empreintes croisées concordent | 0 |

Injections rejouées par le vrai `main` (sonde locale `m5_origine.py` sur le pilote du HEAD, puis porte durcie) :

| Cas | Pilote du HEAD | Pilote durci |
| --- | --- | --- |
| `auditeur_feuille_manquante_identite_vraie` (`missing=1`, identité vraie, six prises de `ng00_k5_l24`) | adopté | refusé (7 refus) |
| `auditeur_empreinte_differente_identite_vraie` (`digest` seul changé) | adopté | refusé (7) |
| `auditeur_grands_livres_hote_absents` (`ledger` et `reference_ledger` retirés des lignes d'identité) | adopté | refusé (43) |
| `auditeur_sanitizer_sans_mesures` (`total_ms=[]`, `resident_ms=[]` dans les trois prises) | adopté | refusé (3) |
| `grand_livre_gpu_different_identite_vraie` (un nœud de plus) | adopté | refusé (7) |
| `feuilles_de_reference_hors_vidage` (feuilles et référence égales entre elles, pas au vidage) | adopté | refusé (7) |
| `empreinte_de_reference_forgee` (`digest = reference_digest`, autre que celle de l'identité hôte) | adopté | refusé (7) |
| `empreintes_absentes_identite_vraie` (deux empreintes absentes, `None == None`) | adopté | refusé (7) |

La fabrique de la porte publie désormais tous les champs du vrai producteur (`traversal_bench.cu`,
`traversal_identity.cpp`) ; ses prises périmées sont complètes (seuls le jeton et l'effacement les écartent) ; une
exception du pilote devient un écart de la porte, jamais un succès muet.

**Diagnostic `stats` (observation du reçu `audit_juges_emst_20261007/m5`).** `Driver::run` réserve le diagnostic par
niveau avant la boucle, à une borne fixe indépendante des totaux (au plus `3 B + 1` niveaux) : entre la lecture des
totaux d'un niveau et ses gardes `wide_leaf` et capacité, plus aucune allocation, même sur l'hôte. Nouvelle porte
`host/driver_selftest.cpp` (cible `mhgp12_traversal_driver_selftest`), sur le modèle de la sonde de l'auditeur :
exécuteur factice, totaux fabriqués, opérateur `new` global compté.

| Porte | Résultat | Code |
| --- | --- | ---: |
| `mhgp12_traversal_driver_selftest` (GCC 13 et Clang 18, `-O2 -Wall -Wextra -Wpedantic -Werror`) | 29 cas : compteur vivant ; refus parents, tâches et feuille large aux profondeurs 0, 1, 2, 4, 8, 16, 32 sans réservation, noyau ni allocation après les totaux ; niveau admis : réservation suivante atteinte | 0 |
| même porte, `driver.hpp` du HEAD | 21 écarts : une allocation de l'hôte après les totaux à chaque refus (agrandissement de `stats`) | 1 |
| construction CMake hôte (`-j3`) | identité, lecteur, gardes : 0 avertissement ; lecteur 21 cas, 0 écart | 0 |
| identité hôte (`--unit --nodes --mutants all`, 3 fils) sur les six fixtures synthétiques | portes unitaires vraies, 6/6 identiques | 0 |

## 2. MES-M6 : relecture stricte d'un rapport v2 (`CST-0018`)

La relecture (`run_m6.py --rejuger`) d'un rapport v2 applique un schéma strict : champs obligatoires typés
(`date_utc`, `cleared`, `nvcc`, `gpu`, `gpu_apps`, `uptime_since`, `runs`, `refusals`, `verdict`), empreinte SHA-256
du binaire et des **deux** sources attendues, compilation de code 0, relevés d'isolation (début, avant et après chaque
prise) dont le booléen `quiet` vaut exactement « code 0 et aucune ligne de processus », aucun refus publié, prises
déclarées conformes sans problème publié, et médianes publiées égales à celles recalculées depuis les prises. Le
rapport v1 (historique, session A) garde sa relecture propre, ses limites déclarées non rejouables.

| Porte | Résultat | Code |
| --- | --- | ---: |
| `tests/test_juge_m6.py` (`-S -O`, Python 3.10.21) | 54 cas, 0 écart (44 anciens, 10 nouveaux) | 0 |
| relecture des sorties réelles de la session A (`g4_t0a_20261007`, rapport v1) | `mes_m6_ok`, 9 prises, 585 lignes, médianes égales à celles de l'auditeur, trois limites déclarées | 0 |

Rapports v2 falsifiés depuis une base conforme produite par le vrai `main` (sonde locale `m6_origine.py` sur le pilote
du HEAD, puis porte durcie) :

| Cas | Pilote du HEAD | Pilote durci |
| --- | --- | --- |
| `auditeur_provenance_vide` (`binary_sha256=''`, `sources_sha256={}`) | code 0, `mes_m6_ok` | code 3 |
| `auditeur_isolation_contredite` (`quiet=true`, code 9, un processus) | code 0 | code 3 |
| `auditeur_refus_explicite` (refus publié, verdict positif gardé) | code 0 | code 3 |
| `empreinte_du_binaire_vide`, `sources_vides`, `sources_incompletes` | code 0 | code 3 |
| `isolation_du_debut_contredite` (code -1, `quiet=true`) | code 0 | code 3 |
| `prise_declaree_non_conforme`, `medianes_publiees_fausses`, `champ_obligatoire_absent` | code 0 | code 3 |
| `code_non_nul`, `isolation_non_prouvee` (déjà refusés) | code 3 | code 3 |

La relecture ne prétend toujours ni recompiler le binaire ni établir sa stabilité historique : elle respecte ce que le
passage a publié (refus, problèmes) et recoupe ce qui se recoupe (empreintes des prises, médianes, cohérence des
relevés).

## 3. Tables de découpes (`CST-0218`, données régénérées hors dépôt)

Lus **en lecture seule** : les quatre manifestes `v12_donnees/data/<jeu>/manifest.json` et les quatre
`v12_donnees/bundles/g4_<jeu>/bundle_manifest.json` du rejeu du développeur (journal `preparer.log` : début 17 h 43,
découpes 17 h 52, fin 18 h 00 UTC ; `verify_inputs.py` : zéro écart sur chaque manifeste et chaque paquet). Aucune donnée
touchée ; aucune coordonnée reprise (ni centre ni translation : noms, tailles visées, comptes, retours, côtés des
carrés, bits et empreintes seulement). Générateur local : `_travail/tables_decoupes.py` (refuse un manifeste hors
schéma, une découpe incohérente `returns >= count >= target_sites > 0`, ou une découpe absente des tables).

| Jeu | Découpes | Sites distincts, réel moins visé | Retours couverts, nouveau moins ancien | Empreintes | Côtés, bits |
| --- | ---: | --- | --- | --- | --- |
| IGN LiDAR HD | 21 | +5 à +317 | +5 à +317 | 21/21 changées | inchangés |
| ETH3D | 15 | +109 à +3 586 | +109 à +3 917 | 15/15 changées | inchangés |
| FOR-instance | 16 | +65 à +6 287 | +71 à +6 322 | 16/16 changées | inchangés |
| Boreas | 17 | +2 à +308 | +2 à +308 | 17/17 changées | inchangés |

Les écarts égalent exactement ceux mesurés en lecture seule par le lot 1 avant le rejeu (IGN 5 à 317, ETH3D 109 à
3 586, FOR-instance 65 à 6 287, Boreas 2 à 308) ; les empreintes des 24 scènes entières sont inchangées.

`docs/DONNEES.md` : en-tête (découpes régénérées, plus « à rejouer »), § 3 (provenance des tables, colonnes « Taille
visée », « Sites distincts » réels, « Retours couverts », écarts par jeu), les quatre tables de découpes (69 lignes),
§ 7 (durée du rejeu complet du 7 octobre, 17 min téléchargements compris ; tailles des paquets) et § 9 (tailles) :

| Paquet | Avant | Après (octets de données du manifeste) |
| --- | --- | --- |
| `g4_ign_lidarhd` | 3,2 Go | 3,19 Go (3 185 391 160 o), 27 cas, 81 fichiers |
| `g4_eth3d` | 3,1 Go | 3,11 Go (3 114 613 940 o), 19 cas, 57 fichiers |
| `g4_forinstance` | 1,4 Go | 1,36 Go (1 362 182 340 o), 22 cas, 66 fichiers |
| `g4_boreas` | 1,7 Go | 1,73 Go (1 725 451 476 o), 25 cas, 73 fichiers |

Non touché : `docs/PLAN.md` (« attendent les outils de données corrigés ») et le registre `audits/CONSTATS.md`,
laissés à l'intégrateur.

## 4. Mutants des juges (`microbancs/outils/mutants_juges.py`)

Commande (Python 3.10.21, `-S -O`, 19 h 26 – 19 h 38 UTC) : `mutants_juges.py --recu-g4-m2 <g4_t0a>/…/004_m2_publier/files/m2
--recu-g4-tour <g4_t0b> --recu-g4-tour-d <g4_t0d> --recu-g4-m5 <g4_t0c>/…/001_m5/files/m5 --recu-g4-m6
<g4_t0a>/…/002_m6/files/m6 --binaires-tour <construction locale de mes_m3_m4_tour au HEAD, -j3, 0 avertissement>`.

Résultat : **8 témoins non mutés conformes** (m2, tour, m4, m5, m5_driver, m5_format, m5_unit, m6 ; code 0 chacun)
puis **81 mutants, 81 tués, aucun vivant** (code 0). Par porte : m2 16, tour 14, m4 2, m5 19, m5_format 4, m5_driver 1,
m5_unit 1, m6 24. Les 15 nouveaux et leur cas tueur :

| Mutant | Garde retirée | Tué par |
| --- | --- | --- |
| `m5_identite_sans_recoupement` | identité vraie ⇒ statut, grand livre, feuilles, écarts nuls, empreintes égales | `auditeur_feuille_manquante_identite_vraie` adopté |
| `m5_grand_livre_facultatif` | grand livre présent et typé | `auditeur_grands_livres_hote_absents` : exception du pilote (écart) |
| `m5_empreintes_non_typees` | empreintes présentes et hexadécimales | `empreintes_absentes_identite_vraie` : exception du pilote (écart) |
| `m5_feuilles_de_reference_libres` | feuilles de référence égales au vidage | `feuilles_de_reference_hors_vidage` adopté |
| `m5_empreinte_croisee_ignoree` | empreinte de référence égale à celle de l'identité hôte | `empreinte_de_reference_forgee` adopté |
| `m5_mesures_non_comptees` | effectifs des séries de mesures | `auditeur_sanitizer_sans_mesures` adopté |
| `m5_stats_alloue_avant_la_garde` (natif) | réservation d'avance du diagnostic par niveau | porte `driver_selftest` : 21 cas différents |
| `m6_rejuge_schema_ignore` | champs obligatoires typés | `champ_obligatoire_absent` relu code 0 |
| `m6_rejuge_binaire_non_type` | SHA-256 du binaire | `auditeur_provenance_vide` (motif absent) |
| `m6_rejuge_sources_non_typees` | SHA-256 des deux sources | `sources_vides` relu code 0 |
| `m6_rejuge_isolation_contredite_admise` | cohérence `quiet` / code / processus | `auditeur_isolation_contredite` relu code 0 |
| `m6_rejuge_isolation_du_debut_ignoree` | relevé du début | `isolation_du_debut_contredite` relu code 0 |
| `m6_rejuge_refus_publies_ignores` | refus publiés respectés | `auditeur_refus_explicite` relu code 0 |
| `m6_rejuge_prise_non_conforme_admise` | prise déclarée conforme sans problème | `prise_declaree_non_conforme` relu code 0 |
| `m6_rejuge_medianes_non_recoupees` | médianes publiées recalculées | `medianes_publiees_fausses` relu code 0 |

Deux mutants (`m5_grand_livre_facultatif`, `m5_empreintes_non_typees`) sont tués par une exception du pilote plutôt
que par une adoption : sans la garde de présence, la règle de cohérence lit un champ absent ; la porte compte toute
exception comme un écart, jamais comme un succès.

## 5. Portes jouées et codes

| Porte | Python ou compilateur | Résultat | Code |
| --- | --- | --- | ---: |
| `mes_m5_parcours/scripts/g4_traversal_bench.py --selftest-judge` | 3.10.21 et 3.12.1 | 31 scénarios, conforme | 0 |
| `mes_m5_parcours/tests/test_juge_m5.py --recu-g4 <g4_t0c>` | 3.10.21 et 3.12.1 | 38 cas | 0 |
| `g4_traversal_bench.py --rejudge <g4_t0c>` | 3.10.21 | « adopté », identique au publié à l'octet | 0 |
| `mhgp12_traversal_driver_selftest` | GCC 13.3 et Clang 18 | 29 cas | 0 |
| `mhgp12_traversal_format_selftest` | GCC 13.3 (CMake, `-j3`) | 21 cas | 0 |
| `mhgp12_traversal_identity --unit --nodes --mutants all` (six fixtures, 3 fils) | GCC 13.3 | portes unitaires vraies, 6/6 identiques | 0 |
| construction CUDA de `mes_m5_parcours` (nvcc 12.9, `sm_120`, `-j3`) | nvcc 12.9 | 0 avertissement, registres inchangés | 0 |
| `mes_m6_session/tests/test_juge_m6.py --recu-g4 <g4_t0a>/…/002_m6` | 3.10.21 et 3.12.1 | 54 cas | 0 |
| `mes_m2_feuille/tests/test_juge_m2.py --recu-g4 <g4_t0a>/…/004_m2_publier` | 3.10.21 et 3.12.1 | 40 cas | 0 |
| `mes_m3_m4_tour/tests/test_pilote.py --recu-g4 <g4_t0b> --recu-g4-d <g4_t0d> --binaires <HEAD>` | 3.10.21 et 3.12.1 | 41 cas, binaires réels | 0 |
| `outils/mutants_juges.py` (sessions A à D) | 3.10.21 | 8 témoins conformes, 81 mutants tués | 0 |
| `bench/data_cache_test.py` | 3.10.21 et 3.12.1 | 8 cas | 0 |
| `bench/donnees_test.py` | 3.12.1 (numpy) | 37 cas | 0 |
| `tools/check_style.py --root <copie>` | 3.12.1 et 3.10.21 `-S` | `style_ok fichiers=246` | 0 |
| règles de `tools/check_docs.py` sur les Markdown modifiés ou écrits | 3.12.1 | 0 erreur | 0 |
| scripts Python de la copie | 3.10.21 `ast` | 121 lisibles, aucun `assert` | 0 |

## 6. Correctif

`../patch_lot2.diff` (sha256 `9afe9116bfab498d89a581c50a238e8c4fa6be242c50f807c266b5617dfc55b7`, 11 fichiers : 10
modifiés, 1 nouveau ; chemins `a/morsehgp3D_v12/…` et `b/morsehgp3D_v12/…`), produit par `git diff --no-index` entre
la copie vierge du HEAD `98ca07556` et la copie de travail, sans `__pycache__`, sans chemin local ni coordonnée.
Vérifié sur une extraction neuve du HEAD (`git archive 98ca07556 morsehgp3D_v12`) : `git apply --check -p1` et
`git apply -p1` de code 0, arbre obtenu identique à la copie de travail, `patch -p1 --dry-run` de code 0.
Application : à la racine du dépôt, `git apply patch_lot2.diff`.

## 7. Ce qui reste

- **G4** : rien n'est joué sur GPU ni sur G4 (GCP non utilisé). Les pilotes de MES-M5 et MES-M6 durcis sont à jouer à la
  prochaine session ; la session C reste « adoptée » à la relecture, le rapport v1 de la session A reste relu avec ses
  limites déclarées.
- **Registre** : `audits/CONSTATS.md` non touché (canal de l'auditeur) ; à l'intégrateur : résidus `CST-0018` de M5 et
  M6 fermés en local, observation `stats` de M5 traitée, `CST-0218` données régénérées et tables publiées. Les
  contrôles de l'auditeur épinglés à `1f7642e1` refuseront les nouvelles sources (« différente du pin ») : attendu ;
  leurs témoins `gpu_missing_but_identical`, `gpu_wrong_digest_but_identical`, `host_no_ledgers`,
  `sanitizer_no_measurements` et les trois mutations M6 attendaient l'ancien comportement (« adopté », code 0).
- **Sous `--rejudge` de M5**, le grand livre des vidages de cas vient de la sortie publiée de l'outil de vidage
  (`summary`), celui des fixtures de leur `oracle_check` : relus, pas recalculés.
- `docs/PLAN.md` dit encore que `MES-E` et `MES-P` « attendent les outils de données corrigés » ; non modifié.
