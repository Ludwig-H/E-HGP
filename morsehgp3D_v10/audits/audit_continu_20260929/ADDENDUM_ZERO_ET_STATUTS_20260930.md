# Niveaux zéro et limites du lecteur CUDA

30 septembre 2026. Contre-vérification de la réponse du développeur
`e9eab2754` et de la sonde corrigée `779dd38a9`. Le moteur n'a été ni
modifié ni reconstruit. GCP non utilisé ; `public_status=not_claimed`.

La réponse prévoit un raccord utile sur une extraction commune, mais ne
constitue pas son intégration. Deux précisions évitent de transformer des
tests limités en garanties trop larges : la tête consomme un niveau zéro
même pour une racine singleton sous `min_cluster_size`, et le lecteur CUDA
vérifie une enveloppe statut/code, pas la validité numérique d'un résultat.

## La garde numérique doit couvrir la racine singleton

Notre [sonde native courte](../../receipts/audit_continu_20260929/head_numeric_corrected/singleton_20260930/README.md)
appelle la copie corrigée de la tête sur un arbre public valide à un point.
Le niveau positif 1/4 donne des quantités finies. Au niveau zéro, les quatre
variantes produisent une date de sortie et une stabilité infinies, y compris
avec `min_cluster_size=5` ou 200 pour une masse de 1. Le validateur accepte
l'objet. La compilation et l'appel terminent avec code 0 ; ce code confirme
la reproduction du cas, pas la qualification du produit.

L'en-tête actuel documente λ(0)=+∞. Ce cas ne viole donc pas cette convention :
il précise le domaine à traiter si la nouvelle garde veut garantir des
sorties numériques finies.

La raison est localisée : la condensation ouvre toujours la racine puis
calcule la date des points qui lui sont attachés. Une règle qui ne contrôle
que les naissances zéro de masse au moins `min_cluster_size` ne couvre donc
pas à elle seule ce chemin. La borne de finitude démontrée pour les niveaux
entiers **strictement positifs** reste utile, mais ne traite ni ce singleton
ni tous les zéros de K1.

Action avant raccord : déclarer une convention pour les niveaux zéro et
les singletons, ou un refus explicite si l'EOM demandé n'est pas défini ;
contrôler tous les niveaux effectivement consommés. Ajouter ce singleton
comme fixture causale de la garde commune. La garde annoncée n'est pas
encore implémentée dans la bibliothèque testée : nous ne prétendons donc
pas avoir fait échouer cette future garde. Le cas avec racine autorisée
n'est pas présenté comme un nouveau défaut de politique de sélection.

## La sonde CUDA corrigée ne possède pas encore un juge numérique complet

La correction publiée contrôle le statut et les durées avant de calculer
les débits. C'est un progrès distinct de la correction arithmétique unsigned.
Notre [capture hôte normal et −O](../../receipts/audit_continu_20260929/timeout/cuda_corrected/status_20260930/README.md)
teste vingt entrées du lecteur, sans lancer CUDA. Il accepte notamment
`status=ok` seul, un compteur de désaccords positif, une durée déclarée
invalide, un débit négatif ou NaN et une clé de statut dupliquée dont la
dernière valeur est `ok`. Il refuse bien trois contradictions d'enveloppe.

Ce résultat respecte la fonction annoncée du lecteur : vérifier une ligne
JSON et sa cohérence avec le code du processus. Ce n'est ni un défaut
arithmétique constaté sur GPU ni un contre-exemple à la sortie du programme
CUDA corrigé. Pour qualifier un futur reçu GPU, ajouter un juge de schéma
et de valeurs : champs requis, types et comptes, absence de désaccords,
nombres finis, durées positives, débits cohérents, refus des clés dupliquées
et des constantes JSON non standard.

Huit simulations de flot de contrôle montrent aussi que l'absence de nvcc,
l'échec de compilation et le délai dépassé ne remplacent pas
`cuda_probe.json`. Puisque le répertoire de sortie peut déjà exister,
un ancien résultat peut y rester ; les sorties partielles du délai ne sont
pas sauvegardées. Ce sont des simulations, pas des délais ou signaux CUDA
réellement exécutés. Exiger un nouveau dossier par tentative ou produire
un reçu distinct d'échec avec code et sorties partielles suffit à fermer
ce risque documentaire. Il ne bloque pas une sonde exploratoire dans un
dossier neuf. Un consommateur qui contrôle aussi le nouveau code de sortie
ne doit pas qualifier l'ancien fichier comme réussite.

## Priorités de la réponse du développeur

La [réponse de raccord](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md)
retient une extraction figée commune, la correction CSR avant lecture,
des juges renforcés et la désactivation du filtre SiteTree hors arrondi
au plus proche. Ces choix répondent aux contre-audits ; leur exécution
et leurs reçus restent à examiner.

Pour la frontière, les bras proposés vont dans le sens de la thèse.
Ajouter avant condensation le diagnostic de **masse fractionnaire conservée**
du chapitre 9, distinct d'une affectation dure. Comparer le rappel des points
frontière avant fusion parasite, les attaches retardées et leur coût,
pas seulement l'ARI après EOM. Garder la majorité uniforme comme contrôle :
notre [contre-épreuve géométrique](AUDIT_LAMINARITE_POINTS_20260929.md)
montre qu'elle peut retarder des points alors qu'une seule composante les
couvre déjà. Les poids fixes en 1/β sont une piste testée sur ces petits
nuages, pas une reproduction de Sτ ni une amélioration statistique acquise.

Aucun nouveau chrono G4 ou GPU, aucune tête intégrée et aucun contrat
FULL K5 explicite en 100 ms ne sont acquis par cette tranche.
