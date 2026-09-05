# Domaine CPU des preuves et des exécutions

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le domaine retenu est **Linux x86-64, GCC 13.3 et sa bibliothèque C++/mathématique, types entiers du profil et binaire64 sans réassociation**. La qualification complète exécutée appartient au snapshot D. Le delta E q2 a une contre-épreuve locale distincte ; les clauses arithmétiques du front concernent des sources communes inchangées.

## Commandes et options effectivement observées

Le [journal de compilation D](receipts_20260905/release/compile_commands.txt) contient pour la CLI :

```text
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror
```

Il n'y a pas de `MHGP7_TESTING` dans la commande produit ; les portes causales le définissent séparément. La configuration enregistre `MHGP7_ENABLE_CUDA=OFF`. Le [cache](receipts_20260905/release/CMakeCache.txt), les hashes des [37 binaires](receipts_20260905/release/binaries_after.json) et les [115 sources](receipts_20260905/release/sources_after.json) identifient ce run. Il ne qualifie aucune compilation CUDA ou autre ABI.

La [porte d'arrondi](AUDIT_ARRONDI_20260905.md) est compilée avec les mêmes options numériques et `-pthread`. Son [reçu](receipts_20260905/rounding.json) conserve compilateur, cible, bibliothèques chargées et options effectives : association, calcul rapide et hypothèse de finitude désactivés ; `fp-contract=fast`, `rounding-math` désactivé. Les FMA explicitement écrites et les marges conservatrices restent les contrats des preuves. Cette observation des options et les tests ne constituent pas une preuve du compilateur lui-même.

## Préconditions d'exécution qui restent nommées

Les filtres affine/Jung/corde supposent conversions et opérations binaire64 conformes, FMA conforme et aucune réassociation détruisant leurs bornes. Le localisateur demande aussi `FLT_EVAL_METHOD==0`. Les calculs concernés restent dans le domaine normal fini par les bornes déjà établies. Le mode numérique de chaque thread reste stable pendant son calcul ; un appelant ne doit pas modifier cet environnement au milieu d'une opération.

La garde coupe les filtres sous `__FAST_MATH__` ou un mode différent de `FE_TONEAREST`, et la construction des grilles est refusée dans cet environnement. Elle n'est pas un détecteur de toute option de compilateur isolée : ajouter par exemple une hypothèse de finitude ou de réassociation sort de cette qualification, même si une macro particulière reste absente. Le domaine positif est celui des commandes épinglées, sans demander une preuve universelle de toute toolchain.

Les racines entières corrigées ont une exigence plus faible : la proposition flottante doit être finie, non négative et convertible ; les boucles entières prouvent ensuite le résultat exact. Les [bornes fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md#4-racines-corrigées--ce-qui-est-prouvé-ce-que-lenvironnement-fournit) et [cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md#5-racine-proposée-en-flottant-puis-corrigée-en-entier) rendent les casts et carrés représentables avant correction. Une précision finale parfaite de `sqrt` n'est pas une prémisse supplémentaire.

## Ce qui est exercé et ce qui reste à intégrer

Les 40 appels compilés de la porte d'arrondi exercent effectivement les quatre modes et un/deux threads : 65 588 certificats au plus proche, 2 292 grilles et 189 582 replis entiers dans les autres modes ; objets identiques sur les fixtures. Les 323 CTests D et les sondes indépendantes ont leurs reçus séparés. La nouvelle fermeture du front ajoute des preuves de largeur et des certificats Python, sans les compter comme des exécutions C++.

La prochaine porte utile raccorde les quelques frontières nouvelles des certificats aux helpers compilés, en réutilisant les fixtures permanentes existantes. Elle n'a besoin ni de reconstruire Gamma ni de balayer le domaine u16. Ces valeurs doivent être intégrées à la qualification du prochain delta constructeur, puis le certificat horizontal doit nommer son domaine de régularité et son payload réduit. Verticale, vote, compteurs globaux et coût de bout en bout gardent des contrats distincts.

GCP non utilisé. Aucun code produit ou drapeau de compilation modifié.
