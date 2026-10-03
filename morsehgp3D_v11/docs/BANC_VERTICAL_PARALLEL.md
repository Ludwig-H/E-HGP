# Banc des descentes verticales parallèles

Cette tranche prépare une comparaison CPU, sans mesure ni gain natif acquis.
Le contrat produit est décrit dans [FULL_VERTICAL_PARALLEL.md](FULL_VERTICAL_PARALLEL.md).
Le raccord part de `90dd48bd2` ; il conserve la correction `population().size()`
et le contrôle des sommes de durées en nanosecondes entières. Les campagnes
antérieures restent liées à leurs scripts figés, jamais réinterprétées par ce banc.

## Options et calendrier

La sonde FULL accepte les masques 0 à 255. Le bit128 active les descentes
verticales parallèles et exige le bit8 des lanes. Le défaut0 et les bits
précédents sont inchangés ; le bit128 ne modifie pas les options du catalogue.
Les masques128..255 dépourvus de bit8 sont refusés avant lecture des entrées.

`full_parallel.py --parallel-verticals` choisit exactement20 requêtes K5 :

- trois trames LiDAR entières × u21/u24 × modes127/255, W48 ;
- ng00/u21/mode255 avec W1 puis W8 ;
- uniformes8k/16k/32k u21 × modes127/255, W48.

Cette option est exclusive de `--optimized-catalogue`. Les calendriers
historiques19 et27 restent identiques. Une défaillance n'efface aucun autre
mode. L'omission n'est permise qu'avant lancement pour le budget de campagne ;
une omission ou un échec rend la campagne non conforme. Un processus neuf par
requête ne constitue pas une série statistique de répétitions.

Le plan `bench/plans/full_vertical_g4.json` réserve850s à la matrice,
180s au complément ASan18 et650s au banc (budget propre570s), soit1680s.
Chaque enfant reste borné à60s. Le calendrier planifié ne prouve ni clôture
G4 ni qualification ; source, archive et arrêt gardé doivent être capturés.

## Diagnostics et comparaisons

Le rapport `full_campaign.v8` / `full_parallel_campaign.v3` garde
`full_work.v4` et `full_parallel.v1`, et ajoute `full_vertical_parallel.v1`.
L'événement FULL contient `parallel_verticals`, un booléen exact. Chaque
ordre contient `vertical_parallel` et ses sept entiers u64 : trois compteurs
(lots, résolutions, maximum du lot) et quatre durées (dispatch, somme des
lanes, maximum d'une lane, balayage). Les valeurs sont nulles à k1 et hors
option. Sinon, avec b naissances et Q=4096, les trois compteurs sont
respectivement ceil(b/Q), b et min(b,Q).

Le maximum d'une lane est au plus la somme des lanes et la durée totale des
dispatchs. La somme des lanes est au plus min(W,L,Q) fois cette durée totale,
avec L=48. Dispatch et balayage sont disjoints et leur somme ne dépasse pas
la phase verticale. Toutes ces bornes utilisent les nanosecondes entières ;
les conversions en millisecondes servent seulement à l'affichage. Les temps
cumulés de lanes ne se soustraient pas du temps mur. Les métadonnées et ces
bornes sont validées avant toute réutilisation d'un résumé sémantique.

Les sorties sémantiques et les octets canoniques dans un même profil doivent
être identiques entre modes et nombres de workers. Le travail structurel
reste comparé entre toutes les voies. Le travail effectivement payé de MEB,
census et mémo est comparé seulement à mode de descente constant, défini par
`mode & 143` : changer bit128 peut changer les hits des mémos privés. Les
comptes des lots verticaux sont comparés entre toutes les voies actives ;
les chronos sont exclus des signatures de travail. L'égalité des dates
initiales et la fermeture des verticales relèvent des juges géométriques
natifs ; ces sept mesures ne constituent pas une preuve géométrique.

Les portes préparées utilisent de vrais petits payloads encodés/décodés et
des enfants factices : frontière temporelle exacte, lots traversant Q,
corruptions de compteurs/types/temps, refus avant lancement, réutilisation
avec contrôle du résultat courant, omissions et interruptions. Les snapshots
des campagnes entièrement factices sont copiés par aller-retour JSON ; les
portes de sauvegarde, interruption et nettoyage conservent de vrais fichiers.
La porte IO native couvrira les192 masques autorisés, permutation et W,
ainsi que le refus du bit128 sans bit8. Elle n'a pas été exécutée localement.
