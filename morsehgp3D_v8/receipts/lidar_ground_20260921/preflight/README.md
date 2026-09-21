# Douze préflights d'intégration conservés

[initial_12](initial_12/) archive six entrées synthétiques, douze reçus
de commandes (six cas sur les builds Release et Sanitize), dix masques
produits et le [générateur original](initial_12/preflight.py).
Les contenus sont copiés sans modification ; les commandes conservent
leurs chemins temporaires originaux. Les binaires ne sont pas copiés :
leurs hashes sont consignés dans les reçus et les
[capsules de construction](../builds/README.md).

Les cas observés sont :

- entrée vide : masque vide, aucun appel à la bibliothèque ;
- trois retours dont deux hors rayon : masque 2,0,0 ; réflectance NaN
  ignorée conformément au contrat ;
- quatre retours tous conservés inconnus : hors rayon, hors domaine
  numérique ou sentinelle z minimale normale ; aucun appel bibliothèque ;
- XYZ non fini : refus avec code1 avant création du masque, dans les
  deux builds ;
- douze points répétés et distance de graine 1e-300 : douze non-sol ;
- plan incliné avec petit objet : 425 points classés sol et25 non-sol.

Les dix appels valides terminent avec code0. Les deux refus attendus
terminent avec code1. Aucun diagnostic de sanitizer n'a été observé.
Ces douze essais sont des préflights d'intégration, pas une campagne de
vérité terrain ni une qualification de performance. Les six cas ne sont
pas des trames SemanticKITTI et ne prouvent pas la qualité de segmentation
sur celles-ci. Leur générateur n'est pas le runner de qualification fermé
et aucun statut de campagne autonome ne lui est transféré.

Le test de distance de graine très petite ne démontre pas une lecture
non initialisée dans cette configuration : RVPF reste activé, avec son
seuil initial fixe0,25 avant le raffinement qui utilise cette distance.
Cette observation ne justifie aucune option future désactivant RVPF.

[BUILD_CAPSULES_EXPORT.json](../BUILD_CAPSULES_EXPORT.json) lie les hashes
des sources et sorties avant/après export ; il certifie seulement
l'intégrité des copies. Aucun résultat FULL/GPU ni contrat de tour n'en
découle.
