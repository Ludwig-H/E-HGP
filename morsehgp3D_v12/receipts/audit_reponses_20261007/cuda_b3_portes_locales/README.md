# B'' : contrelecture des trois campagnes locales

Lecture seule au main `4981b09cd`. Les 352 fichiers du perimetre du lanceur de
mutants dans `scratchpad/v12_gpu/frozenB3/morsehgp3D_v12` sont identiques au
commit local `3b445b763057adc405141a6650efa14bfd4e276a` de `repo99` (B'',
parent A'' `66c41ede...`). Leur empreinte agregee `eac084e2...` et celle du
manifeste `948e6388...` redonnent le compte rendu des mutants. Ce prototype
n'est pas une livraison sur main. Les sources, configurations et journaux
epingles sont dans `capture.json` ; aucun journal brut n'est recopie.

| Campagne | Traces concordantes | Portee exacte |
| --- | --- | --- |
| Mutants B'', annonce 23:13 | `mutB3/report.json`, code0, temoin vert, 15/15, manifeste integral | 14 par code ; `finition_prefixe_inclusif` par signal, donc pas 15 diagnostics geometriques. Aucun delai ni echec de construction. `g1_repere_enfant` demande le profil32, les autres le profil21 par defaut. |
| ASan/UBSan, annonce 23:24 | `asanB3/ctest_fast.log` et LastTest : 44 passes, zero saut parmi les tests selectionnes | Ensemble exact des 48 portes configurees moins les quatre portes LiDAR `long` ; les differentiels v11 ne sont pas configures. Les 44 incluent aussi des lecteurs Python, inventaires et controles de manifeste. |
| TSan, annonce 23:28 | `tsanB3/ctest.log` et LastTest : 12 passes, zero saut | Neuf groupes de `device_unit` **plus son inventaire**, determinisme catalogue et scale8000. Etat Pool resident a 1/3/8 fils ; catalogue CPU a 1/4/8. |

Les deux CMakeCache pointent bien `frozenB3`, GCC13.3, Release, profil21,
module `catalogue`, dependances `core,num,sched,cloud,io`, **CUDA=OFF**. Flags
de la bibliotheque et des trois executables controles, liens compris :
`-fsanitize=address,undefined -fno-sanitize-recover=all` ou `-fsanitize=thread`.
Chaque porte relue contient `run_expect_verdict conforme` : le wrapper exige
le code0 des unites/lecteurs, code2 pour l'usage volontairement invalide.
Aucun diagnostic sanitizer observe dans ces journaux. `setarch -R` est declare
dans RAPPORT ; la commande du parent CTest n'est pas conservee dans LastTest,
donc ce detail de lancement n'est pas independamment atteste par ces traces.

**Appareil :** dans les deux lots `device_open` passe avec un seul controle
du refus `device_unavailable`, puis retourne. Ce n'est pas un saut CTest,
mais la branche CUDA n'est pas jouee. Les autres groupes exercent les sources
partagees par l'executeur CPU Pool, pas l'ordonnancement ni la memoire GPU.

**Cache : precision au RAPPORT 23:24.** Le socle livre sous `7b7d025b3`, dont
le code d'empoisonnement ASan, est compile et instrumente. En revanche, les
22 constructions de MemoryBudget relevees dans les unites catalogue, les
tests device et la sonde n'ont qu'un argument ; `buffer.hpp:99` donne
`cache_bytes=0`. Aucun test du module core n'appartient aux 44/12 portes.
Ces campagnes ne requalifient donc pas le cache actif, ni l'empoisonnement
de ses blocs inactifs. Les preuves propres de CST-0007/0019, deja clos,
restent leur autorite ; aucun etat n'est modifie.

Relecture reproductible : `python check.py <scratchpad/v12_gpu>` ; meme sortie
sous `python -O`, conservee dans `results.json`. Le script controle les 47 artefacts epingles avant et apres lecture, puis
recoupe les configurations, flags, inventaires et resultats existants, sans lancer de
binaire v12. Aucun nouveau natif, CUDA, GCP, FULL ou chrono contractuel.
