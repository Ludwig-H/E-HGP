# Borne MEB pour un futur dispatch exact

30 septembre 2026. Complément mathématique indépendant sur le commit
`408d1ffe4d90a7ee6393716c6ddc8319b2b77daa`. Le reçu `wide_order_followup/`
déjà clos reste intact. Aucun code produit, build natif, GPU/GCP ou note
active n'est modifié ; neuf petits MEB sont calculés par Fraction.

Si un support S est contenu dans une boîte de largeurs Δx, Δy, Δz, la boule
centrée au milieu de cette boîte, de rayon carré ΣΔ²/4, couvre S. La boule
minimale de S a donc **4β(S) ≤ Δx²+Δy²+Δz²**. Cette preuve s'applique à
la boîte du support et à toute boîte globale contenant ses points.

Il faut transporter la certification MEB. Des sites cosphériques pi et
des poids λi≥0 de somme 1 avec c=Σλipi suffisent : pour tout autre centre z,
Σλi‖pi−z‖²=β+‖c−z‖²≥β, donc une boule couvrant le support ne peut avoir
un rayon inférieur. Le support minimal à poids strictement positifs fournit
cette certification. Une simple équidistance ou un statut arithmétique
`READY` ne la fournit pas. Le contre-exemple exact (0,0),(16,0),(8,1) a
un circumrayon² 4225/4, alors que ΣΔ²/4=257/4 : son centre a un poids
négatif. Sa vraie MEB est le diamètre (0,0)—(16,0), de rayon² 64.

Dans le cube u32, M=2^32−1, la borne est 3M²/4 < 2^64. Elle concerne la
valeur du rayon carré ; les distances entières KNN atteignent 3M² et
nécessitent 66 bits. Pour un seuil entier e, **4e ≥ ΣΔ² ⇒ β≤e** est un
raccourci fermé exact. Sur la boîte globale, il couvre tous les niveaux
dont les supports MEB sont certifiés et appartiennent au même nuage.
Le calcul de 4e nécessite jusqu'à 68 bits : multiplier après promotion
vers u128, jamais en u64. Aucun gain de débit n'est mesuré ici.

Conditions de raccord : mêmes coordonnées de grille, origine, pas physique,
profil et univers d'IDs ; boîte réellement enveloppante ; support/certificat
immuable transporté jusqu'à la requête ; refus conservés si ces conditions
ne sont pas acquises. Le test utilise des rayons carrés en unités de grille.
Pour une sortie physique, multiplier β et le seuil par le même h² exact.
Une boîte issue d'un morceau différent ou une simple petite étendue locale
ne certifie pas les supports d'un catalogue global.

La borne générique du comparateur N<2^266, D<2^200 reste un domaine
arithmétique sûr. Sous **les deux** certificats MEB et D<2^200, N=βD est
même <2^264 ; cela demande toujours cinq mots de numérateur. Cette borne
conditionnelle n'établit pas la borne du dénominateur ni celle des calculs
intermédiaires du constructeur. La réduction PGCD ne justifie pas un
type court universel, et une borne sur la valeur β ne remplace pas sa
représentation rationnelle exacte. Aucune activation du moteur u24/u32
ni qualification de constructeur, tri, FULL ou performance n'en découle.

[check.py](check.py) résout les centres par système de Gram et énumère au
plus quatre sites de support parmi cinq. Il vérifie les supports positifs,
la couverture, les boîtes et le seuil fermé sur q2 diagonale, tétraèdre
régulier et nuage 3D clairsemé, à M=31, 2^24−1 et 2^32−1. Le triangle
obtus exerce séparément la précondition manquante. Normal/−O donnent les
mêmes sorties ; les sources et commandes sont identifiées dans les manifestes.
