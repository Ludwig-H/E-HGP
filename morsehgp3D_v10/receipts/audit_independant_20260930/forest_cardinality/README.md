# Cardinalités de forêt : protection amont et port massif

30 septembre 2026. Source au commit 408d1ffe4 ; aucun moteur modifié, aucune allocation massive, aucun GCP.

**Conclusion après contre-relecture : aucun nouveau dépassement de forêt démontré sur le chemin FULL valide actuel.** L'absence de gardes locales dans kruskal() ne suffit pas : les gardes de l'atlas et des représentants protègent déjà ce constructeur. La première interprétation omettant ces invariants est rejetée, conservée exactement dans preflight_before_atlas_review/ avec sa clôture. Les expériences arithmétiques restent justes ; leur portée est abstraite.

## Preuve à conserver dans le port

Noter b les naissances, j les jonctions traitées, m les fusions effectives et R les racines. Le constructeur n'ajoute un nœud que pour au moins deux racines pré-lot distinctes. Donc N=b+m, E=N−R, m≤min(j,b−R).

- Pour K≥2, site_births=0 ; une cellule d'ordre est birth OU join. Les préfixes d'atlas refusent atlas.cells≥kNone, donc b+j≤atlas.cells<kNone. Il en résulte N≤b+j<kNone et E<N.
- Pour K1, les feuilles sont les sites. Chaque enfant ajouté correspond à une racine pré-lot distincte parmi les représentants consommés dans pre/members. Les groupes sont disjoints et les plateaux utilisent des plages disjointes. Donc E≤nr=sr[1]<kNone. Pour une forêt connectée complète, N=E+1≤kNone ; les IDs 0..N−1 restent distincts de kNone. Plus précisément, une jonction ayant r_t représentants fait diminuer les composantes d’au plus r_t−1 ; donc b−1≤nr−j et b+j≤nr+1≤kNone. Ce raisonnement utilise la connexion finale de K1, garantie par le modèle FULL complet ; le test root_count est tardif. Il ne certifie pas un catalogue forgé/incomplet de plusieurs milliards de sites ni une mémoire disponible.

Les extraits de kruskal() sont joints ; les gardes amont sont à tower.cpp:1321–1336 et les cellules exclusives à 1374–1385. Le code et sa provenance sont retrouvables par le commit et le hash de source du manifest.

**Recommandation limitée :** promouvoir avant calcul les réserves b+j et b+j+1. Pour K1 connecté valide, b+j ne dépasse pas kNone ; b+j+1 peut cependant se réduire à zéro au cas frontière b+j=kNone. Cela sous-réserve un vector qui peut ensuite grandir, sans démontrer une corruption d’ID. Le budget global reste un chantier distinct. Lors du futur découpage en segments ou d'un passage aux IDs 64 bits, transporter explicitement les invariants GLOBAUX d'atlas, de représentants et de forêt : des gardes locales par segment ne les remplacent pas.

Un préflight alternatif sur b+min(j,b−1), calculé en u64 pour b>0, borne N avant allocation. S'il dépasse la limite, il ne prouve pas que N la dépasse effectivement : préférer un comptage des fusions réelles ou annoncer le refus conservateur. Un plafond b≤2^31 serait excessif : une grande fusion en étoile reste représentable.

## Expériences et limites

Le programme C++ reprend uniquement trois expressions arithmétiques, sans exécuter kruskal(). Cent quadruplets virtuels donnent les résultats modulaires attendus en normal/UBSan, quatre lectures Python normal/−O identiques. Aucun avertissement UBSan : sommes et conversions non signées sont définies. L'oracle abstrait vérifie 1 440 forêts et 26 867 relations au total, dont 300 comparaisons natives.

Le peigne abstrait b=2 147 483 649, j=2 147 483 648, N=4 294 967 297, E=4 294 967 296 montre que des comptes b/j isolément représentables ne suffisent pas. Il échoue aux invariants amont actuels : ce n'est pas un contre-exemple FULL admis. Le nombre de boules avant atlas reste à protéger, comme l'indique l'audit massif.

Première erreur de compte rendu conservée dans preflight_reporting_error/ : résultats arithmétiques corrects, métadonnées JSON écrasées par les variables de boucle ; version finale contrôlée séparément. Aucune exécution native de milliards de nœuds ni capacité LiDAR nouvelle revendiquée.

Fichiers : [source](source_excerpts.txt), [manifest](source_manifest.json), [oracle](check.py), [expressions natives](arithmetic.cpp), [exécutions](execution.json), [clôture](SHA256SUMS).
