# T2-c : tables, résidence et frontières des chronos

Lecture du produit de la session H, publié dans `136e0762898ce8dd40c96dafba6b2d616f3b7b1f`, et du prototype Gc en
chantier issu de `9c5809919`. [Capture des sources et extraits](capture.json), stable avant/après. Aucun build,
calcul natif, données privées ou GCP. Cadre v12 hors registre, `cpu_reference`, `full_pi0`, u21, `not_claimed`.

**La cible « tables à 3 ms, résolution deux fois plus rapide » ne suffit pas à elle seule.** La
[relecture indépendante de H](../session_h_mesures/README.md) calcule les différences sur chaque passe chaude,
avant les médianes. Sur ng00/01/02 K5 W48, le reste hors count/fill/tables/resolve vaut respectivement
**10,842 / 9,041 / 11,219 ms**. Garder ce reste, remplacer tables par 3 ms et diviser resolve par deux donne
**40,163 / 32,558 / 38,068 ms**. Ce diagnostic algébrique conditionnel ne prédit pas une autre implantation ; il
montre les postes supplémentaires à traiter pour viser 25–30 ms. Aucune somme de médianes n'est utilisée.

## Le prototype Gc change les frontières

La lecture de `stage.cpp` et `tower.hpp` établit les différences suivantes ; la nouvelle inclusion de tables est
déjà indiquée dans le commentaire du prototype. Elle doit aussi être visible dans les prises et leurs lecteurs.

| Champ | Produit mesuré en H | Prototype Gc capturé |
| --- | --- | --- |
| `tables_ns` | Durée du `parallel_for` entre ordres, chaque table construite séquentiellement | Somme des constructions d'index par ordre ; chaque construction utilise le Pool |
| `resolve_ns` | Résolution des ordres, **après** la construction des tables | Parcours des ordres **avec** construction des index, jointures et libération de chaque index/candidats |
| `order_ns[k]` | Résolution de l'ordre et ses contrôles | Index + jointure + résolution + contrôles + libérations de l'ordre |
| `table_bytes` | Somme des tables simultanément vivantes | Maximum de l'index vivant, un seul ordre à la fois |

Dans Gc, `tables_ns ⊂ resolve_ns` : les soustraire tous deux du mur compterait deux fois la construction. La baisse
de `table_bytes` change aussi le mode de coexistence ; ce n'est pas, à elle seule, une baisse à nombre d'entrées
identique de la taille d'une fiche. Comparer d'abord `wall_ns` au même périmètre, puis des sous-postes homogènes.

Deux corrections de protocole possibles avant adoption : nommer explicitement `orders_total_ns` l'enveloppe actuelle,
avec ses sous-postes index/jointure/résolution ; ou garder `resolve_ns` exclusif, en additionnant des intervalles de
résolution par ordre ouverts après l'index/jointure. Dans les deux cas, déclarer les inclusions, publier le chrono de
jointure également hors construction de profil, et compter les allocations/libérations dans un poste ou un résidu
nommé. Ne pas attribuer à une optimisation un déplacement de frontière.

## Ce que contient le reste de H

Le mur commence avant `resolve_tower`. Les quatre sous-chronos ne couvrent pas : `check_catalogue` (scan séquentiel
des incidences et supports), `prepare_points` (validation et stockage de n points), les admissions/allocations
avant count et entre count/fill, le scan `population_entries` avant le chrono des tables, la création des espaces
de census, et les libérations locales après la fin des sous-chronos. En particulier, tables, points et espaces de
census sont détruits avant le retour de `resolve_tower`, donc leur libération appartient au mur de H. La destruction
du résultat retourné est, elle, après sa lecture chronométrique par la sonde.

**Pas de remise à zéro globale des espaces de census identifiée.** `CensusWorkspace::make` alloue son tableau sans
l'initialiser ; `Buffer` ne construit ni ne zéroïse ses éléments. Les cases de la table sont explicitement vidées
dans `PopulationTable::build`, donc dans `tables_ns`. La sonde H construit `MemoryBudget(o.budget)` sans capacité de
cache. Ces faits désignent des lieux à instrumenter, pas une attribution des 9–11 ms à un allocateur particulier.

## Optimisations exactes, dans l'ordre de T2-c

**Construction parallèle des tables.** En H, il n'existe que K−1 tâches : augmenter W au-delà ne divise pas le plus
gros ordre. Le prototype Gc compte et remplit par blocs, trie par base stable, puis produit un répertoire et des
fiches contiguës `(hash, naissance, rang, population)`. La valeur garde l'indice de naissance original ; le rang
copié vient de `birth_ranks`. Cette disposition retire des lectures indirectes sur `birth_keys`, les boules et la
CSR, sans modifier LEM-POP : la réponse exige toujours les k SiteIdx exacts. Préserver le filtre `p+m=k`, les
préfixes u64, les écritures disjointes et l'égalité complète sous collisions. Le tri et les copies supplémentaires
sont à payer ; le nouveau parallélisme intérieur remplace la concurrence entre ordres, il ne s'y ajoute pas.

**Résidence : distinguer capacité mémoire et contenu calculé.** Des buffers et espaces de travail peuvent rester
alloués entre trames en rétablissant leurs tailles/états ; cela peut éviter allocation et libération, sans rendre
les anciennes tables valables. Une table, les points préparés et la classification des cellules ne se réutilisent
que pour le même domaine immuable, même génération de Cloud/catalogue, même K et mêmes indices de naissance.
Même n ou mêmes nombres de boules ne suffisent pas. Pour une nouvelle trame, leur construction reste dans la
latence par trame, même déplacée à la finition du catalogue. Ce déplacement apporte un gain seulement s'il évite
des scans, copies ou reconstructions réellement redondants ; il ne permet pas de déduire `C+G` en ôtant le poste de G.
La construction de la table doit attendre ou transporter correctement la numérotation finale des naissances de
chaque ordre : un BallIdx n'est pas un indice de naissance. Une préparation possédée commune peut lier ces données
et amortir leurs validations sur un même objet ; chaque nouvelle entrée reste validée.

**Premières sondes et recherche des supports.** La
[proposition G-L5](../g_l5_proposition/README.md) contient déjà preuve, états de reprise et budgets ; elle n'est pas
répétée ici. Dans le prototype capturé, `Merge::body` appelle encore `candidate(key)` par requête, lequel repart au
début du seau. Le tri améliore la localité mais ne donne pas la borne d'une fusion monotone sous collisions ;
`verify` protège bien l'exactitude. Le rapport de travail local constate un apport faible du tri à trois fils et
explore G-L7, une file de sondes préchargées. Aucun gain G4 de cette copie n'est acquis par notre lecture.

Pour la table S* du catalogue, une fiche contiguë de clé complète et BallIdx, ou un index exact équivalent, peut
réduire les sauts mémoire de la dichotomie indirecte. Conserver les SiteIdx strictement croissants, l'arité et le
remplissage par sentinelle ; comparer la clé entière. Un autre support minimal de la même sphère n'est pas S* :
le carré doit toujours pouvoir manquer la table et suivre la voie certifiée/census prévue. Ce levier change la
recherche, pas le certificat géométrique ni la population à contrôler dans LEM-T1.

Le prochain bilan doit donc juxtaposer les murs appariés, index/jointure/résolution à frontières explicites,
préparation et libérations, mémoire vivante et staging. Les profils locaux du prototype et les temps G4 de H
restent distincts. Aucun nouveau défaut d'objet ni qualification FULL/GPU n'est déduit de cette lecture.
