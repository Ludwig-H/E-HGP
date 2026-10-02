# Revue du catalogue parallèle WIP — capsule 17

Lecture indépendante de sources figées **avant** analyse : DEV HEAD `7f1922c7743d8682e2665a491b01d32e8f2d546c`, port parallèle non commis dans cette capture. La v10 R2 `865f5e64ddd08bedf6ab8f94e8bb94812e380e79` est un différentiel de lecture, sans transfert de qualification. Aucun produit, test natif, build, GCP ou allocation massive exécuté. `SOURCE_BEFORE.json` conserve la capture initiale ; `SELECTION.json` exclut caches Python et chantier tower indépendant. Les dépendances sched sont conservées ; leur audit appartient à la racine et est joint séparément dans `pool_review/` (copie intégrale vérifiée, manifeste SHA `e953602e10aeac61e14b760460dc8ebfe0e45e470e677cb1a21bdbe51d3201f2`). Cette lecture et son modèle ne simulent ni pthread ni les atomiques C++.

Deux conseils concrets nouveaux : l’admission fixe de frontière refuse très tôt le régime massif, et G1 effectue à la racine des tests nécessairement négatifs. Ce sont des limites d’admission et de travail, **aucun défaut géométrique exécuté**. Le raccord count/fill est favorable à la lecture.

## 1. Admission de frontière, même avec W1

`src/catalogue/parallel.cpp:110–112` appelle sans paramètre public de profondeur `frontier_memory_bound(n,8)` ; `frontier.cpp:29–33` donne **4n(2⁸+8+2)=1064n octets supplémentaires**, avant construction, pour toute surcharge `build_catalogue(...,Pool&)`, W1 compris. Le mode séquentiel sans Pool ne passe pas cette porte. Cette majoration compte jusqu’à 256 listes finales de capacité n, la pile et la racine ; elle ne suppose pas que les listes soient disjointes.

Avec plafond L et réservations préexistantes stables U, cette première porte impose U+1064n≤L. À **8 Gio=8 589 934 592 octets** : n≤8 073 246 pour U=0 ; n≤7 866 240 lorsque le même budget porte déjà un Cloud unitaire de 28n+8 octets. Ce sont des conditions nécessaires de la première porte, jamais des capacités suffisantes du catalogue.

| Sites n | Frontière préadmise seule, Go | Gio | Cloud unitaire + première admission, octets |
| ---: | ---: | ---: | ---: |
| 39 885 | 0,04243764 | 0,03952313 | 43 554 428 |
| 30 000 000 | 31,92 | 29,72781658 | 32 760 000 008 |
| 50 000 000 | 53,2 | 49,54636097 | 54 600 000 008 |

Ce plafond préadmis n’est **pas un pic atteint**, ni une panne mémoire constatée. Le contrat v11 actif reste une trame entière ; l’architecture place le massif hors de ce contrat. Le conseil est de rendre ce refus et sa marge diagnostiquables avant un futur port massif.

Une profondeur d choisie en tenant compte de **toutes les coexistences** permet de garder moins de jobs et de poursuivre chaque suffixe DFS complet : la première borne devient 4n(2ᵈ+d+2), soit 12/20/88/1064 octets par site pour d=0/1/4/8. Réduire d ne suffit pas seul : les bornes de DFS suffixes peuvent augmenter. Une autre option est un count/replay du préambule pour admettre les **capacités payées réelles**, avec coût du passage supplémentaire explicitement mesuré. Aucun quota de candidats, tronquage de listes ou résultat partiel ne remplace ce raccord.

## 2. Tests G1 impossibles lorsque le candidat appartient à la fermeture

`boxes.cpp:54–67` prépare le réservoir puis teste chaque candidat jusqu’à K dominateurs, sans raccourci d’appartenance. Si x appartient à la **fermeture** de Q, choisir le centre c=x donne dist(x,c)=0≤dist(y,c) : aucun y ne domine x strictement sur toute la fermeture. G1 doit donc conserver x, y compris **x=hi**, même si la propriété des centres utilise ailleurs Q demi-ouvert.

La racine est l’enveloppe entière de tous les sites (`make_root`, `boxes.cpp:159–163`). Ses n sites sont tous conservés ; le filtre y paie exactement n·min(n,3K) tests, par passe. Pour n=39 885,K5 : **598 275 par passe, 1 196 550 pour les deux passes**. Leur impossibilité est démontrée ; leur contribution au temps n’a pas été mesurée. Il s’agit de O(nK), K≤12, pas d’un carré.

Optimisation sûre à étudier : conserver directement tout x∈Qbar, sans les comparaisons de dominance ; garder distincts les compteurs de tests effectivement exécutés, de retenues par appartenance et de scans. Une racine spécialisée pourrait aussi transférer la liste racine au ReadyNode, au lieu de copier un second tableau, si elle préserve exactement comptage de nœuds, ownership et refus. L’optimisation ne remplace ni réservoir pour les sites extérieurs, ni ajustement d’enveloppe, ni census I/U. La fixture frontière x=hi doit passer et les sorties canoniques doivent rester identiques ; les compteurs de travail changeraient légitimement.

## 3. Raccord propriétaire et deux passes : lecture favorable

- La frontière coupe **après** filtre et ajustement, déplace les listes dans des ReadyNode possédés (`frontier.cpp:8–16,55–57`). Les listes peuvent se recouvrir : ligne 0..4, K1, coupe d=1 → listes [0,1,2] et [2,3,4], six éléments logiques pour cinq sites, mais **dix places payées**. La partition porte sur les boîtes de **centres**, jamais sur les sites.
- `split_ready` pave la boîte ajustée par intervalles demi-ouverts gauche/droite ; `center_in_box` teste exactement les bornes rationnelles, contacts affectés au côté droit. Un centre admis reste dans l’enveloppe du support positif conservé. Reprendre un ReadyNode ne refiltre et ne recompte pas sa visite ; feuilles/préfixes restent dans le suffixe (`boxes.cpp:145–156`).
- Count et fill ont les mêmes ordinaux DFS, sans dépendre du worker choisi. Les comptes et offsets sont u64, addition gardée avant allocation ; chaque fill reçoit ses spans exacts (`parallel.cpp:47–65,85–98`). Le **population_begin local est rebasé** après comparaison des comptes et de tout le ledger. Le tri global exact niveau/S* puis l’assemblage CSR suivent ce rebasage ; aucun plateau exact n’est fractionné par worker.
- Le préambule est rejoué et ses listes/boîtes/profondeurs/capacités et ledger comparés ; chaque suffixe compare également son ledger. Les quinze champs additifs et deux maxima sont agrégés. Le quota de nœuds est partagé par le préambule et les suffixes **d’une passe** ; un second quota sert le fill. Sur succès, les compteurs logiques sont ceux du mono, génération réellement payée deux fois.
- Cloud/paramètres/Pool sont empruntés stables jusqu’au retour joint ; aucune copie Cloud/index par job. La frontière et les brouillons restent privés, sorties publiées seulement après assemblage réussi. La garantie de joins et la réutilisation du Pool dépendent de sched, revu séparément par la racine.

Soit F=4Σ(capacités des listes possédées), A=min(n,max_leaf), W'=min(W,J), S=W'·(sizeof(Point)·A+8A·ceil(A/64)+8A), H=somme des W plus grandes bornes de DFS suffixes, V=4n(d+2), T=sizeof(Emission)·B+4P. La lecture justifie les coexistences suivantes, où U reste vivant :

| Phase | Majorant des Buffer simultanés |
| --- | --- |
| Préparer la frontière | U+4n(2ᵈ+d+2) |
| Count suffixes | U+F+S+H |
| Sorties + replay/fill | U+F+S+T+max(V,H) |
| Assemblage | U+T+R_catalogue |

H utilise au plus (3B_coord−profondeur) tableaux descendants, chacun de capacité au plus count de la tâche ; aucun nouveau tableau au ReadyNode lui-même. Tous les workspaces sont réservés avant les workers ; listes de frontière déjà comprises dans used(). Frontier/scratch sont détruits avant Assembly::finish. Ces formules portent sur les Buffer ; métadonnées fixes, piles/threads/IO/RSS ne sont pas des Buffer mesurés. Les sizeof restent des expressions de l’ABI, sans nouvelle mesure native.

Les nouvelles portes déclarées comparent W0/W1/W2/W4/W8, 17 compteurs, listes recouvrantes, profondeur huit et suffixes, limites **globales** nœuds/boules, résultats retenus/refus et chaque allocation avec reprise. L’oracle Fraction parallèle ajoute les mêmes demandes aux quatre tailles de Pool. Ces tests sont présents et relus, **non exécutés ici**. Pour renforcer une prochaine mesure, séparer temps/réservations du préambule, count, replay, fill et assemblage ; le banc W8/W48 présent ne fournit pas de nouveau chrono W0 apparié.

## 4. Comparaison critique v10 figée

V10 R2 `generator.cpp:701–789` prépare une frontière par cible **64W** et charge locale : scan des sites de chaque parent pour `inside`, sous-frontières vectorielles, copies vers `shared_ptr<const vector>` et brouillons par worker. Elle paie également des copies globales P/X/X²/w (`681–693`) puis références/tri/bandes/sorties (`797+`). Ces tableaux ne sont pas les nouveaux Buffer de v11. Une taille de jobs v10 ou son chrono ne justifie donc pas une capacité v11 ni son budget.

Le port v11 est plus explicite sur ownership, deux passes et capacités simultanées, et évite le scan de charge v10. Sa profondeur fixe peut toutefois refuser bien avant les capacités réelles et laisser un déséquilibre non mesuré. Ne restaurer ni facteur global 64W, ni copie de parents sans budget, ni bandes flottantes pour lever ce refus. Le point utile est de mesurer les capacités/charges et choisir la coupe sous admission globale, tout en conservant les suffixes exhaustifs et le tri exact.

## Preuve et limites

`scalar_model.py` est autonome : 5 173 contrôles normal/−O identiques, boîtes fermées et contact hi, partition et reprise exactes d’un modèle 1D K1 **à tous témoins** (distinct du réservoir natif), coexistences par somme des plus grandes tâches, seuils u64 sans gros tableau, populations hétérogènes et mutants de rebasage/troncature. Ce modèle ne rejoue ni ne qualifie le produit. `SOURCE_AFTER.json` consigne toutes les différences LIVE après lecture ; la capsule ne juge que ses copies initiales. Aucun ancien reçu ni note active modifié.
