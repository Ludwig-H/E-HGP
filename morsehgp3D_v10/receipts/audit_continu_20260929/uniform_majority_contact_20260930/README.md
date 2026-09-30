# Majorité uniforme : retrait d’un vote fort au contact

Preuve autonome **Fraction**, K=2, quatre sites d’affinité 3D. Ce reçu ne teste ni le moteur natif, ni EOM, ni la qualité statistique, ni GPU/G4. Il réfute une stabilité géométrique universelle de la majorité uniforme de boules fortes canoniques, même avec une bande figée strictement séparée de son bord. Ce n’est **pas** une scission de boule cosphérique : la boule reste présente et vivante, mais cesse d’être un vote fort.

## Témoin principal

IDs `A=0, B=1, Z=2, Y=3` :

`a=(0,0,0), b=(10,0,0), z=(9−ε,3,0), y=(0,0,9)`.

Le déterminant est 270. Pour ε=0 puis 0<ε≤1/16, η=1/8 et α_a²=81/4. La bande s’arrête à 6561/256. Ses paires géométriques couvrantes sont exactement A:{AB,AZ,AY}, B:{BZ}, Z:{BZ}, Y:{AY}, avant et après perturbation. Le script vérifie une marge strictement positive à **chacun des quatre points**, pour chaque boule de paire couvrante ; aucune entrée/sortie au bord de la bande.

Les six sommets de Γ₂ et leurs dates β=r² sont :

| Paire | β |
|---|---|
| BZ | (10+2ε+ε²)/4 |
| AY | 81/4 |
| AZ | (90−18ε+ε²)/4 |
| AB | 25 |
| ZY | F=(171−18ε+ε²)/4 |
| BY | 181/4 |

Les **quatre** événements FULL sont ABZ à 25, AYZ à F, ABY et BZY à 181/4. Chaque triangle est droit ou obtus ; son MEB est le diamètre de sa plus longue paire. Une seconde construction par systèmes de Gram rationnels et supports à poids strictement positifs recoupe chaque MEB. Toutes les coupes ouvertes/fermées et leurs ensembles de points couverts sont conservés.

AB a centre (5,0,0), β=25, q_min=2. À ε=0, sa coquille est {A,B,Z}, p=0 : p+q_min=2, donc vote fort K2. À ε>0, la puissance de Z est −8ε+ε²<0, p=1 : p+q_min=3>K. AB reste géométrique et vivant (`p<K≤population`) ; **l’événement ABZ doit rester dans Γ₂**, bien que son vote fort soit retiré.

À ε=0, A a trois votes uniformes AY/AZ/AB. À β=25, AZ et AB sont dans la même composante R, donc 2/3>1/2 : A rejoint R. Après perturbation, A n’a que AY/AZ ; R n’en porte que 1/2, donc A attend F. B et Z rejoignent leur composante BZ tôt. La hauteur projetée u(A,B) passe ainsi de 25 à F, avec limite 171/4 lorsque ε→0+. En rayons : 5 devient √171/2. Le nombre total de boules géométriques canoniques reste dix dans chaque cas ; la perte d’incidence forte est la cause.

Sur grille entière u18, S=1024 donne b=(10240,0,0), y=(0,0,9216), z=(9216,3072,0) puis (9215,3072,0). Un seul déplacement d’une unité amplifie le même changement projeté. Il s’agit d’un contrôle mathématique sur ces coordonnées, pas d’un appel au moteur entier.

## Réduire η ne garantit pas la stabilité

Famille : T=M²+1, a=0, b=(2T,0,0), z=(2M²−ε,2M,0), y=(0,0,2T), M≥2.

À ε=0 : α_a²=M²T, β_AB=β_AY=T². Le même schéma exige **les deux** inégalités strictes

`1+1/M² < (1+η)² < 2+1/M²`.

La première conserve AB/AY dans la bande ; la seconde écarte ZY/BY. Une petite perturbation intérieure retire AB, puisque sa puissance devient −2(M²−1)ε+ε²<0. La hauteur passe de T² à T(2M²+1)−M²ε+ε²/4. Cela fournit des contre-exemples pour des η positifs **arbitrairement petits**, notamment tout 0<η<√2−1 avec M suffisamment grand. On ne prétend pas que ce même schéma fonctionne pour tous les η très grands. Le reçu contrôle M3/η1/8 et M32/η1/1000, sans balayage exploratoire.

## Contrôle : univers fixe des K-parties

Pour le contrôle uniquement, les votes de x sont les K-parties F contenant x et dont le MEB appartient à sa bande. Ici les trois votes AB/AZ/AY de A restent présents après perturbation, même lorsque la boule AB devient faible. Son entrée et u(A,B) restent 25. **Ce contrôle ne propose pas d’énumérer C(n,K) en production** et ne fournit aucune borne de coût pour un tel calcul.

Lemme conditionnel utile : si les identités et poids des votes de chaque point restent fixes, et si deux filtrations Γ s’intercalent à décalage δ dans les deux directions en conservant ces identités, admissions et propriétaires, leurs partitions projetées par majorité stricte s’intercalent également à δ. En effet, plus de W_x/2 votes d’un point dans une composante à r sont transportés ensemble à r+δ ; poids et dénominateur ne changent pas. Cette composante est alors son unique composante majoritaire. Le transport commun à un groupe de points donne l’inclusion de partitions ; la seconde application donne la réciproque. Avec une seule application, on n’obtient qu’une inclusion unilatérale.

Pour des K-parties fixes, une perturbation maximale δ de chaque site déplace chaque rayon MEB de ≤δ (transport de la même boule dans les deux directions). Les dates des K- et (K+1)-parties donnent ainsi l’intercalage du Γ complet par leurs IDs. α_x est également 1-Lipschitz. La sélection par bande conserve ses membres si chaque marge en **rayons** satisfait `|r_F−(1+η)α_x|>(2+η)δ`. Dans le témoin de base, une marge conservatrice ≥1/16 rend δ<1/34 suffisant ; les captures vérifient aussi directement ε=1/16. Ces hypothèses ne s’appliquent pas au retrait des votes forts canoniques, ni aux poids 1/β changeants. Elles ne prouvent pas stabilité de la sélection EOM ou des labels aplatis.

## Capture et lecteur

`check.py` contient l’oracle géométrique/Γ indépendant ; aucune dépendance LIVE sur le dépôt. `PROTOCOL.txt`, `check.py`, `record.py` sont hachés avant et après capture. Onze cas, deux mutations à mauvaise valeur (pas binaires) : supprimer l’événement faible ABZ ; fusionner deux votes de même composante en un vote. Les deux donnent une mauvaise hauteur.

Premier transport du résultat de `record.py` tronqué par le budget de sortie : conservé tel quel dans `transport_preflight.txt`, sans prétendre être un reçu complet. Les deux calculs internes avaient code0 ; ce n’est pas un échec mathématique. Capture finale complète : deux nouveaux calculs normal/−O, code0, stderr vide, sorties de 72 727 octets identiques. Donc **quatre** petites exécutions de preuve lors de l’acquisition, puis les seuls rejeux du lecteur publiés à part. Zéro compilation, zéro appel natif/GCP. La capture est dans `receipt.json` et `logs/`.

`verify.py` contrôle d’abord l’inventaire et tous les hashes, puis les liens source/commandes/captures, puis rejoue la preuve en normal et −O. Les chemins historiques sont une provenance, pas une dépendance de relocalisation. Le manifeste exclut uniquement lui-même. Les résultats restent une preuve mathématique petite, pas une qualification FULL, de croissance, de clustering ou du contrat 100 ms.
