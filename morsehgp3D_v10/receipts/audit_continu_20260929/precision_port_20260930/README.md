# Audit du port au-delà de u18

30 septembre 2026. Preuves nouvelles, fermées séparément des premières
fondations u32. Le moteur refuse toujours les entrées hors de son contrat u18.
Ces sondes éprouvent volontairement ses corps sur les domaines futurs :
elles interdisent de relever seulement un plafond, sans démontrer une erreur
sur le moteur u18 protégé. `public_status=not_claimed`, hors registre.

## Ce qui a été réellement exécuté

| Paquet | Exécution et conclusion | Limite |
| --- | --- | --- |
| [Géométrie](geometry/FINDINGS.json) | 24 appels C++ normal/UBSan ; 20 paires de juges Fraction normal/−O, dont huit désaccords numériques | Corps géométriques inchangés et copie exacte de la fonction propriétaire, pas générateur ni FULL |
| [Filtres scalaires](filters_scalar/AUDIT.md) | 614 contrôles Python/Fraction par passage normal/−O ; contacts perdus, faux intérieurs et départage exact non préservé | Reconstruction scalaire fidèle, pas une compilation SiteTree ; un préflight erroné est conservé et rejeté |
| [Filtres natifs](filters_native/FINDINGS.json) | Trois fixtures, neuf requêtes au total dont la paire finale normal/UBSan ; quatre juges Fraction normal/−O à 72 contrôles | Vrai SiteTree copié, centres réduits et trois sites construits directement hors factory u18 ; code0 atteste les erreurs attendues, pas l'exactitude du moteur |
| [Interfaces](interfaces/report.json) | Lecture et clôture de 28 empreintes ; préparateur décimal exact déjà disponible, consommateur v10 encore absent | Aucun nouveau processus moteur, compilation ou campagne |
| [Grille et niveaux MEB](grid_metric/README.md) | 117 arrondis exacts, 183 couples de rayons, trois petits nuages 3D et trois pas ; normal/−O | Univers des retours étiquetés fixé ; ni FULL pondéré, ni stabilité EOM/ARI |

La [relecture mathématique indépendante](grid_metric_review/findings.json)
rejoue le même petit panneau, sans l'élargir, et confirme les limites du lemme.
Les contrôles natifs et scalaires restent distincts.
Les fichiers des sous-paquets et leurs manifestes sont conservés octet pour
octet. Aucun nuage LiDAR, binaire natif ou identifiant sensible n'est embarqué.
Les chemins `/tmp` et les commandes des JSON restent ceux des exécutions
réelles ; ils ne sont pas renommés pour paraître actuels.

## Obstacles démontrés

Le tétraèdre régulier u24 construit avec M=2^24−1 a un centre strictement
intérieur et un rayon carré exact `844424829468675/4`. L'ancien `level4`
renvoie zéro en normal **et** sous UBSan sans diagnostic : un redimensionnement
entier refusé est ignoré et le dénominateur est tronqué. Déjà u21 échoue
sur le dénominateur de cette famille. Un code0 de sonde n'est donc pas un
jugement d'exactitude.

Les tests q3 u24 démontrent aussi des débordements du côté de sphère et
du propriétaire demi-ouvert T6. Les arrêts UBSan sont conservés ; les
observations après overflow signé dépendent du compilateur, contrairement
au rayon q4 nul silencieux. À u32, `dot` peut déborder en i64 avant toute
promotion. Les cas positifs q2/côté et orientation de sites ne qualifient
que leurs sous-chemins, pas le moteur complet.

La marge absolue `0.02` du SiteTree n'est pas transportable. Les contre-cas
à centres réduits isolent le filtre des débordements : un vrai contact peut
être écarté ou classé intérieur, même pour une petite géométrie translatée
près de 4e9. La correction proposée est un centre relatif, des intervalles
arrondis vers l'extérieur et un repli exact sur toute ambiguïté. Pour nearest,
le contrôle de départage démontre une mauvaise sélection d'ID à distance
égale, **pas** une mauvaise valeur du K-ième rayon.

Un PGCD positif peut économiser des bits, mais deux tétraèdres stricts u24
gardent après réduction des niveaux de 194/146 bits. Il ne répare aucun
produit déjà tronqué. Les voies courtes doivent être gardées sous des
bornes certifiées par opération, pas sous une seule largeur de support.

## Précision physique et conservation des retours

u18/u24/u32 borne une plage entière ; le pas h fixe la précision physique.
La quantification doit repartir des mots float32 originaux, pas d'une grille
ancienne. Le préparateur v8 possède déjà le pas rationnel, la translation
commune, les fusions et les correspondances de retours : le chantier manquant
est son raccord contrôlé aux types et au domaine du moteur v10.

À univers étiqueté fixé, déplacer chaque point d'au plus ε change tous les
rayons MEB d'au plus ε. Pour la grille, ε≤√3·h/2. Les graphes intrinsèques
Γ_K en rayon s'incluent réciproquement après ce décalage ; les composantes
sont ainsi entrelacées. Les attaches dures, la sélection EOM et l'ARI ne
sont pas couverts par ce lemme. Fusionner des retours puis ignorer leurs
multiplicités change aussi la population de l'estimateur K-NN.
Le décalage ε porte sur le rayon r, pas directement sur le niveau r² :
ce dernier est borné par `(r+ε)²`. Le lemme ne compare pas automatiquement
des secteurs capteur recalculés qui n'ont plus les mêmes IDs.

Les petits tests exercent des fusions, des changements de coupe capteur et
des métadonnées d'origine hors plage u32. L'origine signée n'est pas une
coordonnée locale stockée ; ces derniers cas ne sont pas des dépassements
du fichier de sites. L'énumération des sous-ensembles sert uniquement à cet
oracle borné, pas à un algorithme proposé pour les grandes trames.

## Vérification et suite

`verify.py` relit les manifestes et les observations closes, exige les champs
natifs attendus par mode, puis rejuge les sorties géométriques avec Fraction.
Il n'exécute ni binaire moteur, ni GCP. Son schéma strict compense localement
la réserve du premier oracle, qui juge les champs présents sans refuser
toutes leurs omissions : cet oracle et ses captures ne sont pas réécrits.

Ordre recommandé : raccord du manifeste sans perte d'ID ; noyaux relatifs
et intervalles certifiés ; centres/côtés/orientations/niveaux larges avec
comparateurs exacts ; port commun au catalogue, census, tour et exports.
Mesurer ensuite le taux de replis et la chaîne totale. Les bornes locales
ne démontrent aucune sous-quadraticité globale ni un contrat 100 ms.

GCP non utilisé : ces verrous se reproduisent sur deux à quatre points.
Les preuves restent sous `receipts/` ; la vue courante reste
[l'index des audits](../../../audits/AUDIT_ETAT_COURANT.md) et le
[suivi du développement](../../../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md).
