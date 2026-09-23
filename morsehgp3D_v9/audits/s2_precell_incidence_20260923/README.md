# Crible d'incidence avant cœur : sûr, mais trop lâche sur huit cellules

23 septembre 2026. Expérience **audit-only**, sans port produit. Elle reprend
le [shadow exact à huit cellules](../s2_precore_node_shadow_20260923/README.md)
sur les arêtes q3/q4 survivant à S2, K5/s8, trame SemanticKITTI 08/000000,
coordonnées sur la même grille 1 mm/u18. Les deux entrées sont la **trame
entière dense** et le **quart physique `x≥0,y≥0` à densité 1/4**. Le front,
les IDs bruts, les masques, les 3 986 433/169 900 arêtes et les charges
`F=559 661 741/2 229 643` rejoignent exactement les traces du panel S2.
L'archive CPU utilisée par le sidecar a SHA `208aabb30a76…` ; les traces
viennent du commit `c265a5da…`, avec leurs SHA dans le [reçu](RECEIPT.json).

Pour chaque cellule fermée `C`, le [sidecar](measure.cpp) construit `E_C`
en excluant **seulement** les arêtes dont le disque nominal q4 ne peut pas
rencontrer `C`. Avec coordonnées multipliées par quatre, `M=2(a+b)`,
`d=b−a`, `D=|d|²` et `δ_i=distance(M_i,[L_i,H_i])`, il rejette si
`Σδ_i²>2D`, si `δ_i²>2(D−d_i²)` pour un axe, ou si la forme linéaire
`d·(v−M)` garde un signe strict sur toute la boîte. **L'égalité est
incidente** : les cellules partagent leurs faces. La [fixture](test_incidence.py)
contrôle les contacts sur sphère, projection, plan et face commune, plus
1 185 petites boîtes contre une recherche entière indépendante.

Dans chaque cellule non vide, `Q_{E_C}` est le minimum exact des sommes
de distances de ses seules arêtes incidentes aux huit sommets. Quatre
gardes disjointes de `A∪B` et entre elles, issues de nœuds disjoints de
l'index global, doivent satisfaire `2U_N(v)<Q_{E_C}(v)` à chaque sommet.
Une arête n'est fermée que si **toutes** ses cellules incidentes sont
prouvées ; une cellule vide n'exige pas de garde. Aucun crédit S2 ou
intercellulaire n'est ajouté. Les 18 arêtes ainsi fermées que le cœur
diamétral antérieur laissait ouvertes ont leurs coordonnées et gardes
conservés dans le reçu ; le [lecteur indépendant](verify.py) recalcule
les huit incidences, la couverture extérieure du disque clippé et les
inégalités strictes aux sommets. Ce lecteur requiert Python sans `-O`.

| K5/64, proche d'abord | Incidences gardées / 8·arêtes ciblées | Arêtes fermées | F fermable / F total | Termes `Q` | Temps shadow |
| --- | ---: | ---: | ---: | ---: | ---: |
| Plein dense, `E_C` | 2 738 013 / 3 171 848 = **86,32 %** | 1 393 | 600 315 / 559 661 741 = **0,1073 %** | 21 904 104 | 0,564 s |
| Plein dense, E commun antérieur | 100 % | 974 | 458 001 / 559 661 741 = 0,0818 % | 10 704 987 | 0,232 s |
| Quart clairsemé, `E_C` | 16 168 / 18 744 = **86,26 %** | 0 | 0 / 2 229 643 | 129 344 | 0,0037 s |
| Quart clairsemé, E commun antérieur | 100 % | 0 | 0 / 2 229 643 | 63 261 | 0,0018 s |

Le crible ferme **419 arêtes** et au plus **142 314 F** de plus sur le
plein, soit **0,0254 % de F total**. Il ne ferme aucune arête du quart.
Son code naïf recalcule les distances aux sommets pour chaque incidence,
d'où environ deux fois plus de termes `Q` ; une table arête–sommet peut
les partager et ramener cette composante à `≤27` termes par arête. La
faible réduction des incidences et de `F` demeure le constat déterminant.
Les temps shadow sont des observations sur hôte CPU partagé et excluent
lecture des traces, reconstruction du front, énumération de `A×B`, coût
catalogue et exécution GPU. `F fermable` est une borne d'évitement **avant
le cœur**, pas une baisse mesurée du temps de chaîne ni une preuve de
croissance sous-quadratique.

**Décision :** ne pas porter ce simple crible boîte/plan sur les huit
cellules comme solution au cœur. La direction à tester ensuite reste
une subdivision adaptative du domaine de centres, ou un domaine 2D
sur le plan bissecteur avec incidence certifiée plus fine ; juger le
travail total incluant sélection, certificats et aval. Cette ablation
n'écarte pas ces variantes. Elle ne qualifie qu'une trame et un quart
à K5, sans-sol, K10, plusieurs scènes, FULL et G4 restent à mesurer.

Relecture statique, après arrêt du codespace :

```sh
python3 -B morsehgp3D_v9/audits/s2_precell_incidence_20260923/test_incidence.py
python3 -B morsehgp3D_v9/audits/s2_precell_incidence_20260923/verify.py
(cd morsehgp3D_v9/audits/s2_precell_incidence_20260923 && sha256sum -c SHA256SUMS)
```

Pour rejouer les deux mesures pendant que les entrées `/tmp` existent,
compiler `measure.cpp` avec `-std=c++20 -O2 -Wall -Wextra -Werror` contre
`build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a`, puis lui passer
`points.u32le`, `raw_return_ids.u32le`, les huit `part_*.bin` de la
trace S2, éventuellement la trace post-cœur, `--near --budget=64`.
Le reçu épingle les entrées, l'archive et le binaire exacts.
