# Audit général de la v11 — 5 octobre 2026

Base publiée : `238734f1d03ab32e2a722bf036eb8fc5626dfd44`. Travail dans
des worktrees détachés, aucun branchement Git. Les notes vivantes restent
les six fichiers du dossier [audits](../../audits/README.md) ; ce reçu
conserve les preuves de cette revue, sans créer un second journal actif.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## Verdict utile au développeur

Aucun nouveau résultat FULL faux ni omission de supports valides n'est
établi dans les chemins relus. La contre-épreuve exacte nouvelle renforce
les tests bornés de connexité, de plateaux et de verticales. Elle ne
qualifie pas une version native.

Les travaux importants sont l'intégration et la qualification de L1
assemblé, puis une publication L2 qui respecte son propriétaire et son
budget. Le défaut de publication entre Sessions étrangères est décrit
avec ses sources WIP dans [la contrelecture native](native/README.md).
Les prochaines mesures doivent payer le journal actif, l'assemblage et
les supports réellement livrés ; les 100 ms restent un objectif.

Les conclusions et actions sont dans
[la note moteur](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md)
et [la note mathématique](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

## Périmètre et limites

| Objet | Examen de cette revue | Limite restante |
| --- | --- | --- |
| `core`, `cloud`, `sched`, `index` | Propriété, admission, tailles et progression ; modèle indépendant du nombre de nœuds de l'index | Aucun nouvel ASan/TSan ni essai natif |
| `num` | Budgets exacts u18/u21/u24, candidat q3 différé et certificats des nouveaux prédicats | Le contrôle algébrique ne remplace pas les portes des fabriques C++ et de FENV |
| `catalogue` et voie GPU | Delta depuis la précédente revue, count/fill, offsets, masques, repli entier des feuilles non résolues, sortie canonique | Aucune nouvelle exécution CUDA ni qualification GPU |
| `tower` | Traces strictes, descentes datées, clôture des plateaux, parents et verticales | Pas de preuve exhaustive de toutes les exécutions du moteur |
| `io`, API/CLI WIP | Publication atomique, refus après publication, empreinte du manifeste, fin de Session, propriétaire du produit | Identité de Session à corriger ; hook variadique IO déjà signalé ; portes G4 attendues |
| Supports | Contrat W_K/Q_b, coquilles étendues, complétude indépendante des cofaces, rattachement fermé | S3/S6 non commités au pin ; assemblage L1 et sa mémoire à qualifier |
| Points | Dates et propriétaires exacts, convention m(1)=1 | Sortie native absente ; frontière K=n à fixer avant L3 |
| Qualifications et temps | Relecture vérifiée des reçus c40 et des 81 prises | Source c40 seulement, K5/u21, trois trames d'une seule séquence, hors préparation/IO/exports/points |

L'inventaire porte sur **104 fichiers natifs, 13 621 lignes et huit
modules** ; avec CMake/pins/raisons, `src/` contient 117 fichiers.
[Matrice de delta](native/DELTA.md) et [empreintes par fichier](native/native_delta.json) :
depuis `e02a6c235bc4a706519cdaa15f4b1465a6275eba`, 68 natifs sont identiques
à l'octet, 21 modifiés et 15 ajoutés, sans retrait. La relecture couvre
les deltas, et réutilise la portée des anciennes lectures uniquement sur
les fichiers identiques. **Aucun résultat de test ou de performance n'est
transféré par cette égalité.** Les fichiers IO sont contre-lus par le
pilote ; la sous-revue native précise ses lectures par sections.
[source_manifest.json](source_manifest.json) épingle les fichiers publiés
et les documents d'entrée ; les brouillons sont séparés ci-dessous.

## Contre-épreuve mathématique nouvelle

[Rapport complet](math/REPORT.md), [programme](math/check_math.py),
[sortie normale](math/normal.json), [sortie `-O`](math/optimized.json),
[empreintes et environnement](math/SHA256.json).

Dix nuages nouveaux, 65 ordres jusqu'à K=n, 17 276 unions arbitraires de
K-parties, 1 453 coupes strictes/fermées et 15 925 contrôles des faces
verticales. Le nerf complet et la définition ont les mêmes composantes
de K-parties. S1 juge 563 boules et 635 supports. Sur les huit nuages u21,
A=B à tous les ordres ; 304 dates et 304 propriétaires de points sont
égaux. Les deux nuages u24 n'exercent que A/S1.

Le graphe et la DSU de cette contre-épreuve sont distincts de ceux de la
définition, mais les MEB sont partagées avec A. Ce résultat ne démontre
pas indépendamment la géométrie de A. NumPy sert aux structures du banc
de points ; les décisions comparées restent exactes.

Les deux sorties finales sont identiques. Deux essais initiaux du
harnais ont échoué sur le codage du rôle `naissance` ;
[attempts.json](math/attempts.json) en conserve les commandes, codes et
messages transcrits. Leurs anciens scripts et stdout n'ont pas été
conservés : cette limite est explicitement déclarée, sans reconstruire
de faux instantané. Aucun échec produit n'en est déduit.

Rejeu depuis la racine d'un worktree contenant le pin :

```sh
MHGP11_AUDIT_TREE="$PWD/morsehgp3D_v11" python3 -B morsehgp3D_v11/receipts/audit_geant_20261005/math/check_math.py
MHGP11_AUDIT_TREE="$PWD/morsehgp3D_v11" python3 -O -B morsehgp3D_v11/receipts/audit_geant_20261005/math/check_math.py
```

## Contrats natifs et brouillons

[Revue native](native/README.md), [modèle portable](native/check_native.py),
[résultat normal](native/result.json) et [résultat `-O`](native/result_opt.json).
Les 263 848 gardes sont des contrôles **de modèles arithmétiques**, pas
263 848 appels C++. Le nombre de nœuds est comparé à une récurrence
indépendante ; les certificats entiers sont évalués en entiers Python.
Le manifeste du sous-dossier est une capture, pas un lecteur LIVE des
worktrees du développeur.

[wip_manifest.json](wip_manifest.json) et `wip_sources/` préservent les
octets des dix fichiers nécessaires à la contrelecture du raccord
S5/S6. Ces sources sont **non commises, non importées dans le produit**.
La [fixture inter-Session](native/api_session_identity.cpp) est proposée
pour une future porte G4 ; elle n'a été ni compilée ni exécutée ici.
Le refus `parameter_out_of_range` qu'elle propose doit être entériné
avec le contrat public du correctif.

## Qualification historique rejugée

Le lecteur publié `bench/verify_full_captures.py` a rejugé QUAL et PAIRED
du reçu `qualification_performance_20261003`, avec son paquet source c40,
en normal et `-O` : [rapports](qualification/full.normal.json) identiques
à [la lecture optimisée](qualification/full.opt.json), code 0,
intégrité conforme, **4 073 portes, 326 mutants et 81/81 prises**.
Il relit la preuve du worker ; il ne relance pas les portes et ne rehache
pas des dumps/binaires retirés.

```sh
python3 -B morsehgp3D_v11/bench/verify_full_captures.py --qualification morsehgp3D_v11/receipts/qualification_performance_20261003/captures/qualification --paired morsehgp3D_v11/receipts/qualification_performance_20261003/captures/paired --source-package morsehgp3D_v11/receipts/qualification_performance_20261003/captures/sources/c40f40798/package.tar.gz --out /tmp/v11-audit-full-normal.json
python3 -O -B morsehgp3D_v11/bench/verify_full_captures.py --qualification morsehgp3D_v11/receipts/qualification_performance_20261003/captures/qualification --paired morsehgp3D_v11/receipts/qualification_performance_20261003/captures/paired --source-package morsehgp3D_v11/receipts/qualification_performance_20261003/captures/sources/c40f40798/package.tar.gz --out /tmp/v11-audit-full-opt.json
```

La source qualifiée est `c40f40798375a0fc37917499401f16876cccbd2a`, et la
baseline `895680ff8` est elle aussi une v11. Aucun différentiel canonique
v10/v11 sur trames LiDAR entières n'est clos par ces 81 prises.

## Fermeture de cette publication

`check_style.py` rend `style_ok fichiers=424` en normal et `-O` sur le pin
audité ; commandes et résultats dans `style_normal.json` et
`style_opt.json`. Les liens locaux des six notes actives ont été contrôlés.
Les empreintes des artefacts de cette publication sont dans `SHA256SUMS`.

Aucun build/test natif, benchmark, fit HDBSCAN ni donnée KITTI dans cette
publication. **GCP non utilisé.** Aucun nouveau contrat 100 ms, K10,
multi-millions, plusieurs séquences ou sortie native points n'est acquis.
