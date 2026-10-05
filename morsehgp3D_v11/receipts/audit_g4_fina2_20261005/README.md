# Relecture de fina2 close — 5 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Les variantes u18 et u24 restent distinctes du défaut u21.

Session `v11.20261005.claudefina2`, source publiée
`38b76701b9b0198fc1c37afe16e1480e638e513c`. DONE=3,
`failed_remote`, worker=1 ; arrêt ciblé certifié. Les SHA du reçu, du plan,
du paquet et de l’archive sont dans `summary.json`. Une archive Git sélective
établit l’égalité exacte des 635 fichiers utiles du paquet (src, tests,
CMake, bench, reference, tools, cli et worker), sans copier ces sources.
Le reçu certifie aussi les données vérifiées côté worker ; les treize noms
incluent les entrées uniformes 8k/16k/32k et les trois trames sans sol.

| Configuration | PASS | Échecs explicites | Sans résultat | Sélection |
|---|---:|---:|---:|---:|
| gcc_release / u18 | 977 | 6 | 8 | 991 |
| bits21 | 890 | 0 | 11 | 901 |
| bits24 | 884 | 6 | 11 | 901 |
| poison / u21 | 891 | 0 | 11 | 902 |

Les quatre configurations atteignent leur budget de 2100 secondes ; les
processus CTest sont arrêtés par le runner. Les six échecs par profil u18/u24
sont les portes `api_supports_route` aux trois tailles et aux trois trames.
Ce sont des verdicts CTest terminés, distincts des portes sans résultat.
Leurs sorties détaillées ne sont pas conservées : `ctest.log` donne les
verdicts, tandis que `LastTest.log` contient seulement son en-tête. Cette
capture n’attribue donc pas de cause native précise à ces six échecs.

PASS dans les quatre profils : les huit `head_unit_*` dont `huge`,
`cli_plat` normal/−O, les douze portes `plat` d’échelle/LiDAR normal/−O,
`points_unit_sort_refusal`, `num_roots` normal/−O et les six `num_roots_cost`.
`api_supports_route_oracle` normal/−O passe partout ; les six portes route
échelle/LiDAR passent en u21 et poison. Les verdicts prioritaires exacts et
les listes complètes failed/missing sont figés dans le résumé.

Les quatre `head_vs_python` ne sont pas sélectionnées par cette campagne
ordinaire. Aucun transfert aux différentiels Python longs, à une campagne
sanitizer, à une mesure de temps de tour ou à une qualification globale close.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script vérifie les hashes, la fermeture ciblée, l’identité exacte des
sources utiles, les inventaires uniques et les verdicts terminés connus,
puis compare le résultat au résumé figé. Il ne lance aucun build, test
natif, réseau ou accès cloud. Il dépend des archives locales de la session
et du commit Git ; cette capsule n’est pas un reçu autonome.

Aucun octet LiDAR, sortie binaire native ni journal brut dans cette capsule.
Les lectures de journaux restent locales ; seules leurs empreintes sont publiées.

La [contrelecture de la porte supports_route](supports_route/README.md)
établit séparément un défaut de ses attentes communes aux trois profils :
le fichier et le manifeste incluent les bits. Elle conserve la distinction
entre cette preuve de source et les causes runtime absentes des journaux.
