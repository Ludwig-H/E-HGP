# Réduction ordonnée du flux : conformité et mesure mono

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype privé, sans remplacement du moteur actif. GCP non utilisé.

## Ce qui change réellement

Le producteur fenêtré conserve sa géométrie, ses semis complets, ses quatre
buffers de fenêtre et l'ordre original des occurrences. Un Kruskal ordonné
remplace la pile de certificats : une DSU par domaine de hubs, une visite
par arête et aucun retri ni recompaction des hubs. Les lookups d'extrémités
restent binaires. Le compact natif final est encore exécuté : ses tris,
sa DSU et sa résidence ne sont pas annoncés supprimés.

Le certificat conserve toutes les coupes ouvertes/fermées. Le départage
des arêtes à date égale peut différer de la pile, sans changer les
multifusions atomiques. Dans ce flux précis, chaque premier représentant
est une arête pivot retenue ; leur contraction laisse déjà une forêt.
Le producteur vérifie les identités de ces pivots, pas seulement leur nombre.
Il préserve les dates des consommateurs, les marques et les identités natives.

## Qualifications exécutées

Les captures `qualification_o2` et `qualification_san` passent chacune
23 commandes, avec sorties identiques et ASan/UBSan/LSan actifs pour SAN :

- 114 vrais census, s8/10/12, 456 essais W1/7/31/4096 ;
- 253 224 terminales comparées directement, 30 394 993 contrôles ;
- 29 784 coupes et 15 594 832 contrôles verticaux par l'oracle T2 borné ;
- 82 368 pivots retenus, autant de boucles projetées, aucun cycle après
  projection ; 54 612 comparaisons d'arêtes entre frontières de fenêtres ;
- dix mutations de données/compteurs et trois refus causaux, code 4 ;
  arguments absents/inconnus, code 2.

La comparaison au Builder utilise une bijection des identités natives.
La comparaison physique est exigée entre les voies du NOUVEL encodage
graph_full et les consultations CPU1/4, avec UNE banque partagée. Elle ne
promet pas les anciens indices bruts du Builder. Le census exact complet
validé reste une prémisse et la géométrie est la référence scalaire test-only.
Les essais n32 sont différentiels, pas un oracle exhaustif supplémentaire.
La nouvelle voie ne matérialise toujours ni targets[R] ni le graphe complet.

La primitive abstraite est qualifiée séparément : 29 graphes, 116 essais,
3 834 paires de coupes, 257 refus exacts, 16 contrôles d'empoisonnement,
521 934 vérifications. Une fixture franchit réellement W4096 ; les triangles
à égalité changent de certificat mais conservent la multifusion ternaire.
O2 et SAN ROOT rendent le même résultat. La première capture SAN de l'agent
échoue réellement sous LSan/ptrace : conservée comme `failed`, jamais promue.
La relance ROOT passe avec détection des fuites inchangée.

## Mesures et limites

Le benchmark inclut index, génération, census, validation commune et
reconstruction jusqu'à toute la tour K1..10 retenue, libérations comprises.
Synthèse du nuage, digests et comparaison sont séparés. Aucun oracle
exhaustif ni ensemble des descendants n'appartient à ce chemin mesuré.
Les sondes n200/400/800 font une comparaison physique linéaire directe ;
elles chevauchent SAN et ne donnent pas une latence isolée ni un RSS par bras.

Les deux processus n8000 sont successifs, après fermeture des compilateurs
et gates ROOT, avec un thread par étape. Hôte virtuel partagé AMD EPYC 9V74,
huit CPU logiques exposés ; aucune exclusivité matérielle revendiquée.
Uniforme u16, seed3, s8, W65536. Résultats de la paire fraîche ci-dessous.

| Voie n8000 | Tour entière (s) | Depuis census validé (s) | MEB | RSS (KiB) |
| --- | ---: | ---: | ---: | ---: |
| Flux ordonné, exécuté en premier | 207,866762405 | 98,658965782 | 4 359 540 | 2 769 680 |
| Référence matérialisée fraîche | 213,064476440 | 102,225339042 | 3 947 627 | 2 731 660 |

La paire fraîche n'établit pas un speedup stable de 2,4 %. L'extraction
ordonnée, compact natif inclus, paie 72,111171096 s contre
59,344379867+6,453759612 s pour extraction/certificat de référence ;
les histoires et l'export coûtent en revanche davantage dans ce second
processus. Le résultat revendiqué est le travail de tri supprimé, pas
l'attribution causale de tous les écarts de temps d'une paire sur hôte partagé.

La voie ordonnée donne le même digest dense
`a19a83fa4d646e4e0505ba2968ed9120fe8cc80aacccbf0fec61e53ad09b331f`
que les deux anciens bras, mêmes 3 976 472 nœuds et 10 456 312 occurrences.
Elle conserve les 4 359 540 MEB du flux à fenêtres identiques. Les lookups
des hubs et leurs unions ne sont payés qu'une fois par arête : 10 456 312
visites au lieu des 48 390 815 de la pile précédente. Les tris d'arêtes
des hubs passent à zéro. Le compact natif ne reçoit plus que 2 404 636
arêtes, au lieu de 3 910 849 ; il les conserve toutes.

La précédente mesure de pile était de 250,407612046 s et 2 770 676 KiB.
Le correctif donne 207,866762405 s et 2 769 680 KiB : amélioration observée
du temps par rapport à ce run historique, pas une statistique de speedup
sur hôte isolé. La mémoire ne baisse pratiquement pas. Les comparaisons
inter-processus portent sur les digests denses et compteurs, pas sur un
comparateur linéaire exécuté entre ces processus.

Les capacités DSU, domaines, métadonnées, fenêtres et certificats sont
séparées ; leur somme n'est pas un pic RSS. La DSU des hubs reste vivante
pendant le compact natif. Les masques u16[R], φ, catalogue, semis, marques,
histoires et sortie demeurent. Aucun gain universel sous-quadratique en n
n'est déduit : la taille des intermédiaires et de la sortie reste à mesurer.
La mesure n16k est concurrente à deux sondes externes probe_dag et une
compilation ; ses temps ne sont pas présentés comme isolés. Des compilations
externes sont également observées pendant n32k. Le facteur
s10/s12 à grande taille reste à requalifier. Aucun contrat 50k/1 s, 100 ms
ou dizaines de millions de points sur G4 n'est acquis.

Triplet clos, même famille uniforme/seed3, s8, K1..10, W65536, un thread
par étape. Les trois captures passent, avec une visite par occurrence,
zéro tri d'arêtes de hubs et compact natif toujours exécuté.

| n | Tour entière (s) | Occurrences R | MEB payés | Nœuds FULL | Pic RSS (KiB) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 207,866762405 | 10 456 312 | 4 359 540 | 3 976 472 | 2 769 680 |
| 16 000 | 595,243697109 | 21 948 186 | 9 364 101 | 8 310 399 | 5 663 636 |
| 32 000 | 1 076,969155067 | 45 453 599 | 19 784 213 | 17 166 975 | 11 604 252 |

Les facteurs à chaque doublement valent 2,099 / 2,071 pour R,
2,148 / 2,113 pour les MEB, 2,090 / 2,066 pour les nœuds et
2,045 / 2,049 pour le RSS. Cela observe une croissance sous-quadratique
de ces volumes sur ce triplet uniforme, pas une preuve pour les autres
régimes ni une loi de latence déduite des temps sous charge. La sortie
explicite est déjà volumineuse : 17,17 millions de nœuds à 32k points.
Les 13 502 432 pivots sont retenus et seuls projetés en boucles ; les
10 380 954 arêtes du compact natif final sont toutes conservées.
Digest dense n32k :
`6b68997e111e910c4243cc5ce714b9efc9e63754f2ee5524920f5e591734301e`.
À n16k/32k, il n'y a ni seconde voie exécutée ni oracle exhaustif ; le
verdict porte sur le parcours de la sonde et ses contrôles enregistrés,
sans transformer ces grands runs en nouvelle preuve géométrique.

Comparaison s=8/10/12 à n800, W65536, K1..10 : chaque bras passe son
comparateur physique direct ; entre facteurs, le lecteur compare les digests
FULL, compteurs par K et travail géométrique. Les trois donnent
238 123 boules retenues, R=792 736 et MEB=300 926 pour le flux ordonné.
Les candidats bruts valent 240 719 / 239 504 / 238 935. Les temps de ces
petites sondes ne choisissent pas un optimum ; cette couverture ne remplace
pas la comparaison à grande taille ni celle des autres familles de nuages.

## Prochain delta : brouillon distinct, NON compilé

`birth_streaming_graph.hpp`, SHA256 `b2a472dbcc4f5f82e14b66146b99ff1d873d58a79ec889240500c9e62f056853`,
reprend la [proposition de contraction immédiate de l'auditeur](../../audits/receipts_birth_stream_20260911/README.md),
publication 03198682. Son modèle et lecteur ont été contre-exécutés par ROOT
normal/−O : 14 cas, 42 essais, 3 276 comparaisons BFS, neuf rejets. Il s'agit
du modèle indépendant, pas d'une qualification du présent brouillon C++.
Il affecte un label dense de naissance à local0, puis ne soumet au DSU que
les autres occurrences. φ reste une identité stable, jamais une racine DSU,
et sa conversion en BlockId attend la fin du K. Ce draft retire le domaine
hubs et ses lookups, le certificat intermédiaire et la compaction finale.
La preuve est favorable sous les prémisses d'ordre et d'antériorité stricte ;
aucune compilation, qualification ou vitesse de ce draft n'est revendiquée.
Le lecteur impose son absence de toutes les fermetures de capture.

Les fenêtres indépendantes arrivant hors ordre gardent la voie composable.
Ce correctif mono ne parallélise ni la génération WSPD, ni la géométrie,
ni les histoires. La distribution massive reste à implémenter et mesurer.

## Reproduction et stockage

Parent unique : [streaming_graph_20260911](../streaming_graph_20260911/README.md),
manifeste `348810e5501edad16b7ef3fa1a846be82248bce526fc8e3b5a1a8e5dd9c859a4`.
Les sources partagées sont empruntées par hash, les objets locaux sont
adressés par contenu. Aucun ELF, vendor ou arbre de build complet embarqué.
Le manifeste distingue sources consommées, notes, outils et brouillon.
Les commandes, captures brutes, dépendances, snapshots et fermetures
source/compilateur/headers/ELF restent reproductibles, sans prétendre à
un sysroot hermétique du linker/runtime. Quinze captures closes : quatorze
réussites et le refus LSan initial conservé, 78 commandes. Le lecteur vérifie
les résultats contre les captures brutes avant toute publication.

```bash
python3 -B morsehgp3D_v7/receipts/ordered_streaming_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/ordered_streaming_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/ordered_streaming_20260911/verify.py --extract /tmp/mhgp7-ordered-replay-neuf
```

Choisir une destination absente. L'extraction recrée les chemins `build/`
pour les includes relatifs ; consulter `record.py` et `record_primitive.py`
pour reconstruire dans une destination neuve, sans écraser les captures.
Les sources actives full_ball_tower `83f1c78e…` et anchor_meb `386072c8…`
restent inchangées. Aucun CTest du moteur actif n'est réattribué à ce paquet.
