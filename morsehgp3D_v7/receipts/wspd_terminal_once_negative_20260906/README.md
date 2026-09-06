# Terminal-once WSPD : piste privée non retenue

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La suppression du premier comptage cœur aux rectangles terminaux conserve les sorties sur les [174 petits fronts](payload/RESULTATS_R1.md) et sur [une paire front8k](payload/RESULTATS_8K.md), mais n'apporte aucun gain de temps observé sur cette paire : **37,767 s → 38,287 s**. Les tests de coins passent de **167 115 088 à 335 509 837**. Ce doublement du travail est observé exactement sur cette entrée ; la différence de temps de +1,375 % n'est pas une conclusion statistique ni un théorème de régression. **Pas d'intégration produit.**

Les 754 686 rectangles et tous les champs sémantiques sérialisés sont identiques. Le temps couvre seulement le front, sans construction de l'index ni sérialisation ; ce n'est ni la génération complète ni la tour FULL. Mesure unique par bras, référence puis candidat, CPU6, un thread, n=8000/s=8, uniforme/graine3/coord65536, h=10/9/8. Aucun timeout ou nouveau rlimit pour les mesures. Les petits tests O2 ne transmettent pas de qualification SAN.

[Preuve et limite](payload/README.md), [diff minimal](terminal_once.patch), [reçu174](payload/receipt.json), [résultat8k](payload/measure_logs/result.json). Les snapshots source, depfiles, commandes, PID/codes et flux sont conservés ; leurs chemins absolus sont des enregistrements historiques, pas des chemins portables à exécuter. Les scripts de capture sont archivés, aucun nouveau run n'a servi à cette publication. Aucun ELF n'est publié et aucun fichier auditeur n'est copié.

Les deux grands JSON stdout sont stockés en gzip déterministe, sans altération des octets décompressés. `publication.json` donne pour chaque fichier le chemin public, l'encodage et les SHA-256/taille avant et après compression. Le SHA original du flux reste celui de sa commande historique.

Depuis la racine de ce paquet, `sha256sum -c SHA256SUMS` vérifie les copies. Pour relire un grand JSON sans moteur, utiliser `gzip -cd payload/measure_logs/reference_measure.stdout.gz` (ou `candidate_measure.stdout.gz`). La vérification de décompression exacte a été faite lors de la copie. Les chemins du manifeste sont canoniques, sans préfixe `./`.

GCP non utilisé. Aucun contrat 50k/1s ou massif/GPU acquis.
