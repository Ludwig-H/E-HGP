# Relecture de MES-M6 : entiers stricts, effectif validé, schémas connus — rapport (lot 3)

7 octobre 2026. Développeur v12 ; résidus de `CST-0018` relevés par l'auditeur Codex (`audits/AUDIT_CODEX_20261007.md`,
puce « Juges, `0018` » ; reçu `receipts/audit_reprise_20261007/juges/`). Base : HEAD de `main` **`58761b36d`**
(`58761b36d69c83ff1893049cdfdc4b0061a34bdc`), copie vierge `_base/` et copie de travail `morsehgp3D_v12/` par
`git archive`, sans `receipts/` ; le lot 2 (`2b2113264`) y est intégré tel quel. Rien n'est écrit sous
`/workspaces/E-HGP` ; aucune commande git qui écrit ; au plus 3 fils.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

État : terminé (21 h 22 UTC).

## 1. Résidus reproduits (lecture du reçu de l'auditeur)

- **Identifiants non entiers** : la couverture des prises compare l'ensemble des clés `(mode, process)` avant tout
  typage ; `False == 0` et `0.0 == 0` la satisfont, puis la boucle de lecture saute l'entrée (`continue`) sans refus,
  et le nombre de prises validées n'est jamais comparé à l'attendu : `mes_m6_ok` avec 8 prises, ou 0 prise et aucun
  fichier quand tous les indices sont flottants et les médianes publiées vides. Même revue des types : `compile.code`
  et `runs[i].code` à `false` passaient pour 0.
- **Schéma inconnu** : seul `schema == v2` décidait ; toute autre valeur (`v999`, absente, mal typée) prenait la branche
  historique, sans les gardes v2 et sans respecter un refus publié.

## 2. Correction (`microbancs/mes_m6_session/run_m6.py`, relecture `--rejuger`)

- **Entiers stricts** : `is_int` exige `type(x) is int` (booléens et flottants refusés) ; `compile.code`, le code et le
  nombre de lignes de chaque prise, l'effectif et le nombre de processus passent par lui.
- **Identifiants typés avant toute comparaison** : pour chaque entrée de `runs`, mode connu et `process` entier strict
  dans `[0, processus)`, sinon refus (« prise d identifiant illisible ou hors plage ») — jamais un saut silencieux ;
  une entrée en double est refusée.
- **Effectif réellement validé** : après la lecture, l'ensemble des prises **validées** (lues, conformes) doit égaler
  exactement les `3 × processus` identifiants attendus ; sinon refus avec les absentes. L'ancienne couverture sur les
  clés déclarées non typées est retirée.
- **Liste fermée de schémas** : `ehgp.v12.mes_m6.v2`, ou `ehgp.v12.mes_m6.v1` (historique, celui de la session A) ;
  tout autre schéma (absent, inconnu, mal typé) est refusé, code 3. La branche v1 n'est plus un repli : un rapport v1
  doit porter exactement les champs du format historique (`compile`, `date_utc`, `gpu`, `gpu_apps`, `nvcc`, `runs`,
  `schema`, `uptime_since`), et chacune de ses prises exactement `code`, `lines`, `mode`, `process`, `seconds`,
  `stderr` : un refus publié (`refusals`) ou tout champ étranger fait refuser le rapport.

## 3. Injections gravées (`tests/test_juge_m6.py`) et comportement du pilote du HEAD

Chaque cas part d'une base conforme produite par le vrai `main` (rapport v2) ou d'une copie des sorties réelles de la
session A (rapport v1). Sonde locale `m6_origine.py` : le même cas relu par le `run_m6.py` du HEAD `58761b36d`.

| Cas | Pilote du HEAD | Pilote corrigé |
| --- | --- | --- |
| `auditeur_indice_false` (`runs[0].process = false`) | code 0, `mes_m6_ok`, 8 prises | code 3 |
| `auditeur_indice_float` (`runs[0].process = 0.0`) | code 0, 8 prises | code 3 |
| `auditeur_indices_float_sans_fichiers` (tous flottants, médianes vides, neuf fichiers effacés) | code 0, **0 prise** | code 3 |
| `auditeur_schema_inconnu_refus_explicite` (`v999` et un refus publié) | code 0, 9 prises | code 3 |
| `auditeur_code_compilation_false` | code 0 | code 3 |
| `auditeur_code_prise_false` | code 0 | code 3 |
| `schema_absent` | code 0 | code 3 |
| `v1_schema_inconnu` (session A, schéma `v0`) | code 0 | code 3 |
| `v1_avec_refus_publie` (session A, `refusals` ajouté) | code 0 | code 3 |
| `v1_prise_avec_champ_inconnu` (session A, `sha256` dans une prise) | code 0 | code 3 |
| `prise_retiree_du_rapport`, `prise_en_double`, `indice_hors_plage`, `effectif_flottant` (gardes seules) | code 3 (autres gardes) | code 3, par la garde visée |

Porte : **68 cas, 0 écart** (54 anciens, 14 nouveaux), sous Python 3.10.21 et 3.12.1, normal et `-O` ; sorties par cas
identiques dans les quatre modes (aux chemins temporaires près). Session A relue dans les quatre modes : `mes_m6_ok`,
9 prises, 585 lignes, schéma v1, trois limites déclarées.

## 4. Mutants des juges (`microbancs/outils/mutants_juges.py`)

Commande (Python 3.10.21, `-S -O`, 20 h 55 – 21 h 21 UTC, machine chargée par d'autres travaux) : mêmes reçus que le
lot 2 — `--recu-g4-m2 <g4_t0a>/…/004_m2_publier/files/m2 --recu-g4-tour <g4_t0b> --recu-g4-tour-d <g4_t0d>
--recu-g4-m5 <g4_t0c>/…/001_m5/files/m5 --recu-g4-m6 <g4_t0a>/…/002_m6/files/m6` et les binaires de
`mes_m3_m4_tour` construits au lot 2 (sources identiques au HEAD `58761b36d`).

Résultat : **8 témoins non mutés conformes** (m2, tour, m4, m5, m5_driver, m5_format, m5_unit, m6 ; code 0 chacun),
puis **90 mutants, 90 tués, aucun vivant** (code 0) ; par porte : m2 16, tour 14, m4 2, m5 19, m5_format 4,
m5_driver 1, m5_unit 1, m6 33. Les neuf nouveaux, un par garde :

| Mutant | Garde retirée | Tué par |
| --- | --- | --- |
| `m6_entiers_laxistes` | `is_int` strict (laxiste : `int` ou `float`) | `auditeur_indice_false` relu code 0 |
| `m6_rejuge_identifiants_non_types` | identifiant typé et dans la plage | `auditeur_indice_false` relu code 0 |
| `m6_rejuge_doublons_admis` | entrée en double | `prise_en_double` relu code 0 |
| `m6_rejuge_effectif_non_valide` | prises validées égales aux attendues | `prise_retiree_du_rapport` relu code 0 |
| `m6_rejuge_schema_ouvert` | liste fermée de schémas | `v1_schema_inconnu` relu code 0 |
| `m6_rejuge_v1_champs_libres` | champs exacts du rapport v1 | `v1_avec_refus_publie` relu code 0 |
| `m6_rejuge_v1_prises_libres` | champs exacts d'une prise v1 | `v1_prise_avec_champ_inconnu` relu code 0 |
| `m6_rejuge_code_compilation_laxiste` | code de compilation entier strict | `auditeur_code_compilation_false` relu code 0 |
| `m6_rejuge_code_de_prise_laxiste` | code de prise entier strict | `auditeur_code_prise_false` relu code 0 |

`m6_rejuge_code_ignore` (lot 2) est remis au nouveau code et reste tué (`code_non_nul`).

## 5. Portes jouées et codes

| Porte | Python | Résultat | Code |
| --- | --- | --- | ---: |
| `mes_m6_session/tests/test_juge_m6.py --recu-g4 <g4_t0a>/…/002_m6` | 3.10.21 normal, 3.10.21 `-O`, 3.12.1 normal, 3.12.1 `-O` | 68 cas, 0 écart dans chaque mode ; sorties par cas identiques | 0 |
| `run_m6.py --rejuger <g4_t0a>/…/002_m6` (session A, v1) | les quatre mêmes modes | `mes_m6_ok`, 9 prises, 585 lignes, 3 limites déclarées | 0 |
| `outils/mutants_juges.py` (sessions A à D) | 3.10.21 `-O` | 8 témoins conformes (portes m2, tour avec binaires réels, m4, m5, m5_driver, m5_format, m5_unit, m6), 90 mutants tués | 0 |
| `tools/check_style.py --root <copie>` | 3.12.1 et 3.10.21 `-S` | `style_ok fichiers=247` | 0 |
| règles de `tools/check_docs.py` (Markdown modifiés ou écrits) | 3.12.1 | 0 erreur | 0 |
| scripts Python de la copie | 3.10.21 `ast` | 123 lisibles, aucun `assert` | 0 |

## 6. Correctif

`../patch_lot3.diff` (sha256 `95543f3add29624fc15134b51ba21a1ae8fbd4494613ed5902bba66ec492d0cd`, 4 fichiers modifiés :
`microbancs/README.md`, `microbancs/mes_m6_session/run_m6.py`, `microbancs/mes_m6_session/tests/test_juge_m6.py`,
`microbancs/outils/mutants_juges.py`), base **HEAD de `main` `58761b36d`**, produit par `git diff --no-index` entre la
copie vierge et la copie de travail, sans `__pycache__` ni chemin local. Vérifié sur une extraction neuve
(`git archive 58761b36d morsehgp3D_v12`) : `git apply --check -p1` et `git apply -p1` de code 0, arbre obtenu identique
à la copie de travail, `patch -p1 --dry-run` de code 0. Application : à la racine du dépôt, `git apply patch_lot3.diff`.

## 7. Ce qui reste

- Le passage (`run`) de MES-M6 n'a pas changé : il écrit toujours un rapport v2 aux identifiants entiers ; seule la
  relecture est durcie. Rien n'est joué sur G4 (GCP non utilisé).
- Registre `audits/CONSTATS.md` non touché : à l'intégrateur. Le contrôle de l'auditeur `juges/check.py` (pin
  `f601b36ac`) attend encore code 0 sur ses cas `indice_false`, `indice_float`, `indices_float_sans_fichiers`,
  `schema_inconnu_refus_explicite`, `code_compilation_false`, `code_prise_false` : il échouera désormais, comme prévu.
