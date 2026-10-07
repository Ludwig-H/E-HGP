# Reçu local de MES-M2 et MES-S (7 octobre 2026)

Microbanc de la feuille du catalogue (tranche T0), joué sur le codespace sans GPU : vidage des feuilles de la v11
gelée (`ac081a06f`, construite en u21) sur ng00, ng01 et ng02 à K5/16, K5/24 et K10/24 ; identité exacte des deux
formes data-parallèles, jouées sur un warp simulé, avec la feuille de référence `leaf.cpp` ; histogrammes des étendues
locales (`MES-S` du [contrat numérique](../../docs/CONTRAT_NUMERIQUE.md)). Le code est dans
[`../../microbancs/mes_m2_feuille/`](../../microbancs/mes_m2_feuille/README.md) ; le rapport de l'agent qui l'a écrit
est [`RAPPORT.md`](RAPPORT.md) (ses chemins `v12_feuille/` désignent ce dossier de code, ses `results/` ce reçu).

- **Ce qui est établi** : identité (0 feuille non résolue, 0 écart de compteur, 0 écart d'émission sur 2 748 544
  feuilles) ; étendues : toutes les feuilles ont $s\leq 17$ (13 sur 430 579 au plus à $s=17$), tous les supports
  $s\leq 15$, sur trois trames de la séquence 08 au millimètre.
- **Ce qui ne l'est pas** : aucun temps GPU (le banc est compilé pour `sm_120`, à jouer sur G4 par
  `scripts/g4_leaf_bench.py`) ; ni dixième de millimètre, ni autres séquences, ni scènes de plusieurs millions de sites.
- **Données** : les vidages (942 Mo) sont dérivés de SemanticKITTI et restent hors dépôt ; ce reçu ne contient que des
  comptes et des empreintes (`results/`).

`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. GCP non utilisé.
