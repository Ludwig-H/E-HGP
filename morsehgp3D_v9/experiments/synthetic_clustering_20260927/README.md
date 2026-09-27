# Synthétiques 3D : taille, difficulté et nombre de groupes

27 septembre2026. Campagne CPU locale ; moteur v9 et consommateurs précédents
inchangés. Le [plan fixé avant les calculs](PLAN.md) organise17scénarios
sur deux nouvelles graines, soit34scènes et612sélections. Les coordonnées,
labels, paramètres et graines sont conservés, sans sous-échantillonnage.

## Résultat de cette capture

[Tableaux complets](../../receipts/synthetic_clustering_20260927/r1/README.md),
[qualification](QUALIFICATION.md), [contrelecture](POST_AUDIT.md).
Les34exports natifs,68fitsHDBSCAN et612sélections ont terminé sans échec.
Au profil principal K5/m20/z1, moyenne sur les34scènes : ARI0,5574 pour
le routage ponctuel HGP contre0,4198 pour HDBSCAN commun ; F1 apparié
0,6506 contre0,5415. Pour chacun de ces deux critères,21gains,4égalités,
9pertes : amélioration sur ce lot, **pas** domination systématique.

Les deux graines restent parfois très différentes : n400/G8/δ4 donne
ARI HGP0,1672 puis0,8492. Les anneaux δ3 bruités favorisent HDBSCAN dans
les deux tirages. Le fort recouvrement gaussienδ2 reste mal résolu par les
deux méthodes. Le routage n'est donc pas un remplacement aveugle des autres
voies ; catalogue et sélection doivent encore être étudiés séparément.

La croissance400/800/1600 est publiée par graine et par exposant. Les
comptes de facettes, incidences et nœuds source augmentent environ×2,3–2,5
aux doublements, mais ne recensent pas tout le travail géométrique. Les
entrées8k/16k/32k restent non exécutées : pas de conclusion globale de
complexité ni de vitesse. Cette capture CPU partagée a duré582,900s.

## Ce qui est comparé

À partir du seul niveau K5 de FULL, trois consommateurs sont mesurés :

- Routage exclusif : une branche fixée par point, arbre ponctuel explicite,
  condensation selon le nombre de points, puis EOM.
- Vote pondéré : masses sur les facettes, condensation/EOM, puis vote plat.
  Le seuil est une **masse de facettes**, pas la cardinalité finale des groupes.
- Première couverture : baseline ponctuelle, même condensation/EOM.

HDBSCAN reçoit exactement les mêmes sites, avec K5 auto-inclus, le même EOM
ponctuel, les mêmes seuils20/50 et exposants1/2. Sa sélection standard en
expZ1 est conservée séparément. Racine exclue et bruit non rempli dans le
protocole commun. La vérité terrain n'est décodée qu'après les18prédictions
de chaque scène ; aucun réglage n'en dépend.

Le catalogue reste Gabriel complet. Ce n'est pas une reproduction du
catalogue contributif d'ordre-Voronoï de HGP-old/Clusterer3D. L'ablation de
cette différence reste distincte, et aucun ancien score n'est réutilisé ici.

## Lecture des mesures

Le profil principal est K5/m20/expZ1. Tous les réglages sont publiés, sans
choisir le meilleur exposant pour chaque scène. L'ARI tous points compte
le label −1 comme un groupe ; l'ARI des vrais points de classes avec rejets
en singletons, la couverture et le F1 apparié complètent cette mesure.
Les scènes bruitées ajoutent précision/rappel/F1 du rejet de bruit.

Le F1 utilise une affectation hongroise qui maximise le nombre total de
points appariés, **pas** le macro-F1. Le vrai bruit est exclu comme classe,
mais sa présence dans les groupes prédits reste une contamination.
Les classes trop petites pour le seuil restent dans les tableaux.

expZ2 n'est pas une ablation de la seule transformation des rayons : dans
les deux voies pondérées HGP, il modifie aussi les scores, puis les votes
ou le routage. Dans HDBSCAN et la première couverture, il ne modifie que
la sélection. Ni r^-1 ni r^-2 n'est ici une densité volumique3D calibrée.
Deux graines ne suffisent pas à conclure à une supériorité statistique générale.

Les coordonnées originales binary64 sont conservées. Une grille u18 commune
à toutes les tailles d'une même série est choisie sans labels ; arrondi
rationnel exact, aucune collision acceptée. Cette unité synthétique n'est
ni un mètre ni la grille LiDAR1mm. Les méthodes reçoivent les mêmes coordonnées.

## Exécution et vérification

`qualify.py --output NEW` exécute les quatre petites suites normal/−O.
`prepare.py --output NEW` prépare les34scènes de qualité et neuf scènes
de croissance8k/16k/32k, avec manifestes et empreintes.
`run.py --manifest MANIFEST --output NEW --workers 2` exécute les34scènes
de qualité entières, sans reprendre ni omettre de cas. Le pilote réutilise
le binaire natif qualifié d'export des attaches et les consommateurs gelés.
Les workers appartiennent au lanceur ; leurs sous-processus sont joints ou
arrêtés à l'échec. Les échecs et sorties partielles restent conservés.

La contrelecture `post_audit.py` et le résumé `summarize.py` sont séparés
du calcul scientifique. Les captures volumineuses restent hors Git sous
`/tmp`, les sources et bilans compacts sont versionnés. Les anciennes preuves
sont LIVE : leur disponibilité locale est requise pour rejouer ces lanceurs.

Les temps incluent un export de diagnostic CPU et des références Python,
avec deux scènes concurrentes ; ils ne qualifient pas une performance FULL
de production. Les neuf **entrées**8k/16k/32k ne sont pas encore des mesures
de croissance. Aucune borne globale sous-quadratique ni nouveau contrat
GPU100ms ne découle de cette campagne statistique. GCP non utilisé.
