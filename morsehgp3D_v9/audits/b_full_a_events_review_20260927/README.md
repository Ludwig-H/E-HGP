# Contre-audit — constructeur événementiel A sériel

27 septembre 2026. Lecture indépendante des sources et des lecteurs du
[prototype événementiel](../b_full_a_events_20260927/README.md), sans
modification de ses sources, sans nouveau build, benchmark ou GCP.

## Verdict

**R1 passe la contrelecture LIVE normale et `-O`, puis les 25 corruptions
du lecteur dans les deux modes.** Aucun défaut bloquant restant trouvé.
Le [reçu](../b_full_a_events_20260927/receipts/r1/capture.json) ferme
17 commandes et 1 141 dépendances. Release GCC et Clang ASan/UBSan/LSan
rendent les mêmes objets et compteurs.

Ce résultat qualifie un autre calcul de A **séquentiel**, conditionnellement
aux entrées natives du manifeste. Ni le générateur, ni B/C, ni l'encodage,
ni une tour complète ou une accélération GPU ne sont qualifiés ici. La
[contrelecture du manifeste](../b_full_a_manifest_review_20260927/README.md)
reste une preuve distincte.

## Points mathématiques et de structure vérifiés

Les arêtes de représentants ont le rang de leur bloc source ; leurs cibles
sont strictement antérieures. Leur parcours suit donc déjà les poids
croissants. Kruskal peut les consommer sans tri ni copie globale d'arêtes
supplémentaire. Les unions par rang ne changent pas les composantes des
coupes ; elles ne fixent pas les IDs de sortie.

La forêt couvrante minimale conserve les composantes pour chaque coupe
ouverte ou fermée. Les tables ancêtre/maximum rendent une étiquette de
composante ; le maximum du chemin empêche de franchir une arête trop haute
simplement parce qu'une arête suivante est basse. Deux enracinements
opposés sont testés : les étiquettes internes peuvent changer, pas les
objets HGP reconstruits.

Les groupes fermés sont ordonnés par `(rang, premier ordinal de bloc)`.
Les prédécesseurs des parents sont cherchés strictement avant le rang du
groupe. Les redirections à un parent remontent à un événement antérieur,
jamais à un futur successeur : les racines par occurrence restent donc
historiques, non remplacées par la racine finale de toute la tour.

La matérialisation retrouve l'ordre des nœuds et des contributions natifs,
les parents uniques triés et les niveaux rationnels du premier bloc de
chaque plateau entier. K1 crée explicitement les sites de rang zéro, dans
l'ordre du domaine, sans références ball-tagged ; les trous des autres
rangs restent intacts. Les compteurs sémantiques excluent ces sites des
statistiques de lots de boules.

Dans cette couture à étiquette « plus haut ancêtre », un événement muet
peut changer l'étiquette interne sans changer de segment HGP. Son entrée
d'historique relie alors cette nouvelle étiquette à l'ancien segment ;
la supprimer n'est pas valide. Ce n'est **pas** une nécessité absolue de
conserver tous les événements muets dans toute représentation imaginable :
voir la [piste de labels stables](NEXT_MIN_LABEL.md), non implémentée et
non couverte par R1.

## Couverture réelle, pas héritée

Le corpus géométrique fournit 22 variantes, 44 captures et 376 appels du
constructeur événementiel : 188 manifestes rejugés dans deux enracinements.
Il compte 4 632 sommets, 5 416 occurrences, 3 784 groupes, 384 groupes muets,
32 continuations et une fusion à 32 parents. Les incidences sont distinctes :
3 408 entre événements, 3 024 dans les drafts, 2 992 dans les forêts de
nœuds. Les requêtes d'ancêtres sont exactement `V+E = 10 048`, avec
54 032 étapes de table sur ce corpus.

Deux cas limites supplémentaires rejouent K1 avec un seul site non
identitaire et aucune boule ; 64 autres appels portent sur 32 programmes
combinatoires de 8 à 257 blocs. Ces données abstraites ne sont pas des
catalogues géométriques et ne sont pas incluses dans les totaux V/E/G
ci-dessus. Leur génération par scans de préfixes n'est pas le candidat.

Les trois branches mutantes du même binaire sont tuées causalement :
coupe parent fermée (`event.predecessor_missing`), retrait des historiques
sans contribution, inversion des groupes d'un plateau (ces deux dernières
divergent sur `replay.occurrence_roots`). La deuxième retire aussi des
fusions sans contribution, pas seulement des groupes inertes à un parent.
Ce ne sont pas trois compilations mutantes indépendantes.

## Coûts et limites à conserver

La borne publiée `O(N+B+(V+E) log(V+E+1))` tient compte des tris locaux
d'occurrences, pas seulement des tables d'ancêtres. La mémoire
`O(B+E+V log(V+1))` exclut l'entrée possédée. Aucune borne sous-quadratique
en nombre de points LiDAR n'en découle sans mesurer le générateur et le
manifeste. Kruskal, l'enracinement et plusieurs scans restent séquentiels.

Le maximum observé de capacités temporaires + sorties est 13 004 octets
sur ce corpus et cette ABI. Il ne comprend ni entrée, ni piles de tri,
ni surcoût d'allocateur, ni coexistence interne à une réallocation ; ce
n'est pas un pic RSS ou une borne universelle. Les allocations initiales
en V et les libérations explicites sont payées dans le temps interne.
Aucun chrono de gate n'est promu en mesure de performance.

Les lecteurs épinglent source du port, modèle, dépendances effectives,
objets, binaires, recettes et sorties. Ils restent LIVE et dépendants des
builds locaux. Les vingt-cinq corruptions testent leurs rejets, pas des
motifs de refus du moteur ni une nouvelle qualification de concurrence.

## Reproduction et hashes

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_events_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_events_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
python3 -B morsehgp3D_v9/audits/b_full_a_events_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_events_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
```

- `events.hpp` : `8c1280e9082d20e69cc1c47240160c336cb54f30d88594d34722db944d6de94f`.
- `probe.cpp` : `133756d8b9a64016774ff5c8378d3958f3c8bef5067f35bb2dff3ac5805c53f2`.
- `run.py` : `f232eb0312cfa68e9ff52d9c27a4dfb2f96d688eb0b9a4b613c8763bca15b5d3`.
- `selftest.py` : `2a9a52b6eb852fbf9438e4662ead847aadc5f14a0891a24baae4f3ec10c08105`.
- R1 `capture.json` : `4287d5ed5802adc564b13e73a8475eb4753ffefe3317b02a27c6a552f6ac2a93`.
- R1 `summary.json` : `4c6fadbe42cd017b665e261e8db9db10b70a8cd17944b80016890ada80a7a763`.

Note close ; aucune preuve d'autrui modifiée.
