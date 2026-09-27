# Pilote parallèle : même objet, orchestration distincte

Le lanceur actif est **`benchmark_full_weighted_parallel_r2.py`**.
`benchmark_full_weighted_parallel.py` reste historique : le gate calcul/IO
ci-dessous passe, mais la relecture a identifié un défaut de nettoyage si le
leader worker meurt et laisse un enfant natif. Cette V1 ne doit pas lancer
la campagne. Les deux fichiers sont des ports explicites du lanceur
`benchmark_full_weighted.py`, sans modification de ce dernier ni des modèles.
Il conserve les 13 cas déclarés, K=5/10, seuils de masse 20/50 et exposants
z=1/2 : 26 exports natifs et 364 lignes, comparateurs figés inclus. Chaque
export natif conserve `--workers 1`. Aucun changement des poids, attaches
FULL, condensation, vote ou données ; aucun nouveau réglage par les scores.

Un processus privé traite une unité complète (cas,K). Deux processus sont
autorisés simultanément par défaut ; trois exigent une option explicite.
Les résultats sont regroupés dans l'ordre déclaré, non dans l'ordre de fin.
Les archives restent sous `/tmp`, séparées des sources et captures figées.
Le nombre de processus n'est **pas** une borne mémoire : l'encodage JSON
prépare en mémoire les données nettoyées, la chaîne puis ses octets.

L'écriture gzip devient groupée, niveau 1, au lieu des petits fragments
compressés au niveau par défaut. Les hashes gzip changent ; le JSON strict,
trié et décompressé doit rester identique. Les durées série, parallèle,
rejouées et réutilisées ne constituent pas des mesures comparables du moteur.

## Gate réel isolé

Commande exécutée, code 0 :

```text
python3 -B /tmp/mhgp9-parallel-replay-case1k5-20260927-LjrApq/record.py
```

Le worker a rejoué intégralement `spherical_g2_d8_s1`, K=5, puis comparé à
R2 les deux mesures et les quatre payloads condensation/vote. Les six JSON
sont identiques champ par champ **et octet par octet après décompression**,
donc masses et labels compris. Les 14 lignes attendues sont présentes.
Le reçu conserve argv, environnement mono-thread des bibliothèques CPU,
stdout/stderr, hashes des entrées, dépendances et artefacts, et fermeture des
pins. Le temps de 39,209 s inclut orchestration et vérifications ; ce n'est
pas un nouveau chrono FULL. Aucun maximum RSS n'a été capturé par ce gate.

Reçu privé :
`/tmp/mhgp9-parallel-replay-case1k5-20260927-LjrApq/receipt.json`,
SHA-256 `92c6b2f3487ec0a9b6cfa3552e06733897e05d56156eb3dc812a1292ad57e944`.

Source du lanceur au gate :
`f303f876c52941c158a75f2a91018b40273b62d5cc5569906e931f639c0d927e`.
Modèles inchangés :

- `weighted_model.py` : `2ac34bf0f72fd809c26063774f1d5f42ced714565dbeff32ee59a0f52205b93e` ;
- `weighted_eom.py` : `16181b12b2f99e2e5df55c548dde88476df157fcbcd88cc1b548bd1499c6bee9` ;
- `full_weighted_tree.py` : `351c090e8bab4f1d43c867341b5196b2fa0a50eb2f980195751442b72c6e9635`.

Ce gate valide une unité et l'identité du port/IO sur celle-ci, pas une
campagne parallèle complète ni une borne de performance ou de mémoire.

## Correctif lifecycle R2 distinct

R2 conserve le groupe de processus possédé jusqu'à sa disparition vérifiée,
même si son leader est déjà mort. Les signaux Python sont différés entre
création et enregistrement du handle, sans transmettre un masque de signaux
bloqués aux enfants. Le parent Linux adopte les orphelins comme subreaper ;
il récolte uniquement ceux de ses groupes, après que `Popen` a enregistré
le statut du leader. L'escalade INT/TERM/KILL vise le groupe réel. Un défaut
de nettoyage est archivé dans le reçu et interdit le statut `completed`.

`test_parallel_lifecycle.py` passe **4 gates en normal et −O** : leader tué
avec enfant survivant ignorant INT/TERM, interruption du parent, interruption
avant enregistrement du handle, et nettoyage idempotent d'un groupe absent.
Le test vérifie notamment que l'enfant orphelin a disparu après nettoyage.
Le gate conserve l'identité textuelle exacte des fonctions `worker`,
`prepare`, `reuse_units` et `save_gzip` avec la V1 déjà rejouée. Aucun nouveau
calcul géométrique n'a été nécessaire pour ce correctif d'orchestration.

Commande, code 0 :

```text
python3 -B /tmp/mhgp9-parallel-lifecycle-r2-20260927-cXMKJm/record.py
```

Reçu privé :
`/tmp/mhgp9-parallel-lifecycle-r2-20260927-cXMKJm/receipt.json`,
SHA-256 `d9f9f8aa86f3cd65e39ac26b4ceb9ab4ae6f3a4157be1c8e6a837c2353e2f924`.
Source R2 : `58855796ea8c6c9110542719ca344e98923b48a706fb2a084bd5676c17fe03a4` ;
tests : `ae256db7ed9b425a6198aa735b6c6e68daa4c77200526e0b92e8738919f13041`.
Ces petits gates ne revendiquent pas une isolation générale de programmes
qui s'échapperaient volontairement de leur groupe de processus.

## Reprise conservatrice

Le responsable de campagne initie et joint lui-même l'interruption de R2.
Le nouveau lanceur exige ensuite son reçu réellement échoué par
`KeyboardInterrupt()`, sans modifier ce reçu. Il revérifie sources, binaire,
qualification, manifeste, entrées, commandes, flux natifs et tous les
artefacts enregistrés. L'ancien chemin d'échec ne capturait pas
`sources_after` : `missing_original_failure_source_closure` le signale et
`resume_verification_currentpins` conserve une **nouvelle** vérification,
pas une fermeture historiquement prétendue.

Seules les unités possédant leurs 14 lignes, deux mesures et quatre payloads
entiers sont réutilisées par liens vers leurs archives. Chaque ligne et
commande réutilisée porte `reused_from`. Une unité partielle est recalculée
entièrement, jamais reprise au milieu de sa sélection. Les 13 cas restent
obligatoires ; aucun résultat partiel n'est promu en campagne complète.
Le ledger parent, les commandes workers et les 26 commandes natives sont
distincts. Les anciens essais, y compris les échecs, restent conservés.

Aucun GCP, GPU ni changement de moteur dans cette tranche.

## Campagne et lectures closes

La reprise R2 s'est terminée : 26 unités complètes, dont dix réutilisées
et seize nouveaux workers, 364 lignes. Les contre-audits normal et `-O`
ont vérifié les 104 sorties pondérées et conservé les 260 comparateurs.
Les résultats et leurs limites sont publiés dans [RESULTATS.md](RESULTATS.md).
Les 894,405 secondes de cette reprise ne comprennent pas le travail initial
des dix unités réutilisées et ne constituent pas un chrono du moteur.

Capture privée :
`/tmp/mhgp9-weighted-full-gaussian-parallel-20260927-b9SXyO/capture_r2/receipt.json`,
SHA-256 `577930f26f4f97fd4f13fb99bb1e356e232a8479055c7f6e09a1bab87bbea720`.
Les deux rapports arithmétiques identiques sont dans
`/tmp/mhgp9-weighted-full-postchecks-20260927-o95pB0/{normal,optimized}/audit.json`,
SHA-256 `b048e8d34e8e3548e4e05340f333301c276ef04a0cacbbde4ad99814e010ee00`.
Toutes les commandes et tous les groupes possédés sont clos ; aucun worker
de cette campagne ne reste à surveiller.
