# Juges M5/M6 : corrections confirmées, deux résidus M6 — 7 octobre 2026

**`CST-0018 reste en cours`.** Le lot `2b2113264` corrige les huit réserves éprouvées de M5 et les trois dernières
réserves M6. Mais le vrai `run_m6.main --rejuger` accepte encore un rapport v2 sans aucune prise lue, et un schéma
inconnu contourne les gardes v2. Ces témoins sont synthétiques ; aucune falsification d'une campagne G4 historique
n'en est déduite.

Pin : `f601b36ac` ; comparaison causale au parent de `2b2113264` (`5c5fc7109`). Cadre :
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

## Preuves et corrections acquises

[check.py](check.py) exécute les vrais pilotes sur les bases complètes des fabriques officielles, avec uniquement les
frontières externes simulées ; `subprocess.run` est interdit pendant ces probes. Les anciens juges sont pris par
`git show` au parent du correctif, sans modifier les sources du dépôt. Le script vérifie les empreintes du pin et
leur stabilité avant/après. [resultats.json](resultats.json) contient **31 observations**, identiques octet pour octet
en Python normal et `-O` ; les rapports volumineux restent temporaires.

| Vérification | Avant lot 2 | Après lot 2 |
| --- | --- | --- |
| M5, base complète | adopté | adopté |
| M5, huit mutations : missing malgré identité, digest différent, livres hôte absents, sanitizer sans mesures, livre GPU faux, compte de feuilles de référence faux, référence forgée, empreintes absentes | huit adoptions | huit refus |
| M6, provenance vide / isolation contradictoire / refus explicite conservé | trois `mes_m6_ok`, code 0 | trois refus, code 3 |
| Driver M5, porte native de l'ordre des gardes | 21 écarts sur 29 cas | 29 cas, zéro écart |

La porte native utilise le même petit `driver_selftest.cpp`, avec seulement `driver.hpp` substitué par son ancien
contenu pour le contrôle négatif : [check_driver.py](check_driver.py), [driver.json](driver.json). Deux compilations
mono, GCC, C++20, `-O1 -Wall -Wextra -Wpedantic -Werror`. Aucun grand nuage, CUDA, GCP ou sanitizer réel.

Les portes officielles passent aussi dans les deux modes : **M5 38 cas, M6 54 cas**, sans écart. Elles relisent les
captures historiques C et A dans leurs portées annoncées. [verification.json](verification.json) conserve les bilans
et empreintes des sorties ; cela ne remplace pas une nouvelle campagne du pilote durci sur GPU.

## Résidu 1 : des identifiants non entiers sautent la validation des prises

Base : rapport v2 valide, trois processus par mode, **neuf prises, 585 lignes**, médianes vérifiées. Les changements
suivants sont indépendants, depuis cette base :

| Mutation | Fichiers présents | Prises réellement lues | Résultat du vrai rejuge |
| --- | ---: | ---: | --- |
| Aucune | 9 | 9 / 585 lignes | `mes_m6_ok`, code 0 |
| Seulement `runs[0].process = false` | 9 | 8 / 520 lignes | `mes_m6_ok`, code 0, aucun refus |
| Seulement `runs[0].process = 0.0` | 9 | 8 / 520 lignes | identique |
| Tous les indices en flottants, `summary_median_us` à `{spin:{},yield:{},blocking:{}}`, suppression des neuf fichiers | **0** | **0 / 0 ligne** | **`mes_m6_ok`, code 0, aucun refus ni limite déclarée** |

Cause dans [run_m6.py](../../../microbancs/mes_m6_session/run_m6.py) : la comparaison de l'ensemble des clés a lieu
**avant** le typage. En Python, `False == 0` et `0.0 == 0` ; la couverture semble complète. La boucle suivante fait
`continue` sur un indice non entier, sans refus. Le nombre de prises validées n'est plus comparé au nombre attendu.
Les médianes constantes de la base rendent la perte d'une prise invisible au contrôle des résumés ; la version
zéro prise n'a même plus besoin d'un fichier de mesures.

Correction suggérée : valider le type exact et la plage de chaque clé avant sa mise en ensemble (`type(index) is int`,
`0 <= index < processes`, mode reconnu), refuser chaque entrée invalide, puis exiger **exactement `3 * processes`
prises validées**, et non seulement autant d'entrées déclarées. Garder les cas ci-dessus comme contrôles causaux.
Deux trous de typage supplémentaires sont reproduits : `compile.code=false` et `runs[0].code=false` sont acceptés
comme code zéro. Ils relèvent de la même revue des types, sans nouveau numéro de constat.

## Résidu 2 : tout schéma inconnu devient implicitement historique

La seule décision de version est `report.get('schema') == SCHEMA`. Un rapport v2 complet dont on change `schema` en
`ehgp.v12.mes_m6.v999`, tout en laissant `refusals=['binaire modifie pendant les prises']`, rend **code 0,
`mes_m6_ok`, neuf prises, aucun refus**. Le lecteur indique bien **trois limites historiques non rejouables** ; il
accepte néanmoins un format jamais déclaré historique et ignore le refus explicite du passage.

Correction suggérée : liste fermée des versions connues ; n'admettre le format sans schéma réellement utilisé par A
que par sa branche historique documentée, refuser les autres valeurs, et préserver les refus explicites lorsqu'ils
sont présents. Le mode historique ne doit pas être un repli automatique pour une version inconnue ou mal typée.

## Reproduire

Au pin indiqué, depuis la racine du dépôt :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/juges/check.py > /tmp/reprise-juges-normal.json
python3 -B -S -O morsehgp3D_v12/receipts/audit_reprise_20261007/juges/check.py > /tmp/reprise-juges-optimized.json
cmp /tmp/reprise-juges-normal.json /tmp/reprise-juges-optimized.json
python3 -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/juges/check_driver.py
python3 -B -S morsehgp3D_v12/microbancs/mes_m5_parcours/tests/test_juge_m5.py
python3 -B -S -O morsehgp3D_v12/microbancs/mes_m5_parcours/tests/test_juge_m5.py
python3 -B -S morsehgp3D_v12/microbancs/mes_m6_session/tests/test_juge_m6.py
python3 -B -S -O morsehgp3D_v12/microbancs/mes_m6_session/tests/test_juge_m6.py
```

La stabilisation M5 est confirmée **dans les cas exercés** ; les deux résidus M6 empêchent la clôture globale de
`CST-0018`. Cette note ne tranche pas les juges G1 ni les autres sous-portées suivies séparément.
