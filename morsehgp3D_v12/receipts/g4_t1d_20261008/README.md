# Sessions G4 T1-d : catalogue en flux pour les scènes de plusieurs millions de sites — adopté

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

## Session `v12.20261008.t1d2` (commit `c31beaf22`, correctif de l'étape FUL1) : **T1-d adopté**

VM de 14:35:10 à 14:45:17 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 14:45:38 UTC.
Commande unique `t1d_flux` : ok, 349 s. Détails dans [`t1d2/`](t1d2/), dont les
[tableaux](t1d2/resultats/cmd/000_t1d_flux/files/t1d/tableaux_t1d.md). Verdict de `REGLE_T1D` : **adopté**, aucun
refus, aucun rejet.

- **Identités** : les empreintes sont identiques partout. Voie appareil = voie CPU = F2 ; voie en flux sous chaque
  budget de l'appareil ; FUL1 avant = après = session K ; portes `device_open` et `device_open_budget` jouées sur
  l'appareil.
- **Coût pour la trame** (catalogue K5, rapport après / avant, 10 tours × 10 passes) : ng00 0,9998 (0,9963–1,0034),
  ng01 1,0024 (0,9988–1,0063), ng02 1,0024 (0,9970–1,0082). Toutes les bornes sont sous 1,01 : le contrat ne paie rien.
  A/A de 1,0002 à 1,0022.
- **Voie en flux sur l'appareil réel** : à K5, 3 tranches ; à K10, 6 à 7 tranches et 7 lots d'arène rapatriés un à
  un. Sur les 36 prises sous budget de l'appareil (de 1/2 à 1/32 de ses octets), 12 rendent F2 et 24 refusent
  proprement (`memory_budget`) ; aucune autre issue.

**Conséquence.** T1-d reste dans le produit (`5f8e777cf`). Le régime (b), scènes LiDAR entières de plusieurs
millions de sites, se rejoue sur ce produit (plans `MES-B` L1 et L2 de l'agent du chantier C). La session L1p
refusait au budget de l'appareil toute scène de plus de 5 M de sites à K5, et K10 dès 1,5 M.

**Juge durci après coup.** Le port `port_c31.patch` de l'auditeur
([admission T1-d](../audit_reponses_20261008/t1d_admission/README.md)) a été appliqué tel quel. Il ajoute dix
contrôles d'admission : mémoire, cohorte, refus et types. Son auto-test passe 36 injections. Le rapport réel de
`t1d2`, rejugé par ce juge, reste **adopté**, sans refus ni rejet. Celui de `t1d` reste refusé, avec 18 prises FUL1.

GCP utilisé pour ces deux sessions seulement, arrêts certifiés.
