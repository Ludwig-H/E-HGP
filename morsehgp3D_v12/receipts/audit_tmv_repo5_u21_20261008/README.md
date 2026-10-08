# TMVR repo5 : jalon u21 clos dans les traces — 8 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`public_status=not_claimed`. Prototype repo5 sur base `c903774b1`, patch
`6f0643ac…`, arbre `848c7a0b…` (376 fichiers), admission R corrigée.
Lecture des sources et traces, sans compilation, rejeu natif ni lecture de payload.

Les commandes effectivement terminées sont cohérentes entre codes et journaux :

- build Release/u21, CUDA **OFF**, modules complets ; CTest : **681 sélectionnés,
  680 réussis et une sentinelle LiDAR sautée**, aucun échec ;
- MES-M0 : neuf cas, deux modes de cibles, sortie à l'octet attendue au profil 21,
  déterminisme à 1/8 fils ; JUG-EMST sur l'ordre un en mode `graines` ;
- chaîne CPU native **index → catalogue C → G → T/M/V/R → export** : neuf cas
  en trois lots disjoints (trois uniformes K5, ng00/01/02 à K5/K10), sémantique
  v11 conforme et déterminisme à 1/8 fils ; dernier lot clos à 00:35:16 UTC ;
- **16 mutants tués par code**, témoin vert, aucun signal, délai ou échec de
  construction ; clôture à 00:49:34 UTC. Le rapport porte exactement le hash
  de repo5 et celui de son manifeste, incluant `admission_r_sans_decalages`.

La sortie native détaillée de la porte `admission`, maintenant disponible,
confirme **160 étages, 40 égalités de pic, 25 383 lignes et 685 contrôles sans
échec**. Elle renforce la réponse documentée dans
`../audit_tmvr_admission_20261008/` : budget illimité sans cache, pic supplémentaire
comparé aux octets admis ; le mutant omet seulement les offsets CSR de R.
Elle ne prouve pas à elle seule le refus anticipé sous budget fini ou avec cache.

Les chaînes de ce lot construisent réellement C et G, contrairement aux seuls
adaptateurs des vidages v11 de MES-M0. Elles utilisent la voie CPU du catalogue
avec sa finition partagée ; **aucune exécution CUDA n'est incluse ici**.
Les données restent celles déclarées u21 : aucune nouvelle quantification.
Les fichiers FULL temporaires sont effacés par le pilote ; cet audit contre-lit
ses traces et son contrôle, sans rehasher les données ni recalculer la géométrie.
Les hashes de binaires sont observés à la fermeture, sans reconstruction
indépendante de leur chaîne historique de compilation.

Les profils 24/32 de final5 restent hors de cette clôture. L'intégration main
et la combinaison **G-c + TMVR** ne sont pas qualifiées par ces traces : leurs
sources diffèrent. Les résultats des anciennes campagnes repo3 ne sont pas
transférés. T7 demeure un chantier distinct. Aucun nouveau chrono HGP ou
contrat de latence n'est déduit de ce reçu.

```sh
python3 -B -S check.py --prototype DOSSIER_TMVR
python3 -O -B -S check.py --prototype DOSSIER_TMVR
```

Résultats identiques au champ `result` de `capture.json`. Les sources, logs clos,
caches et binaires sont épinglés avant/après ; le lecteur vérifie seulement
les sept entrées closes de `codes.txt`, que la campagne des profils larges
peut continuer à compléter. Aucun journal complet, chemin privé ou donnée
sous licence n'est copié dans ce reçu.
