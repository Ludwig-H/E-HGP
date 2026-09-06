# Histogrammes par blocs : preuve bornée, résultat uniforme négatif

`public_status=not_claimed`. Prototype privé, aucun changement produit ; pas de qualification FULL ni de résultat SLO.

Le [certificat indépendant Hmin/Ximax](../../audits/receipts_block_histograms_20260906/README.md) permet le crédit exact à ancre fixée. La [porte O2/SAN](qualification/receipt.json) compare 126 cas contre les histogrammes produits et un calcul entier indépendant. Elle exerce crédits stricts, rejets, diagonale, grand domaine u16 et populations originales ; elle n'établit pas un gain de performance.

Sur **un seul nuage uniforme n8000/s8**, même front et histogrammes littéralement égaux, les durées instrumentées sont 93,819 ms (scalaire), 186,560 ms (blocs forcés), 101,318 ms (hybride8). Le maximum des facteurs est 7 : aucun bloc n'est activé par hybride8. Conserver le scalaire sur cette population. L'intérêt sur de grands facteurs reste à mesurer séparément, sans extrapoler ce résultat à tous les nuages ni à s10/s12.

Le [brut de mesure](measurement/s8.stdout) et la [commande close](measurement/s8.command.json) précisent CPU6, durées, distribution et compteurs. Processus 479755 fermé, code 0, stderr vide ; attente directe sans quota temps/CPU/fichier, garde RAM 26 GiB. La [note copiée](prototype/RESULTAT_N8000_S8.md) donne le détail par cohortes.

[publication.json](publication.json) lie chaque copie à son origine/hash et énumère les trois ELF volontairement omis. `source_snapshot/` conserve les headers consommés ; `prototype/`, `qualification/` et `measurement/` sont des copies byte-exactes. Les chemins absolus et liens relatifs historiques à l'intérieur des copies restent ceux des répertoires privés d'origine : ils ne sont pas réécrits comme une nouvelle capture. L'ancien recordeur de compilation est conservé séparément ; les reçus clos restent inchangés. `SHA256SUMS` utilise uniquement des chemins relatifs canoniques sans `./`.

GCP non utilisé. Aucun moteur relancé pour cette publication.
