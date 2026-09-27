# Contrôles hors ligne de la campagne G4 core

`capture.json` conserve quatre commandes : onze tests du lecteur normal
puis sous `-O`, et deux relectures complètes de la capture publiée.
Les sources, le snapshot privé, les reçus et les sorties sont hachés.
Le lecteur reconstruit aussi le paquet depuis les objets Git du commit
déclaré, rejoue le protocole, compare les bras GPU aux deux témoins CPU,
et vérifie l'arrêt de la génération exacte. Aucun appel GCP dans ces contrôles.

`prestart_refusal/` conserve le refus local initial du lanceur : la clé
éphémère était en mode 0644, refusée avant création du répertoire hôte du
contrôleur et avant son premier appel GCP. Les logs ne contiennent aucun
octet de clé. La relance r2 explicite 0600 ; sa campagne close est dans
[`g4_core_warm_20260927`](../g4_core_warm_20260927/README.md).

```sh
python3 -B morsehgp3D_v9/audits/b_gpu_next_20260927/checks.py check
python3 -O -B morsehgp3D_v9/audits/b_gpu_next_20260927/checks.py check
```

Lecteur LIVE : le snapshot et les sources épinglées doivent être présents.
