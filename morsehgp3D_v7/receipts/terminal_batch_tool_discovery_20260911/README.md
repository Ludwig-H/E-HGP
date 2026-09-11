# Diagnostic de présence des outils — qualification locale

Le worker `aaf8bcc1` conserve désormais les candidats, sélections et manquants dans `tool_discovery` avant le refus de précontrôle. Aucun chemin CUDA, critère d'admission, ordre de test, compilateur, garde ou moteur n'est changé. Les versions ne sont toujours contrôlées qu'après la présence des outils.

Le worker historique `043197e4` reste disponible octet pour octet dans `origins/worker_043197.py.source`, lié au [paquet historique du worker](../terminal_batch_worker_20260911/README.md), manifeste `8758776c41abbbe08009a55c6e97bc10f7d040ccaadb4d5bcc5a6d93d5bf16b2`. `worker.diff` expose le delta exact ; le lecteur vérifie aussi que tout le préfixe historique et toute la suite de `main()` sont inchangés hors remplacement des quatre lignes de découverte. Le snapshot GPU historique de 59 sources n'est pas recopié ni réattribué.

Quatre commandes Python locales sont conservées, sans compilation ni GCP :

- Selftest principal normal et `-O` : 44 contrôles, 239 rejets, 512 combinaisons de présence/liaison confrontées à la règle historique, priorité PATH et replis inchangés.
- Vrai `main()` avec garde, métadonnées et découverte entièrement simulées, normal et `-O` : cinq refus avant toute commande (nvcc, g++, nvidia-smi ou time absents ; autre liaison g++). Le reçu écrit réellement dans un dossier temporaire doit conserver l'observation, l'erreur exacte et `commands=[]`, avec code 1. Aucune commande du support n'est permise.

Ces mocks qualifient la persistance du diagnostic, pas le cycle de vie GCP. Les champs invités et gardes sont des fixtures, non des mesures d'une VM. Les anciens échecs ne gagnent pas rétroactivement ce diagnostic : l'outil qui manquait lors de l'échec EU demeure inconnu à partir du seul reçu historique. Aucun nouveau snapshot ou résultat GPU n'est créé ici.

Lecture autonome, sans compilation, cloud ou réexécution des tests :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

Les sources sont lisibles sous `sources/`; le README historique de la révision est stocké avec suffixe `.source`. Les deux seuls fichiers de soutien copiés du paquet historique permettent de rejouer les tests locaux directement depuis ce dossier :

```bash
python3 -B sources/gcp-migration/selftest_terminal_batch_worker_v7.py
python3 -B -O sources/gcp-migration/selftest_terminal_batch_worker_v7.py
python3 -B sources/gcp-migration/selftest_terminal_batch_tool_preflight_v7.py
python3 -B -O sources/gcp-migration/selftest_terminal_batch_tool_preflight_v7.py
```

Ce replay ne crée que les dossiers temporaires privés des mocks, détruits en fin de test. Il ne faut pas invoquer directement le worker comme une session réelle. Le manifeste ferme les sources et captures, pas une installation Python hermétique. Aucun ELF, vendor, clé ou profil OS Login n'est embarqué.
