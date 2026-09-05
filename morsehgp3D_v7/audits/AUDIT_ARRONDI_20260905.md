# Arrondis : garde effective et replis du pipeline v7

5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict : la garde d'arrondi fonctionne dans les exécutions contrôlées.**
Quarante appels, cinq nuages, les quatre modes d'arrondi et un/deux threads
conservent le catalogue après préfiltre et les forêts. La sonde vérifie
aussi les compteurs de travail : ce résultat ne se réduit pas à comparer
deux chemins où les filtres seraient inactifs.

## Résultats indépendants

La [sonde permanente](rounding_probe_20260905.cpp) appelle directement le
pipeline courant, sans mutant produit, avec les avertissements stricts et
les mêmes options d'optimisation que la Release. Le
[reçu](receipts_20260905/rounding.json) conserve source, binaire, commandes,
environnement et hashes des sorties ; les [40 lignes brutes](receipts_20260905/rounding.stdout)
séparent chaque combinaison. Le [diagnostic stderr](receipts_20260905/rounding.stderr)
est vide. Compilation et exécution rendent chacune 0.

| Contrôle | Résultat observé |
| --- | --- |
| Domaine local | GCC 13.3.0, cible x86_64-linux-gnu, binaire64 IEC 559, `FLT_EVAL_METHOD=0` |
| Arrondi au plus proche | 65 588 décisions certifiées affine/Jung, 2 292 grilles construites |
| Arrondis vers le bas, vers le haut et vers zéro | Zéro décision flottante certifiée et zéro grille construite ; 189 582 replis entiers |
| Threads | Deux véritables ouvriers visités à chaque contrôle héritent du mode demandé ; le pipeline restitue le mode de l'appelant |
| Objets | Catalogue après préfiltre et forêts identiques entre les huit configurations de chaque nuage ; objets non vides |
| Route complétée | Huit appels sur le nuage régulier à onze points, tour K=1..10, tous achevés et identiques |
| Frontière égale | Tétraèdre régulier entier, route Gabriel, tour K=1..3, tous achevés et identiques |

Les trois autres nuages comptent dix-neuf points chacun et utilisent un
générateur entier déterministe décrit dans la sonde. Ils sont testés sur
la route Gabriel K=1..10. Une réussite Gabriel ne qualifie pas Gamma ; le
choix de route est explicite pour chaque famille. Les comparaisons portent
sur les digests canoniques existants, pas sur les seules cardinalités.

## Ce que cela ferme

Le [filtre](../src/pipeline/float_filter.hpp) interroge `FE_TONEAREST`
à l'entrée du générateur. La valeur désactive conjointement les filtres
affine/Jung et la construction de grille. Les ouvriers du
[parallélisme courant](../src/parallel/pool.hpp) sont créés après cette
lecture ; le générateur ne reçoit pas de callbacks utilisateur capables
de modifier leur environnement au milieu des calculs. Les callbacks de
fold surviennent après la génération. Pour cette route et cet hôte, la
propagation du mode et les replis sont maintenant exercés directement.

La preuve statique des [marges flottantes](FILTRES_FLOTTANTS_COURANTS.md)
reste l'autorité pour les signes dans son domaine. Le présent test ferme
une obligation d'exécution qui y restait non exercée : le passage effectif
par les replis lorsque le mode demandé n'est pas au plus proche.

## Prochaine action du constructeur

Conserver cette porte dans la qualification du domaine CPU : elle détecte
la suppression de la garde par ses trois modes exclus et la vacuité d'une
désactivation totale par ses compteurs positifs. Elle est réutilisable
sans construire une mosaïque, un catalogue exhaustif ou une structure
globale supplémentaire dans le produit.

Les [options effectives GCC](receipts_20260905/rounding_environment_4.stdout)
désactivent réassociation, `finite-math-only` et `unsafe-math-optimizations` ;
la contraction flottante vaut `fast` et `rounding-math` est désactivé par
défaut. Ces valeurs sont déclarées, pas supposées. La compilation doit
continuer d'épingler les options et de traiter une autre toolchain ou des
options mathématiques partielles comme un autre domaine : `__FAST_MATH__`
ne décrit pas à lui seul toutes les réassociations possibles. Cette
obligation ne signifie pas qu'une divergence ait été observée ici.

La sonde ne prouve ni toutes les conversions i128 vers double, ni toutes
les FMA de la bibliothèque, ni l'équivalence de tout graphe machine aux
expressions de la preuve. Elle ne modifie donc pas le statut public.
GCP non utilisé ; aucun test GPU.
