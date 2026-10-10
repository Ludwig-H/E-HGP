# G — Arrêter la recherche dès l'égalité exacte

10 octobre 2026, produit B3-K `2aaed1847`. `phase=exploration_v12_hors_registre`,
`objet=full_pi0`, `public_status=not_claimed`. **Proposition non intégrée**,
indépendante de la [vue virtuelle Q3](../g_premiere_sonde/README.md). Aucun moteur,
compilateur, test natif ou GCP exécuté ; aucun gain de temps mesuré.

Un changement plus petit que `candidate(key)` suivi d'une vérification virtuelle
est possible dans `PopulationTable::find` : rechercher `(empreinte, population)`
par dichotomie et **rendre la naissance dès que le comparateur exact vaut zéro**.
Aujourd'hui, `lower_bound` continue à gauche après cette égalité, puis `find`
compare encore la ligne trouvée. Le patch proposé touche seulement ce corps ;
gardes table vide/cardinalité, table, index, empreintes, préchargements, resolveur,
rangs, compteurs et API restent identiques. Les helpers `lower_bound`, `candidate`
et `verify` restent présents et inchangés.

Preuve : la construction des fiches vérifie l'ordre **strict** de
`(empreinte, population)` dans `Layout::records`. Les empreintes seules peuvent
être toutes égales ; les populations exactes restent distinctes. Une comparaison
nulle identifie donc l'unique fiche recherchée. Avant la première égalité, les
deux recherches font exactement les mêmes comparaisons et mouvements de bornes.
Sur hit, la nouvelle suite est un **préfixe strict** de l'ancienne : l'ancienne
continue sa borne inférieure puis effectue une dernière vérification. Sur miss,
aucune égalité n'existe ; les boucles parcourent les mêmes positions et la nouvelle
recherche supprime seulement la vérification finale éventuelle. Les intervalles
exclus ne peuvent contenir la clé, par ordre strict. Les résultats sont identiques,
et le nombre de comparaisons `(empreinte,population)` n'augmente sur aucune requête.
Cela ne prouve pas une baisse du temps machine : branchements et code produit par
le compilateur restent à mesurer.

`candidate(key)` puis `verify` est **déjà implémenté pour G-L5** dans
`first_probes.cpp` ; ce n'est pas un algorithme nouveau. Le candidat est la première
fiche portant cette empreinte, pas forcément la population cherchée. Après une
inégalité exacte, il faut conserver la dichotomie de collision dans la suite du
seau. Cette voie peut réduire à une seule comparaison de population un hit à
empreinte unique, mais fait d'abord une recherche de borne par empreinte ; sous
collision elle ajoute une recherche et une comparaison de la première fiche.
Elle n'offre pas la propriété de préfixe strict de la proposition. L'ancienne
jointure G-L5 triant toutes les premières sondes avait déjà été rejetée : rien ne
la réintroduit ici, et aucune nouvelle liste de candidats n'est nécessaire.

Exemples du modèle, nombre de comparaisons exactes de populations :

| Requête | `find` actuel | Retour à égalité | `candidate` + `verify` |
| --- | ---: | ---: | ---: |
| Hit à empreinte unique | 2 | 1 | 1 |
| Dernier hit, dix populations, toutes les empreintes nulles | 4 | 3 | 5 |
| Miss intérieur au seau, empreintes nulles | 3 | 2 | 5 |

Le modèle couvre **53 560 requêtes**, dont **20 615 hits** et **13 391 requêtes
avec empreinte forcée à zéro**. Tous les hits satisfont le préfixe strict ; les
32 945 misses conservent le chemin, avec suppression de zéro ou une vérification
finale. Énumération de tous les sous-ensembles d'une table de dix populations,
quatre masques de hash ; cas dirigés k=2..12, table vide, mauvais cardinal,
requêtes absentes et SiteIdx `0xFFFFFFFE` sous la sentinelle. Les trois recherches sont comparées
à une recherche linéaire par population exacte. Les décomptes dans `results.json` sont des
opérations du modèle, pas des accès mémoire natifs ni un profil LiDAR.

`proposition.patch` est limitée à `src/tower/populations.cpp` de `2aaed1847`.
Le lecteur vérifie les sources, applique le patch uniquement à une copie
éphémère, contrôle son empreinte finale et l'absence de changement hors du corps
de `find`, puis rejoue le modèle. Aucun fichier produit ni index Git n'est modifié.
La vue virtuelle pourra employer le même arrêt sur égalité, avec son comparateur
exact prouvé ; les deux leviers doivent pouvoir être mesurés séparément.

```sh
python3 -B check.py /chemin/du/depot
python3 -B -O check.py /chemin/du/depot
```

Avant adoption : déclarer le microbanc sur une base épinglée ; vérifier les portes
natives de population (collisions zéro/faibles, hit/miss/cardinalité), FUL1 et
compteurs ; mesurer G et FULL sans attribuer au seul nombre de comparaisons un
gain d'horloge. Le bénéfice potentiel dépend des tailles des seaux et des hits
réels. Ne pas changer simultanément le format de la table ou le préchargement.
