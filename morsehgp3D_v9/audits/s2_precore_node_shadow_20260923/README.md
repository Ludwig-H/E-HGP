# Shadow exact avant le cœur : huit cellules et gardes par nœuds

23 septembre 2026. Une expérience **audit-only**, sans modification du
générateur, teste la [proposition de certificat corrélé](../CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md)
sur de **vraies arêtes survivantes S2**. Source de la trace S2 :
`c265a5dae4dd92059fc78acc0a1d7f52de9c1435`, CPU, trame brute
SemanticKITTI 08/000000, grille commune 1 mm/u18, K5/s8, q3+q4. Le
[panel des segments](../s2_segment_panel_20260923/README.md) fournit le
plein dense et le quart physique `x≥0,y≥0` à densité 1/4. Aucune hypothèse
d'alignement des points ou des captations n'entre dans le certificat.

Le [sidecar](measure.cpp) reconstruit le même front WSPD `MidpointSamples`
et le filtre de rectangles `Affine` avec l'archive `libmhgp9_gen.a` de SHA
`208aabb30a76…`, **distincte du binaire qui a écrit la trace**. Il joint
chaque arête par ses IDs bruts, vérifie masque et unicité, puis essaie
seulement les segments de **≥16** survivantes. Les cinq comptes
front/filtre, les arêtes, les masques q3/q4, `F` et la masse des grands
segments sont identiques au panel épinglé dans les cinq sorties. Il ne
relance ni le générateur complet, ni le cœur, ni la tour FULL.

Pour un segment `E`, l'AABB extérieure des disques nominaux **q4** de
ses arêtes est clippée à la boîte réelle du sous-nuage, puis divisée en
huit sous-cellules 3D fermées. Ses 27 sommets distincts sont dyadiques
en coordonnées u18. À chaque sommet `v`, le sidecar calcule exactement
`Q_E(v)=min_(a,b)∈E (|a−v|²+|b−v|²)`. Pour un nœud `N` de l'index
global, `U_N(v)` majore la distance carrée de **chacun** de ses sites à
`v` par sa boîte. Le nœud, entièrement hors des plages originales
`A∪B`, crédite sa population seulement si `2U_N(v)<Q_E(v)` aux huit
sommets de la cellule. Les nœuds crédités sont disjoints **dans une
cellule**. Quatre gardes distincts (`K−1`) dans **chacune** des huit
cellules ferment ensemble q3 et q4 pour **toutes** les arêtes du segment ;
aucun crédit n'est transmis entre cellules ou ajouté à S2. Les voies
propres à chaque arête restent dans la trace. Les coordonnées des sommets
sont multipliées par quatre, et les sommes de distances directes tiennent
en i64 signé pour cette échelle u18 ; tous les calculs et le test strict
sont entiers. Une recherche arrêtée au budget **ne prouve rien** et
retombe sur la voie existante.

`F` ci-dessous est `core_sites`, extrémités comprises, et « F fermables »
la charge du cœur **théoriquement évitable avant `load`** pour les seuls
segments complètement certifiés. Ce n'est ni une baisse de temps ni une
économie du cover/catalogue effectivement mesurée. `Q` compte les termes
paire–sommet ; « visites » est la somme des nœuds d'index visités sur
toutes les cellules. Le temps `shadow` comprend préparation des cellules,
`Q_E` et recherche, mais **exclut** la lecture des traces, la
reconstruction du front/filtre, l'énumération de `A×B` et l'assemblage
des segments : ce n'est pas le surcoût intégré d'un port S2.

| Cas / parcours / budget par cellule | Segments ciblés | Cellules prouvées | Segments fermés | F fermables / F total | Q | Visites | Shadow |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Plein dense, proche d'abord, 64 | 11 174 | 28 759/89 392 | 36, 974 arêtes | 458 001 / 559 661 741 = **0,0818 %** | 10 704 987 | 4 594 952 | 0,232 s |
| Plein dense, proche d'abord, 256 | 11 174 | 36 148/89 392 | 103, 2 421 arêtes | 753 058 / 559 661 741 = **0,1345 %** | 10 704 987 | 15 308 549 | 0,677 s |
| Quart clairsemé, préordre, 64 | 101 | 0/808 | 0 | 0 / 2 229 643 | 63 261 | 51 712 | 0,0010 s |
| Quart clairsemé, proche d'abord, 64 | 101 | 267/808 | 0 | 0 / 2 229 643 | 63 261 | 40 177 | 0,0018 s |
| Quart clairsemé, proche d'abord, 4 096 | 101 | 358/808 | 1, 16 arêtes | 848 / 2 229 643 = **0,0380 %** | 63 261 | 1 872 246 | 0,040 s |

Sur le plein, passer de 64 à 256 visites ajoute seulement **295 057 F
fermables** pour **10 713 597 visites** et environ 0,445 s de tentative
supplémentaires sur cet hôte partagé. Toutes les cellules non prouvées
s'arrêtent au budget dans ces configurations. Le préordre à 64 visites
ne trouve aucun nœud admis dans le quart, tandis que l'ordre proche
d'abord trouve des cellules : l'ordonnancement compte. Un crible entier
distinct du certificat, dans le même sidecar, constate qu'aucune cellule
testée ne contient le disque q4
nominal complet d'une arête S2 : **zéro obstruction de ce type** dans
les deux cas. Ce zéro ne signifie pas que le certificat réussirait sans
budget ; la boîte 3D commune surapproxime les vrais plans bissecteurs.

La trace après le **cœur diamétral** garde 9 arêtes avec une voie ouverte parmi les
974 arêtes fermées par le shadow à budget64, et 39 parmi les 2 421 à
budget256. Le masque post-cœur est celui d'un autre prouveur, limité au
cœur diamétral et à sa profondeur 6 : une non-fermeture n'est pas un
contre-exemple géométrique. Pour les **39 IDs distincts**, le
[lecteur indépendant](verify.py) recontrôle, avec des coordonnées
u18 conservées dans [`RECEIPT.json`](RECEIPT.json), les huit cellules
fermées, quatre gardes distincts par cellule, **toutes** les inégalités
strictes à leurs sommets et la couverture du disque q4 clippé de chaque
arête. La [voie exacte par arête](check_exact_edges.cpp)
`run_q34_edge_candidates` donne **0 émission q3 et 0 émission q4** pour
ces 39 arêtes, avec 165 567 sites de cover et 35 403 visites en tout
([sortie](exact_edges.stdout)). Ce contrôle ne mesure pas un catalogue
FULL avec le nouveau certificat activé et ne vaut pas preuve indépendante
de toutes les 2 421 fermetures : pour les autres, le sidecar repose sur
sa preuve entière de groupe et ses vérifications structurelles.

**Décision utile :** ne pas porter en production cette première grille
3D commune de huit cellules comme réponse principale à la croissance
du cœur. Elle cible 85,52 % de F par ses grands segments, mais en ferme
moins de 0,14 % de F total sur le plein testé, avec `27S` calculs et
des millions de visites supplémentaires. La prochaine ablation la
plus informative doit, sur le même flux S2, restreindre chaque cellule
aux arêtes dont le **disque peut l'intersecter** (`E_C` sûr, avec
sur-approximation), écarter les cellules vides et mesurer si un domaine
2D dans le plan bissecteur améliore la fermeture sans reporter un travail
par site du cœur. Séparer `E_q3/E_q4` et le seuil q4 seul `K−2` est une
autre relaxation exacte. Garder un budget fixe et le repli, puis juger
brut/sans-sol, K5/K10, s8/10/12, plusieurs scènes et le coût de chaîne
avant tout port G4. Aucun contrat 1 s, croissance sous-quadratique globale
ou gain GPU n'est acquis ici.

Les cinq sorties, la [provenance](RECEIPT.json), les sources et les SHA
sont versionnés ; les traces et binaires d'entrée restent en `/tmp`.
Relecture statique après fermeture du codespace :

```sh
python3 -B morsehgp3D_v9/audits/s2_precore_node_shadow_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/s2_precore_node_shadow_20260923/verify.py
(cd morsehgp3D_v9/audits/s2_precore_node_shadow_20260923 && sha256sum -c SHA256SUMS)
```

Pour refaire les sorties pendant que les octets éphémères existent,
depuis la racine du dépôt :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/s2_precore_node_shadow_20260923/measure.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-shadow-replay-bin
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/s2_precore_node_shadow_20260923/check_exact_edges.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-exact-edge-replay
```

Appeler `measure` avec `points.u32le raw_return_ids.u32le trace/`, plus
la trace post-cœur pour le plein, `--near` pour l'ordre proche et
`--budget=N` ; produire les cinq noms `*.stdout` du tableau ci-dessus
dans un répertoire de rejeu. L'oracle `check_exact_edges` prend les deux
sorties `full_near64.stdout` et `full_near256.stdout` après les deux
fichiers d'entrée. Par exemple, pour contrôler les cinq sorties et les
traces LIVE après rejeu dans `/tmp/mhgp9-shadow-replay` :

```sh
python3 -B morsehgp3D_v9/audits/s2_precore_node_shadow_20260923/verify.py \
  --capture --outputs /tmp/mhgp9-shadow-replay \
  --receipt /tmp/mhgp9-shadow-replay/RECEIPT.json \
  --inputs /tmp/mhgp9-s2-scaling-20260923-inputs \
  --traces /tmp/mhgp9-edge-core-audit-20260923 \
  --after-traces /tmp/mhgp9-edge-core-after-audit-20260923 \
  --archive build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a \
  --shadow-binary /tmp/mhgp9-shadow-replay-bin \
  --exact-binary /tmp/mhgp9-exact-edge-replay
```

Le lecteur vérifie notamment les SHA des huit parties de chaque trace
LIVE, puis refuse d'écraser un reçu existant. Le reçu versionné et les
sommes SHA gardent l'identité des octets de cette capture après l'arrêt du
codespace ; les durées mur d'un rejeu varieront.
