# LiDAR massif : résidence et frontière du port — 2 octobre 2026

Source publiée lue : **3816db61ca9d001e82d5e48e9487e55116eddc12**. Les 29 fichiers src/cli figés sont identiques à `4b7d70422` ; aucune évolution du moteur depuis ce port de primitives isolées. Le moteur conserve sa garde **u18**. Les résultats historiques G4 et les prototypes Python de projection ne qualifient ni FULL u24/u32 ni 10–50 M sites. Ce reçu n'exécute aucune campagne, aucune allocation massive, aucun GCP/GPU ; il ne modifie ni produit ni notes actives.

## Nouvelle preuve bornée : nearest et mémoire conservée

La [sonde native](nearest_probe.cpp) compile le **vrai SiteTree figé**, ses prédicats et Buffer. Le nuage contient 144 sites distincts entiers u18 sur la sphère de centre (64,64,64), rayon²=125. Deux sites antipodaux donnent une vraie MEB diamétrale de ce centre : aucune requête à centre hors hull n'est nécessaire.

`nearest(count=1)` collecte les 144 ex æquo avant tri exact et limitation à une sortie. Le candidat TLS (éléments de 16 octets) et le vecteur de sortie (éléments de 32 octets) atteignent chacun une capacité 256. Une requête suivante au centre d'un site ne collecte que 2 candidats et rend 1 sortie, mais conserve les deux capacités. Voir [nearest.stdout.json](nearest.stdout.json), stderr vide. **Ce seul exemple prouve que scratch n'est pas borné par K ; il ne prouve ni un nuage 10 M cosphérique atteignable, ni une panne mémoire, ni un coût LiDAR.**

Causes : [site_tree.cpp](sources/morsehgp3D_v10/src/cloud/site_tree.cpp#L105) : `thread_local QueryScratch`; lignes 140–177 : clear sans libération, collecte avant seuil final ; lignes 179–180 : tri des candidats exacts puis resize. La capacité TLS reste jusqu'à destruction du worker ou libération explicite ; les sorties restent à la capacité atteinte tant que leur objet Scratch est conservé. Dimensionner la **somme des capacités simultanées des workers**, pas W fois un maximum mesuré sur un autre processus.

Le code core partage déjà **une requête kmax par site** entre tous les K ([tower.cpp](sources/morsehgp3D_v10/src/tower/tower.cpp#L1600)) ; il reste une résolution, un rang et une remontée par (site,K). `cover` balaie les boules/incidences séparément par ordre et résout jusqu'à n premières boules, ou toutes les boules couvrantes si ball_nodes. Ne pas compter nK requêtes K-NN dans la voie core actuelle ni supposer une visite K-NN logarithmique bornée par K dans toutes les données.

## États qui coexistent effectivement

Octets logiques, hors capacités des vecteurs, métadonnées allocateur, pages et travaux non listés. N=retours, n=sites, B=boules, P=occurrences I/U, L=niveaux catalogue. Q/E=**nœuds/arêtes d'un ordre choisi**, Rmax=plus grand rang catalogue lu par ce dendrogramme, Ld=dates double publiées, Chead=clusters condensés.

| Phase | Charges additionnelles simultanées |
| --- | --- |
| Catalogue, tri PSRS | 168B+4P ; second Ref libéré avant assemblage |
| Catalogue, assemblage | 179B+8P+56L+8 ; socle ≈160n si N=n, listes de tâches/capacités en plus |
| FULL, attaches core | 12nK, au-dessus des forêts/atlas/index/runs encore vivants |
| FULL, attaches cover extra0 | (16K−4)n ; les K−1 tableaux point_level nuls représentent 8n(K−1) évitables si l'API distingue les deux entrées |
| FULL cover, ball_nodes demandé | 4B(K−1), pour K≥2 ; en CLI cluster `only_order`, seulement 4B pour l'ordre courant |
| Construction PointDendrogram | 28Q+4E+20n+4(Rmax+2)+8Ld+4, **en plus du catalogue et de Tower encore vivants** |
| Condensation près du retour | tableaux connus : 16Q+16n+28Chead+4, en plus du dendrogramme/forêt/catalogue ; work/stack/big/capacités à ajouter |
| Vote CLI, inversion des incidences | 8(n+1)+4Pv conservés, Pv≤P ; 8n supplémentaires pendant fillc, puis libérés avant condensation |

Pour PointDendrogram ([tower.cpp](sources/morsehgp3D_v10/src/tower/tower.cpp#L1775)), A/B, merged, start, cnt, fill et le résultat sont encore dans le même scope au retour. Le résultat recopie parents, CSR et attaches. Avec une racine E=Q−1, le pic logique devient ≈32Q+20n+4Rmax+8Ld ; rmax est un rang **global du catalogue**, pas le nombre de niveaux utilisés par l'ordre. À K10/n50 M, les seules attaches core/cover valent 6/7,8 Go, avant ces copies.

Le [vote CLI](sources/morsehgp3D_v10/cli/mhgp10_cluster.cpp#L155) construit les ordres successivement ; leurs tours/ball_nodes ne s'additionnent pas dans cette boucle. L'API FULL construit et conserve tous les ordres. La tête et l'export de labels (4N par fichier) ajoutent leurs propres allocations ; le nombre de fichiers dépend des K/configurations/entrées. Le mode de sortie doit faire partie du contrat capacité, avec débit et espace disque. Les listes de clusters `vector<vector<u32>>` font déjà 24 octets par cluster pour leurs seuls en-têtes sur cet ABI.

**Répétitions, à distinguer des chronos historiques :** [mhgp10_tower.cpp](sources/morsehgp3D_v10/cli/mhgp10_tower.cpp#L89) affecte `built = build_catalogue(...)`, puis `tw = build_tower(...)`. Après la première passe, l'ancienne valeur reste vivante pendant l'évaluation du membre droit. Pic catalogue répété = charge de la nouvelle phase catalogue **+ catalogue précédent + tour précédente rendue** ; pic tour répété = nouveau catalogue + états de nouvelle construction tour **+ tour précédente**. L'ancien catalogue est libéré entre les deux appels. Cela conserve **une** passe précédente, pas toutes les répétitions ; HWM garde aussi le maximum passé. Aucun nouveau chrono ou correction des mesures historiques n'est déduit ici.

## Scénarios analytiques, aucune prédiction LiDAR

Hypothèses **P=8B, L=B/4, N=n, K10**, sans preuve de ratios/atlas/forêt atteignables. Grandeurs en Go=10^9 octets. Tableau complet, également en Gio, dans [normal.stdout.json](normal.stdout.json).

| Sites | Boules/site supposées | B | Catalogue persistant | Assemblage + socle actuel | Candidate large 5/4, mêmes comptes |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 M | 20 | 200 M | 17,6 | 53,0 | 61,0 |
| 30 M | 20 | 600 M | 52,8 | 159,0 | 183,0 |
| 50 M | 20 | 1 Md | 88,0 | 265,0 | 305,0 |
| 10 M | 100 | 1 Md | 88,0 | 258,6 | 298,6 |
| 30 M | 100 | 3 Md | 264,0 | 775,8 | 895,8 |
| 50 M | 100 | 5 Md | 440,0 | 1293,0 | 1493,0 |

La dernière ligne dépasse déjà l'espace BallIdx u32 ; les autres ne prouvent **pas** que leurs cellules atlas/forêts sont représentables. Les coefficients du catalogue sont réutilisables car leurs sources sont inchangées ; l'unique différence contre le premier manifeste du 30 septembre est le correctif de recherche de rang dans tower.cpp.

La [sonde de layouts](layout_probe.cpp) mesure Level actuel=56 octets ; modèles Wide<4>/Wide<3>=72 et Wide<5>/Wide<4>=88 (signe et padding compris). Le modèle des champs Rec fait respectivement 104/120/136 octets. Choisir le layout 5/4 porterait les formules de tri à 200B+4P et d'assemblage à **211B+8P+88L**, avant autres évolutions. Ce sont des **modèles de stockage candidats**, aucune borne de constructeur u24/u32 ni performance acquise. Élargir aussi les point_level inutilisés de cover augmenterait encore inutilement la résidence.

## Décisions utiles avant architecture v11

1. Fixer le livrable massif : n/N, K servi, FULL durable ou interface d'accès équivalente, projection/tête/retours, latence, enveloppes RAM/disque et comportement de refus/reprise. Réserver par phase **tous** les états et sorties simultanés ; le budget Buffer actuel ne couvre pas les grands vector et reste illimité par défaut.
2. Poser un format/manifeste commun h exact, origine/repère, profil certifié u24 puis u32, hashes, IDs de retours et carte retour→site. Coordonnée u32 et indice/offset global sont deux décisions séparées : 10–50 M sites peuvent garder des rangs locaux u32, mais B/P/atlas/external IDs/spool nécessitent des domaines explicites et des spans contrôlés avant cast. Pas de prétendue corruption géométrique globale ExtCell ni de réouverture de la borne forêt protégée.
3. Garder l'index global et la certification des boîtes de centres ; produire des segments à émission unique et I/U complets, puis fusion externe **exacte** des clés/niveaux et clôture globale des plateaux. Une segmentation du nuage ou un halo fixe ne conserve pas à lui seul la tour. La capacité disque/IO et l'accès aux unions/descentes, atlas et verticales demeurent ouverts après le spool catalogue.
4. Conserver le rang exact jusqu'aux sorties de composantes et attaches ; une vue double ne représente pas tous les plateaux. Morton96/distances u128, filtre relatif et comparateur 466 bits restent isolés. Prévoir K-NN 66 bits et départage stable, supports positifs, replis exacts certifiés, unités h² pour β. Ne pas enlever la garde u18 devant des consommateurs courts.
5. Séparer payload de frontière (incidences/cohortes/poids), projection et labels. FULL refuse encore les multiplicités : quantification/déduplication de plusieurs scans doit annoncer le modèle de masse. Optimiser les vues/stockages par entrée et les capacités TLS avec un budget réel ; ne pas attribuer au natif le coût ou la maturité d'un prototype Python.

35 sources/docs capturés : leurs copies initiales sont conservées ; les **29 dépendances produit/CLI sont inchangées** avant/après. Le root a volontairement actualisé AUDIT_MASSIF pendant la clôture : son ancien texte et les deux hashes sont conservés, sans forcer une égalité documentaire. La première clôture a refusé ce changement (`source changed`, puis `source mismatch`) ; [SOURCE_AFTER_COORDINATION_CHANGE.json](SOURCE_AFTER_COORDINATION_CHANGE.json) conserve ce ledger avant séparation explicite entre source compilée et note de coordination. Aucun programme ne compile ou ne juge la note modifiée.

Lecteur scalaire normal/−O identique, stderr vides. Première compilation refusée car le snapshot omitait reasons.def : erreur de préparation conservée, puis dépendance ajoutée ; aucune correction produit. Native normal uniquement, pas de nouvelle porte UBSan/TSan. [COMMANDS.txt](COMMANDS.txt), [dependencies.mk](dependencies.mk), [SOURCE_AFTER.json](SOURCE_AFTER.json) et [SHA256SUMS](SHA256SUMS) ferment la preuve locale.
