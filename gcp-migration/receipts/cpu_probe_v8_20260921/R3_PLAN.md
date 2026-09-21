# R3 : obtenir un diagnostic utile, sans extrapolation prématurée

Plan neuf : `gcp-migration/gcp_plan_r3.json`, SHA256
`57eb2cc1892f0d1757b81f7d822777d10d98e8ef2eca10d32a6a7ad02d2fda26`.
Tous les cas utilisent K5, s8, masque6, Local28 :

| Ordre | Points | Workers | Objet |
|---|---:|---:|---|
| 1 | 1 000 | 1 | Référence CPU locale à la VM |
| 2 | 1 000 | 48 | Comparaison exacte payload/travail et gain de parallélisme |
| 3 | 2 000 | 48 | Premier doublement |
| 4 | 4 000 | 48 | Second doublement ; décision utile avant 8k |
| 5 | 8 000 | 48 | Seulement tant qu'il reste du budget utile |

Le fichier unique est la copie inchangée `data/scan0_n8000.u16le`, hash
`ed5aa97941551e027a76d7c4eb4a03c739cdf0a83cbaf53035c1536bed0b5194`.
Les cas 1k/2k/4k sont des préfixes explicites de ce fichier, pas des nouveaux
échantillons indépendants du scan. Leurs rapports de croissance sont un
diagnostic de cette famille, pas une preuve universelle de complexité.

Le résultat local 8k d'environ22minutes àW4 fourni par le responsable motive
le retrait de16k/32k/50k du pilote. Diviser ce temps par12 supposerait un
équilibrage et une accélération parfaits non établis ; ce n'est pas une
prévision de performance. Si W48 apporte peu à1k, ou si les coûts de2k/4k
croissent fortement, privilégier le diagnostic de `expanded_pairs`,
`cover_sites`, lectures census q3, construction atlas et lectures actives q4.
Davantage de CPU ne change pas le nombre de ces opérations géométriques.

La recette, les gardes et le worker R2 restent inchangés. Le plan n'ajoute
aucun quota de recherche ni limite au produit : seule la session complète
reste bornée à900secondes utiles, compilation et gate comprises. « Si temps »
signifie ici que le budget n'est pas épuisé au lancement du cas ; il n'existe
pas de prédiction automatique garantissant que8k terminera. Le responsable
peut interrompre après le diagnostic4k et doit fermer la génération exacte.

Les résumés des cas achevés sont des fichiers exclusifs immédiatement écrits.
Sur expiration, le groupe de la commande active est arrêté, ses logs, son
code−9 et son reçu sont conservés ; aucun résumé « completed » n'est créé pour
ce cas. Le reçu worker est FAILED, mais le contrôleur tente quand même de
récupérer tous les fichiers avant l'arrêt gardé. Cette récupération ne peut
pas être garantie après préemption SPOT ou perte SSH ; une telle perte doit
rester explicitement non qualifiée.

La simulation locale `check_partial_deadline.py` du helper épinglé passe en
Python normal et−O sans processus réel ni cloud ; voir
`r3_partial_deadline_checks.json`. Son premier défaut de chemin local a été
archivé, puis corrigé uniquement dans cet auxiliaire. Aucun fichier parmi les
196sources produit, aucun worker/contrôleur gelé, aucun build n'a été modifié.

Le nouveau snapshot attend les pins fermés après correction des trois
comparaisons rationnelles de la gate. Aucun paquet ancien n'est promu à ces
nouvelles sources. Aucun démarrage GCP n'est effectué par ce préparateur.
