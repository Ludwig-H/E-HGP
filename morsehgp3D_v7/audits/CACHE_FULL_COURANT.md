# Cache, lots et normalisation FULL : qualification indépendante

## Normalisation v2 qualifiée : 85c27ab9

**Les nouveaux rejeux indépendants conservent les forêts et les 32 autres compteurs ; seule la charge des successeurs change comme démontré.** Sur le header `85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad`, O2 et ASan/UBSan exécutent chacun 114 ordres, 912 sorties et 69 120 coupes. Les quatre politiques et les deux représentations rationnelles passent. Les 17 808 opérations supprimées sur ce corpus valent exactement deux fois `normalized_anchors` ; cette identité porte uniquement sur les sorties réussies. Les [preuves nouvelles](receipts_full_successor_20260905/README.md) restent distinctes des témoins singleton et lazy.

Le helper réel, compilé sans `MHGP7_TESTING`, passe aussi 3 851 appels par build : tableau entier, racine partielle, charges prospectives et incrément après lecture terminale. Les préfixes incluent 466 refus avant écriture de compression et 700 refus après une compression partielle. Deux mutants causaux sont réfutés : ancienne dernière paire et écriture avant sa charge. Les compteurs synthétiques proches de MAX vérifient une condition nécessaire de charge ; ils ne prouvent pas un historique global atteignable du Builder.

Les 16 plafonds exacts, 180 refus cap−1 et douze conflits d’API passent avec le marqueur effectif `full_successor_reads_writes_no_last_pair_v2`, y compris sur refus. Les caps exacts sont recalculés dans cette unité. Les captures constructeur 20+20 CTests sont contre-vérifiées séparément, avec leurs 49 pannes eager et 209 lazy toutes refusées sans échappement ; elles ne sont pas présentées comme des CTests relancés par l’auditeur. Le [contrat principal](../docs/CONTRAT_NORMALISATION_FULL.md) documente désormais le calendrier et le verrou de l’ancienne sonde.

Aucun défaut trouvé dans ce périmètre relatif aux catalogues fournis complets, exacts et réguliers. Aucune structure géométrique globale supplémentaire ; aucune qualification des temps, de la sonde v3 ou de la réussite du précédent K9/32k refusé. `public_status=not_claimed`.

## Lot unitaire qualifié : 21b77d29

La [qualification singleton historique](receipts_full_singleton_20260905/README.md) conserve ses 114 ordres, 912 sorties et 69 120 coupes par build. Les 872 sorties antérieures sont identiques à `13c6`, compteurs et refus compris. Le supplément rationnel de cinq points exerce une naissance avant fusion au niveau 25 et K=2, absente de l’ancien corpus ; le mutant perdant le quatrième parent est réfuté sur 136 sorties. Les captures constructeur 17+17, avec 49 fautes eager et 209 lazy refusées sans échappement, y restent contre-vérifiées séparément. Le [contrat principal](../docs/CONTRAT_LOT_UNITAIRE_FULL.md) porte les obligations désormais satisfaites ; ses mesures mono ne sont pas transférées à la normalisation v2.

## Témoin lazy antérieur : 13c6cc72

La [qualification lazy historique](receipts_full_lazy_20260905/README.md) conserve ses 109 ordres, 872 sorties et 67 920 coupes par build, budgets et trois mutants propres : J1 supprimé (96 sorties touchées), minima confondus (54 témoins), capacité ignorée (58 dépassements). Les 14+14 CTests, l’admission n=8 du digest et le supplément first-C y sont contre-vérifiés séparément. Les trois mutants ne sont pas transférés à `21b77d29`. Le [contrat principal](../docs/CONTRAT_CACHE_FULL_PARESSEUX.md) porte désormais le calendrier et ses obligations ; aucune demande satisfaite n’est rouverte ici.

## Lot contenant une seule directe : avis statique

L’avis antérieur à l’implémentation repose sur une identité simple : la DSU locale unit toutes les q racines rendues à la première ; son unique classe est donc exactement leur ensemble. Trier puis dédupliquer au plus quatre tokens après toutes les résolutions donne les mêmes parents, même si les composantes recouvrent des points communs. La qualification `21b77d29` ci-dessus vérifie le raccord ; le [contrat constructeur](../docs/CONTRAT_LOT_UNITAIRE_FULL.md) conserve les demandes, leur ordre, le premier token, les naissances simultanées, les no-op et le suffixe de fermeture. Cet argument ne supprime pas toutes les allocations du lot.

## Normalisation : supprimer la dernière paire redondante

Le calendrier historique v1 facture 3d+1 opérations pour un appel terminé de profondeur d. **Le calendrier v2 conserve exactement le tableau final avec deux opérations de moins lorsque d>0**, sans allocation supplémentaire : retenir pendant la recherche le dernier nœud avant la racine, puis arrêter la compression avant ce nœud. Son successeur vaut déjà la racine ; le relire et réécrire la même valeur est inutile. Pour d=0, garder la lecture terminale ; pour d=1, aucune écriture ne reste nécessaire. L’incrément de `normalized_anchors` reste obligatoire dans ce dernier cas.

Preuve par appel : tous les nœuds antérieurs au dernier reçoivent la même racine ; le dernier et la racine restent identiques. Le tableau entier après succès est donc celui de `13c6`. Par induction, les normalisations suivantes ont les mêmes profondeurs, racines, demandes géométriques et effets first-C, tant qu’aucun refus n’interrompt la comparaison.

En lazy réussi, noter T=`face_visits`, D le nombre de directes, S=`successor_steps`, A=`normalized_anchors` et Z=`no_op_connections`. Chaque demande stricte normalise une fois : minimum, cache ou terminal. Après un miss, `put_alias` ne peut rencontrer une nouvelle clé identique, puisque sa résolution n’a pas modifié le cache. La fermeture normalise ensuite chaque directe une fois. D’où :

$$N=T+D,\qquad H=\frac{S-N}{3},\qquad S_{\mathrm{nouveau}}=S-2A.$$

H est la somme des profondeurs, A le nombre d’appels de profondeur positive. Les clôtures ont profondeur zéro pour les no-op, un sinon : les parents étaient des racines pré-lot et un seul étage de fusion est installé. Leur charge est donc `4D−3Z`, et leur contribution à H et A vaut `D−Z`. Les [48 ordres réussis recalculés](successor_work_review.json) satisfont ces identités, même avec cache saturé. Les moyennes ainsi reconstruites ne donnent ni profondeur maximale ni distribution des chemins.

**Changer 3d+1 en 3d−1 pour d>0 change le contrat de charge.** Versionner explicitement les unités/calendriers et leurs lecteurs ; facturer prospectivement les opérations restantes, jamais soustraire des dépenses après coup. Conserver virtuellement 3d+1 permettrait une optimisation machine mais conserverait l’ancien refus budgétaire. Les plafonds numériques peuvent rester identiques ; leurs admissions ne sont alors plus des comparaisons à charge identique.

Le K9 refusé n’entre dans aucune de ces égalités de succès : une demande peut être inachevée, des directes pas encore fermées et A déjà incrémenté avant un refus de compression. Les données anciennes ne permettent pas d’affirmer sa réussite sous 128 millions. Les charges d=0/1/2/3 deviennent 1/2/5/8 ; la qualification v2 ci-dessus ferme les tests de préfixes et de forêts demandés par cette preuve. Les latences restent une question expérimentale distincte.
