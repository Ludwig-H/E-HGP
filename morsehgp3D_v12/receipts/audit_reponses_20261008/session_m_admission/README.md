# FULL M : admission indépendante, 8 octobre 2026

La cohorte est complète et le verdict recalculé est **non tenu**. Les deux lecteurs — LF livré `c3e9e0f4` au
commit `957e9784fb19ccd2d6348e779ed8b7affadc1f3d`, puis la [proposition de gardes temporelles](../lf_recouvert_gardes/README.md)
`15437e5f` — admettent exactement les mêmes **38 processus, 610 passes, 392 chaudes**. Toutes les statistiques,
empreintes et décisions publiées dans `rapport_full.json` sont reproduites, sans tolérance numérique. Rejeu Python
normal et `-O` identique ; aucun moteur, compilation, GCP ou octet XYZ/IDs lu par cet audit.

La [provenance et la fermeture](../session_m_provenance/README.md) sont un reçu distinct : sources du paquet égales
au Git, cinq commandes closes à code zéro, arrêt certifié. Le présent lecteur ne transforme pas cette preuve en
attestation de compilation de l'ELF. La sonde FULL déclare `e5698390…` à l'ouverture ; elle est distincte de la sonde
APPARIÉ, construite séparément. Aucun hash de fermeture de l'ELF FULL n'est publié par ce pilote. La comparaison
au régime de la v11, ou l'attribution causale à un levier de la v12, ne résulte pas de cette campagne.

## Temps admis (ms), u21 / W48 / voie recouverte

Chaque ligne distingue la médiane de toutes les passes chaudes réunies (règle du pilote), la médiane des médianes
par processus (utile pour comparer les résumés de A), le maximum de ces médianes et le maximum brut chaud.

| voie | trame | chaude réunie | médiane des médianes | maximum des médianes | maximum brut |
| --- | --- | ---: | ---: | ---: | ---: |
| GPU K5 | ng00 | 94,822136 | 94,660948 | 95,944275 | 100,777846 |
| GPU K5 | ng01 | 78,118768 | 78,255338 | 80,901886 | 93,878805 |
| GPU K5 | ng02 | 94,792166 | 94,761397 | 95,993467 | 101,608416 |
| CPU K5 | ng00 | 382,078093 | 381,804272 | 382,511833 | 388,792141 |
| CPU K5 | ng01 | 323,883224 | 323,095784 | 326,520668 | 328,988413 |
| CPU K5 | ng02 | 386,392493 | 385,942412 | 388,669597 | 391,583582 |
| GPU K10 | ng00 | 593,731643 | 593,258762 | 595,950290 | 639,462434 |
| GPU K10 | ng01 | 443,895811 | 442,818041 | 460,025516 | 475,836740 |
| GPU K10 | ng02 | 504,632681 | 504,788266 | 508,523130 | 524,665501 |

Le sous-contrat ng00–02 est tenu selon la règle fixée : médiane **94,792166 ms**, maximum des médianes processus
**95,993467 ms**. Trois des 135 prises chaudes dépassent 100 ms ; cela ne déclenche pas le critère préétabli, qui
n'utilise pas le maximum brut. Les 37 trames, cinq processus et deux tours chacun, donnent 185 chaudes (second
tour seulement) : médiane des 37 médianes **160,640467 ms**, plus grande médiane par trame **319,783873 ms**,
maximum contractuel **358,860297 ms**. Ici ce dernier est aussi le maximum brut, puisqu'une trame fournit une
seule chaude par processus. 25/37 médianes par trame, 27/37 maxima processus et 127/185 prises dépassent 100 ms.
La trame du maximum est `kitti_ng_08_002119`, 99 099 sites. Le contrat global exige les deux sous-cohortes et reste
donc non tenu. Les prises K10 et CPU sont informatives pour ce verdict K5 appareil.

Le catalogue CPU K5 reste le poste dominant : médianes C **318,766578 / 272,697915 / 320,924354 ms** contre
**28,063618 / 24,240620 / 27,987499 ms** sur GPU pour ng00/01/02. Ce sont des statistiques par poste ; leur somme
n'est jamais présentée comme la médiane FULL. En recouvrement, G est la fenêtre jusqu'au dernier calcul de G et
TMVR la queue après cette fin : ni la résolution pure G ni la somme des temps de travail T/M/V/R.

## Contrôles et limites

`reader.py` ferme l'inventaire exact des 38 JSONL, les types/champs/statuts, u21/K/W48/voie, l'alternance
FULL/libération, l'ouverture GPU et la sortie, les noms/sites demandés et l'ordre tournant des 37 trames. Il refuse
doublons de clefs JSON, non-finis, texte non ASCII, lignes étrangères et cohorte manquante. Tous les digests sont
comparés, passes froides comprises ; K5 CPU et GPU sont identiques. K10 et les 37 sont comparés dans leurs cohortes
propres, sans identité implicite entre un alias ng et une autre trame. Les gardes renforcées contrôlent les horloges
et dépendances G→noyau→M→R, ainsi que M(k), M(k−1)→V(k), sans inventer un ordre global V/R.

Le plan épinglé et ses paramètres sont ceux du [protocole M](../session_m_protocole/README.md). Les codes zéro des
38 appels internes sont **inférés** de `refus=[]` et du pilote épinglé, qui refuse tout appel non nul ; ils ne sont
pas des codes externes archivés individuellement. Le code externe de la commande FULL relève du reçu de provenance.
La campagne publie un GPU connu vide avant/après ; cela ne certifie pas l'absence de toute concurrence entre ces
deux observations. Les 37 noms/sites et tailles de fichiers proviennent seulement des métadonnées fermées par E ;
aucun rehash des données ni nouveau calcul géométrique. Les empreintes FUL1 attestent ici l'identité entre prises,
sans nouveau passage par l'oracle ou la v11.

Mémoire : `pic_octets` est le pic du **MemoryBudget partagé hôte/appareil/épinglé**, cache désactivé par défaut,
distinct du RSS et d'un pic VRAM. `read_u32le` conserve trois Buffer u32 et un Buffer PointId u32 dans `frames`
pendant la Session ; le lecteur impose donc le plancher sûr `16 × somme des sites des entrées distinctes chargées`.
`restart_peak` repart de l'usage courant. Les capacités CUDA/épinglées résidentes sont réservées dans ce budget
partagé et chacune reste sous ce pic ; `pic_appareil_octets` vaut zéro par convention dans ce régime. Les sources
justificatives sont épinglées dans `capture.json`. `rss_max_octets` est le maximum cumulatif du processus Linux,
pas la mémoire isolée de chaque trame ; `cpu_ns` est le delta CPU de tous les fils autour de l'appel instrumenté.
Le résultat conserve ces métriques séparées.

`check.py` utilise seulement la fabrique JSON du test LF épinglé. Nominal de 38/610/392 et dix contre-JSON refusés
(cohorte, label, identité CPU/GPU, plancher mémoire, budget partagé, statistiques, isolation et refus déclaré).
Son témoin nominal distingue explicitement **2 / 1 / 2 / 101 ms** pour les quatre statistiques, tout en conservant
le verdict tenu imposé par le protocole. Il s'agit d'une vérification du lecteur, jamais de performances natives.
Les refus sont des exceptions explicites ; aucun `assert` nécessaire à l'admission.

## Rejeu

Depuis ce répertoire, avec le dépôt Git et les seuls rapport/JSONL rapatriés :

```sh
python3 -B check.py --repo DEPOT --reader-patch ../lf_recouvert_gardes/proposition.patch
python3 -B reader.py --repo DEPOT --reader-patch ../lf_recouvert_gardes/proposition.patch --plan PLAN_JSON --returned DOSSIER_FULL
```

Rejouer avec `-O` donne respectivement `checks.json` et `results.json` identiques. Aucune source complète ni aucun
journal de données n'est dupliqué dans ce reçu ; le rejeu dépend des objets Git, du patch voisin et de l'archive
de résultats externe épinglés. Tous les nombres de `results.json` sont en unités natives, sans arrondi d'adoption.
