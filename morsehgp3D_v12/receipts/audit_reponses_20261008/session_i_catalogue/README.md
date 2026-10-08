# Session I : preuve du catalogue GPU — 8 octobre 2026

Le [reçu T1bi](../../g4_t1bi_20261008/README.md), publié en `0a5ebf29f`,
est contre-relu sans moteur ni appel cloud. **Verdict strict `adopte`
reproduit exactement**, sans refus ni rejet. Les [pins](pins.json) fixent
les objets Git ; [preuves.json](preuves.json) porte les résultats.

- 83 empreintes du manifeste vérifiées ; 57 résumés reconstruits depuis
  les journaux et identiques au rapport : 18 d’identité, 12 CPU, 24 GPU,
  3 mutants. Aucun journal ni rapport volumineux dupliqué ici.
- La session a empaqueté un `worktree_snapshot:d2f39fe82` (`receipt.commit`
  vaut null). Les 110 fichiers sous `src/`, CMake, la sonde, le test GPU et
  les trois lecteurs — 116 fichiers — égalent les objets Git de ce commit.
  Pilote/juge/schéma sont bien les octets stricts livrés en `781fbe8d1`.
- Sur chacun des neuf cas, une empreinte CPU et trois empreintes appareil
  sont égales entre elles et à F2. Les champs de chaque passe et les comptes
  physiques sont contrôlés par le juge. Il s’agit des empreintes canoniques
  émises par `--digest`, sans copie des exports complets dans ce reçu.
- GPU effectivement joué : journal `device_open`, neuf témoins,
  217 contrôles sans échec ; 665 portes rapides vertes. Les journaux
  d’isolation GPU avant et après les chronos sont vides et sans erreur.
  La voie reste hybride : reprises CPU K5 ng00/01/02 = 1/3/0 feuilles,
  K10 = 2/13/0 ; les réécritures restent comptées.
- Les deux mutants d’identité sont tués par les tests unitaires ; leurs
  empreintes ng00 restent égales à la référence. Le mutant « un fil par
  feuille » est tué selon son critère de temps. Ne pas attribuer aux neuf
  données réelles les échecs obtenus sur les témoins unitaires.
- Arrêt archivé : `closure=stopped`, arrêt ciblé certifié, code 0,
  `observed_after.status=TERMINATED`, dernier arrêt
  `2026-10-08T00:24:12.424Z`. Ce rejeu vérifie le certificat conservé ;
  il n’effectue pas une nouvelle observation distante.

Portée acquise : catalogue hybride u21, K5 sous la règle de cette cohorte ;
K10 informatif. Aucun transfert vers u24/u32, plusieurs séquences, le
multi-millions, la tour FULL ou le contrat de 100 ms. Les passes de temps
GPU n’émettent pas de digest : les empreintes appartiennent aux prises
séparées d’identité (trois passes appareil), avec comptes des passes de
temps rapprochés par le juge.

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/session_i_catalogue/replay_preuves.py --check
python3 -O -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/session_i_catalogue/replay_preuves.py --check
```

Rejeux normal/`-O` identiques. Les lectures passent par `git show` aux pins,
les trois modules Python sont importés depuis une copie temporaire.
`--repo-root CHEMIN` permet un autre checkout possédant ces objets.

Les temps ci-dessous sont aussi recalculés directement depuis **36 journaux,
315 passes**, sans utiliser les agrégats du juge (`replay_mesures.py --repo
/chemin/E-HGP --check`, normal et `-O`). La première passe de chaque processus
est exclue ; les sommes d'étages sont formées par passe avant leur médiane.

| Catalogue C, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| GPU K5, médiane / maximum, 45 passes chaudes | 35,066 / 36,924 | 30,817 / 32,302 | 37,576 / 38,514 |
| GPU K5, maximum des médianes par processus | 35,459 | 30,952 | 37,710 |
| GPU K10, médiane / maximum, 12 passes chaudes | 137,381 / 139,872 | 112,895 / 115,114 | 146,222 / 147,496 |
| CPU K5, feuille 16, médiane, neuf passes chaudes | 330,736 | 289,396 | 333,756 |
| CPU K5, feuille 24, même régime | 327,790 | 280,876 | 331,990 |
| CPU K10, feuille 24, même régime | 1 066,816 | 889,363 | 1 054,336 |

CPU : **un processus par cellule**, donc comparaison descriptive à F2, sans
attribution causale à A ou B séparément. GPU K5 : cinq processus ; K10 : trois.
Sur les 171 passes chaudes GPU, le compteur d'allocations appareil/épinglées
vaut zéro. Cela ne signifie pas absence d'allocations hôte : la sortie et les
niveaux exacts sont matérialisés à chaque appel.

Frontières vérifiées dans les sources : `wall_ns` encadre
`build_catalogue_device`, avec reprise CPU, copies et publication du catalogue
hôte. Préparation du nuage, ouverture du contexte, Pool, digest, impression et
destruction du catalogue retourné sont hors de cette durée. La première passe
n'inclut pas `open_ns`. La somme de deux médianes issues de H et I ne mesure
pas une latence FULL ; elle ne prouve pas non plus que chaque trame dépasse
100 ms. Le prochain juge de trame doit chronométrer directement P+C+G+T/M/V/R.

La lecture des transferts donne une piste secondaire concrète. Au profil u21,
les six tableaux finaux transportent
`44 C + 4 I + 48 (L−1) + 8 n + 16` octets, avec C boules, I incidences,
L niveaux (zéro compris), n sites. Cela représente 108,7–134,6 Mio à K5 et
507,9–637,6 Mio à K10. Le poste niveaux en constitue 36–40 % ; aucune réduction
de précision n'est suggérée. Ces comptes décrivent le format courant, pas une
borne incompressible ni un débit PCIe isolé.

Sur ng02, réparer **9 éléments à K5 / 16 à K10** fait aussi rapatrier les
trois tableaux complets `verdict`, `order`, `keys`, soit **22 526 160 /
87 733 120 octets** (`16 C`). C'est visible dans le code `finish_repair` et
réconcilié aux octets D2H de toutes les passes. Compacter sur l'appareil les
chaînes incertaines contenant une inversion, puis ne rapatrier que ces chaînes,
pourrait supprimer ce trafic. Il faut garder les chaînes entières et le même
départage exact, compter le travail du compactage et le juger sur G4 ; aucun
gain en millisecondes n'est acquis. **Priorité actuelle : G**, C ayant tenu
son budget déclaré.
