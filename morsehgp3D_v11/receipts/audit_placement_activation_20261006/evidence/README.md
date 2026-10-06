# Placement O1 — chaînes fermées et activation du plan

Capture locale de trois sessions fermées du 6 octobre 2026, non encore publiée par le développeur lors de cette lecture. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Le [résumé](summary.json) et le [rejeu stdlib](replay.py) conservent seulement identités, empreintes, verdicts et compteurs d’activation ; aucun calcul natif, build, appel cloud ou nouvelle campagne de l’auditeur.

| Session | Source figée | Arrêt ciblé certifié UTC | Portes natives conservées |
|---|---|---|---|
| `claudeo1place` | `86b3cbf14247f2cff523757ceb38168d365df82e` | 19:42:41.310 | 9 PASS ; `orders=960 pipelines=192 placements=0` |
| `claudeo1place2` | `9d10de21341fee138c15c3caf9cb30925e030cb9` | 19:58:51.810 | 9 PASS ; `orders=960 pipelines=192 placements=72` |
| `claudeo1place3` | même `9d10de213` | 20:08:53.245 | pas de campagne CTest dans ce plan |

Les trois sessions sont `completed`, worker/DONE 0, sans membre évincé, tronqué ou ignoré. Les plans, paquets, archives de résultats et leurs manifestes sont rehachés ; les sources utiles de chaque paquet sont comparées exactement à une archive Git selective du pin. Les deux rapports de chaque session utilisent un même binaire, nouvellement construit au premier appel du banc dans cette session. Aucun résultat sanitizer ou mutant nouveau n’est transféré.

La première session conserve le témoin de l’option perdue au déplacement et est explicitement exclue par le troisième plan comme comparaison A/A involontaire. Au pin corrigé, le move transporte `place_pipeline_` ; la porte d’équivalence vérifie `pipeline_placement_requested == placed.place_pipeline` sur tous ses appels et rapporte 72 appels avec `pipeline_placement_cores != 0`. Ce compteur atteste des **plans non nuls**, pas le succès de chaque appel `sched_setaffinity`. La porte d’affinité est un contrôle autonome distinct.

Les rapports LiDAR jouent K5/W48 sur les trois trames : CPU feuilles 16 avec masques 16379/278523, GPU+C feuilles 24 avec masques 81915/344059. `place3` ajoute un bras A/A de même masque que la base. Dans les deux premières sessions, chaque bras contient 30 processus froids et 6 chauds de 10 passes (90 appels FULL, 36 hashes) ; dans la troisième, 54 froids et 9 chauds (144 appels FULL, 63 hashes). Les sorties froides et dernières chaudes égalent les trois empreintes canoniques K5. Le verdict du juge épinglé impose aussi un registre catalogue présent et égal ; le rapport conserve son registre de référence, sans les registres individuels de chaque prise. Les passes intermédiaires conservent leur succès, sans dump intermédiaire.

**Limite d’activation LiDAR :** `full_probe.cpp` émet `full.pipeline_tasks.placement_cores`, mais `gpu_ab.py::take_summary()` supprime cet objet. Les rapports historiques ne permettent donc pas d’observer `placement_cores > 0` ni `pipeline_placement_requested` sur chacune des prises LiDAR `place2`/`place3`. L’activation du plan est observée sur la porte native `place2` ; sur les trames, seuls les masques demandés, le chemin de source et les identités sont conservés. Aucune statistique de temps, décision d’adoption ou qualification des nouveaux paramètres par défaut API/export du WIP n’est déduite ici.

Le rejeu dépend du Git local contenant les pins et des trois archives/session locales sous `/workspaces/.ehgp-sessions`. Il ne contient ni les archives volumineuses ni les données LiDAR, dumps, journaux bruts ou identités de compte ; ce n’est pas un reçu natif autonome.

```sh
sha256sum -c SHA256SUMS
python3 -B replay.py
python3 -B -O replay.py
```

Les modes normal et `-O` ont produit exactement le même résumé à la fermeture.
