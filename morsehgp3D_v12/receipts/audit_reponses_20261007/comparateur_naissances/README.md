# Comparateur des naissances : effet de bord et auto-comparaison

Lecture du 8 octobre 2026. Le prototype TMVR repo5, basé sur `c903774b1`, contient un refus conditionnel
reproductible par lecture de la bibliothèque : une cohorte de centres distincts serait refusée lorsque le tri
effectue une auto-comparaison. **La configuration native `_GLIBCXX_DEBUG` n'a pas été exécutée** ; aucun échec des
campagnes Release ni défaut FULL livré n'est déduit. Au pin main `8fe509df9`, ce fichier n'est pas encore livré.

Épingles dans [capture.json](capture.json) : source `forest_births.cpp` SHA-256 `101184e078e18c45…`, patch TMVR
repo5 `6f0643acca9d6c27…`, base complète `c903774b1d16c3b18c15b86fa2a55d11d44b8bc8`. Le lecteur reconstruit
le fichier ajouté depuis ce patch ; il ne confond pas les profils 24/32 de repo3 avec le build 21 de repo5.

## Cause et portée

Dans `src/tower/forest_births.cpp:66–72`, le comparateur de `std::sort` modifie un booléen partagé par référence :
`equal = equal || s == 0`. Ce booléen entraîne ensuite `tower_invariant`, sans vérifier que les deux indices
comparés sont distincts. Pourtant `compare_centers(spheres[a], spheres[a]) == 0` est normal et le prédicat
retourne correctement faux. Le résultat relationnel du comparateur n'est donc pas fautif ; son usage comme
détecteur de doublons dépend du protocole d'appels du tri.

Deux extraits minimaux des headers libstdc++ 13 locaux, hachés avec la source :

```cpp
// bits/stl_algo.h:4892, dans std::sort(first, last, comp)
__glibcxx_requires_irreflexive_pred(__first, __last, __comp);
// debug/macros.h:330, expansion lorsque _GLIBCXX_DEBUG est actif
_GLIBCXX_DEBUG_VERIFY(_First == _Last || !_Pred(*_First, *_First), ...);
```

`debug/debug.h:64–86,136–137` rend le premier appel vide sans `_GLIBCXX_DEBUG`, et le relie au second avec ce
mode. Sur toute cohorte non vide, celui-ci compare le premier élément à lui-même ; le contrôle d'irréflexivité
réussit mais positionne `equal`. `sort_balls` étant appelé pour une cohorte d'au moins deux boules (`:104–109`),
le refus suit même si leurs centres sont tous distincts. Les flags relus des builds `build21e`, `build24c`,
`build32c` ne définissent ni `_GLIBCXX_DEBUG` ni `_GLIBCXX_ASSERTIONS` : ces traces ne démontrent pas ce défaut
en Release. Aucun interdit d'auto-comparaison n'est nécessaire pour établir le cas concret de ces headers.

Un témoin géométrique possible est `(0,0,0),(2,0,0),(10,0,0),(12,0,0)` : les deux paires proches définissent des
boules de rayon carré 1, centres `(1,0,0)` et `(11,0,0)`, sans autre site intérieur ni sur leur coquille. Elles
peuvent donc partager le même rang à l'ordre 2 sans être des doublons. Ceci est un témoin mathématique, pas une
capture de son passage dans TMVR.

## Proposition et preuve bornée

[proposition.patch](proposition.patch), hors produit, retire tout effet de bord du comparateur puis compare les
centres adjacents après le tri, comme le fait déjà `sort_sites`. Toute classe de centres égaux est contiguë dans
un ordre lexicographique exact ; elle contient une paire adjacente si et seulement si elle compte au moins deux
éléments. Les auto-comparaisons du tri n'affectent plus ce contrôle. Aucun tampon ni calcul de sphère ajouté ;
au plus `n−1` comparaisons exactes supplémentaires, complexité totale inchangée `O(n log n)`.

[check.py](check.py), normal et `-O`, vérifie neuf hashes avant/après, l'absence du fichier aux pins main/base,
sa reconstruction depuis le patch, l'applicabilité du correctif sur une copie temporaire, le préappel de la macro
et 3 276 séquences de longueur 2 à 7 sur trois valeurs. Le contrôle adjacent est équivalent à la présence d'un
doublon sur toutes ces séquences. Il vérifie également les distances du témoin à quatre sites. Aucun compilateur,
moteur, benchmark, GCP ou modification du prototype n'est invoqué. Résultats normal et `-O` identiques.

```sh
python check.py --repo /workspaces/E-HGP --tmv "$TMV"
python -O check.py --repo /workspaces/E-HGP --tmv "$TMV"
```

`TMV` désigne le scratch `v12_tour_TMV` ayant produit le patch épinglé ; `--headers` permet de désigner la copie
épinglée des headers libstdc++ 13. Lecture vivante, dépendante de ces artefacts locaux. Intégration et qualification
native du correctif restent à faire ; aucun état du registre n'est fermé par cette proposition.
