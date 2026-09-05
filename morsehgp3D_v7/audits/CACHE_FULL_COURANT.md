# Cache et lots FULL : qualification indépendante

## Lot unitaire qualifié : 21b77d29

**La spécialisation du lot à une directe conserve les forêts et les compteurs sur le corpus indépendant.** Le header `21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5` est capturé avec ses 19 dépendances produit. Deux builds neufs, O2 et ASan/UBSan avec détection des fuites, exécutent chacun **114 ordres, 912 sorties et 69 120 coupes**. Aucun défaut nominal trouvé ; qualification relative aux catalogues complets, exacts et réguliers fournis. `public_status=not_claimed`.

Les [preuves propres à ce delta](receipts_full_singleton_20260905/README.md) distinguent :

- 109 ordres historiques réexécutés : les 872 sorties sont identiques octet pour octet à `13c6`, y compris compteurs et préfixes refusés ; 16 plafonds exacts, 180 refus cap−1 et douze conflits d’API.
- Le calendrier déduit de Gamma, sans observation de branche : 247 lots q2, 134 q3, 22 q4 ; 19 lots à quatre parents, 42 avec 1<U<q, 14 multi-directes et cinq consommations ultérieures d’ancres sans fusion.
- Un supplément nécessaire : le corpus précédent n’avait aucune naissance simultanée à une directe unique. Un nuage de cinq points, proposé par le constructeur, est recalculé rationnellement sur ses 26 sous-ensembles non triviaux. Au niveau 25 et K=2, la naissance précède bien la fusion à trois parents. Ses cinq ordres ajoutent 40 sorties et 1 200 coupes par build.
- Une mutation privée conserve les q résolutions mais perd le quatrième token au regroupement : 136 sorties changent sur 872, le juge rejette ; build et transport restent réussis. Les trois mutants historiques lazy ne sont pas réexécutés dans ce nouveau paquet.

Les captures constructeur sont contre-vérifiées séparément : **17/17 Release et 17/17 ASan/UBSan**, sept binaires dont seul le différentiel singleton porte `MHGP7_TESTING`. Les nouveaux balayages comptent 49 fautes eager et 209 lazy, toutes refusées sans échappement ; les 357 paires refusées du différentiel passent. Ce sont des captures inspectées, pas des CTests relancés par l’auditeur.

La relecture du delta retrouve les conditions démontrées ci-dessous : demandes dans l’ordre, premier token conservé, déduplication après résolution, naissances avant fusion et suffixe commun fermant aussi les no-op. Aucune cellule, coface ou incidence géométrique supplémentaire n’est nécessaire. La génération q4 et la normalisation ne changent pas. Les campagnes mono ultérieures, la réduction du compteur des successeurs et le SLO restent hors de cette qualification.

## Témoin lazy antérieur : 13c6cc72

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

Ces conditions sont désormais exercées par la qualification `21b77d29` ci-dessus. Les fautes d’allocation sont recalculées sur les sites restants : les 434 ordinaux du témoin lazy antérieur ne sont pas une cible à conserver.

## Normalisation : supprimer la dernière paire redondante

Le [contrat actuel](../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) facture déjà 3d+1 opérations pour un appel terminé de profondeur d. **On peut conserver exactement le tableau final avec deux opérations de moins lorsque d>0**, sans allocation supplémentaire : retenir pendant la recherche le dernier nœud avant la racine, puis arrêter la compression avant ce nœud. Son successeur vaut déjà la racine ; le relire et réécrire la même valeur est inutile. Pour d=0, garder la lecture terminale ; pour d=1, aucune écriture ne reste nécessaire. Ne pas supprimer l’incrément de `normalized_anchors` dans ce dernier cas.

Preuve par appel : tous les nœuds antérieurs au dernier reçoivent la même racine ; le dernier et la racine restent identiques. Le tableau entier après succès est donc celui de `13c6`. Par induction, les normalisations suivantes ont les mêmes profondeurs, racines, demandes géométriques et effets first-C, tant qu’aucun refus n’interrompt la comparaison.

En lazy réussi, noter T=`face_visits`, D le nombre de directes, S=`successor_steps`, A=`normalized_anchors` et Z=`no_op_connections`. Chaque demande stricte normalise une fois : minimum, cache ou terminal. Après un miss, `put_alias` ne peut rencontrer une nouvelle clé identique, puisque sa résolution n’a pas modifié le cache. La fermeture normalise ensuite chaque directe une fois. D’où :

$$N=T+D,\qquad H=\frac{S-N}{3},\qquad S_{\mathrm{nouveau}}=S-2A.$$

H est la somme des profondeurs, A le nombre d’appels de profondeur positive. Les clôtures ont profondeur zéro pour les no-op, un sinon : les parents étaient des racines pré-lot et un seul étage de fusion est installé. Leur charge est donc `4D−3Z`, et leur contribution à H et A vaut `D−Z`. Les [48 ordres réussis recalculés](successor_work_review.json) satisfont ces identités, même avec cache saturé. Les moyennes ainsi reconstruites ne donnent ni profondeur maximale ni distribution des chemins.

**Changer 3d+1 en 3d−1 pour d>0 change le contrat de charge.** Versionner explicitement les unités/calendriers et leurs lecteurs ; facturer prospectivement les opérations restantes, jamais soustraire des dépenses après coup. Conserver virtuellement 3d+1 permettrait une optimisation machine mais conserverait l’ancien refus budgétaire. Les plafonds numériques peuvent rester identiques ; leurs admissions ne sont alors plus des comparaisons à charge identique.

Le K9 refusé n’entre dans aucune de ces égalités de succès : une demande peut être inachevée, des directes pas encore fermées et A déjà incrémenté avant un refus de compression. Les données ne permettent pas d’affirmer sa réussite sous 128 millions. Pour le futur delta, comparer l’état complet après d=0/1/2/longue, les appels répétés, puis les forêts et autres compteurs existants. Les charges restantes d=0/1/2/3 sont 1/2/5/8 ; exercer leurs budgets exacts et cap−1, les refus au milieu des deux passes et les arènes vides. Aucun changement de normalisation n’est qualifié ici.
