# Trois paires mono D/E q2 — WSPD s=8,10,12

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

E est uniquement le delta de précontenance MEB q2. La proposition par pivots
et le candidat de pile inline F ne sont pas inclus dans ces mesures.

Une nouvelle observation D puis E est conservée à chaque séparation :
uniforme n=8000, coord=65536, seed=3, **tour candidate complétée K1..10**
(`smax=11`, `--complete-incidences`), CSR, digest inclus, aucune archive.
CPU logique 6, threads=1, fold-inflight=1 et fold-join=1. Le contrôle
d'absence réelle de création de threads appartient aux portes produit dédiées.

| s | Statut D/E | D processus (s) | E processus (s) | Baisse observée |
|---|---|---:|---:|---:|
| 8 | paired_equal | 189.000305866 | 184.178172403 | 2.55 % |
| 10 | paired_equal | 192.556029131 | 192.477287858 | 0.04 % |
| 12 | paired_equal | 198.641944467 | 192.730151691 | 2.98 % |

Les ratios ne sont calculés que pour des paires achevées avec toutes les
projections strictement égales à s fixé. Aucun temps comparatif n'est
attribué à une paire censurée, refusée ou divergente ; ses fichiers bruts
restent conservés, avec leurs diagnostics historiques non réécrits.

Comparaison géométrique inter-s : `objects_equal`.
Les dix digests chaînés et toutes les cardinalités sont les critères inter-s ;
les compteurs de travail et le champ s n'ont pas à être identiques entre s.
Les résultats complets, étages en millisecondes, RSS en KiB et égalités sont
dans [results.json](results.json), les observations originales dans `pairs/s*/runs.json`.

Il s'agit d'une seule paire froide ordonnée par s sur un hôte partagé, pas
d'un gain statistiquement qualifié ni d'un classement robuste des séparations.
Aucun SLO 50k/1 s/100 ms, résultat de dizaines de millions de points, GPU
ou exactitude industrielle globale ne découle de cette campagne.

## Autorités et bornes

D est lié à son build et à sa qualification historiques scellés, jamais aux
sources E. Les deux C historiques et D restent protégés. E est lié à son
build réel Release CPU, cache/base de compilation, source et SHA explicitement
vérifiés. Le filename `build_E/build_D.json` est hérité du builder ; son
contenu désigne E. C'est une liaison enregistrée, pas une attestation hermétique.

Chaque processus est borné à 600 s et RLIMIT_AS=26 GiB d'espace virtuel,
pas de RSS physique. Proxy partiel de payload=16 GiB. Caps silent par ordre :
8M core, 2M chain, 2M cofaces, 1 milliard de visites et de supports MEB.
HEAD/porcelain, caractéristiques et charge de l'hôte, intervalles UTC et
commandes restent dans les métadonnées originales de chaque paire.

Un changement documentaire de HEAD pendant une paire est enregistré, pas
assimilé à un changement de moteur : le critère reste l'identité stricte
des snapshots de sources/helpers/binaires. `review.json` rapporte les HEAD
avant/après ; l'exporteur ne prétend pas refaire l'audit des commits.

## Revue et conservation

[review.json](review.json) est une revue en lecture seule : intégrité des
manifestes, reclassification de chaque sortie brute par les helpers épinglés,
égalité de tous les champs extraits, redécision des paires, contrôle inter-s
et stabilité terminale. Aucun moteur, build, selftest, Git ou GCP n'est
lancé par l'exporteur. Les temps historiques D ne sont pas réutilisés.

Les 58 fichiers sources copiés sont identiques octet pour octet :
aucun LF ajouté, aucune normalisation de sortie. [provenance.json](provenance.json)
donne chaque origine, destination, hash, taille et transformation nulle.
Aucun binaire, objet ou archive exécutable n'est publié. Les hashes de binaires
privés restent des références historiques, pas des fichiers manquants à livrer.

`SOURCE_HASHES.json` désigne les noms privés d'origine, pas ceux de cette
projection. Les scripts `protocol/*.snapshot.txt` sont inertes ; leurs
chemins/imports d'origine sont inchangés et ne sont pas directement rejouables
depuis l'archive. [dependencies.json](dependencies.json) épingle les snapshots
et reçus déjà publics nécessaires ; aucune dépendance n'est implicitement héritée.
Les notes de préparation conservées décrivent leur état historique préparatoire,
pas une absence des exécutions fermées maintenant publiées.

Depuis ce dossier : `sha256sum --check SHA256SUMS`. Depuis la racine :
`sha256sum --check morsehgp3D_v7/receipts/meb_q2_mono_20260905/SHA256SUMS.root`.
Les deux listes couvrent les mêmes fichiers et s'excluent elles-mêmes ainsi
que l'une l'autre. `manifest.json` énumère les fichiers publics avant ces listes.

Une publication n'est fermée que si les deux listes existent et se vérifient.
Un dossier partiel après exception n'est pas un reçu publiable et n'est
jamais repris ni écrasé automatiquement.

GCP non utilisé par cette campagne locale et son export.
