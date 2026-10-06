# Relecture R3 — campagne complète de mutants

Campagne du 5 octobre 2026, close le 6 octobre à 00:47:47 UTC. Cadre :
`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

Session `v11.20261005.clauderepriser3`, source publiée
`98a00955083d483306c4f92b9031e382e81b0e59`. DONE=0,
`completed`, worker=0 ; arrêt ciblé certifié. Reçu, plan, paquet et archive
vérifiés par SHA. Les 635 fichiers utiles livrés sont exactement identiques
à Git au pin. Les données sont vérifiées côté worker, sans recopie ici.

**39/39 CTests PASS**, zéro échec, manquant ou coupure : 13 campagnes
individuelles et 26 validations de manifestes normal/−O. Les inventaires,
verdicts CTest et JUnit concordent. Ce nombre de CTests ne sert pas à
inférer le nombre de mutants jugés.

Les 485 verdicts individuels sont directement lus dans les sections de
`LastTest.log`, liés aux IDs et empreintes des 13 manifestes livrés :

| Catégorie observée | Nombre |
|---|---:|
| Déclarations / jugements / TUE | 485 / 485 / 485 |
| Rejet par code de la porte | 479 |
| Écart de ligne attendue | 4 |
| Refus de construction explicitement attendu | 2 |
| Survivant / invalide / non jugé | 0 / 0 / 0 |
| Signal / délai | 0 / 0 |

Les deux refus de construction sont prévus par le manifeste core ; les
483 autres mutants sont rejetés après compilation. Aucun refus de
compilation accidentel n’est pris pour une détection causale. Toutes les
références non mutées sont acceptées avant les jugements, selon le
contrôle du runner épinglé. Les treize résumés individuels et planchers
concordent avec les lignes conservées. Aucun rapport individuel
`mutants_<module>.json` n’est présent dans l’archive ; les catégories
observées viennent du journal, dont seule l’empreinte est copiée.

Les anciens blocages sont requalifiés : **API 23/23** et **CLI 28/28**,
tous tués par code, sans référence rouge. Les mutants API
`provenance_tailles_ignorees`, `provenance_budget_nul_admis` et
`voie_supports_order_tree` sont directement observés TUE ;
`sp_masque_16379` CLI est aussi observé TUE après son raccord à la porte
points. Le complément du relecteur natif peut être joint séparément pour
les portes témoins, la calibration et la cause de ce raccord.

Portée exacte : **campagne complète, base u18, profils locaux déclarés**.
Les options locales sont ajoutées après les options de base aux témoins
et copies mutées par le runner épinglé : num force 15 cas u21 et 16 u24 ;
`export_points_trois_mots` de tower force u24 ; `poison_efface` de core
active POISON. Les déclarations effectives comptent ainsi 453 u18, 15 u21
et 17 u24. Ces nombres dérivent des manifestes et de l’ordre des options,
sans prétendre conserver les flags de compilation de chaque copie.

R3 ne transforme pas ces options locales en qualification des six
anciennes campagnes L/u21 sans résultat. R1/R2 ordinaires gardent leurs
reçus et R4 ASan/UBSan garde ses propres résultats. Aucun transfert
sanitizer, contrat de temps de tour/GPU ou qualification globale.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script contrôle fermeture, hashes, sources utiles, inventaires et
verdicts ; il reclassifie tous les individus, vérifie leurs manifestes
et planchers, puis compare le résultat au résumé figé. Aucun build,
test natif, réseau ou accès cloud. Dépendances : archives locales de R3
et commit Git ; cette capsule n’est pas un reçu autonome.

Aucun journal brut, sortie binaire native, identité de compte ou octet
LiDAR copié. Les journaux restent locaux ; seules leurs empreintes et
les classifications des mutants sont conservées.
