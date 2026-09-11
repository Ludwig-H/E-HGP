# Résolutions réellement fenêtrées : conformité close, coût mono défavorable

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype privé : le moteur actif n'est pas remplacé. GCP non utilisé.

Le producteur ne possède plus de tableau global de terminales ni de graphe
complet. Il résout une fenêtre de facettes, conserve un pivot par bloc et
alimente des certificats MSF sur hubs, ensuite projetés vers les naissances.
Catalogue, atlas et masques u16[R], semis, marques, certificats et sortie
FULL restent nécessaires. Les contributions datées et les verticales sont
reconstruites par le raccord graph_full épinglé.

## Qualification exécutée

Chaque capture O2 et ASan/UBSan/LSan passe 114 vrais census, s=8/10/12,
456 essais de fenêtres W=1/7/31/4096 et 253 224 terminales observées.
Les juges confrontent les terminales au témoin global, les certificats à
leurs graphes, puis FULL au Builder et à T2 : 29 784 coupes et
15 594 832 contrôles verticaux bornés par build. Huit mutations et trois
refus sont causaux, code 4 ; arguments absents ou inconnus : code 2.
Les sorties O2/SAN sont identiques. Les deux builds ont 21 commandes chacun.

La comparaison au Builder préserve les identités natives par bijection.
L'égalité physique des buffers est exigée entre les voies du NOUVEL encodage
et entre consultations CPU1/4 ; elle n'est pas attribuée à ses anciens
indices de banque ou représentants rationnels bruts. La banque est partagée
par un propriétaire unique. Le helper géométrique reste la référence
scalaire test-only, avec census exact complet validé comme prémisse.

Les compteurs vérifient le travail réellement payé, par K et agrégé.
W=1 exerce l'absence de déduplication ; d'autres fenêtres exercent la
déduplication et le semis après échange. Dans la gate bornée, W4096 ne
traverse pas la fin d'un ordre ; ces frontières sont exercées ensuite par
les sondes n200/n400. Les quatre buffers nommés sont contrôlés, pas un
prétendu RSS déduit de leur capacité. L'abandon du callback survient après
trois occurrences acceptées, sans publication de résultat partiel.

## Mesures locales closes

Le benchmark paie index → génération → census → validation commune → FULL
retenu, libérations incluses. Synthèse d'entrée, digests et comparaison sont
séparés. Il n'embarque ni oracle exhaustif ni étiquettes de descendants.
Le digest dense linéaire inclut niveaux bruts, banque, références, parents,
successeurs, contributions et verticales dans la convention graph_full.

Les sondes n200/n400/n800 passent une comparaison physique linéaire directe.
Elles conservent deux tours, donc leur RSS est combiné ; elles chevauchent
la compilation SAN et ne sont pas des observations de latence isolée.
Les deux processus n8000 ci-dessous sont successifs, après fermeture des
compilateurs et gates du chantier, sans autre benchmark lancé par ROOT.
Hôte virtuel partagé AMD EPYC 9V74, huit CPU logiques exposés ; un thread
amont, un géométrique et un de consultations. Aucune isolation exclusive
de l'hôte n'est revendiquée. Uniforme u16, seed=3, s=8, toute la tour K1..10.

| Voie n8000 | Total jusqu'à FULL (s) | Reconstruction depuis census validé (s) | MEB payées | RSS du processus (KiB) |
| --- | ---: | ---: | ---: | ---: |
| Référence avec terminales et graphes matérialisés | 188,638222200 | 84,949451592 | 3 947 627 | 2 731 664 |
| Flux, W=65 536, pile de certificats composés | 250,407612046 | 146,461485958 | 4 359 540 | 2 770 676 |

Les deux processus donnent le même digest
`a19a83fa4d646e4e0505ba2968ed9120fe8cc80aacccbf0fec61e53ad09b331f`,
les mêmes 3 976 472 nœuds et 10 456 312 occurrences, ainsi que les compteurs
FULL et R par K. Cette comparaison inter-processus est une égalité de
digests/compteurs, pas un rejeu du comparateur linéaire des petites sondes.

**Résultat négatif mono : +32,7 % de temps observé, +10,4 % de MEB et
aucune baisse du RSS (+1,4 %).** Les quatre buffers de fenêtre occupent
6 553 600 octets, mais cela ne paie pas tous les autres objets. La pile
retraite 48 390 815 arêtes pour 10 456 312 arêtes source, en 318 compactions
et 154 fusions de certificats ; elle paie plus de quatre milliards de
comparaisons de tri comptées. L'export et la sortie restent coûteux.
Ces chiffres portent sur cette paire, pas sur une loi de performance.

La campagne de cette variante n'est pas prolongée à 16k/32k ni s10/s12
à grande taille avant correction du réducteur mono. Le triplet et ces
facteurs restent à requalifier sur la variante retenue ; les anciennes
mesures du moteur actif ne sont pas transférées à ce prototype. Aucun
contrat 50k/1 s, 100 ms ou dizaines de millions sur G4 n'est acquis.

## Piste mono suivante, explicitement NON compilée dans ces captures

Les sources `ordered_msf.hpp`, `ORDERED_MSF.md` et `NOTE_ORDERED_MONO.md`
sont un brouillon séparé. L'émission est déjà ordonnée par date/ordinal :
un Kruskal incrémental peut éviter les retris, avec un DSU et un certificat.
Sa preuve conserve toutes les coupes. Dans ce flux précis, tous les pivots
doivent être retenus ; leur projection serait déjà une forêt. Garder
d'abord la compaction native et qualifier cette propriété avant suppression.

Ce réducteur ordonné ne remplace pas les fenêtres indépendantes reçues dans
un ordre arbitraire pour le futur chemin parallèle. Il n'est ni raccordé,
ni compilé, ni crédité des tests ci-dessus. Aucun speedup n'est annoncé.

## Stockage et reproduction

Le manifeste référence un seul parent direct, atlas_graph_full, au SHA
`bb4a482385a7f75537c38d41397d4410d02c04f2a899c6377df79515caf0f9bc`.
Son parent rank_atlas est transitif. Les nouvelles sources et captures
brutes sont dédupliquées par SHA ; les sources partagées ne sont pas copiées.
Les compilateurs, dépendances et ELF exécutés sont épinglés avant/après.
Aucun ELF, Boost ou vendor n'est versionné. Les headers/runtimes système
ne constituent pas un sysroot hermétique.

```bash
python3 -B morsehgp3D_v7/receipts/streaming_graph_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/streaming_graph_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/streaming_graph_20260911/verify.py --extract build/v7_streaming_replay_20260911
```

La destination doit être neuve. Dans l'arbre extrait, les includes relatifs
des sources restent valides. Compiler `build/v7_streaming_graph_20260911/gate.cpp`
avec C++20, `-O2 -pthread -Wall -Wextra -Wpedantic -Werror`, l'include
`build/v7_atlas_graph_20260911` et un Boost système déjà disponible. Pour la
sonde, remplacer la TU par `bench.cpp` ; elle n'exige pas Boost.

Le recorder restitué accepte `--kind gate --out nouveau_nom` (ajouter
`--san` pour les sanitizers), `--kind bench --out nouveau_build`, puis
`--kind bench-run --build nouveau_build --out nouveau_run -- --n=8000
--s=8 --kmax=10 --window=65536 --mode=streaming` sur une seule commande.
Les sorties originales ne sont jamais écrasées. Deux smokes préalables
n32/n200 non scellées ne sont pas réinterprétées comme ces captures.
