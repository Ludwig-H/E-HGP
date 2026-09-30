# Moments de groupe : boîtes réelles et témoin cubique

30 septembre 2026. Audit mathématique, moteur inchangé, GCP non utilisé.
Complément séparé au [premier certificat](../group_moments_20260930/README.md) ;
sa clôture reste intacte. Aucun gain LiDAR ou contrat FULL acquis.

Trois résultats précisent le prochain essai du développeur :

- Le générateur recadre chaque boîte par l'enveloppe des candidats **avant**
  la subdivision. La coupe suivant le plus grand axe ne garantit donc pas
  des boîtes presque cubiques. Deux sites verticaux donnent une première
  feuille de largeurs T6 `(1,1,16777153)` dans le miroir exact des règles.
- Le certificat fonctionne aussi dans une boîte cubique. Sur seize sites
  u18 distincts, un vrai tétraèdre à centre strictement intérieur possède
  douze intérieurs et quatre masques de dominance individuelle vides.
  Les moments certifient trois intérieurs pour chacune des quatre ancres :
  le seuil q4/K5 vaut deux. Coordonnées et hypothèses : [protocole](PROTOCOL.txt).
- Si l'ancre a appartient à la boîte **fermée** Q, le certificat ne peut
  rien rejeter : à c=a, chaque puissance vaut `||z-a||² ≥ 0`, donc leur
  somme positive ou nulle impose `σ ≥ 0`. Tester cette appartenance avant
  les moments évite un calcul sans utilité.

Cette dernière propriété interdit aussi de convertir nos petits témoins
en gains du moteur. Avec n≤M16, tous les sites restent dans la première
liste : dans la racine, c=x empêche tout autre site de dominer x. Le
recadrage contient donc toutes les ancres et les moments y donnent zéro
crédit. La petite boîte choisie pour le témoin cubique n'est pas cette
première feuille. Même limite pour les anciens quatorze sites.

La nouvelle boîte cubique est un domaine géométrique valide, **pas une
feuille atteinte attestée**. Les règles de racine/recadrage sont recopiées
arithmétiquement depuis `generator.cpp`, empreinte publiée dans le reçu ;
aucun appel au générateur n'a été fait. Les 503 contrôles Fraction,
normal/−O identiques, complètent les preuves affine/convexe : les centres
échantillonnés ne prouvent pas seuls une propriété universelle.

Prochain test utile : relever S et sa liste réellement atteintes, puis
préparer un petit nombre fixe de groupes par feuille. Tester seulement
les ancres hors S fermé, avant les boucles de supports ; conserver le
nuage et le census complets. Ne pas additionner les crédits anonymes à
Dom sans disjonction démontrée. Publier fraction d'ancres éligibles,
rejets nouveaux, préparation et travail résiduel **total**. Pas de choix
de groupe par tuple ni nouvelle recherche quadratique déplacée.

`python3 -B verify.py` et `python3 -B -O verify.py` contrôlent les empreintes
avant de rejuger la géométrie, sans processus natif. La clôture privée
initiale est conservée dans [CAPTURE_SHA256SUMS](CAPTURE_SHA256SUMS) ;
[receipt.json](receipt.json) fixe les deux lectures et leurs limites.
