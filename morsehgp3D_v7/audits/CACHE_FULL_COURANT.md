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

La [qualification lazy historique](receipts_full_lazy_20260905/README.md) conserve ses 109 ordres, 872 sorties et 67 920 coupes par build, budgets et trois mutants propres : J1 supprimé (96 sorties touchées), minima confondus (54 témoins), capacité ignorée (58 dépassements). Les 14+14 CTests, l’admission n=8 du digest et le supplément first-C y sont contre-vérifiés séparément. Les trois mutants ne sont pas transférés à `21b77d29`. Le [contrat principal](../docs/CONTRAT_CACHE_FULL_PARESSEUX.md) porte désormais le calendrier et ses obligations ; aucune demande satisfaite n’est rouverte ici.

## Lot contenant une seule directe : avis statique

L’avis antérieur à l’implémentation repose sur une identité simple : la DSU locale unit toutes les q racines rendues à la première ; son unique classe est donc exactement leur ensemble. Trier puis dédupliquer au plus quatre tokens après toutes les résolutions donne les mêmes parents, même si les composantes recouvrent des points communs. La qualification `21b77d29` ci-dessus vérifie le raccord ; le [contrat constructeur](../docs/CONTRAT_LOT_UNITAIRE_FULL.md) conserve les demandes, leur ordre, le premier token, les naissances simultanées, les no-op et le suffixe de fermeture. Cet argument ne supprime pas toutes les allocations du lot.

## Normalisation : supprimer la dernière paire redondante

Le [contrat actuel](../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) facture déjà 3d+1 opérations pour un appel terminé de profondeur d. **On peut conserver exactement le tableau final avec deux opérations de moins lorsque d>0**, sans allocation supplémentaire : retenir pendant la recherche le dernier nœud avant la racine, puis arrêter la compression avant ce nœud. Son successeur vaut déjà la racine ; le relire et réécrire la même valeur est inutile. Pour d=0, garder la lecture terminale ; pour d=1, aucune écriture ne reste nécessaire. Ne pas supprimer l’incrément de `normalized_anchors` dans ce dernier cas.

Preuve par appel : tous les nœuds antérieurs au dernier reçoivent la même racine ; le dernier et la racine restent identiques. Le tableau entier après succès est donc celui de `13c6`. Par induction, les normalisations suivantes ont les mêmes profondeurs, racines, demandes géométriques et effets first-C, tant qu’aucun refus n’interrompt la comparaison.

En lazy réussi, noter T=`face_visits`, D le nombre de directes, S=`successor_steps`, A=`normalized_anchors` et Z=`no_op_connections`. Chaque demande stricte normalise une fois : minimum, cache ou terminal. Après un miss, `put_alias` ne peut rencontrer une nouvelle clé identique, puisque sa résolution n’a pas modifié le cache. La fermeture normalise ensuite chaque directe une fois. D’où :

$$N=T+D,\qquad H=\frac{S-N}{3},\qquad S_{\mathrm{nouveau}}=S-2A.$$

H est la somme des profondeurs, A le nombre d’appels de profondeur positive. Les clôtures ont profondeur zéro pour les no-op, un sinon : les parents étaient des racines pré-lot et un seul étage de fusion est installé. Leur charge est donc `4D−3Z`, et leur contribution à H et A vaut `D−Z`. Les [48 ordres réussis recalculés](successor_work_review.json) satisfont ces identités, même avec cache saturé. Les moyennes ainsi reconstruites ne donnent ni profondeur maximale ni distribution des chemins.

**Changer 3d+1 en 3d−1 pour d>0 change le contrat de charge.** Versionner explicitement les unités/calendriers et leurs lecteurs ; facturer prospectivement les opérations restantes, jamais soustraire des dépenses après coup. Conserver virtuellement 3d+1 permettrait une optimisation machine mais conserverait l’ancien refus budgétaire. Les plafonds numériques peuvent rester identiques ; leurs admissions ne sont alors plus des comparaisons à charge identique.

Le K9 refusé n’entre dans aucune de ces égalités de succès : une demande peut être inachevée, des directes pas encore fermées et A déjà incrémenté avant un refus de compression. Les données ne permettent pas d’affirmer sa réussite sous 128 millions. Pour le futur delta, comparer l’état complet après d=0/1/2/longue, les appels répétés, puis les forêts et autres compteurs existants. Les charges restantes d=0/1/2/3 sont 1/2/5/8 ; exercer leurs budgets exacts et cap−1, les refus au milieu des deux passes et les arènes vides. Aucun changement de normalisation n’est qualifié ici.
