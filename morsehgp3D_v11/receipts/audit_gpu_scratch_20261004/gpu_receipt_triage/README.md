Instantané de triage du dossier **non versionné** `gpu_g4/`, capturé le 2026-10-04T16:46:13.257546+00:00. Les 26 JSON consultés sont copiés sous `metadata/`, avec empreintes avant/après. Aucun README n'était présent. Ce reçu privé ferme cet instantané, jamais le dossier du développeur, ni l'état présent d'une VM. Aucun nuage, dump binaire, archive source/résultat, profiler binaire, native, GPU ou GCP n'a été lu ou lancé.

Les cinq receipts historiques déclarent un arrêt ciblé certifié, `closure=stopped` et un état observé après arrêt `TERMINATED`. gpu1/gpu2 sont `failed_remote` aux pins77db/d5 ; gpu3/gpu4/gpu5 sont `completed` aux pins **00800dd88**, **b74f9ea3a**, **16b482169**. Les cinq plans locaux ont le SHA déclaré par leur receipt. Les archives sont déclarées vérifiées par le contrôleur, mais n'ont pas été rouvertes ici. Les pins et SHA complets sont dans `derived_normal.json`, ainsi que les empreintes des binaires enregistrées par le banc.

Les neuf rapports complets de gpu3–5 contiennent **300 dumps froids + 72 derniers dumps chauds**, ainsi que 384 événements de passes chaudes. Dans les JSON copiés, les codes/statuts finaux sont bons, les hashes enregistrés égalent leur référence par trame, et toutes les séquences chaudes sont exactement1..P avec statut `ok`. Ces résultats peuvent servir de diagnostics comparés aux pins indiqués. L'égalité des registres est un contrôle du banc producteur ; le rapport conserve son registre de référence, pas chaque registre natif de chaque prise. Les compteurs records/population de lot correspondent aux émissions/incidences de référence. Cela ne remplace pas une qualification des cas numériques, limites, mutations, ni une vérification nouvelle des dumps binaires. Les plans lus demandent mesures et profiling, aucune suite native de qualification.

**Dernier lot gpu5, source16b482169, même binaire SHA65f6b83c99fefa7e850cb2f2825074b00944397f776309938274832565907baa.** Médianes FULL en ms, colonnes CPU / GPU, W48 : froid7 processus par bras à K5, froid5 à K10 ; chaud un processus par bras avec médiane des passes2..8 ou2..6. Ces passes dans un processus ne sont pas des répétitions froides indépendantes.

| K / feuille | Trame | Froid CPU / GPU | Chaud CPU / GPU |
| --- | --- | ---: | ---: |
| 5 / 16 | ng00 | 368.6 / 451.7 | 371.5 / 422.0 |
| 5 / 16 | ng01 | 294.1 / 380.4 | 278.4 / 346.6 |
| 5 / 16 | ng02 | 374.7 / 436.5 | 336.8 / 387.1 |
| 10 / 24 | ng00 | 2489.9 / 2401.1 | 2454.0 / 2373.7 |
| 10 / 24 | ng01 | 1896.8 / 1796.7 | 1816.0 / 1778.3 |
| 10 / 24 | ng02 | 2112.6 / 2048.1 | 2064.8 / 1993.2 |

Comparaison des médianes chaudes des phases, en ms. Leur ordre de médiane peut différer ; elles ne doivent pas être additionnées. L'exécuteur du lot contient des durées CPU/GPU et des chevauchements : il ne constitue pas un temps de noyau isolé.

| K | Trame | Domaine CPU / GPU | Forêt CPU / GPU | Exécuteur lot GPU |
| --- | --- | ---: | ---: | ---: |
| 5 | ng00 | 208.0 / 252.0 | 163.4 / 169.4 | 53.3 |
| 5 | ng01 | 169.0 / 209.1 | 108.7 / 136.9 | 46.2 |
| 5 | ng02 | 201.6 / 244.8 | 136.5 / 143.7 | 51.1 |
| 10 | ng00 | 827.0 / 751.6 | 1628.7 / 1628.5 | 300.3 |
| 10 | ng01 | 655.7 / 611.8 | 1155.7 / 1167.9 | 249.4 |
| 10 | ng02 | 774.2 / 711.6 | 1288.2 / 1286.3 | 282.1 |

À K5, le mur GPU est supérieur de13,6 /24,5 /14,9% à chaud ; le domaine augmente de40–44ms et la forêt varie aussi, particulièrement ng01. À K10, le mur GPU baisse de3,3 /2,1 /3,5% ; le domaine baisse de44–75ms et la forêt reste proche, à1,17–1,63s. C'est une localisation dans ces mesures, sans attribution individuelle ni gain statistique acquis. La collecte/count du lot reste notable :147–184ms à K10, fill76–82ms, matérialisation Level49–62ms ; ces postes sont partiellement inclus/chevauchants et ne se somment pas au mur. À K5, count25–31ms et fill≈14ms. L'ouverture initiale du contexte distingue aussi les premières passes des suivantes. Aucun contrat100ms n'est établi.

Les lots sont non vides. Premier GPU ng00, K5 : jobs353456, fill_jobs6198, records1306696, population6097121 ; K10/feuille24 : jobs530259, fill_jobs59665, records5512670, population45383538 ; unresolved0. `fill_jobs>0` documente la seconde passe des feuilles débordantes au pin16b. Les agrégats ne donnent pas le nombre de records des feuilles non débordantes copiés depuis scratch : aucun compteur `scratch_records` n'est exporté. Ne pas convertir `jobs-fill_jobs` en nombre de records copiés. Aucun transfert vers la compression scratch ultérieure, source22, u24 ou régime GPU global n'est revendiqué.

`read.py` relit uniquement les JSON figés et leurs empreintes, puis recalcule les médianes à partir des événements/prises conservés. Lectures privées normal/−O concordantes (`python3 -B -S read.py`, `python3 -B -O -S read.py`). `AFTER.json` signale si chaque fichier lu est resté identique dans le dossier vivant. L'inventaire SHA exclut uniquement sa propre racine.
