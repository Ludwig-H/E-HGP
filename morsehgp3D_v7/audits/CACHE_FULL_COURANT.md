# Cache FULL paresseux : qualification indépendante

Header gelé `13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627`, capturé après `6f4b4de5`. Cadre CPU u16 hors registre ; `public_status=not_claimed`.

**Les quatre politiques produisent les forêts FULL attendues sur les 109 ordres du corpus indépendant, y compris à cache nul ou saturé.** Aucun défaut nominal trouvé. La qualification reste relative à des catalogues fournis complets, exacts et réguliers.

| Preuve propre à l’audit | Résultat par build nominal |
| --- | --- |
| O2 et ASan/UBSan, sorties identiques | 109 ordres, 218 représentations, EAGER/C0/C1/C100000 : 872 sorties et 67 920 coupes |
| Comparaisons entre politiques | 654 forêts lazy/EAGER ; 218 égalités de travail sans skip ; 200 représentations EAGER historiques inchangées |
| Budgets | 16 plafonds exacts, 180 refus cap−1, douze conflits d’API |
| Trois mutants privés causaux | J1 supprimé : 96 sorties touchées ; minima confondus : 54 témoins ; capacité ignorée : 58 dépassements |

Les [sources, fixtures, commandes et jugements](receipts_full_lazy_20260905/README.md) détaillent ces comptes. Cent ordres historiques sont réemployés sous hashes ; neuf nouveaux ordres sont recalculés rationnellement. Normal et `-O` concordent. J1, descente à deux étapes, ancres muettes, minima simultanés, K=n et réutilisation dans un même lot sont exercés. Les 218 sorties EAGER de chaque mutant restent identiques au nominal ; une erreur de transport ne compte jamais comme réfutation.

Le [contrat constructeur](../docs/CONTRAT_CACHE_FULL_PARESSEUX.md) porte le calendrier. La [revue sémantique](receipts_full_lazy_20260905/semantic_review.md) confirme que minima, ancres de toutes les directes et successeurs restent permanents. Le cache facultatif ne contient que des résolutions dérivées. Les dépenses restent cumulatives après saturation.

Le corpus expose le coût déplacé : EAGER exécute 22 MEB, C0 94, C1 88 et C100000 82. Sans skip, les 60 MEB supplémentaires sont exactement les J1 supplémentaires ; les douze pas de descente par bras restent identiques. Cela ne mesure aucun gain de temps ou de RSS.

Les [14+14 CTests constructeur](receipts_full_lazy_20260905/publication_binding.md), les [24 admissions n=8 de la sonde](receipts_full_lazy_20260905/probe_admission_review.md) et le [supplément first-C](receipts_full_lazy_20260905/first_c_companion_review.md) sont contre-vérifiés séparément. Le dernier contrôle exige `inserts=min(C,portails)` en succès, en composition obligatoire avec le v2 gelé et le sceau. Il réfute la corruption coordonnée d’un reçu réel ; les captures historiques restent intactes.

Le digest lie entrée, ordres et topologie dans son domaine valide ; une empreinte égale n’est pas une preuve géométrique ou de complétude. Les compilations et moteurs d’audit sont clos à 17:42:45 UTC le 5 septembre. Les grandes campagnes ultérieures du constructeur, l’export, la verticale, les masses et le SLO ne sont pas qualifiés par ce reçu. GCP non utilisé.

## Lot contenant une seule directe : avis statique

Sur le même header `13c6cc72`, la branche `de-db==1` peut supprimer la DSU locale. Le lecteur impose 2≤q≤4. Les q appels à `locate` rendent des racines pré-lot, toutes unies à la première par la DSU actuelle : son unique groupe non vide est exactement l’ensemble des tokens rendus. Le tri/unique d’un tableau de quatre éléments produit donc les mêmes parents, sans hypothèse géométrique supplémentaire. Cela supprime les allocations des structures locales de regroupement, pas toutes les allocations du lot.

La transformation doit garder les q `charge(face_visits)` puis `locate` dans l’ordre du support, même si des tokens se répètent. Dédupliquer les demandes elles-mêmes changerait le cache first-C, les dépenses et les frontières de refus. Sauver le premier token avant tri et conserver son `normalize` final : utiliser directement le nouvel ID de fusion éviterait des opérations de successeurs facturées.

Le nombre de naissances n’est pas borné par q. Garder `prior_count` avant elles, résoudre tous les parents avant leur installation, attribuer les IDs aux naissances puis à l’éventuelle fusion. Avec un seul parent distinct, aucune fusion ni charge de parents ; conserver néanmoins l’ancre fermée, `no_op_connections` et les alias eager avant l’élimination éventuelle du batch vide. Les lots à zéro ou plusieurs directes restent hors de cette branche.

La comparaison ciblée doit conserver forêts **et compteurs logiques** entre politiques eager/C0/C1/grande capacité, avec budgets exacts et cap−1. Couvrir q2/q3/q4, quatre parents distincts, racines répétées, no-op consommé ultérieurement, une directe avec naissances simultanées et deux directes dans le même lot. Les corpus existants fournissent plusieurs de ces témoins ; les compléments géométriques doivent être admis par l’oracle. Le balayage des fautes d’allocation doit être recalculé sur les sites restants : mêmes refus transactionnels et arènes vides, sans imposer les 434 ordinaux de l’ancien programme. Cet avis ne qualifie pas le futur delta C++.

## Normalisation : supprimer la dernière paire redondante

Le [contrat actuel](../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) facture déjà 3d+1 opérations pour un appel terminé de profondeur d. **On peut conserver exactement le tableau final avec deux opérations de moins lorsque d>0**, sans allocation supplémentaire : retenir pendant la recherche le dernier nœud avant la racine, puis arrêter la compression avant ce nœud. Son successeur vaut déjà la racine ; le relire et réécrire la même valeur est inutile. Pour d=0, garder la lecture terminale ; pour d=1, aucune écriture ne reste nécessaire. Ne pas supprimer l’incrément de `normalized_anchors` dans ce dernier cas.

Preuve par appel : tous les nœuds antérieurs au dernier reçoivent la même racine ; le dernier et la racine restent identiques. Le tableau entier après succès est donc celui de `13c6`. Par induction, les normalisations suivantes ont les mêmes profondeurs, racines, demandes géométriques et effets first-C, tant qu’aucun refus n’interrompt la comparaison.

En lazy réussi, noter T=`face_visits`, D le nombre de directes, S=`successor_steps`, A=`normalized_anchors` et Z=`no_op_connections`. Chaque demande stricte normalise une fois : minimum, cache ou terminal. Après un miss, `put_alias` ne peut rencontrer une nouvelle clé identique, puisque sa résolution n’a pas modifié le cache. La fermeture normalise ensuite chaque directe une fois. D’où :

$$N=T+D,\qquad H=\frac{S-N}{3},\qquad S_{\mathrm{nouveau}}=S-2A.$$

H est la somme des profondeurs, A le nombre d’appels de profondeur positive. Les clôtures ont profondeur zéro pour les no-op, un sinon : les parents étaient des racines pré-lot et un seul étage de fusion est installé. Leur charge est donc `4D−3Z`, et leur contribution à H et A vaut `D−Z`. Les [48 ordres réussis recalculés](successor_work_review.json) satisfont ces identités, même avec cache saturé. Les moyennes ainsi reconstruites ne donnent ni profondeur maximale ni distribution des chemins.

**Changer 3d+1 en 3d−1 pour d>0 change le contrat de charge.** Versionner explicitement les unités/calendriers et leurs lecteurs ; facturer prospectivement les opérations restantes, jamais soustraire des dépenses après coup. Conserver virtuellement 3d+1 permettrait une optimisation machine mais conserverait l’ancien refus budgétaire. Les plafonds numériques peuvent rester identiques ; leurs admissions ne sont alors plus des comparaisons à charge identique.

Le K9 refusé n’entre dans aucune de ces égalités de succès : une demande peut être inachevée, des directes pas encore fermées et A déjà incrémenté avant un refus de compression. Les données ne permettent pas d’affirmer sa réussite sous 128 millions. Pour le futur delta, comparer l’état complet après d=0/1/2/longue, les appels répétés, puis les forêts et autres compteurs existants. Les charges restantes d=0/1/2/3 sont 1/2/5/8 ; exercer leurs budgets exacts et cap−1, les refus au milieu des deux passes et les arènes vides. Aucun changement de normalisation n’est qualifié ici.
