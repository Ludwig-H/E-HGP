# Relecture R2 — reprises ordinaires échelle/LiDAR

Campagne du 5 octobre 2026, reçue et relue le 6 octobre. Cadre :
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Les profils u18 et u24 sont explicitement distincts.

Session `v11.20261005.clauderepriser2`, source publiée
`98a00955083d483306c4f92b9031e382e81b0e59`. DONE=0,
`completed`, worker=0 ; clôture le 6 octobre à 00:14 UTC, arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés par SHA. Les 635 fichiers utiles livrés sont exactement identiques
à Git au pin. Les données ont été vérifiées côté worker ; aucune recopie ici.

| Configuration ordinaire | Profil | Lots gros + reste | PASS | Failed | Sans résultat |
|---|---|---:|---:|---:|---:|
| gcc_release | u18 | 52 + 66 | 118 | 0 | 0 |
| bits21 | u21 | 52 + 66 | 118 | 0 | 0 |
| bits24 | u24 | 52 + 66 | 118 | 0 | 0 |
| poison | u21 + poison | 52 + 66 | 118 | 0 | 0 |

Les huit inventaires sont uniques et sans porte désactivée. Les deux lots
sont disjoints dans chaque profil ; leur union de 118 noms est identique
dans les quatre profils. Les 472 verdicts CTest terminés sont PASS, avec
concordance des résultats et JUnit. Toutes les configurations terminent
avant leur budget ; aucune coupure ni porte manquante.

Les **41 anciennes absences de fina2** sont comparées nom par nom à son
archive close, puis aux verdicts R2 : 8/8 PASS en u18, 11/11 en u21,
11/11 en u24 et 11/11 en poison. Le résumé conserve les noms, leurs
verdicts explicites et les empreintes des inventaires/journaux anciens.
Aucun résultat sanitizer ne remplace ces verdicts ordinaires −O.

Les six `api_supports_route` — synthétiques 8k/16k/32k et trames
ng00/ng01/ng02 à K5 — ont un PASS explicite dans chacun des quatre profils,
soit **24/24**. Les douze occurrences u18/u24 auparavant rouges sont ainsi
requalifiées au pin corrigé. Les assertions natives et les préfixes
dépendant du profil restent épinglés dans les sources livrées ; le
complément indépendant du relecteur natif peut être joint séparément.

Portée R2 : portes ordinaires d’échelle/LiDAR sélectionnées par les huit
lots ; labels `long` et `mutant` exclus. Les 28 portes fonctionnelles L
ne sont pas rejouées ici. R1 court garde son reçu ; R3 mutants et R4
ASan/UBSan gardent leurs propres résultats. Cette capture ferme la
reprise des 41 occurrences et des routes ordinaires, sans anticiper R3/R4,
sans qualification globale ni contrat de temps de tour/GPU.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session`, `--baseline-session` et `--repo` permettent de déplacer les
dépendances locales. Le script contrôle fermeture, hashes, identité des
sources utiles, profils, inventaires et tous les verdicts, puis compare
le résultat au résumé figé. Il relit aussi les métadonnées de fina2 pour
établir exactement les 41 anciennes absences. Aucun build, test natif,
réseau ou accès cloud. Dépendances : archives locales R2 et fina2 et
commits Git ; cette capsule n’est pas un reçu autonome.

Aucun journal brut, sortie binaire native, identité de compte ou octet
LiDAR copié. Les journaux restent locaux ; seules leurs empreintes sont
conservées dans la capsule.
