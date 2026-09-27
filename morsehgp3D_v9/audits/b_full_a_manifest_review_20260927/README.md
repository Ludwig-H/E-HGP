# Contre-audit indépendant — manifeste FULL A

27 septembre 2026. Lecture de sources et rejeux des lecteurs seulement ;
aucun moteur modifié, aucun build ou test géométrique supplémentaire, aucun
GCP. Sources examinées sur `78b1147c4` avec le nouveau dossier isolé
[FULL A](../b_full_a_manifest_20260927/README.md).

## Verdict borné

**R2 qualifiée pour capturer et rejouer A native sur le corpus déclaré.**
Les lectures LIVE normale et `-O`, puis les vingt corruptions du lecteur
dans les deux modes, ont été exécutées indépendamment et passent. Aucun
défaut bloquant restant trouvé dans cette couture. Ce verdict ne qualifie
ni le futur constructeur événementiel, ni sa parallélisation, ni une tour
LiDAR ou le contrat G4 de 100 ms.

Le [reçu R2](../b_full_a_manifest_20260927/receipts/r2/capture.json)
ferme 17 commandes et 1 140 dépendances : Release GCC et Clang
ASan/UBSan/LSan ont les mêmes résultats. Les 22 variantes de onze fixtures
donnent 44 captures, 376 rejeux d'ordres, 4 176 blocs, 5 416 occurrences,
3 400 actions, 3 024 incidences de parents et 2 456 contributions. Ces
totaux incluent hash/tri et les deux ordres de lecture des slots K ; ce
ne sont pas 376 nuages indépendants.

Non-vacuité contrôlée : 624 blocs inertes, 32 continuations, 480 blocs
étendus, 352 trous de rang global, 888 répétitions de racines dans un bloc,
1 120 partages entre blocs d'un plateau, et une fusion de 32 parents.
Les 708 refus agrègent les contrôles du harnais ; ils ne sont pas 708
motifs de refus du produit. Les cinq mutations sont trois corruptions de
manifeste et deux branches de rejeu dans le même binaire, pas cinq builds
mutants indépendants.

## Pourquoi le contrôle n'est pas circulaire

[instrument.py](../b_full_a_manifest_20260927/instrument.py) a été rejugé
avec `--check` : classe native épinglée, deux renommages et trois hooks
exactement. La copie observe les entrées réellement préparées par A, les
racines avant `sort/unique`, puis sa sortie avant remappage des populations.
Elle ne remplace aucune décision géométrique ou de construction.

Le flux de décision est `Input → Manifest → replay_a`. Le rejeu ne consulte
jamais `Output` capturé pour reconstruire ; celui-ci sert uniquement à la
comparaison finale. Sa DSU locale regroupe les blocs par racines pré-lot,
au moyen d'une table de propriétaires, au lieu de recopier le tri natif des
incidences. Ses historiques sont suivis sans compression. L'égalité porte
sur les racines de chaque occurrence, ancres, successeurs, naissances,
rangs, CSR du draft, références, niveaux représentés et onze compteurs
sémantiques. Les forêts publiées, banques et verticales sont aussi comparées
entre copie observée, copie sans observation, natif static4 et natif mono.

L'invariant inductif essentiel est conservé : toutes les racines d'un
plateau sont lues avant la moindre fusion de ce plateau. Les groupes sont
ensuite traités par premier bloc, parents historiques triés et contributions
en ordre de programme. Un bloc muet garde son ancre ; une continuation
garde sa contribution datée sans créer de nouveau nœud.

K1 distingue rang dans le domaine ordonné, ID original et rang géométrique.
Ses naissances initiales portent le rang zéro et aucune boule de naissance.
Les autres rangs restent `level_run + 1`, avec leurs trous globaux. Le niveau
d'une action vient du premier bloc du plateau entier, même silencieux : les
fixtures ont maintenant des fractions égales de représentations distinctes,
donc choisir le premier contributeur est réellement détecté.

Cette indépendance commence **après** la préparation native des programmes
et des cibles. Leur résolution MEB, le census et la complétude géométrique
des catalogues ne sont pas requalifiés par le rejeu. Le manifeste reste une
structure d'audit publique, pas un certificat géométrique scellé.

## Correctifs obtenus avant gel

La capture doit être admise globalement, et non slot par slot. L'entrée
dans `CaptureScope` invalide désormais une admission précédente ; un succès
suivi d'un réemploi échoué ne laisse plus l'ancienne capture admissible.
`make_manifest` impose aussi `slot.input.k == k`.

Un second témoin termine tous les hooks A puis déclenche une panne native
en B, dans le chemin non recouvrant. Tous les slots sont complets, le motif
de panne est celui du natif, mais aucun manifeste n'est admis. Cela vérifie
le cas que le seul échec précoce de réemploi ne couvrait pas. Les lectures
ont lieu après les jointures ; l'observateur global reste explicitement
non réentrant et non utilisable par deux captures concurrentes.

Le lecteur lie désormais chaque binaire au build correspondant et refuse
des booléens à la place des cinq compteurs entiers de mutations. Le selftest
modifie et repinne les sorties sémantiques avant relecture : ces refus ne
sont donc pas tous de simples échecs de hash. Il vérifie vingt rejets,
pas vingt priorités exactes de motifs.

## Échec conservé et portée des preuves

[R1](../b_full_a_manifest_20260927/historical/R1_FAILURE.md) avait ses
17 commandes réussies mais son lecteur final refusait des dépendances
absentes du pin préalable. L'inventaire `-M` omettait les options
d'optimisation/sanitizers et leurs includes effectifs. R1 n'est pas promue
en qualification par son champ de commandes `completed`. R2 refait les
inventaires avant compilation avec les options correspondantes ; aucun
changement C++ n'est dissimulé par cette reprise du harnais.

La capture recopie les métadonnées du catalogue entier par K, puis les
sorties A. Le juge sans compression est volontairement borné aux petites
fixtures. Aucun temps de gate ou temps natif instrumenté n'est un benchmark
candidat ; aucune croissance sous-quadratique en nombre de points n'en
découle. Pas de gate TSan ajoutée dans cette tranche. Les lecteurs LIVE
nécessitent encore sources, dépendances et builds locaux épinglés.

## Reproduction et identités

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_manifest_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B -O morsehgp3D_v9/audits/b_full_a_manifest_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B morsehgp3D_v9/audits/b_full_a_manifest_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B -O morsehgp3D_v9/audits/b_full_a_manifest_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
```

SHA256 de fermeture examinés :

- `capture.hpp` : `7d3f3e9c925ad670776c6b81a18e76eb06e0d1d41ba360f9f6645a266a21496b`.
- `manifest.hpp` : `4a90d092be2e832fc776a2605ab7817ef85cf44303e7abb7da2eaaaa4135c9fc`.
- `probe.cpp` : `cf32386e6810296c58ec45828894ba3dca52ff6529e36504c12bdb5b199cf799`.
- `run.py` : `dd36210c8f9a42aeb8a4b597d137745b1ae1f8e9bf6079c7a6bd27fc262d5c61`.
- `selftest.py` : `6fd20a29a069bc251c14117807540bd435aaf0c006f29502bbda5a23092c1cb5`.
- R2 `capture.json` : `3a288f2e945054c613f58835b05d2222a6edb0e6f353a0720341dbfeb6500a77`.
- R2 `summary.json` : `ea5f9e55de735150c97e312b5e63423cb869afa7b0ae58bbe3e8ffa2477e014f`.

Note close. Les sources et reçus d'autrui n'ont pas été modifiés.
