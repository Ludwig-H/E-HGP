# Contre-épreuves exactes Q1/Q2 — 6 octobre 2026

Question relue au pin `fd85f3bb5` ; le pin complet et les empreintes des deux
documents de contexte sont dans `sources.json`. Source développeur intacte.

`replay.py` est un oracle indépendant en Python standard/Fraction. Il dérive
les domaines de Voronoï d'ordre k par toutes les inégalités de distance, puis
leurs sommets/arêtes duaux et les dates de deux filtrations : maximum des
distances carrées (HGP) et moyenne des distances carrées (DTM). Aucun binaire,
constructeur du produit, SciPy, jeu LiDAR ou appel GCP n'est utilisé.

## Deux objets et une erreur d'attribution

- `P={0,1,10}`, k=3, rayon r=5. Le seul sommet de la mosaïque est 11/3,
  actif au niveau carré 25 ; la multi-couverture sur l'axe est {5}, tandis que
  d3(11/3)=19/3>5. En R3, Omega3(5) reste ce seul point. Sa dilation par B5
  est une boule de rayon 5 autour de (5,0,0), distincte de ce sommet et de
  la boule DTM3 de centre (11/3,0,0), rayon carré 43/9. Ce cas est
  mathématique à k=n : aucune admission par l'API du produit n'est déduite.
- `P={0,1,2,11}`, k=3, rayon r=21/4. Les domaines pleins dérivés portent
  T={0,1,2} et Q={1,2,11}, barycentres 1 et 14/3. Leurs cellules Voronoï
  ont leur frontière à 11/2. La date HGP de l'arête est 121/4 (carré), celle
  DTM est 251/12 (carré). Au niveau carré 441/16, HGP a deux composantes,
  DTM une seule.
  Les intersections de la multi-couverture avec l'axe sont
  [-13/4,21/4] et [23/4,25/4]. Les deux composantes en R3 sont séparées
  également : leur projection sur cet axe est disjointe, et chacune est
  une intersection non vide de trois boules convexes.
  Le sommet 14/3 appartient strictement à la composante T, alors que son
  domaine Voronoï tronqué C_Q(r) appartient à l'autre composante.
  **Attribuer une cellule par la position de son barycentre donne donc le
  mauvais propriétaire.** L'identité correcte vient des domaines tronqués
  et de leurs incidences. Aucun défaut du moteur natif n'est affirmé.

## Contrôles géométriques bornés

Quatre familles de témoins (les deux cas ci-dessus, tétraèdre régulier/K2,
carré dégénéré/K2) vérifient sur des mélanges convexes rationnels :

1. les coefficients de sélection 0<=mu<=1 et somme mu=k ;
2. l'identité de variance exacte et l'inclusion dans le sous-niveau DTM ;
3. la distance au témoin <=r et le contrôle d_k<=2r ;
4. la distance d'un témoin de couverture à un barycentre actif <=r.

Ce sont des vérifications de formules sur des cas bornés, pas une preuve
calculatoire de toutes les inclusions, un constructeur 3D de mosaïque ou un
oracle d'homotopie. Les preuves générales et la réserve sur les dégénérescences
figurent dans la réponse mathématique de l'auditeur. Le script ne revendique
ni précision métrique inférieure à r, ni égalité des trois objets.

## Rejeu

Depuis ce dossier, les deux commandes produisent exactement le même JSON,
sans champ temporel :

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Résultats conservés : `proof.json`, `proof.optimized.json`, tous deux
`conforme`, **265 contrôles**. La première exécution de chaque mode a réussi ;
aucun essai échoué n'a été rencontré dans cette capsule. Les interpréteurs ne
lisent ni ne modifient les sources développeur ou les notes actives.
