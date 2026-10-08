# A6b : arrêt certifié, résultats absents à la reprise

Observation du **8 octobre 2026 à 17:08:36 UTC**. Lecture locale des sources,
métadonnées et reçus ; aucun lancement natif, contrôleur, appel distant ou
lecture de coordonnées/identifiants LiDAR.

La session A6b a été lancée à **16:00:00 UTC** ; le journal local annonce le
lancement du worker à **16:03:47 UTC**. La récupération commencée à
**17:03:04 UTC** constate la machine **déjà `TERMINATED`** :
`status=stopped`, `closure=already_terminated`,
`targeted_shutdown_certified=true`, clé de session retirée, sans erreur ni
avertissement. Les deux commandes de récupération ont le code 0 et la
description primaire confirme cet état. Ce reçu ne date pas l'arrêt initial
et n'en attribue pas la cause.

À l'observation, **ni `receipt.json`, ni `DONE`, ni archive de résultats**, et
le répertoire local `results/` est vide. Une recherche de noms dans
`/workspaces` ne trouve aucun `rapport_t2d_a6b.json`, aucune archive nommée
A6b ni fichier sous les résultats de cette session. Elle ne démontre pas
l'absence d'une copie renommée ou de résultats restés à distance.
Le code du worker, les codes/durées des portes et du pilote, les journaux
FULL et les ELF de fin sont donc **inconnus ici**. Aucun verdict d'adoption,
rejet, refus du juge, défaut géométrique ou temps FULL n'est déduit de cet
état de récupération.

Le préflight préparé à 16:13 a été rejoué, sans changer sa règle :

- Avant : **47feedc96**, archive `0e812c6b…fa1a`, **369 fichiers** des scopes
  natifs, tests, bancs, CMake et lecteur FULL exactement égaux à Git.
- Après : **f2c106d93**, archive `c299cc07…9892`, **372 fichiers** des mêmes
  scopes exactement égaux à Git. Le pilote et ses deux modules Python sont
  également épinglés ; l'émetteur `bench/full_probe.cpp` est inchangé.
- Plan `7d6e51f5…674d` : W48, u21 prévu par le pilote, catalogue GPU, cache
  par défaut 8 Gio, six tours des 21 trames de plus de 60 000 sites, cinq
  tours de ng00–02 avec dix passes. Cohorte **prévue**, jamais confondue avec
  une exécution constatée : **85 processus, 1 306 passes FULL**, dont
  **783 passes chaudes décisives** et 100 passes d'identité.
- Règle écrite à 14:47 : identités FUL1 ; borne haute de l'IC du rapport
  après/avant **< 0,95** pour les grandes trames et **< 1,01** pour chacune
  des trois ng ; veto A/A au-delà de ±1,5 %. Bootstrap 10 000, graine
  20261008. Aucun nouveau chrono K10 à chaud n'est prévu : seules ses
  identités à froid appartiennent à cette campagne.

La fermeture des sources et du protocole permet une reprise future de
l'admission ; elle ne remplace pas les résultats manquants. Le lecteur
indépendant déjà préparé hors dépôt, dans
`evidence-snapshots/a6b_admission_20261008/admit.py`, reste disponible pour
une archive ultérieure. Il faudra alors fermer le manifeste, les codes,
les ELF finaux, les 85 processus et le jugement, sans réécrire ce constat
daté. Les temps R1 admis restent les derniers temps FULL K5 à chaud des
trois trames ng ; ce reçu ne traite pas les campagnes massives L1t/L2t.

`capture.json` contient uniquement des champs publics sélectionnés et des
empreintes ; aucune identité de compte, cible distante ou commande privée.
`protocol.json` reprend le préflight figé. Rejeu local sans moteur :

```sh
python -B -S check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.a6b
python -B -S -O check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.a6b
sha256sum -c SHA256SUMS
```

L'absence des résultats est historique : le lecteur signale leur apparition
ultérieure sans la transformer en qualification de cette capture.
