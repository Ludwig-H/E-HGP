# Partager la frontière q2 sur les résidus réellement publiés

13 septembre 2026, source produit `f5430f57`. Cadre :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`.
Prototype d'audit du **compte strict uniquement**, distinct du consommateur
avec IDs et coquille actuellement développé dans le moteur.

## Un contrôle effectif du partage

Le [prototype C++](q2_shared_frontier_probe.cpp) consomme les véritables
plages de `AxisQ2Plan` publié. Sur les nappes complètes, il utilise
Additive seul ; sur les grilles 3D, Additive intersecté avec Pool.
Les recettes et IDs sont ceux des deux headers de benchmark publiés.
Kmax=10 et s12 pour les six grandes entrées. Aucun résultat des anciennes
sources `8e406f9b` n'est transféré à cette expérience.

Les deux parcours utilisent le même index de tous les sites et le même
résidu. Le premier interroge chaque paire. Le second partage un compteur
et une frontière de témoins Z entre des plages B à ancre A fixée. Il
construit un arbre de plages sur la permutation B, sans la réordonner,
puis décompose chaque fragment axial en nœuds de cet arbre. La couverture
de ces fragments a donc un coût supplémentaire, explicitement compté.

| Famille | n | Paires candidates | Visites par paire | Classifications partagées | Visites de couverture B |
| --- | --- | --- | --- | --- | --- |
| grille / intersection Pool | 8 000 | 36 960 | 1 992 068 | 1 819 702 | 298 294 |
| grille / intersection Pool | 16 000 | 67 660 | 3 952 846 | 3 689 048 | 773 248 |
| grille / intersection Pool | 32 000 | 114 716 | 6 311 590 | 5 717 707 | 1 539 110 |
| nappe complète / addition | 8 000 | 918 160 | 40 643 530 | 32 686 537 | 2 761 472 |
| nappe complète / addition | 16 000 | 1 912 660 | 89 017 912 | 70 218 310 | 6 290 054 |
| nappe complète / addition | 32 000 | 3 928 390 | 189 649 460 | 147 292 056 | 13 830 224 |

À n32k, les deux parcours trouvent 16 183 paires sous le seuil sur la
grille et 455 418 sur la nappe. L'histogramme complet des comptes saturés
concorde dans les six cas, ainsi que deux digests sur les identités et
profondeurs. Ce contrôle aux grandes tailles n'est pas un oracle
géométrique exhaustif indépendant.

Le partage réduit les classifications de la nappe d'environ 22 %, mais
18 424 278 d'entre elles portent sur une boîte B et coûtent davantage
qu'un test à paire fixée. Il ajoute aussi la couverture, les subdivisions
et les continuations : **aucun gain de temps n'est établi par ces comptes**.
Les sorties sont presque toutes ponctuelles : 3 928 269 groupes finaux
pour 3 928 390 candidates sur la nappe, rejetées comprises. Cette politique
par étendue partage surtout des préfixes de recherche ; elle ne produit
pas ici une forte compression terminale.

## Frontière immuable, mémoire recyclée, écritures cumulées

Un état non saturé contient une ancre A, un nœud B, un compteur exact
uniforme et la tête d'une liste immuable de blocs Z non consommés.
Les blocs de cette liste partitionnent les IDs encore ouverts.

- Un bloc Z strictement intérieur crédite sa population pour tout B ;
  un bloc sans intérieur est retiré sans crédit.
- Un bloc Z indécis est remplacé par ses deux enfants disjoints.
- Une subdivision B transmet aux deux enfants le même compteur et la
  même tête courante. Elle ne redémarre pas à la racine Z.

Le choix de subdivision compare les étendues ; B singleton utilise les
distances exactes au centre doublé, comme la référence par paire. Le
compteur part de zéro, sans crédit du cœur. À Kmax, le groupe est terminal :
ce compteur tronqué **n'est pas un état reprenable à seuil supérieur**.
Le [contrat de reprise](P0_CENSUS_PARTAGE_ET_SEUILS.md) s'applique à une
future API qui promettrait cette réutilisation.

Le parcours en profondeur restaure la taille de l'arène après chaque
enfant. Les nœuds partagés préexistants restent vivants pour le frère ;
les cellules propres à l'enfant sont réutilisables. À n32k sur la nappe,
145 720 627 cellules de continuation sont écrites au total, avec un maximum
de **201 cellules stockées simultanément dans l’arène**. Ce compteur ne désigne
pas 145 millions d'allocations du tas : le vecteur réutilise sa capacité.
Le reçu ne mesure ni RSS, ni capacité réservée, ni coût de l'allocateur.
Sur la grille, les valeurs sont 5 981 445 écritures et 189 cellules.

La nouvelle [section 9.1 de l'autre auditeur](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md)
donne une simplification directement pertinente : sous ordre Z en
profondeur fixé, un curseur en préordre et les liens `escape` de l'index
représentent cette frontière. Notre parcours conserve justement cet ordre.
Ce raccord peut donc éviter les écritures répétées de listes ; il ne
supprime pas les classifications ni les sorties. Le prototype conserve
sa version à listes comme comparaison explicite, sans attribuer de temps
ou d'exécution C++ à la variante à curseur non portée ici.

L'index global construit 2n−1 nœuds ; l'arbre B ajoute 2|B|−1 nœuds et
lit chaque point B une fois. Les unions de boîtes internes restent à
payer. Le parcours de couverture ajoute O(D log |B|) au pire pour D
descripteurs d'entrée. Le partage n'a pas de borne générale meilleure
que le traitement individuel, et ne qualifie pas la complexité P0 globale.

## Exactitude bornée et périmètre des sorties

Le [runner](q2_shared_frontier_checks.py) exporte neuf sources et recettes
exactes de `f5430f57` dans un répertoire neuf. C++20 strict, GCC13/O2 et
UBSan sans récupération concordent sur 57 cas : 36 185 candidates,
8 278 451 tests indépendants de H sur tous les petits produits. Le juge
vérifie chaque paire, la couverture et l'absence de doublons. Trois
mutations C++ sont réfutées : redémarrer un enfant B à la racine, oublier
son compte acquis, perdre la suite de la frontière après subdivision Z.
Normal et −O concordent ; les six grandes exécutions terminent.

Le [reçu](Q2_SHARED_FRONTIER_CHECKS.json) conserve aussi le premier échec
du prototype sur une permutation B vide lorsque le cœur sature le plan.
Le prototype accepte maintenant cet arbre B vide ; cette fixture reste
dans la gate. Il ne s'agissait pas d'un défaut du moteur.

Le parcours partagé agrège des comptes par plage sans développer les
grandes paires : histogrammes et digests utilisent des préfixes d'IDs.
La référence individuelle développe réellement chaque candidate. Le
prototype ne collecte pas les IDs d'intérieur ou de coquille, ne canonise
pas les boules, ne matérialise pas de flux de supports et ne mesure
aucune durée. Les arbres, couvertures et continuations sont comptés ;
la construction du propriétaire et du plan conserve son coût amont
publié. Le consommateur produit devra payer également ses sorties.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/q2_shared_frontier_checks.py --selftest
python3 -B -O audits/morsehgp3D_v8_complementaire/q2_shared_frontier_checks.py --selftest --large
```

Ces preuves ne qualifient pas le nouveau code `q2_census` en construction,
ni la tour FULL. GCP non utilisé.
