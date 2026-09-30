# Deux exports natifs pour les entrées frontière internes K3/K5

30 septembre 2026. Six sites K3 et sept sites K5, W1. Exactement **deux**
appels natifs, codes 0, stderr réellement vide. Leurs durées de quelques
millisecondes ne sont pas des benchmarks. Aucun moteur modifié ou
reconstruit, aucun GCP. La bibliothèque liée est l'archive historique
de source `6206d1d11` : pas de transfert de qualification au moteur R2.

[native_calls.json](native_calls.json) conserve les commandes et sorties ;
[analysis.json](analysis.json) donne la provenance et les résultats.
Le point étudié est retrouvé par ses coordonnées exactes, pas par son ID
d'entrée : les rangs Morton natifs sont 4 à K3 et 6 à K5.
Ses premières couvertures, β=25 et 105625, sont portées par les nœuds
internes 5 et 3, nés respectivement à 169/9 et 116715625/3409.

Le [vérificateur](verify.py) n'appelle aucun binaire : il lit les deux
exports et compare les composantes par leur géométrie, jamais par égalité
naïve d'IDs. Ses passages [normal](normal.json) et [−O](optimized.json),
code 0, ont les mêmes résultats sémantiques :

- 414 coupes du nerf géométrique complet, 2 814 listes point/composantes,
  2 790 résolutions de boules et quarante premières attaches ;
- catalogue fort complet contre un calcul Fraction séparé, cinq boules
  à K3 et quatre à K5 ; dix-neuf parcours boules fortes×ancêtres ;
- aucune feuille couvrant le point pendant sa vie, même après sa naissance ;
- dix-huit contrôles du lemme K2 et les trois échelles ghost reproduits.

Deux algorithmes rationnels distincts partagent Python Fraction ; aucune
indépendance d'implémentation arithmétique n'est revendiquée. Les traces
brutes et le lemme sont complémentaires, pas une preuve générale du moteur.
Le [rejeu de l'auditeur principal](OWN_REPLAY.json) sur la copie de
publication confirme ces comptes, code 0 en 7,167 s, sans appel natif.

Seul le wrapper [probe.cpp](probe.cpp) a été compilé contre l'archive déjà
existante. Le gel initial des `.hpp` omettait `core/reasons.def` ; cette
lacune n'est pas masquée. [closed_build.json](closed_build.json) conserve
une seconde compilation du wrapper avec les douze dépendances projet
réelles de `-MMD`, figées avant/après : le binaire est reproduit bit à bit.
Aucun troisième appel natif n'est effectué. La bibliothèque et les binaires
ne sont pas copiés ici ; reconstruire l'export dépend de l'archive épinglée.

`SHA256SUMS` est le manifeste original des trente-deux fichiers de preuve,
conservé sans réécriture. Ce README et le rejeu principal sont fermés par
le manifeste du dossier parent. Les fichiers `*.stderr.txt` de commodité
et les binaires temporaires sont exclus : le stderr exact est dans le JSON.

Rejeu en lecture seule : `python3 -B verify.py`, ou `python3 -B -O verify.py`.
Les scripts de compilation/collecte écrivains exigent une nouvelle copie
temporaire ; jamais un lancement sur cette archive. Aucun ARI/EOM, coût
global ou contrat FULL/G4 100 ms acquis. `public_status=not_claimed`.
