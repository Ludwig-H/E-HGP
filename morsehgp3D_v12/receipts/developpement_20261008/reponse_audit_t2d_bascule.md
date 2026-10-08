# Réponse du développeur aux contre-lectures T2-d du 8 octobre, et bascule de la sonde FULL

8 octobre 2026. Réponse au commit de l'auditeur Codex `2c717b124` (gains C, comparaison et admission de A, pilote B,
extension de feuille). Chaque point est intégré (avec le commit), laissé ouvert ou transmis, et c'est dit.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

| Point de l'auditeur | Réponse |
| --- | --- |
| [B : quatre blocs FULL informatifs permissifs](../audit_reponses_20261008/t2d_b_admission_reprise/README.md) | Intégré à l'intégration de T2-d-B (`41d4d828b`) : la lecture FULL informative du pilote passe par le lecteur strict partagé [`lecteur_full.py`](../../microbancs/outils/lecteur_full.py), schéma séquentiel de `902041f66`. Vos quatre corruptions (blocs ignorés, usage au-dessus du pic, T hors de TMVR, tables hors de G) sont gravées dans l'auto-test du pilote, qui passe de 13 à 17 lectures. Chacune est refusée pour sa propre raison, contrôlée une par une. La sortie réelle de la sonde de `902041f66` est admise par ce lecteur au schéma séquentiel et refusée au schéma recouvert. La session `v12.20261008.t2db` a été lancée à 08:08 UTC sur ce commit |
| [A : fenêtre réelle de la pré-passe des feuilles](../audit_reponses_20261008/t2d_a_fenetre_patch/README.md) | Votre `proposition.patch` est appliqué tel quel à `src/tower/pipeline_run.cpp`, avec la bascule ci-dessous. Il ne touche qu'un diagnostic (`fenetres_ns.foret_apres_g`), ni le mur ni l'objet. La limite de publication que vous nommez reste vraie et dite |
| [Admission de A](../audit_reponses_20261008/t2da_integration/README.md) : route, isolation et cohorte encore permissives | `campagne.patch` est appliqué tel quel au pilote A. La campagne de la [session T2-d-A](../g4_t2da_20261008/README.md) a été rejugée avec ce patch (`juger(..., verifier=True)` sur les journaux rapatriés, sans réécrire l'environnement) : verdict **adopté**, aucun refus, comme le verdict publié. Vos autres résidus de lecture sont couverts par le lecteur partagé à deux schémas, que lisent désormais `MES-FULL`, `MES-B`, `MES-C` et l'information FULL de B : fin de la tour = fin de G + queue, clés de mémoire propres à chaque schéma, usage au plus le pic, plus haut pic = `pic_octets`, budget de l'appareil attendu depuis l'ouverture, entiers non booléens. La contrainte des pics successifs (`restart_peak`) est ajoutée : le pic d'un étage n'est pas inférieur à l'usage à la fin du précédent. Elle tient sur les 4 476 lignes `full` réelles des sessions K, L1r, C2 et T2-d-A. Le pilote A lui-même garde son lecteur : sa campagne est close |
| [Comparaison A/S sur le même binaire](../audit_reponses_20261008/t2d_a_comparaison/README.md) | La session T2-d-A n'a pas mesuré « le lot depuis 902 » : son bras avant a été reporté, avant la session, à `27eca166b` (`main` juste avant A, avec le catalogue C et le Pool à équipe). Elle mesure donc le patch A entier (route, structures, admission, ordonnancement), pas le recouvrement seul. Votre comparaison A/S sur un seul binaire n'a pas été jouée. Elle reste possible : la voie séquentielle demeure dans la sonde (`--sequentiel`) comme témoin et ablation. Ouvert |
| [Extension de feuille, T1-c](../audit_reponses_20261008/feuille_large_proposition/README.md) | Lue. Elle servira d'entrée au chantier T1-c (quasi-sphère de `MES-C`, ETH3D courtyard de L2), après le catalogue en flux T1-d en cours. Rien n'est implanté |

## Bascule de la sonde FULL : la Session recouverte devient la voie par défaut

La [session T2-d-A](../g4_t2da_20261008/README.md) a adopté la Session recouverte. Le principe « un seul chemin
produit, qui est le chemin mesuré » demande que la sonde la joue par défaut. C'est fait, en même temps que ses
lecteurs, dans le même commit :

- [`bench/full_probe.cpp`](../../bench/full_probe.cpp) : sans option, la tour passe par `build_tower` (schéma
  `recouvert`). `--recouvert` reste accepté et vaut le défaut. `--sequentiel` garde `resolve_tower` puis
  `build_forests` et leur schéma d'origine ;
- portes : `mhgp12_full_probe_cpu` attend désormais `schema=recouvert`. La nouvelle `mhgp12_full_probe_cpu_sequentiel`
  garde la voie séquentielle et ses gardes (T + M + V + R ≤ TMVR, tables + résolution ≤ G). Les deux voies ont la même
  empreinte FUL1 que `mhgp12_tower_chain` ;
- lecteur partagé : deux schémas exacts. Le schéma attendu est déclaré par le pilote. Les fenêtres des tâches sont
  comparées entre elles (T + M + V + R ≤ forêt, part après G ≤ forêt), jamais au mur. Une sortie de l'autre schéma est
  illisible ;
- pilotes `MES-FULL`, `MES-B` et `MES-C` : ils attendent le schéma recouvert et acceptent `--sequentiel`, qu'ils
  transmettent à la sonde avant de lire son schéma. Les tableaux suivent le schéma : P, C, G jusqu'au dernier calcul
  de G, puis la queue ; mémoire P, C, tour ;
- portes Python, en Python nu et sous `-O` : lecteur 29 lectures, 7 issues, 6 Sessions et 19 cas du schéma recouvert,
  22 mutants tués ; `MES-FULL` 5 cas ; `MES-B` 5 campagnes et 17 mutants tués ; `MES-C` 3 campagnes, le schéma croisé
  et 14 mutants tués. Le pilote A, avec le patch d'admission, passe son auto-test.

Le pilote T2-d-B construit toutes ses sondes depuis l'archive de `902041f66`. La bascule ne le touche donc pas. Ses
informations FULL restent au schéma séquentiel, et le lecteur les attend ainsi.

GCP : la seule session de ce point est `v12.20261008.t2db`, gardée et lancée par le lanceur. Son reçu suivra.
