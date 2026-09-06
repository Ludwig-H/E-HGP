# Contrelecture des captures constructeur FULL + MEB

6 septembre 2026. Lecture indépendante des sources et sorties capturées, sans import des juges constructeur, compilation ni moteur. `public_status=not_claimed`. GCP non utilisé.

Le [lecteur indépendant](constructor_capture_review.py) ferme 3 584 vérifications. Les reçus [normal](constructor_capture_review_normal.json) et [optimisé](constructor_capture_review_optimized.json) sont identiques : `8c63e90cfb906671a2ec63a525d18c838f9cf35bea3d811134f169ff5da0358f`. Le sceau constructeur `bbdbc40d…` lie **1 250 fichiers de contenu**, auxquels s’ajoute `SHA256SUMS`. Inventaire fermé, octets, 120 sources CMake avant/après, cinq variantes et intentions/exécutions sont raccordés ; aucun fichier vivant du produit n’est consommé par ce lecteur.

## Qualification CMake attribuée

Les journaux R3 ferment les mêmes **30/30 CTests Release et 30/30 ASan/UBSan**, avec dix exécutables par build. Seuls les deux tests historiques singleton/successeurs portent `MHGP7_TESTING` ; les huit autres passent par le code produit. Le wrapper capturé vérifie le code numérique exact de chaque commande, y compris 2 pour les arguments inconnus ; un signal ne peut devenir un succès. Les flags C++20 stricts et l’environnement ASan/UBSan avec détection des fuites sont déclarés dans les captures. Les identités ELF sont liées aux commandes ; les ELF ne sont pas distribués et ne sont pas réexécutés ici. Les dépendances système identifiées après capture ne constituent pas un environnement hermétique.

La composition rapporte, **par mode et par build**, 93 ordres, 1 488 sorties Gamma et 33 792 coupes. Le mode rejets ajoute 160 caps exacts, 160 cap−1, 28 refus d’entrée et 120 refus conservant une dépense P. Les balayages P0/P1/grand comptent chacun 49 fautes eager et 209 lazy, sans exception échappée ; leurs sources contrôlent aussi le préfixe conservé en fin d’allocation. Ces résumés ne remplacent pas le rejeu géométrique indépendant du présent dossier. Les q4 de catalogue ne sont toujours pas un témoin de dispatch rapide q4 dans FULL.

## Douze injections tardives par build : ce qui est observable

La variante `form_fault` porte une unique modification déclarée du helper `f922544b…` vers `c3e16693…` : `NoObserver::before_form` perd `noexcept` et appelle le hook privé forcé à la compilation. Le gate `079ee371…` et le hook `61f9e541…` sont relus et épinglés. Cela ne prétend pas que le NoObserver nominal puisse lever. Aucun prédicat F n’est interrompu ; cette campagne n’engage même aucun repli F.

Les douze cas sont **deux routes eager/lazy × trois exceptions × wrapper/Builder explicite**. Le hook arme une seule exception après au moins deux MEB certifiées. P est facturé avant le callback, la forme interrompue n’est pas encore évaluée et l’appel géométrique n’est pas encore incrémenté. Les quatre `bad_alloc`/`length_error` publics rendent les raisons exactes et une forêt vide ; les deux `runtime_error` publics se propagent. Les six appels directs du Builder propagent les trois types et rendent leurs statistiques inspectables après destruction, puisque le résultat externe survit au Builder.

O2 et SAN rapportent identiquement : 12 cas, 4 refus publics, 2 propagations runtime publiques, 6 propagations Builder, 10 miroirs observables, 8 comparaisons complètes de statistiques, deux baselines Gamma et six reprises fraîches. `paid_at_throw=36`, joint aux gardes exigeant au moins trois charges dans chacun des douze cas, implique **p=3 et deux certificats à chaque interruption** ; les contrôles imposent alors trois appels FULL contre deux géométriques, avec A et fallback nuls. Cette déduction porte sur les prédicats compilés et leur résumé de succès, pas sur douze dumps d’états : **les états individuels ne sont pas publiés**. La valeur exacte de c et les cinq champs Work ne sont donc pas rejugés indépendamment, cas par cas, par ce lecteur.

Les deux wrappers runtime détruisent leur résultat privé pendant la propagation : ce résultat n’est pas observable ici. Le `out` externe resté par défaut n’en prouve pas le contenu ; le gate distingue correctement cette limite des dix miroirs inspectés. La panne avant finalisation ne prouve pas à elle seule une purge de forêt déjà non vide : cette obligation appartient aux balayages d’allocation séparés.

## Mutations, incidents et limites

Les quatre mutants O2 ont leur remplacement unique, leur compilation réussie, leur code 1 et leur première cause vérifiés : charge après callback, miroir A supprimé, Work réinitialisé et miroir FULL supprimé. Les deux derniers sont réfutés dès le bras P0 sur A ; **leur première cause ne démontre pas isolément la persistance de P**, qui dispose de sentinelles distinctes dans la composition. Aucun de ces rejets n’est assimilé à un signal ou à une erreur de compilation.

Les deux tentatives en échec restent conservées : `release_configure` code 1 sans le chemin Boost, puis `san_build` code 2 sous plafond de fichier temporaire. R3 ne les réécrit pas. La dernière clôture C++ du paquet est `2026-09-06T09:36:43.022417+00:00`.

Le paquet `extra/` est couvert par le sceau, mais ses jugements scientifiques ne sont pas réexécutés ou importés ici. Cette contrelecture n’ajoute aucune qualification K9/K10 de FULL, CLI, archive, verticale, performance ou G4. Pour refaire seulement les vérifications documentaires, exécuter `constructor_capture_review.py` normalement et avec `python3 -O`.
