# Échanges actifs avec le constructeur v7

5 septembre 2026, sources F et résultats publiés dans `4cc804e5`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans `audits/`.

## Acquis conservés

Le [certificat horizontal réduit E](CERTIFICAT_HORIZONTAL_COURANT.md), S1 et les primitives restent fermés. La [contrelecture F](receipts_vertical_20260905/f_qualification/) confirme ses propres campagnes 339/339 Release, 48/48 ciblées Release et 48/48 ASan/UBSan. La conservation de pile et cette qualification restent distinctes des sondes horizontales exécutées sur E.

Cette reprise relit les sorties scellées, les sources et le manuscrit, puis exécute de petits lecteurs Python normal et optimisé. Aucun build, moteur ou benchmark n'est lancé par l'auditeur. La fenêtre mono constructeur est close à 10:20:35 UTC ; ses résultats négatifs restent conservés.

## Verticale : porter la reconstruction par les tokens

Le [contrat vertical](CONTRAT_VERTICAL_COURANT.md#5-construction-totale-depuis-born-et-parents) ferme maintenant la construction : à chaque vraie naissance `parents=[]`, parcourir `born`, supprimer le plus grand PointId de chaque label essayé, puis chercher cette face dans l'état inférieur fermé au même niveau exact. Une facette stricte de la coface de naissance est nécessairement directe en bas ; elle garantit un succès en au plus `|born|` essais. Propager ensuite l'ancre par les parents, normaliser sa cible à la coupe demandée et vérifier l'accord des ancres lors d'une multifusion.

Le [lecteur d'audit](vertical_anchor_replay.py) n'utilise aucune géométrie. Sur les 16 sorties E originales, il retrouve 764 cartes, 720 carrés et 400 compositions par provenance O2/UBSan. Un réindexage explicite impose cinq misses avant le sixième succès ; un flux mathématique séparé exerce une multifusion source absente de ce petit corpus produit. Les [reçus](receipts_resolver_20260905/README.md) distinguent ces provenances et les corruptions du lecteur.

**Le resolver géométrique général n'est plus un verrou de cette route.** Le prochain livrable est le port de ce scan, de la propagation et des contrôles de coupe, puis l'export lié aux identités source/cible et au succès terminal. L'API et l'archive déclarent encore `vertical_maps=none`.

## Vote p3 : raccorder les incidences et l'autorité numérique

Le [contrat d'incidence](CONTRAT_MASSES_VOTE_COURANT.md) fixe les univers de facettes et cofaces ; leurs contributions doivent être conservées avant la réduction H0. `build_render` et les événements encore accessibles au callback offrent un point de raccord pour un univers déclaré. Aucun poids sparse ne remplace silencieusement celui du manuscrit.

L'[autorité p3](AUTORITE_VOTE_P3_COURANTE.md) ferme la comparaison des numérateurs de vote : regrouper les racines de même classe de carrés pour reconnaître exactement les égalités, puis séparer les signes par intervalles rationnels `isqrt`. Le dénominateur positif commun d'un point s'élimine. Un plafond atteint rend `indeterminate`, sans égalité approchée. Les 27 cas, quatre permutations et quatre corruptions passent normalement et sous `-O`.

Cette autorité porte sur les numérateurs. Les quotients de masses, les seuils de condensation, les probabilités et leur coût ont encore besoin de leur propre contrat numérique. L'export pondéré et sa qualification restent à intégrer.

## Palier F : traiter les occurrences avant la déduplication

Les [trois paires E/F à 8k](AUDIT_QUALIFICATION_20260905.md) concordent sur les objets pour s=8/10/12. F termine à 16k en 413,816 s, pic RSS 5,113 GiB ; à 32k il refuse à K9, code 2, `silent_core_record_budget`, sans tour publiée. Ce refus fermé n'est pas un timeout et ne donne aucun temps d'achèvement de la tour.

Le plafond de huit millions compte les occurrences temporaires produites par les retraits de supports, avant tri et déduplication. Le `core=0` du refus ne mesure pas ce travail. Précompter ces occurrences, les distinguer des facettes uniques dans les diagnostics, puis évaluer une compression ou un internement sous budgets distincts de travail et de stockage donne une suite concrète. Un relèvement aveugle du cap ne résout pas le coût intermédiaire. Les anciens leviers de [coexistence mémoire](RETOUR_MEMOIRE_COURANT.md) restent séparés de ce refus mono.

## Entretien

Un rapport de référence porte chaque conclusion. Les anciennes demandes de résolution générale et d'attente des paliers F sont retirées des entrées actives. Les preuves, contre-fixtures, refus et essais d'audit invalides sont conservés. Les identités publiques, les plateaux à étendre et les coûts de bout en bout restent distincts. Aucun Gamma exhaustif ne devient le chemin produit. GCP non utilisé.
