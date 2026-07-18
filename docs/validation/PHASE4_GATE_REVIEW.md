# Revue de porte Phase 4 — canonisation et oracle spatial

> [!IMPORTANT]
> Cette revue ferme la Phase 4 pour le backend de référence `reference_cpu`, le profil prioritaire `hgp_reduced` et le mode `certified`. La qualification G4 vérifie en plus le parcours CUDA LBVH parallèle comme couche de proposition recertifiée sur CPU. Elle ne construit encore aucun catalogue critique, aucune forêt et ne promeut aucun résultat vers `public_status=exact`.

## Décision

- Phase : `4` — canonisation et oracle spatial.
- Backend de décision : `reference_cpu`.
- Backend de proposition qualifié : `cuda_g4`, cible AOT `sm_120` sur G4.
- Profil prioritaire : `hgp_reduced`.
- Mode : `certified`.
- Porte d'entrée : satisfaite par les Phases 2A, 2B et 3 fermées.
- Verdict de sortie : **satisfait**.
- Effet opérationnel du commit de fermeture : Phase 4 `completed`; Phase 5 `in_progress`; Phases 6 et 7 `ready`.

La décision repose sur l'oracle rationnel indépendant, la campagne complète de la matrice de tailles spatiale prévue, les contrôles négatifs, la qualification CUDA réelle et la fermeture GCP ciblée. Elle ne repose ni sur un benchmark, ni sur une hypothèse de voisinage local, ni sur les distances proposées en binary64.

## Contrat spatial fermé

Pour une requête rationnelle exacte, le top-$k$ publie le seuil exact, tous les identifiants strictement sous ce seuil et le shell d'égalité complet. Le choix canonique de $k$ identifiants ne remplace jamais ce shell. La boule fermée publie une partition globale, triée et disjointe, entre intérieur, shell et extérieur; son rang fermé est la somme des cardinalités de l'intérieur et du shell.

Le backend `reference_cpu` évalue les distances entre coordonnées dyadiques et requêtes rationnelles comme des `ExactLevel`. Le Morton-LBVH CPU n'élague un sous-arbre que par une comparaison rationnelle stricte entre une borne AABB exacte et le cutoff. L'égalité descend jusqu'aux feuilles. Les deux ordres de parcours publient la même partition canonique.

Le backend `cuda_g4` conserve une séparation stricte :

- le device propose une antichaîne de feuilles candidates et de sous-arbres strictement extérieurs;
- l'hôte reconstruit la couverture complète de tous les `PointId`;
- chaque rejet est recertifié par le calcul rationnel exact de la borne AABB;
- chaque feuille restante est classifiée par sa distance exacte;
- les propositions FP64 ne décident ni cutoff, ni égalité, ni shell, ni rang.

Une requête ou un cutoff hors du domaine binary64 désactive le kernel et utilise le chemin exact CPU complet. Aucun chemin exact ne dépend d'un epsilon, d'un nombre maximal de visites ou d'une troncature du shell.

## Campagne de fermeture

Le commit propre et déjà poussé `c846ed7b253840ef6fe1f0f39f7f10c63af64b8e` a été qualifié sur une NVIDIA RTX PRO 6000 Blackwell Server Edition. L'artefact hors dépôt `phase4-spatial-c846ed7b253840ef6fe1f0f39f7f10c63af64b8e.json` utilise le schéma `morsehgp3d.phase4.spatial_gpu_reference_and_lbvh_qualification.v3` et porte le SHA-256 `1c8794763a0ff5bcd0c24694d5b2ae2c82402f5881d8999a046bb408c2b14f65`.

| mesure | résultat |
|---|---:|
| cas `Fraction` indépendants | 1 013 |
| tailles couvertes | toutes les tailles de 1 à 1 000 |
| requêtes top-$k$ | 1 013 |
| requêtes de boule fermée | 1 013 |
| traversées LBVH logiques sur GPU | 2 019 |
| lancements de fronts CUDA | 23 331 |
| fronts parallèles | 21 312 |
| largeur maximale observée | 570 |
| nœuds traités sur GPU | 759 117 |
| sous-arbres rejetés et recertifiés | 73 962 |
| cas distincts sous `memcheck` | 20 |

Toutes les partitions coïncident avec l'oracle Python `Fraction`. Les couvertures sont des antichaînes complètes, les partitions de points ferment exactement et chaque rejet GPU possède une recertification CPU exacte. Le passage `racecheck` rapporte zéro hazard, zéro erreur et zéro avertissement. Le passage `memcheck` rapporte zéro erreur et zéro fuite. L'audit AOT trouve seulement `sm_120` et aucune entrée PTX.

## Évaluation explicite de la porte

| critère | preuve | décision |
|---|---|---|
| identifiants canoniques | canonisation, permutations et namespaces vérifiés sur les différentiels CPU et GPU | go |
| ordre top-$k$ exact | strict, shell complet et choix canonique identiques à `Fraction` | go |
| shell et rang fermé globaux | partition intérieur–shell–extérieur exacte sur toutes les requêtes de boule | go |
| exclusions jusqu'à $m_{\star}$ | jusqu'à neuf identifiants exclus, avec fermeture des compteurs admissibles | go |
| bornes AABB sûres | chaque rejet GPU et CPU possède une marge rationnelle strictement positive | go |
| absence de limite arbitraire | fronts bornés seulement par la taille structurelle de l'arbre; aucun plafond de visites | go |
| indépendance de l'ordre de parcours | ordres CPU opposés et fronts GPU donnent la même sortie canonique | go |
| échec fermé | corruptions, faux rejets, doublons, omissions et intervalles invalides empoisonnent le seul contexte concerné | go |
| environnement matériel | CUDA 12.9, AOT `sm_120`, sans PTX ni fast math | go |
| fermeture GCP | génération ciblée arrêtée et relue `TERMINATED` | go |

**Verdict final : porte de sortie Phase 4 satisfaite.** La partie spatiale nécessaire aux futures campagnes de catalogue est fermée. La porte globale G2 du catalogue reste distincte et sera évaluée lorsque les phases d'énumération correspondantes auront livré leurs événements; aucun statut public scientifique n'est accordé ici.

## Limites et optimisations différées

- Le parcours CUDA recopie encore un tampon de couverture de capacité `node_count`; aucune cible de latence n'est revendiquée.
- La boule fermée ne classifie pas encore les sous-arbres strictement intérieurs en bloc. Cette optimisation pourra utiliser $D_{\max}<r^2$ avec recertification rationnelle; elle n'est pas une obligation de correction, car toutes les feuilles non rejetées sont déjà classifiées exactement.
- Les campagnes LBVH consomment des requêtes rationnelles génériques sans reconstruire explicitement leur provenance comme centre critique ou intersection de plans. La composition est couverte par le type commun `ExactRational3`; un test d'intégration dédié restera utile avant que l'énumérateur de catalogue ne consomme cette API.
- Aucun chemin cuVS n'est intégré.
- Aucun catalogue critique, événement de Morse, Gamma, réduction hiérarchique ou morphisme vertical n'est produit par cette phase.
- `full_pi0` reste conditionnel et l'obligation M.1 reste ouverte.

Toute modification future des règles de canonisation, des bornes AABB ou du noyau CUDA certifiant impose de rejouer les différentiels concernés avant de réutiliser cette preuve.

## GCP

La qualification de fermeture a ciblé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, initialement `TERMINATED`, de type `g4-standard-48`, en `SPOT`, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et arrêt invité armé à 45 minutes. La génération démarrée à `2026-07-18T12:18:46.848-07:00` a été arrêtée par `stop_and_verify.sh`. La relecture indépendante certifie `TERMINATED`, avec `lastStopTimestamp=2026-07-18T12:22:52.945-07:00` et preuve finale à `2026-07-18T19:22:59Z`.

La clé OS Login de session a été révoquée et sa copie privée supprimée. Aucune autre VM `project=e-hgp` active n'a été observée au passage de relais.

## Suite de la feuille de route

La Phase 5 devient active sur `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`. Elle doit établir l'ancre $k=1$ en comparant, niveau exact par niveau exact et lot égal par lot égal, l'arbre de fusion EMST aux sphères de rang deux et au Gamma exhaustif de référence.
