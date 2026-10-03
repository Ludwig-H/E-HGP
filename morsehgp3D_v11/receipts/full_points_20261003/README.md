# FULL → hiérarchie de points : résultats clos

3 octobre 2026. Cadre : `phase=exploration_v11_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u21_input_only`,
`public_status=not_claimed`. Travail d'audit, sans modification du moteur.

**17/17 exemples terminés sur G4 : douze synthétiques et cinq scènes Zoltan
entières, 68 fits HDBSCAN, 2553 comparaisons par objet.** La contrelecture
indépendante recoupe 17712 gardes, scores et empreintes. Toutes les sessions
sont closes avec arrêt ciblé certifié. La fermeture qualifiée est stable,
mais les résultats ne justifient pas d'en faire seule la règle finale.

## Résultats et périmètre

Le score est la moyenne, par objet suivi, de son **meilleur IoU parmi les nœuds
actifs de chaque hiérarchie**. Les labels servent uniquement à ce diagnostic ;
la construction ne les utilise pas. Ce score ne qualifie pas une partition
automatiquement choisie. À k=5 :

| Exemples | Core | Première couverture / LCA | Fermeture m=3 | Fermeture m=6 | Fermeture m=20 | HDBSCAN |
|---|---:|---:|---:|---:|---:|---:|
| Douze synthétiques, 96 objets | 0,741753 | 0,845813 | 0,817622 | 0,812322 | 0,811243 | 0,774573 |
| Cinq Zoltan, 15 objets | 0,576728 | 0,645993 | 0,611804 | 0,636648 | 0,599193 | 0,602980 |

m3 face à HDBSCAN : **63 gains / 16 pertes / 17 égalités** en synthétique,
**8 / 3 / 4** sur Zoltan. Face aux premières attaches/LCA : **20 / 60 / 16**
et **3 / 9 / 3**. Canonique et LCA ont les mêmes scores sur ces nuages ; les
fixtures symétriques exactes les distinguent.

Sur Zoltan à k=10, m11 atteint 0,616858 contre 0,613788 pour LCA et 0,567363
pour HDBSCAN, avec 12 gains / 0 perte / 3 égalités face à HDBSCAN. L'identité
`(k,k+1)=(k+1,k+1)` signifie que m11 est le quotient de coassociation d'ordre11 ;
le comparateur préenregistré reste HDBSCAN `min_samples=10`. Ce résultat ne
contrôle donc pas un paramètre effectif identique ni une supériorité générale.

[Synthèse](results/comparison.json), [scores par objet](results/objects.csv),
[sélection et provenance des 17 résultats complets](completed_cases/selection.json).
Recalcul sans natif ni fit, depuis la racine du dépôt :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v11/receipts/full_points_20261003/experiment/summarize.py --campaign morsehgp3D_v11/receipts/full_points_20261003/completed_cases --out morsehgp3D_v11/receipts/full_points_20261003/results --require-all
```

Le [plan préenregistré](experiment/campaign.json) fixe k=2/3/5/10 et les seuils
distincts {3,k+1,20}. Quatre familles synthétiques, graines9331/9332/9333,
2000 sites et huit objets par nuage. Les cinq scènes Zoltan ont
67114 / 76011 / 44339 / 126267 / 72426 sites, trois objets suivis chacune.
Elles viennent de quatre trames de la seule séquence08 ; 01/04 sont la même
trame sans/avec sol. Ce sont des exemples de développement sélectionnés.

Même XYZ u21/grille1mm pour HGP et HDBSCAN, IDs de retours conservés, aucun
sous-échantillonnage. Le domaine observé peut tenir en18bits sans changer le
profil compilé u21. Les void participent à la géométrie et au seuil, mais sont
exclus du dénominateur IoU ; background et autres objets y participent.
[Inventaire, masques et empreintes](experiment/inventory.json). Aucun brut KITTI
dans Git. HDBSCAN est celui de sklearn1.7.2 : `min_samples=k` incluant le point,
`min_cluster_size=2`, euclidien, kd_tree, alpha1, n_jobs1 ; arbre brut avant
condensation. [Sources officielles épinglées](experiment/hdbscan_sources.json).
Les hauteurs HDBSCAN et les β=r² HGP ne sont pas comparées comme mêmes unités.

## Mathématiques et contrôles

La proposition ferme en équivalence les couvertures FULL de cardinal≥m,
avec singletons inactifs pour compléter la partition. Elle suit toutes les
incidences dynamiques et ferme les plateaux. m est un seuil de transmission,
distinct de la condensation. Elle est laminaire en rayon, équivariante et
stable à **1ε en rayon** pour des IDs appariés et k/m fixés. Cette stabilité
n'implique ni stabilité d'IoU, ni absence de percolation, ni consistance
statistique. L'optimum minmax ne porte que sur les échéances de co-couverture.
[Preuve et concessions](qualified_proof/README.md).

| Vérification | Résultat |
|---|---|
| [Fixtures et projection](projection/README.md) | 3415 contrôles |
| [Deux triangles exactement équilatéraux dans R³](equilateral/README.md) | 618 contrôles ; m3 retrouve ABC/DEF, premières attaches irréversibles les perdent |
| [Intervalles indépendants](check_intervals.py) | 121 nuages, 2180 appariements perturbés |
| [Frontières perturbées et identités de seuil](check_jitter_boundary.py) | 8263 gardes ; 1344 bornes sans violation, 2478 identités |
| [Projection directe par forts/faibles](check_strong_projection.py) | 1348 gardes, dont 458 coupes initiales et 568 coupes d'ordre suivant |
| [Premières couvertures qualifiées puis LCA](check_qualified_first.py) | 3274 gardes ; triangles conservés, mais contre-exemple de discontinuité en rayon de8/3 |
| Consommateur d'exports contre Gram/Fraction et Γ | 10091 gardes /32cas /3334coupes |

Ces sorties passent en normal et −O avec empreintes identiques. Les six fixtures
pondérées sont hors du domaine unitaire distinct qualifié. Les tests simulés
des runners, gardes d'hôte, sélection de scènes, restauration et synthèse sont
également conservés ; ils ne remplacent pas les campagnes G4.

Deux identités utiles : **m≤k ne change rien** ; **(k,m=k+1)=(k+1,m=k+1)** pour
les blocs actifs, dates d'entrée et hauteurs. L'extension à m>k+1 est fausse.
Pour k≥2/m≤k, fermer les populations complètes I∪U des seules boules fortes
`p+qmin≤k≤p+|U|` donne exactement ce quotient, sans propriétaires FULL.
Pour m=k+1, les forts d'ordre k+1 sont déjà les faibles du catalogue FULL_k :
`p+qmin≤k+1≤p+|U|`. Aucun nouveau catalogue ni FULL_(k+1) n'est nécessaire.
Ce raccord exige un extracteur à qualifier ; aucun port natif ni gain mesuré.
Il ne construit pas FULL, ne réhabilite pas le foldv4 et ne résout pas la
synthèse de plusieurs ordres k en une hiérarchie unique.

Bridge9331 perturbé à±1mm par axe : 2048 bornes de hauteur sans violation.
m3 garde ses IoU ; m20 perd légèrement un score à k5 et un à k10. Cela confirme
la nécessité de distinguer stabilité de hauteur et qualité des branches.

## Sources figées et sessions

Moteur **c40f40798375a0fc37917499401f16876cccbd2a**, sonde compilée depuis
**a12f7f5425974d398e87271cac0a40b79393f0d2**. ELF561400octets, SHA
`5881224aeae7cd110a6e935cd3490ec9dcbef9ecf616ca4b78b85114c0b0ba36` ;
[manifeste de compilation](sessions/points_synth3/build_manifest.json).
L'orchestration de reprise25c312ac restaure cet ELF, sans compiler b872.
Les populations fortes sont complètes, les propriétaires et plateaux fermés ;
core utilise les vrais dk², même hors des dates du catalogue. Sonde et
consommateur d'audit ne sont pas un module livré dans `src/points/`.

| Session | Statut et preuve conservée |
|---|---|
| [points_synth1](sessions/points_synth1/README.md) | Échec de raccord de chemin après compilation ; aucun test natif commencé |
| [points_synth2](sessions/points_synth2/README.md) | Porte39cas/9379contrôles réussie ; pip absent avant génération/fit |
| [points_python1](sessions/points_python1/README.md) | Préparation d'hôte close, sept phases jointes, DONE0 |
| [points_synth3](sessions/points_synth3/README.md) | DONE0, porte40cas/11203contrôles, 12nuages et48fits complets |
| [points_zoltan1](sessions/points_zoltan1/README.md) | 01–03 complets ; délai1050s pendant l'analyse04, résultat censuré conservé ; 05 non commencé |
| [points_zoltan2](sessions/points_zoltan2/README.md) | DONE0, 04–05 entiers complets sur le même ELF, huit fits ; commande988,630s |

Les deux sessions réelles passent chacune la porte rapide14cas/4105contrôles
au même binaire. Pas de cumul présenté comme nouvelle qualification complète.
Chaque capsule conserve reçu, archive, lancement, préflight et certificat
d'arrêt. La reprise sélectionne uniquement les résultats complets, sans
modifier scènes ou paramètres ; les échecs restent conservés.

Les coûts d'export et d'analyse Python sont distincts du moteur FULL. Ce travail
ne qualifie ni100ms, ni GPU, ni le moteur b872, ni un livrable natif de points.
[Relecture finale](review.json) et [ledger complet](SHA256SUMS).
