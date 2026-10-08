# Microbanc MES-FULL : la tour FULL de la v12 en Session résidente sur G4

8 octobre 2026. Mesure du **contrat** de la v12 (FULL K1..5 en mémoire, verticales comprises, 100 ms sur G4 sur les
trames SemanticKITTI sans sol, plusieurs séquences, médiane et maximum), sur la frontière de mur proposée par
l'auditeur Codex ([`frontiere_full_proposee`](../../receipts/audit_reponses_20261008/frontiere_full_proposee/README.md))
et implantée par la sonde [`bench/full_probe.cpp`](../../bench/full_probe.cpp).

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R) ; bras CPU identifié
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_full.py`](pilote_full.py) (bibliothèque standard, Python 3.10 nu) construit la sonde au profil 21 avec
`MHGP12_ENABLE_CUDA=ON`, puis joue : ng00–02 à K5 sur l'appareil (5 processus × 10 passes, ordre tournant), à K10
(3 × 5), un bras CPU à K5 (3 × 5), et une Session qui enchaîne les 37 trames `v12set` (six séquences, 33 179 à
99 099 sites ; 5 processus, deux tours, le second fait foi). Toutes les passes portent l'empreinte FUL1 (hors du mur) :
une empreinte par trame et par K, identique sur toutes les passes, tous les processus et les deux voies, sinon refus.

**Verdict du contrat, écrit d'avance** : « tenu » si, sur ng00–02 et sur les 37 trames, la médiane et le maximum (sur
les trames, des maximums des médianes par processus) sont au plus 100 ms ; « non tenu » sinon ; « refusé » si un
contrôle manque. Aucune règle d'adoption : c'est la mesure du contrat, publiée telle quelle avec les étages P, C
(transferts compris), G et la queue.

**Voie jouée : la Session recouverte** (`T2-d-A`, adoptée par la
[session T2-d-A](../../receipts/g4_t2da_20261008/README.md)), voie par défaut de la sonde depuis la bascule du
8 octobre. Son mur se partage en P, C, G jusqu'au dernier calcul de G, puis la queue (la forêt qui n'a pas pu passer
sous G). L'option `--sequentiel` joue l'ancienne voie (`resolve_tower` puis `build_forests`, étages P, C, G, T, M, V,
R), transmet le drapeau à la sonde et lit son schéma.

Mode `--essai --sonde <binaire>` : logique jouée en local sur la voie CPU (minima relâchés, deux trames `v12set`),
verdict « essai », jamais publié comme mesure. Essai du 8 octobre : aucun refus, empreintes identiques.

**Lecture stricte et provenance (8 octobre, après la session K).** Chaque sortie de la sonde est lue par le lecteur
partagé avec `MES-B`, [`microbancs/outils/lecteur_full.py`](../outils/lecteur_full.py), qui exige :

- le schéma exact et des entiers u64 non booléens ;
- la séquence `open` / `full` / `liberation` / sortie ;
- à chaque passe, la trame et les sites attendus (taille du fichier / 12 ; la passe p joue la trame p modulo n) ;
- le budget de l'appareil « partagé », des étages inclus dans le mur, une mémoire par étage cohérente avec `pic_octets`
  et un mur non nul ;
- le schéma de la voie jouée : pour la Session recouverte, `etapes_schema` = `recouvert`, un raccord nul, des fenêtres
  des tâches cohérentes, la mémoire P, C, tour, une fin de G égale à l'étage G et une queue égale à la fin de la tour
  moins la fin de G, une fin par ordre (mêmes gardes que `tests/tower/full_probe_check.py`). Une sortie de l'autre
  schéma est illisible.

Un processus non conforme est un refus. La campagne exige un environnement complet et un GPU connu vide avant et après
(chaîne vide, jamais absente), comme le
[contre-lecteur de l'auditeur](../../receipts/audit_reponses_20261008/mes_full_contrelecture/README.md). Le rapport
publie le journal de construction, les empreintes SHA-256 de la sonde, du pilote et du lecteur, un extrait du
`CMakeCache` et le temps CPU médian par trame (`cpu_ns`).

**Portes** : [`test_pilote_full.py`](test_pilote_full.py) (sonde simulée : campagne d'essai conforme, sites faux sur
une trame refusés, empreinte instable refusée, `--sequentiel` transmis et lu, schéma croisé illisible) et
[`../outils/test_lecteur_full.py`](../outils/test_lecteur_full.py) (lecture, issues, Session à plusieurs trames, schéma
recouvert ; vingt-sept mutants du lecteur tués par
[`../outils/mutants_lecteur_full.py`](../outils/mutants_lecteur_full.py)).
