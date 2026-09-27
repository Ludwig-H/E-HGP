# Gaussiennes 3D : communautés, difficulté et arbre condensé

27 septembre 2026. Protocole fixé avant production des scores de cette campagne.
Cadre `exploration_v9_hors_registre / cpu_reference / quantized_u18_input_only /
gaussian_fixed_k_condensation_benchmark / not_claimed`. Pas de GCP prévu :
cette expérience vise la qualité statistique et la sémantique, pas le contrat GPU.

## Ce qui est repris, ce qui est ajouté

Le lot précédent `b_point_hierarchy_k_20260927` reste immuable. On réutilise
explicitement son exporteur natif, la projection première couverture dans le
seul T_K, sa quantification isotrope, sa condensation/EOM et son adaptateur
HDBSCAN sklearn 1.9.1, avec hashes conservés. Pas de vote d'entrées dans cette
campagne : son coût supplémentaire n'était pas justifié par les résultats.

`min_cluster_size` existait déjà dans le calcul EOM. La nouvelle API fournit
**l'arbre condensé explicite**, pas seulement les labels : parents/enfants des
clusters, niveaux de naissance et de sortie, masses, stabilités et sorties
datées de chacun des n points. Ce complément permet d'inspecter le nettoyage.
Il ne supprime aucun point de l'entrée géométrique.

En descendant vers les zones plus denses : aucun enfant assez grand termine
la branche ; un seul enfant assez grand prolonge le même cluster ; plusieurs
enfants assez grands créent une bifurcation. Les petites branches deviennent
des sorties de points. La racine reste structurelle, jamais sélectionnée par
EOM. Un label de bruit ne transforme pas les points rejetés en un cluster de
la hiérarchie : les coupes complètes les gardent comme singletons distincts.

## Plan de données et de calcul

- 1 200 points par scène, sans bruit artificiel ajouté, aucune troncature de
  gaussienne ni suppression de point mal placé. Vérité = composante génératrice.
- Plan principal équilibré/isotrope : G=2,4,8,16 communautés ; séparation
  minimale des moyennes delta=8,4,2 dans l'unité sigma=1 ; trois graines :36cas.
- Stress : G=8, les mêmes trois séparations, deux graines ; gaussiennes
  anisotropes d'écarts-types principaux (2,1,0.5), ou effectifs de rapport4:1
  alternés, soit12cas supplémentaires. Total **48scènes**.
- Moyennes en configuration3D déterministe puis rotation ; séparation minimale
  réellement obtenue publiée. Les réalisations normales sont appariées entre
  séparations, sans sélection selon leurs scores. Détails dans DATASETS.md.
- K=5,10, `min_cluster_size`=10,20,50,100, expZ=1,2. Cas principal fixé :
  **K5, taille minimale20, expZ1**. Tous les résultats sont conservés ; pas de
  meilleur paramètre choisi séparément pour chaque scène.
- HGP et HDBSCAN utilisent exactement les mêmes coordonnées quantifiées u18,
  une seule échelle pour les trois axes, doublons/collisions refusés et publiés.
- `min_samples=K` self-inclus pour sklearn, fixé lorsque la taille minimale
  varie. Même condensation/EOM atomique, mêmes masses unitaires, epsilon0,
  racine non sélectionnée. expZ2 modifie EOM pour les deux arbres : ce n'est
  pas HDBSCAN standard. Les labels standards expZ1 restent également archivés.
- Une tour native par scène/K ; un arbre HGP de points réutilisé pour toutes
  les condensations. Aucun recalcul géométrique pour changer la taille minimale.

La [documentation HDBSCAN](https://hdbscan.readthedocs.io/en/latest/parameter_selection.html)
distingue précisément la taille minimale de cluster de `min_samples` : on
garde ce dernier explicite pour ne pas confondre leurs effets.

## Mesures fixées

Qualité des labels : ARI/NMI tous points, ARI où chaque point rejeté compte
comme un singleton distinct, couverture, nombre de groupes trouvés, erreur
sur G et appariement optimal des labels pour précision/rappel par communauté.
Comme il n'y a pas de bruit ajouté, chaque rejet est une abstention sur un
point d'une vraie communauté, pas une détection de bruit de référence.

Qualité de l'arbre : pureté du dendrogramme avant condensation ; nombre de
nœuds de clusters avant/après ; meilleurs recouvrements F1 des communautés
par les nœuds condensés **comme diagnostic supervisé seulement**, jamais pour
choisir les labels EOM ou les paramètres. La comptabilité conserve n sorties
de points même quand il reste très peu de nœuds de clusters.

Un diagnostic MAP utilisant les vrais paramètres du mélange mesure le
chevauchement intrinsèque des composantes. C'est une classification avec
information oracle, **pas un concurrent de clustering**, ni une borne
supérieure de l'ARI. Les mélanges très recouvrants peuvent avoir moins de
modes de densité que de composantes génératrices : G n'est pas imposé aux
algorithmes et les mauvais scores ne prouvent pas, seuls, un bug géométrique.

Les trois répétitions principales ne suffisent pas à établir une supériorité
statistique générale. Présenter les moyennes et la dispersion, les échecs,
et les différences appariées ; séparer sphériques et stress. Ne pas extrapoler
ces petits nuages à la croissance LiDAR ou à un temps FULL/G4.

## Implémentations de cette tranche

- `gaussian_data.py` : générateur et préparation, paramètres connus conservés.
- `condensed.py` : API exploitable de l'arbre nettoyé et condensation commune.
- `evaluation.py` : diagnostics supplémentaires contre la vérité.
- `run.py` : campagne figée, captures séparées et hashes avant/après.
- `report.py` : tableaux agrégés, données de condensation et rapport visuel.

Les sources du moteur et les captures précédentes ne sont pas réécrites.
Les données et les gros exports restent en privé ; scripts, résultats et
reçus de cette expérience sont publiés dans ce dossier.
