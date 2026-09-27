# Diagnostic Nsight de la vraie chaîne FULL

27 septembre 2026. Répondre à la question « où passe le temps ? » avant
une nouvelle refonte. Moteur inchangé, aucun nouveau build, aucun gain
ni contrat certifié par ce diagnostic. Profil grille1mm/u18, ng00 entière,
K1..5/s8/W48, mêmes options que la
[référence chaude](../../receipts/g4_core_warm_20260927/README.md).

**Essai R1 clos en échec avant profilage** : l'ancien binaire FULL n'est
pas disponible sous la forme attendue dans `/tmp`. Zéro commande worker, aucun FULL,
GPU ou Nsight exécuté ; aucun téléchargement du profiler. Même génération
G4 arrêtée après157,524s d'allocation. [Reçu](../../receipts/full_nsys_20260927/r1/README.md),
[bilan et prochaine condition préalable](../../docs/PROFILAGE_AVANT_REFONTE_20260927.md).
Pas de relance automatique ni de reconstruction implicite.

Le lecteur de R1 est volontairement spécialisé sur cet échec préalable,
pas sur un futur profilage réussi. Relectures LIVE normal/−O passées ;
ses tests comptent 3 contrôles positifs et 17 refus ciblés. Les originaux
privés restent nécessaires. Lancer `readback.py --readback` avec le chemin
du reçu ; ne pas prendre un succès du lecteur pour un succès du profilage.

## Réemploi et sécurité

Le binaire déjà qualifié du commit `ddf4776d754a8db59a1333e11d56b39d8cb6f51a`
et son entrée doivent encore exister sur le disque de la G4 ; leurs hashes
sont vérifiés avant tout calcul/téléchargement, puis après. Absence ou
divergence : échec, sans reconstruction implicite. Le paquet FULL original
sert aux validateurs et à la provenance, pas à une recompilation.

`capture.py` réutilise le `run_session` historique épinglé sans modifier
ses commandes, gardes, récupération ou arrêt `finally`. Seul son validateur
de paquet est remplacé par celui du paquet FULL v9. Le worker de diagnostic
est distinct, publié et haché séparément. L'attente coopérative provient du
wrapper récent épinglé : après600s, SIGINT puis jointure, jamais destruction
du nettoyage. Gardes30min invité/3600s GCE, même G4 SPOT, budget utile300s.
L'arrêt de la même génération est obligatoire, y compris sur échec.

## Une seule paire de processus

Le worker télécharge un paquet **Nsight Systems CLI2025.3.1** depuis le
dépôt officiel NVIDIA, vérifie taille/hash, puis l'extrait dans son dossier
privé. Aucun `apt upgrade`, pilote, CUDA ou installation globale. Version
et aide sont capturées ; aucune GUI ni outil redistribué dans Git.
Les bibliothèques dynamiques observées du binaire sont hachées avant/après ;
cela ne prétend pas rendre tout l'environnement système hermétique.

Il exécute d'abord le vrai FULL sans profiler, puis la même commande sous
`nsys profile --trace=cuda,osrt --sample=none --cpuctxsw=none`, avec export
SQLite. Chaque processus calcule quatre fois la même trame : premier
passage puis trois répétitions, pas quatre scènes. Les sorties passent le
validateur natif, les trois digests et les objets logiques comptés sont
comparés au reçu historique. Ce n'est pas un nouvel oracle indépendant.
Le succès du diagnostic exige aussi un rapport non vide et une table
d'activité CUDA contenant effectivement des noyaux.

Les temps sous profiler servent au diagnostic, pas au contrat ni à une
comparaison de performance robuste. Ce premier essai ne collecte ni
échantillonnage CPU ni compteurs d'occupation des SM. Il n'introduit pas
de marqueurs NVTX dans le moteur. Les rapports binaires et leur SQLite
restent privés ; la synthèse publique doit les lier par hash et distinguer
un transport réussi d'une trace sémantiquement relue.

## Questions et limites

Mesurer les noyaux, copies et trous entre lancements ; les champs existants
appelés `kernel_ms` peuvent aussi contenir des attentes hôte. Ne pas sommer
les temps des K déjà simultanés. Ne pas confondre activité GPU et occupation
des unités de calcul, ni activité divisée par quatre et latence FULL.

Le processus inclut des travaux hors `chain_total` : contexte CUDA,
réservation pinned et environ475ms de digests par passage historique.
Sans marqueurs supplémentaires, les trous entre groupes GPU mêlent census,
tour, digests et préparation suivante : leur attribution détaillée reste
inconnue. Publier seulement les fenêtres identifiées et les bornes réellement
observées, pas une différence « processus moins GPU » attribuée à FULL.

## Vérifications avant lancement

```sh
python3 -B morsehgp3D_v9/audits/b_full_nsys_20260927/selftest_capture.py
python3 -B -O morsehgp3D_v9/audits/b_full_nsys_20260927/selftest_capture.py
python3 -B morsehgp3D_v9/audits/b_full_nsys_20260927/selftest_worker.py
python3 -B -O morsehgp3D_v9/audits/b_full_nsys_20260927/selftest_worker.py
```

Lancement seulement après publication/vérification du commit, dans un dossier
de session neuf sous `build/`. `capture.py` est inerte sans `--execute` :

```sh
python3 -B morsehgp3D_v9/audits/b_full_nsys_20260927/capture.py --execute --commit COMMIT_COMPLET --session-dir DOSSIER_NEUF
```

La préparation locale n'est pas une trace Nsight ni un résultat G4.
