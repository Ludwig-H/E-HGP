# Dendrogramme de points : plan avant mesure — 27 septembre 2026

Cadre : `exploration_v9_hors_registre / cpu_reference /
quantized_u18_input_only / point_tree_after_FULL_reference / not_claimed`.
Travail après FULL, à K fixé. Moteur, anciennes captures et sources gelées
inchangés. Aucun GCP prévu pour ce diagnostic de sélection CPU.

## Objet à qualifier

1. Conserver le routage exclusif exact déjà qualifié, sans revote par coupe.
2. Matérialiser une forêt compacte avec n feuilles-points, fusions datées
   et provenance. Supprimer les nœuds vides/unaires, traiter ensemble les
   événements égaux et conserver tous les singletons extérieurs.
3. Raccorder la même condensation/EOM unitaire que le comparateur HDBSCAN.
   Ce premier raccord EOM exige une seule vraie racine et n≥1 ; la forêt
   générale reste représentable et découpable, sans racine artificielle.

La forêt compacte a au plus n−R nœuds internes pour R racines et n>0,
car chaque nœud interne a au moins deux enfants. Cela borne sa **sortie**,
pas le nombre de facettes source ni le coût de leurs poids/rattachements.
Plus précisément, c'est le squelette ponctuel qui est O(n) ; les tables
facultatives de provenance source conservées par la référence restent O(V).
Construire/valider ce quotient ne doit pas stocker tous les descendants
de chaque nœud ni remonter chaque point jusqu'à la racine indépendamment.

Les dates β restent rationnelles dans l'arbre. L'adaptateur EOM utilise
des rayons `sqrt(β)` en binary64, refuse les collisions de β distincts,
les débordements et les sous-flux positifs. λ et les stabilités restent
approximatifs. Aucune racine sélectionnée, aucun remplissage 1-NN du bruit.
`min_cluster_size` est ici une cardinalité de points ≥2, distincte du seuil
massique sur les facettes du prototype pondéré précédent.

## Diagnostic gaussien préannoncé

Reprendre les **13 scènes complètes n1200** du pilote pondéré clos,
sans nouveau calcul géométrique ni nouveau fit HDBSCAN. Les régimes sont
déjà connus : ce n'est pas une évaluation tenue à l'écart ni une nouvelle
preuve de supériorité statistique.

- K=5, seuils20 et50, expZ1 et2 ; les 52 sélections restent toutes visibles.
- Profil principal : K5, seuil20, expZ1. Aucun meilleur z choisi par scène.
- Trois graines pour sphérique G2/δ8, G8/δ4 et G16/δ2 ; deux pour les
  stress anisotrope et déséquilibré G8/δ4.
- Les 182 lignes héritées à K5 (vote pondéré, première couverture,
  HDBSCAN commun et HDBSCAN standard z1) sont conservées sans recalcul.
- ARI tous points et bruit en singletons, couverture, nombre de groupes
  et F1 macro apparié sont rapportés ; aucun seul score favorable retenu.

Les scores Sτ des deux exposants sont ceux des fichiers `measure_z*.json.gz`
figés. Leur relèvement exact par `Fraction.from_float` donne des rationnels
dyadiques ; le nouveau routage est exact **sur ces scores arrondis**, pas
sur les poids géométriques réels, y compris en z2. Les dates géométriques
exactes sont inchangées. Le code de projection ne reçoit aucune vérité
terrain ; les étiquettes ne servent qu'à l'évaluation après sélection.

Passer de z1 à z2 change potentiellement le routage **et** λ pour cette
méthode. Chez HDBSCAN commun, seule λ change. Ne pas appeler cela une
ablation isolée de λ ni un port identique du vote plat de la thèse.

Les comparateurs sont repris par les hashes du reçu complet
`577930f26f4f97fd4f13fb99bb1e356e232a8479055c7f6e09a1bab87bbea720`,
avec son contre-audit déjà clos et le manifeste d'entrée figé. La première
unité sert aussi à mesurer le coût du routage ; un essai interrompu reste
un essai interrompu, sans retirer les scènes difficiles du bilan final.

## Portes et limites

Comparer les coupes strictes/fermées du quotient à la référence, sur petits
cas, forêts, plateaux, permutations, chaîne profonde et trois objets FULL
qualifiés. Vérifier condensation et EOM contre un oracle rationnel borné,
cardinalités et sorties de bruit comprises. Chaque pipeline capturé conserve
ses commandes, sources avant/après, entrées, arbres, labels et résultats.

Sur les scènes du diagnostic, vérifier aussi des coupes aux niveaux
déterministes de l'arbre compact (zéro, quartiles, maximum et au-delà),
sur les deux côtés des événements. Ces contrôles complètent les petites
portes ; ils ne constituent pas une nouvelle preuve du moteur FULL.

K10, SIPU, les graines tenues à l'écart et les mesures de croissance
8k/16k/32k ne sont pas implicitement couverts par ce pilote K5. Les recettes
historiques SIPU/HGP-old/HGP-Clusterer3D sont auditées séparément avant toute
reproduction, notamment ordre K, catalogue contributif, seuils et projection.
