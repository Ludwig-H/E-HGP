# A6 : fin de l'ordre 5 et filtrage exact des liens du noyau

Lecture du Git `72f622a556130e25afdadbef84984619417286e2`, résultats de FULL M `957e9784…` déjà
[admis](../session_m_admission/README.md). **R de l'ordre 5 termine en dernier dans les cinq prises chaudes de la
trame `kitti_ng_08_002119`, 99 099 sites.** Cela motive A6, sans transformer les fins publiées en durée propre du
noyau. La présente proposition précise le raccord au `LEM-MSTC` déjà inscrit dans `ARCHITECTURE.md` §4.3 et
`OBJET_ET_CONTRAT_MATHEMATIQUE.md` ; elle ne revendique pas un nouvel algorithme qualifié.

## Ce que les diagnostics montrent

Médianes des cinq chaudes, en ms, origine commune au début de la tour après P/C (ouverture comprise) :

| repère | instant de fin |
| --- | ---: |
| G ordre 5 | 101,228 |
| dernier G tous ordres | 156,932 |
| noyau ordre 5 | 200,683 |
| M ordre 5 | 207,816 |
| V ordre 5 | 210,612 |
| R ordre 5 | 231,329 |
| fin de région publiée | 235,051 |

La tour **englobante** dure 237,975 ms en médiane, FULL 319,784 ms. La première prise seule porte précisément
G5=101,167 ; noyau5=199,407 ; R5=228,125 ; fin région=231,856, mais tour englobante=237,975 ms. Le « tour vers
232 ms » du récit développeur correspond à `recouvrement.fin_ns` de cette prise, pas à `tour_ns`.

Les intervalles sont calculés dans chaque prise avant médiane : fin noyau5 moins fin G5 **100,319 ms** ; fin
noyau5 moins dernier G **44,723 ms** ; fin R5 moins fin noyau5 **29,964 ms** ; queue totale après dernier G
**79,090 ms**. Ils comprennent attente, ordonnancement et interférences éventuelles. On ne peut pas soustraire
199−101 pour annoncer « 98 ms de calcul du noyau », ni 228−199 pour annoncer « 29 ms de calcul R ». Le noyau
peut déjà travailler avant la fin G5 ; les débuts et durées propres par ordre ne sont pas publiés dans ces JSONL.
`fenetres_ns.T` vaut 600,683 ms en médiane : somme de temps-fils muraux, tous ordres, naissances, pré-passe, noyau
et historique compris ; ce n'est ni la durée critique de T5 ni un temps CPU. Le CPU processus est une autre mesure.
Les dépendances restent celles de [T2-d-A](../t2d_a_dependances/README.md), sans relation d'ordre globale V/R.

**Limite conditionnelle d'une queue gratuite.** En gardant P, C et la fin G observés inchangés, calculer `P+C+G`
dans chaque passe puis médianer laisse **238,319704 ms** sur cette trame. Sur les 37 trames : médiane des médianes
**115,747646 ms**, maximum **238,319704 ms**, **22/37** médianes au-dessus de 100 ms. Supprimer seulement la queue
ne fermerait donc pas ce contrat dans ce scénario. Ce n'est pas une borne sur toute architecture : G contient déjà
l'effet du recouvrement et de ses interférences ; A6 pourrait aussi modifier G, les allocations et la contention.
Ces valeurs ne prouvent ni un gain A6, ni l'impossibilité générale de 100 ms.

## Raccord mathématique proposé : filtrer avant un rejeu exact

Le noyau courant traite les cellules par `(rang, BallIdx)`, puis leurs représentants dans l'ordre. Une cible
« cellule » a un rang **strictement inférieur** (`resolve_leaves`, LEM-T3) ; son `element` est un sommet de la
composante qu'elle a déjà réunie. Les événements, cellules retenues et attaches dépendent de cette politique.
Un arbre couvrant quelconque conserve les composantes par seuil, mais ne suffit donc pas à conserver ce registre.

La variante prudente suivante conserve la suite exacte d'unions utiles. Il reste un rejeu séquentiel réduit ;
elle ne prétend pas paralléliser directement toutes les mutations de l'union-find.

1. **Ancre fixe.** Pour une naissance, prendre son identifiant canonique `birth_node`. Pour une cellule `t`, suivre
   récursivement son premier représentant jusqu'à une naissance : `anchor(t)`. Les rangs décroissent strictement,
   donc pas de cycle ni de cible de même plateau. Une ancre quelconque ainsi choisie appartient à la composante
   de `t` dès sa coupe fermée. Par induction, à chaque rang ultérieur, elle est dans la même composante que
   l'`element[t]` mutable actuel. Conserver et contrôler toutes les cibles originales.
2. **Étoile fixe avec identité.** Pour chaque cellule `t`, relier l'ancre de son premier représentant à celle
   de chaque représentant `j>0`. Chaque lien garde la clé totale unique **`(rang(t), t, j)`**, équivalente à
   `(rang, BallIdx, position du représentant)` puisque les cellules sont dans l'ordre canonique. Boucles et liens
   répétés restent identifiables. Remplacer les extrémités par leurs ancres ne change aucune décision du Kruskal
   courant : les cellules ciblées sont toutes d'un rang inférieur, donc leurs composantes sont déjà réunies.
3. **Sélection parallèle future.** Calculer la forêt couvrante minimale sous cet **ordre total**, pas seulement
   sous le rang. Sous des poids tous distincts, le MSF est unique et tout algorithme exact (par exemple Borůvka
   sur les seuls liens choisis) rend exactement l'ensemble des liens acceptés par Kruskal. Sur une entrée FULL
   valide connectée avec `B≥1`, il contient `B−1` liens pour `B` naissances. Le cas déconnecté garde le même nombre insuffisant
   d'unions et doit toujours être refusé à la clôture. Ne jamais contracter toute une étoile parce qu'un de ses
   liens est choisi (`REG:238`).
4. **Rejeu dans l'ordre original.** Trier les liens retenus par cette clé, conserver un ancrage pour **chaque**
   cellule, y compris inerte, puis rejouer les unions par taille et le départage actuels. Par induction sur les
   liens : mêmes racines `x/y`, tailles et plus petites feuilles avant chaque union retenue ; mêmes opérandes,
   survivant, événement et `event_cell`. Après chaque cellule, mêmes `element` et `cell_top`. Les recherches
   redondantes omises peuvent changer les demi-compressions dans `UnionCell::up`, mais pas ces résultats ni les
   attaches historiques. L'état interne `up` n'est pas promis octet-identique.

Ainsi M voit les **mêmes événements dans le même ordre** : multifusions et égalités restent inchangées. R garde
les mêmes cellules retenues et lit les mêmes entrées originales ; V retrouve les mêmes attaches/nœuds aux mêmes
coupes. Il ne faut ni remplacer les branches ouvertes par les enfants d'une multifusion générale, ni supposer
indépendantes deux cellules du même rang qui peuvent partager une composante. La piste
[classe contributrice unique](../registre_classe_unique/README.md) reste une optimisation R distincte.

Le coût séquentiel des appels `find` passe de `P` (nombre de représentants, premier compris) à `C+E`, où `C` est
le nombre de cellules et `E` le nombre de liens retenus (`B−1` sur le succès). Les `B−1` unions restent séquentielles
dans cette variante. Les comptes objet et événements/attaches/cellules retenues peuvent rester identiques ; les
comptes physiques du filtrage/rejeu doivent être publiés. La pré-passe des cibles doit encore contrôler et compter
tous les représentants originaux, sans faire disparaître artificiellement `birth_targets` ou `cell_targets`.

## Coût et prochaine preuve utile

Ce n'est pas un gain acquis. L'ancrage peut être calculé par un passage topologique `O(C)` séquentiel, ou par
doublement de pointeurs en `O(log C)` tours et `O(C log C)` travail au pire. Un MSF parallèle ajoute scans,
réductions et contractions ; une version de Borůvka à scans complets peut faire `O(P log B)` travail. Le graphe
peut être implicite dans les représentants, mais ancres, partitions, minima, liens retenus, tri/CSR et sorties
coexistent : leur admission doit être démontrée, pas absorbée dans l'ancien budget de `20(B−1)` octets d'événements.

Surtout, le noyau actuel est **reprenable par tranches de G**. La sélection globale ne permet pas de consommer
une arête future avant que sa cible soit publiée. Sans protocole incrémental prouvé, attendre G complet de l'ordre
perd une partie du recouvrement : mesurer la tour entière, la queue et le pic, pas seulement le rejeu réduit.
Le rang quasi singleton observé sur LiDAR n'est pas une borne ; une barrière par plateau reste exclue.

Avant une ablation native, publier par ordre `P,C,B,E`, la durée propre du noyau, ses reprises/attentes, puis les
durées ancrage/sélection/rejeu et la fin R. Le champ `ForestPhysical::kernel_ns` existe ; FULL agrège actuellement
ces durées dans T. Ces compteurs diront si supprimer les finds redondants a un potentiel suffisant ou si la chaîne
des unions utiles domine encore. Aucun seuil d'adoption n'est inventé à partir des prises M.

`check.py` est un modèle **abstrait d'hypergraphes rangés** : Prim sous l'ordre total choisit les liens, puis compare
le noyau original au rejeu filtré. Cent cas bornés, dont cibles cellule, répétitions, cellules inertes et plateaux :
événements, attaches, `element`, `cell_top` et liens acceptés identiques ; 4 042 appels find du modèle contre 1 320.
Ces comptes ne décrivent pas LiDAR. Ce n'est ni une preuve de réalisabilité géométrique des cas, ni un algorithme
parallèle natif, ni un chrono. La preuve générale est l'induction ci-dessus et l'unicité sous l'ordre total.

**Réemploi des tampons à étudier.** Le témoin R actuel `minleaf[cell_node[target]]` n'est disponible qu'après M :
l'utiliser pour préparer T serait circulaire. En revanche `OrderWork::element[C]` peut porter provisoirement les
ancres, puis tous les `work.leaves[P]` être aplatis en identifiants de naissance canonique avant le réemploi
d'`element` par le rejeu. Les `ForestInput::targets` originaux restent inchangés pour R. Cette discipline évite un
nouveau tableau permanent d'ancres, mais exige une phase fermée : toutes les cibles et écritures de la pré-passe
sont publiées, avec contrôle des drapeaux de tranche. **`g_end_ns` seul ne suffit pas** : dans `run_g`, la fin G est
annoncée avant `slice_leaves`, tandis que les drapeaux sont publiés après. Aucun tampon encore écrit par un autre
fil ne peut être recyclé. La sélection MSF conserve ses propres besoins à admettre ; aucun octet n'est déduit deux fois.

## Rejeu et portée

```sh
python3 -B check.py --repo DEPOT --returned-full DOSSIER_FULL_M
python3 -B -O check.py --repo DEPOT --returned-full DOSSIER_FULL_M
```

Sorties identiques à `results.json`. Les cinq JSONL M sont seulement rehachés et relus comme métadonnées déjà
admises. Les corps noyau/contraction/structures et le calcul des fins dans `pipeline.cpp` sont identiques entre
M et le pin lu. `pipeline_run.cpp` a depuis reçu le correctif de terminaison : aucune qualification native nouvelle
ne lui est transférée. Captures/sourcepins sans copie de sources entières ; aucune modification produit, registre,
GCP ou lecture de données XYZ/IDs. Le lemme a reçu une contrelecture statique favorable de perf_math, sans rejeu.
