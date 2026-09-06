# Ancres partagées et choix de descente pour la tour K-NN

6 septembre 2026. Suite indépendante à la [descente vers les minima](../receipts_gabriel_vertices_20260906/README.md), après la question du constructeur sur la tour verticale et son contre-exemple J=1. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucun nouveau C++, moteur produit, benchmark ou GCP.

**Une même ancre inférieure peut servir la connexion horizontale et la feuille verticale. La descente pure ne doit pas supprimer aveuglément le raccourci J=1 ; tester les retraits essentiels dans le catalogue des minima offre aussi une amélioration locale certifiable.** Ces résultats précisent le prototype précédent ; ses preuves et reçus ne sont pas réécrits.

## 1. Identité des deux ancres et stockage réellement partageable

Soit Q Gabriel de cardinal m et de niveau carré a=β(Q). Q naît comme feuille dans Γ_m ; il est simultanément une coface de Γ_(m−1). Après fermeture **du lot inférieur entier**, toutes ses facettes appartiennent à une même composante. Cette composante est exactement l’image verticale de la feuille Q. L’ancre directe fermée et l’ancre verticale initiale sont donc la même valeur **dans l’espace d’identifiants inférieur**.

Cette identité reste vraie si Q ne provoque aucune fusion inférieure. Dans E5, CE est une feuille K2 née à 11/2, mais les points C,D,E sont déjà réunis à K1 depuis 9/2 : aucun nœud inférieur ne naît à 11/2. L’ancre 5 de cette connexion silencieuse reste l’image de la nouvelle feuille supérieure 2. Le [rejeu indépendant](shared_anchor_probe.py) vérifie les journaux antérieurs et les puissances diamétrales strictes de CE ; il ne déduit pas une naissance inférieure de cette naissance supérieure.

Pour H=min(n,Kmax), une table verticale explicite contient $\sum_{m=2}^{H}\lvert G_m\rvert$ entrées. Le resolver classique conserve les mêmes records comme ancres directes, plus le rang H+1 lorsque H<n. Les premières valeurs se partagent ; les dernières n’ont pas de feuille supérieure demandée. À H=n, X garde son ancre inférieure et il n’existe aucun rang n+1. Cette comptabilité ne prouve pas qu’un mot distinct par feuille soit incompressible : valeurs répétées et autres modes de reconstruction restent possibles.

Dans la [capture 8k déjà épinglée](../receipts_gabriel_vertices_20260906/baseline_read.json), les comptes donnent 2 396 646 records de rangs 2..10, contre 716 735 au rang 11. Une tour verticale explicite doit donc représenter les premières images, même si le resolver n’en dépend plus. Il ne faut pas annoncer la suppression des 3 113 381 ancres comme économie de toute la tour. Ces nombres sont ceux d’une capture antérieure, sans nouvelle mesure ; le code actuel traite ses ordres séquentiellement et n’alloue pas déjà une seconde table verticale simultanée.

Un record partagé peut conserver le label Q, son niveau et deux champs de rôles distincts : `upper_birth` dans l’ordre m, et `lower_anchor` dans l’ordre m−1. La seconde valeur peut servir deux usages ; elle ne remplace pas la première. Dans le témoin à quatre points, AD et CD sont deux feuilles K2 distinctes, mais ont toutes deux l’image inférieure 4 à 13/2. Dédupliquer les feuilles par cette image perd une feuille. Pour CD, `upper_birth=1` désigne B si on l’interprète à tort comme token K1 : c’est un entier en plage mais une fausse image.

## 2. Temps exact, plateaux et refus

Aux coupes ultérieures, transporter l’ancre initiale par les fusions de l’ordre inférieur admises à la **coupe demandée**, avec son côté ouvert ou fermé. La naturalité vient du fait que deux facettes supérieures adjacentes partagent une face inférieure : toutes les feuilles d’une même composante supérieure ont la même image inférieure après normalisation. Les parents horizontaux restent lus strictement avant leur lot ; l’image d’une naissance supérieure est fixée après tout le lot inférieur.

Le `successor` mutable d’un ordre inférieur terminé et compressé peut pointer jusqu’à sa racine finale. L’utiliser pour une coupe ancienne anticipe des fusions. Dans notre témoin, l’image correcte à 13/2 est 4 ; sa racine finale 5 contient déjà B à tort pour cette coupe. Inversement, réutiliser 4 sans normalisation à la coupe fermée 13 laisse un ancien token consommé. Une tour à horloge commune ou les liens parentaux historisés du certificat permettent le bon transport. La vérification limitée à la coupe finale masque même la confusion `upper_birth=1`, puisque B rejoint alors la racine 5.

Le [code actuel](../../src/forest/full_gabriel.hpp) écrit `r.token` après normalisation du plateau, puis appelle encore `keep_batch()`. Ces écritures sont privées au Builder ; un refus ultérieur ne publie aucune forêt. **Les copier immédiatement dans un catalogue partagé change cette propriété.** Une ancre partagée doit appartenir à un état accepté : écriture provisoire jusqu’à acceptation du lot, puis invalidation du propriétaire si l’ordre ou la transaction de tour échoue. Un traitement séquentiel peut ne transmettre les ancres qu’après succès de l’ordre entier ; une tour synchronisée peut garder un état de travail privé, invalidé globalement au refus. Les dépenses déjà comptabilisées ne disparaissent pas avec cet état.

Le modèle d’audit vérifie sept records partagés, 65 transports aux coupes des historiques scellés et les horizons Kmax=1..4. Il rejette six mutations de protocole : mauvais ordre, mauvais label, propriétaire non accepté, mauvais niveau, token hors plage et terminal du même lot. Les erreurs sémantiques de confusion d’IDs, de normalisation finale prématurée et de déduplication par image sont également exercées. Il s’agit d’un protocole proposé et d’un rejeu de reçus, pas d’une injection d’échec d’allocation dans le C++.

## 3. Le contre-exemple J=1 et sa réparation locale

Le constructeur a proposé A=(0,3,3), B=(3,2,9), C=(8,6,12), D=(12,9,3), E=(13,6,11). La requête BD est consommée par la directe ABD à 1909/41. Les [sept certificats fixes indépendants](j1_certificates.py) donnent :

| Label | β | Intrus stricts étrangers |
| --- | --- | --- |
| BC | 25/2 | aucun |
| DE | 37/2 | aucun |
| CD | 53/2 | E seul |
| BD | 83/2 | C seul |
| CDE, BCD | 53/2, 83/2 | aucun |
| ABD | 1909/41 | aucun |

Le raccourci actuel calcule B(BD), puis utilise l’ancre fermée de BCD sans seconde MEB. La descente retirant le premier essentiel suit BD→CD→DE et calcule deux MEB. Ce négatif est correct, mais concerne **ce choix de retrait**, pas toute stratégie de descente.

Après avoir trouvé C comme intrus de BD, deux retraits sont disponibles. Retirer B donne CD, non minimum ; retirer D donne **BC, déjà présent dans le catalogue des minima**. La seconde voie termine après la seule MEB de BD, sans ancre directe. Tous ces chemins restent dans la même composante avant consommation, par les cofaces certifiées BCD et CDE.

| Calendrier local sur cette requête | MEB | Census |
| --- | --- | --- |
| Raccourci J=1 conservé | 1 | 1 |
| Premier essentiel, descente pure | 2 | 2 |
| Hybride utilisant l’ancre disponible | 1 | 1 |
| Tester les deux retraits dans le catalogue des minima | 1 | 1 |

Ces nombres portent sur des succès locaux sans limite active, à P=0, avec miss initial du cache. Les lookups, normalisations, nœuds spatiaux, coûts arithmétiques et temps ne sont pas comparés. Seuls sept labels disposent ici de nouveaux certificats, pas tous les sous-ensembles du nuage. Le comparateur constructeur, observé en préparation puis publié dans `a9ce3639`, est attribué par hash, sans être exécuté ou qualifié par ce lot.

## 4. Test de minima avant la MEB suivante

Le raccourci est général sous les mêmes prémisses que la descente. Pour un intrus strict z de F et chaque essentiel s, former $F_s=(F\setminus\lbrace s\rbrace)\cup\lbrace z\rbrace$. On sait déjà que $\beta(F_s)<\beta(F)$ et que F_s est reliée à F par F+z au niveau β(F). Chercher F_s dans le catalogue exact et complet des minima peut donc terminer la résolution **sans calculer sa MEB**. La valeur et l’identité fournies doivent être cohérentes avec cette antériorité, puis normalisées à la coupe consommatrice.

Pour un intrus choisi, au plus q≤4 labels sont testés. Si aucun n’est un minimum, conserver le descendant choisi par la règle de base poursuit exactement cette descente ; les tests infructueux n’ont coûté que des lookups. Si un test réussit, les appels MEB/census suivants de cette trajectoire sont évités. Pour une requête dans le même état initial, à témoins et règle de base fixés, ce filtre n’augmente donc pas leurs nombres. Cette comparaison exclut les effets de cache entre requêtes et les limites actives ; elle peut augmenter le temps de recherche de catalogue. Aucun gain de latence universel n’est affirmé.

Ce filtre peut compléter un hybride : minimum initial, cache, MEB+census exact de F ; raccourci J=1 si son ancre partagée est disponible ; sinon test des retraits essentiels dans les minima, puis descente si tous manquent. Une ancre volontairement absente autorise ce repli. Une ancre présente mais incohérente, trop récente ou issue d’un propriétaire refusé doit être rejetée, jamais contournée. J=0 après un miss dans le catalogue complet des minima reste aussi un défaut d’autorité.

Une intégration encore plus petite, proposée ensuite par le constructeur, est correcte : dans la branche actuelle J≥2, tester seulement $G=(F\setminus\lbrace s\rbrace)\cup\lbrace z\rbrace$ pour les mêmes premier essentiel s et premier intrus z déjà choisis. Si G est un minimum, son token normalisé termine la requête ; sinon poursuivre le code actuel, sans changer sa trajectoire. Le chemin F→G via F+z est déjà certifié par la MEB et le census de F. Aucun nouveau calcul géométrique n’est nécessaire, le J1 reste intact et aucun graphe de paires de minima n’est construit. Dans E5, cela teste CD avant la MEB de la coface CDE et évite celle-ci. Ce delta ajoute un lookup par J≥2 et change les compteurs sur les hits ; il exige sa qualification C++ et ses lecteurs, mais pas un remplacement préalable de tout le resolver.

Le cas J=1 ne demande pas de MEB/census supplémentaires pour F+z, mais paie toujours lookup et normalisation. Les compteurs de recherches pendant une descente ne sont plus bornés par les seules visites initiales de facettes directes. Il faut distinguer demandes initiales, remplacements, tests candidats, hits de minima/cache, raccourcis J1 et terminaisons. L’ancienne identité `meb_calls = portal_requests + chain_steps` ne qualifie pas ce nouveau calendrier.

Si tous les appels du resolver hybride restent de cardinal au plus dix, le calendrier exhaustif F maximal devient $\binom{10}{2}+\binom{10}{3}+\binom{10}{4}=375$ supports au lieu de 550 à onze sites. C’est une borne prospective de ce resolver, pas un changement des lecteurs ou reçus actuels. Les nouvelles boules visitées et leurs contrôles de support/coquille gardent une qualification distincte. Les bornes, budgets et échecs historiques ne sont pas réinterprétés.

## Reproduction et entretien

Les reçus [J1 normal](j1_normal.json) / [optimisé](j1_optimized.json) et [partage normal](shared_normal.json) / [optimisé](shared_optimized.json) sont identiques par paire, codes 0. Depuis la racine, exécuter `python3 -B morsehgp3D_v7/audits/receipts_shared_anchors_20260906/j1_certificates.py` et `python3 -B morsehgp3D_v7/audits/receipts_shared_anchors_20260906/shared_anchor_probe.py`, puis ajouter `-O`. Les sorties JSON vont sur stdout. Aucun helper produit n’est importé et aucune porte ne repose sur `assert`.

Cette note répond aux questions encore actives. La définition du quotient, la preuve de descente et la première tour restent dans leur [lot publié](../receipts_gabriel_vertices_20260906/README.md) ; les recopier n’apporterait pas une nouvelle qualification. Le [dialogue courant](../DIALOGUE_COURANT.md) porte la décision à prendre avec le constructeur.
