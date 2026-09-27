# Profiler la chaîne entière avant une nouvelle refonte

27 septembre 2026. Décision utilisateur : ne pas poursuivre un chantier
important pour quelques pourcents ; demander d'abord des mesures montrant
qu'il peut changer le résultat de la tour complète. Le contrat 100 ms
reste ouvert. Aucun changement du moteur n'est décidé dans cette note.

## Essai du 27 septembre : pas encore de trace exploitable

Le [protocole court](../audits/b_full_nsys_20260927/README.md), publié en
`acb51d62e`, a été essayé une fois sur G4 SPOT. Il devait reprendre le
binaire FULL historique inchangé, puis comparer quatre passages sans
profiler et quatre sous Nsight Systems CLI 2025.3.1. Le lanceur et le
worker passent respectivement 2/9 et 6/11 contrôles positifs/refus,
en normal et sous `-O`, avec une contrelecture indépendante.

**Échec avant le profilage** : le fichier historique
`/tmp/ehgp-tower-v9-4e3fa578d9c95e0b.jLEtLeGGhi/output/build/mhgp9_tower_probe`
n'a pas satisfait le contrôle de présence comme fichier ordinaire non
symbolique. Le reçu ne distingue pas absence, accès impossible ou type
inadmissible ; il n'établit pas pourquoi ce contrôle échoue.
Le worker refuse avant toute commande de calcul ou
de téléchargement : aucun FULL, aucun noyau GPU, aucun téléchargement
ou lancement Nsight, aucun rapport. Ce n'est pas un échec de l'algorithme
ni une incompatibilité démontrée de Nsight. Voir le
[reçu conservé en échec](../receipts/full_nsys_20260927/r1/README.md).
Son lecteur LIVE spécialisé rejoue le statut failed, l'identité du worker,
les sources et l'arrêt de la même génération. Normal/−O passent ainsi
que 3 contrôles positifs et 17 refus ; aucun succès de profilage n'en découle.

La même génération G4 est certifiée `TERMINATED` :
13:21:02,905 → 13:23:40,429 UTC, soit **157,524 s d'allocation** ; prix
facturé non estimé. Aucune autre VM `project=e-hgp` active signalée par
la fermeture. Aucun essai automatique supplémentaire ni changement moteur.

Avant un prochain profilage, il faudra disposer d'un exécutable épinglé
dans un stockage durable, ou préparer une reconstruction explicitement
qualifiée, sans supposer la présence d'un ancien fichier `/tmp`.
Cela ne justifie pas de relancer une refonte : la question du chemin
critique demeure ouverte et le contrat 100 ms reste non atteint.

## Ce qui manque toujours

Les campagnes v9 examinées n'ont aucune capture **Nsight Systems** ou
**Nsight Compute** attestée. Avant cet essai, la recherche dans les sources,
commandes et reçus v9/v8/GCP ne trouvait aucune exécution `nsys`/`ncu`
attestée ni rapport associé. Le nouveau protocole contient désormais les
commandes prévues, mais elles n'ont pas été exécutées sur G4.
Une mention ancienne dans la roadmap est une intention, pas une preuve.
Le paquet CLI officiel 2025.3.1 a depuis été vérifié et extrait localement,
sans installation globale ; ses aides et sa version fonctionnent. Cela
ne constitue ni une installation sur G4 ni une capture GPU.

Nous avons des compteurs de travail, des chronos hôte et des synchronisations
CUDA. Ils localisent certains coûts, mais n'expliquent pas à eux seuls les
attentes CPU/GPU, les chevauchements, l'occupation ou la bande passante.
Une soustraction de durées ou un pourcentage `nvidia-smi` ne remplace pas
une trace de la chaîne.

La [référence FULL chaude](../receipts/g4_core_warm_20260927/README.md)
mesure 923,417 ms sur ng00 sans sol entière, grille 1 mm, K1..5, s8,
48 workers, pour le bras noyau diamétral ON. C'est une agrégation de deux
processus et de trois répétitions chaudes par processus, pas plusieurs
scènes. Lecture, segmentation et digests sont hors chaîne ; le contexte
CUDA n'est pas payé dans `chain_total`. Ces exclusions restent explicites.
Ce résultat n'est donc ni 100 ms ni un contrat intégral entrée-à-sortie.

## Petit diagnostic, puis décision

1. Reprendre **le vrai `mhgp9_tower_probe` FULL**, son commit publié et
   [sa commande exacte](../receipts/g4_core_warm_20260927/vm/probe_0.command.json).
   Ne pas profiler seulement le comparateur S2 ni les copies de l'observateur A.
2. Mesurer sans profiler, puis capturer une chronologie Nsight Systems
   courte sur la même entrée : activité CPU/CUDA, copies, synchronisations,
   allocations et trous d'activité GPU. Séparer premier passage et suivants.
3. Examiner Nsight Compute seulement si un noyau précis domine réellement
   le chemin critique. Recueillir alors les métriques nécessaires à une
   question précise, pas toutes les métriques de tous les noyaux.
4. Choisir au plus un changement structurel dont le gain potentiel sur
   le **mur FULL** est important, avec coût mémoire et exactitude préservés.
   Sinon publier le constat et ne pas ouvrir la refonte.

Nsight Systems fournit la chronologie des activités et appels CUDA
([guide NVIDIA](https://docs.nvidia.com/nsight-systems/UserGuide/)).
Nsight Compute peut rejouer les noyaux et perturber fortement les temps
observés : ses captures servent au diagnostic, jamais au chronomètre du
contrat ([guide NVIDIA](https://docs.nvidia.com/nsight-compute/ProfilingGuide/)).
Les versions, commandes, rapports et limites d'accès aux compteurs devront
être conservés. Un échec de collecte ne deviendra pas une explication du
goulot. Aucune nouvelle campagne de profilage n'est exécutée par cette note.

## Critère de poursuite

Une accélération locale n'est intéressante pour 100 ms que si elle retire
une part substantielle du temps réellement exposé sur la chaîne complète.
Les étapes déjà simultanées ne s'additionnent pas. Pour une portion isolée
sans recouvrement, même sa suppression complète laisse tout le reste à
payer ; ce reste constitue une borne pratique de son potentiel.

Les [mesures A sur vrais catalogues](../audits/b_full_a_real_20260927/RESULTATS.md)
donnent un gain de 23,2 % de min-label face au **prototype événementiel**
sur ng00, pas face au constructeur natif. Elles ne justifient donc pas à
elles seules un gros portage GPU ou un nouvel index complexe. Les volumes
uniformes8k/16k/32k sont utiles pour la croissance de ces objets ; ils ne
démontrent pas la croissance LiDAR ni une borne générale du générateur.

Le comparatif S2 G4 déjà préparé reste un essai court de décision, sans
activation automatique dans le moteur. Un résultat favorable ne ferme
pas les coûts q3/q4 extérieurs à S2, ni la construction FULL. Les grands
chantiers suivants attendent la lecture du profil de la chaîne réelle.
