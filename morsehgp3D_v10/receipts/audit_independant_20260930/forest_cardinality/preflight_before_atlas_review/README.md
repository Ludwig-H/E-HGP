# Cardinalités de forêt : complément au catalogue massif

30 septembre 2026. Source lue au commit 408d1ffe4 ; mêmes expressions que 4b7d70422. Aucun moteur modifié, aucune allocation massive, aucun GCP.

**Constat utile au port massif : un catalogue représentable ne garantit pas que la forêt et son CSR le soient.** Dans kruskal(), le compte des naissances b et celui des jonctions j sont chacun u32, mais la réserve b+j est calculée en u32. Les nouveaux identifiants de nœud et fins CSR sont convertis depuis size() sans garde. Le correctif RankSearch et la garde catalogue proposée dans une copie R2 ne couvrent pas ce chemin. Les extraits exacts et l'empreinte de tower.cpp sont joints.

## Borne exploitable

Une forêt sans nœud interne unaire, avec b feuilles de naissance, m fusions et R racines, possède N=b+m nœuds et E=N−R liens. Chaque fusion fait diminuer le nombre de composantes d'au moins un ; donc m≤b−R et m≤j. Pour b>0 et une racine finale :

- N≤b+min(j,b−1)≤2b−1 ;
- E=N−1≤b+min(j,b−1)−1.

Ces relations correspondent au constructeur : un nœud n'est ajouté que si au moins deux racines pré-lot distinctes fusionnent. Les jonctions inertes et les jonctions simultanées n'ajoutent pas chacune un nœud. Il ne faut donc pas confondre réserve b+j et nombre réel N.

Contre-dimensionnement virtuel : b=2 147 483 649 et j=2 147 483 648 tiennent chacun sous kNone. Un peigne abstrait connecté donne N=4 294 967 297 et E=4 294 967 296. Les conversions u32 perdent l'information ; l'indice kNone est déjà impropre à un nœud valide. À l'inverse, une seule fusion en étoile donne N=b+1 et peut encore tenir : un plafond arbitraire b≤2^31 serait excessif.

**Correction proposée :** promouvoir les sommes de réservation avant le calcul ; contrôler chaque identifiant et fin CSR avant publication. Un préflight conservateur en u64 sur b+min(j,b−1), avec la convention existante de comptes strictement inférieurs à kNone, permet un refus avant allocation. Quand cette borne échoue, elle ne prouve pas un dépassement réel : choisir un refus conservateur déclaré, ou compter les fusions effectives avant allocation. Les offsets peuvent suivre un contrat distinct des identifiants réservant une sentinelle ; le déclarer au lieu de les traiter implicitement comme des IDs.

## Contre-épreuve et portée

Le petit programme C++ reprend uniquement les expressions arithmétiques concernées. Cent quadruplets virtuels donnent les résultats modulaires attendus en normal et UBSan ; quatre lectures Python normal/−O sont identiques. Aucun avertissement UBSan : les conversions et sommes non signées sont définies. L'oracle abstrait contrôle 1 440 forêts et 26 867 relations au total avec les 300 comparaisons natives.

Ce reçu **ne construit pas une forêt FULL géométrique de plusieurs milliards de nœuds** et ne démontre pas une panne sur les trames publiées. Il montre l'absence de garde par lecture du vrai constructeur et confirme le comportement de ses expressions. La première capture avait des résultats arithmétiques corrects mais un champ JSON de compte rendu écrasé par les variables de boucle ; elle est conservée dans preflight_reporting_error/ et rejetée. Le compte rendu final est vérifié séparément.

Fichiers : [source](source_excerpts.txt), [manifest](source_manifest.json), [oracle](check.py), [expressions natives](arithmetic.cpp), [exécutions](execution.json), [clôture](SHA256SUMS).
