# Session G4 G-APP-2 : les propositions de l'étage G sur l'appareil — D2 et D1 rejetés

8 octobre 2026. Session gardée `v12.20261008.gapp2` (`gcp-migration/v12_session.py`, commit `9815c19b9`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 11:30:15
à 11:35:51 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 11:36:38 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit, témoin) ; cuda_g4 (noyaux du microbanc, hors produit)
objet=full_pi0 (étage G de la tour K1..5 : requêtes et résultats ; K10 en information)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Étape 2 de l'étude « G sur l'appareil » ([note](../developpement_20261008/etude_g_appareil_etape2.md)), après le
rejet de l'étape 1 ([session G-APP](../g4_gapp_20261008/README.md)) sur les propositions DWelzl en binaire64. Le banc
[`mes_g_appareil`](../../microbancs/mes_g_appareil/README.md) mesure deux conceptions, toutes deux à identité exacte des
issues :

- **D2** : voie entière (paires, triangles aigus, boule diamétrale), puis DWelzl en binaire32 sur une file compactée,
  sur l'appareil ; le binaire64 reste sur l'hôte. D2 aurait demandé l'amendement proposé de `CONTRAT_TOUR.md` § 8.
- **D1** : la voie entière suivie du binaire64, politique partagée par l'hôte et l'appareil, sans amendement.

| Commande | État | Durée |
| --- | --- | ---: |
| `g_app_autotest` | ok | 1,4 s |
| `g_app_pilote` | ok, verdicts rendus, aucun refus | 68 s |

## Verdict de `REGLE_G_APPAREIL_2` (écrite à 11:09 UTC, avant toute mesure) : D2 rejeté, D1 rejeté

Bornes hautes des IC 95 % (5 processus, moyenne géométrique ; seuils entre parenthèses) :

| Trame | D2 total (0,10) | D2 propositions (0,20) | D1 total (0,15) | D1 propositions (0,50) | D1 neutralité de l'hôte (1,05) |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 | 0,247 | 1,096 | 0,157 | 0,525 | 0,924 |
| médiane | 0,240 | 0,993 | 0,155 | 0,511 | 0,926 |
| maximum | 0,218 | 0,960 | **0,144** | 0,512 | 0,937 |

**Décision de la règle : aucune tranche « G sur l'appareil » sur cette base.** Les identités sont exactes partout
(issues de `l4` et `l4f32` égales à celles du binaire64 sur toutes les parties), sans requête non résolue. Les
replis restent dans la limite de la règle et les deux mutants sont tués (aucun refus).

Temps sur la trame maximale (ms, hôte à 48 fils → appareil) : census 21,2 → 1,9 ; sondes 16,6 → 0,8 ; propositions
binaire64 7,4 → 5,5 ; voie entière puis binaire64 (D1) 7,0 → 3,8 ; voie entière puis binaire32 (D2) 11,7 → 7,1.

## Lecture

- **D2 échoue nettement.** Sur l'appareil, le binaire32 coûte autant que le binaire64 de l'hôte. Les replis internes
  de DWelzl et le surcroît d'appels (2 à 3 fois plus) annulent son débit de ×64.
- **D1 échoue de peu.** Ses propositions restent à 0,51–0,52 de l'hôte, juste au-dessus du seuil de 0,50. Le total
  passe sur la trame maximale (0,144), pas sur ng00 ni sur la médiane (0,155 à 0,157).
- **Information sur l'hôte** : la voie entière rend les propositions 6 à 8 % plus rapides que le binaire64 seul
  (neutralité 0,92 à 0,94). La porte entière L4, rejetée dans T2-d-B parce que son effet sur tout G restait sous le
  seuil, a donc un effet mesurable sur les seules propositions.
- **À K10, sur ng00 (information, un processus)** : census 71,8 → 4,6 ms et sondes 41,6 → 2,3 ms sur l'appareil, mais
  propositions binaire64 31,2 → 26,8 ms. Les propositions dominent alors G sur l'appareil.

Le census et les sondes sur l'appareil restent acquis en information (×10 à ×20) ; seules les propositions barrent la
route. La réserve de l'étude, une énumération entière exacte sans flottant, n'est pas mesurée. Une étape 3 exigerait
une règle nouvelle, écrite d'avance.

## Ce que cette session n'établit pas

Ni un G complet sur l'appareil, ni l'énumération entière exacte, ni l'amendement du § 8 (sans objet, D2 étant rejeté).
GCP utilisé pour cette seule session, arrêt certifié.
