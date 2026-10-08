# Sessions G4 T1-d : catalogue en flux pour les scènes de plusieurs millions de sites

8 octobre 2026. Cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`, lanceur gardé
`gcp-migration/v12_session.py`, preuve `pushed_commit`. Reçus sans identité de compte, un dossier par session, chacun
avec son `receipt.json` et ses `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (voie CPU témoin)
objet=full_pi0 (catalogue Cat_K, K5 et K10 ; tour FULL pour l'empreinte FUL1)
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Levier.** T1-d, intégré sur `main` avant la mesure (`5f8e777cf`, parent `4171b2653`). Correctif de l'agent du chantier
C, pris tel quel ; il touche le module du catalogue seul, et le catalogue reste identique à l'octet. Il apporte :

- une arène en flux sur la voie appareil ;
- une fin d'étage par tranches de clés, coupées seulement entre voisins d'ordre F4 certain ;
- un choix de voie avec repli.

Juge : [`g4_catalogue_t1d_judge.py`](../../bench/g4_catalogue_t1d_judge.py), `REGLE_T1D` écrite avant toute session.
Bras avant : archive de `4171b2653` (SHA-256 `048a965d…30ee`).

## Session `v12.20261008.t1d` (commit `5f8e777cf`) : **refusée**, défaut d'outillage

VM de 14:20:55 à 14:30:43 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 14:31:51 UTC.
Commande unique `t1d_flux` : ok, 346 s. Détails dans [`t1d/`](t1d/), dont les
[tableaux](t1d/resultats/cmd/000_t1d_flux/files/t1d/tableaux_t1d.md).

**Pourquoi le refus.** Les 18 prises FUL1 sont refusées comme « ligne full hors schéma ». Le lecteur de la sonde FULL
de ce juge, [`g4_catalogue_flux_lecteur.py`](../../bench/g4_catalogue_flux_lecteur.py), a été repris du chantier
T2-d-C. Il attend le schéma séquentiel de `902041f66`. Or, depuis la bascule `86d7e39d8`, la sonde joue par défaut la
Session recouverte, dont la ligne `full` a d'autres clés. C'est un défaut de l'outil, pas du levier : la règle refuse
une mesure dont une preuve manque, elle ne la juge pas.

**Le reste de la session, en information** (sans verdict, puisque la session est refusée) :

- coût pour la trame, rapport après / avant sur le catalogue à K5, 10 tours × 10 passes : ng00 0,9995 (0,9959–1,0036),
  ng01 1,0031 (0,9994–1,0073), ng02 0,9987 (0,9934–1,0040), soit sous la borne de 1,01. A/A de 0,9988 à 0,9991 ;
- voie en flux sur l'appareil réel : 3 tranches à K5, 6 à 7 à K10 et 7 lots d'arène rapatriés à K10.

**Correctif** (commit suivant) : la sonde FULL de l'étape FUL1 reçoit `--sequentiel`. Elle reprend ainsi le schéma que
ce lecteur connaît. L'empreinte FUL1 est la même sur les deux voies : portes `mhgp12_full_probe_cpu` et
`mhgp12_full_probe_cpu_sequentiel`. Localement, la ligne `full` de la voie séquentielle a exactement les clés du
lecteur. La règle et les seuils ne changent pas. La session est rejouée sur le commit du correctif.
