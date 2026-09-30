# Audit indépendant de Morse HGP v10 — 29 septembre 2026

**État courant et fermeture des constats : [SUIVI_AUDIT_INDEPENDANT.md](SUIVI_AUDIT_INDEPENDANT.md).** Ce rapport conserve les résultats de la base ci-dessous.

**Relecture des parties I et II de la thèse, 29 septembre :** les conclusions de projection ci-dessous sont précisées pour préserver les points frontière. [La note courante](audit_independant_20260929/ANCRAGE_AMBIGUITES.md) distingue la référence core de la couverture discrète visée, les masses avant condensation et le coût du report au LCA.

**Objet audité : `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`.** Source effective : `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/`. Demande utilisateur : audit complet du dossier, de la rigueur mathématique et des implémentations servant les contrats, avec priorité ajoutée au passage de FULL vers une hiérarchie laminaire de points fiable et robuste.

Cadre : `phase=exploration_v10_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only`, `mode=audit_independant_math_implementation_contracts`, `public_status=not_claimed`. GCP non utilisé par cet audit. Aucun source produit, reçu existant, build épinglé, index Git ou VM modifié par l'auditeur.

## Verdict et décision utile

**Aucun défaut du catalogue ou de la topologie FULL n'a été démontré sur le domaine non pondéré servi.** Les preuves locales et les contrôles supplémentaires sont cohérents : filtres stricts, support canonique, coquille complète, centre de miniboule certifié, morceaux locaux de Gordan, descente strictement décroissante, plateaux atomiques et verticales. Cela ne vaut ni preuve générale de pire cas, ni qualification statistique.

**La restriction core à K fixé fournit une référence laminaire et stable sur les points.** La cible discrète de la thèse comprend aussi les points frontière couverts par chaque composante, même non core. Ces couvertures recouvrantes et les branches de plusieurs K ne peuvent pas toutes être conservées dans un unique arbre de points. Il faut déclarer une projection supplémentaire et en mesurer la perte de récupération avant fusion ; la première couverture actuelle et le report au LCA sont de tels choix.

La suite recommandée est de conserver les composantes FULL et leurs incidences de couverture, d'utiliser core comme comparateur de stabilité, puis de qualifier masses, récupération avant fusion, hauteurs et règles de projection. La normalisation des contributions frontière avant condensation est centrale au §9.1 de la thèse. Les défauts d'API et de preuves ci-dessous sont à fermer par de petites portes avant de dépenser une nouvelle campagne. Ils ne réfutent pas les chronos et objets déjà obtenus sur des entrées valides.

## 1. Question mathématique prioritaire : FULL → points

### 1.1 À K fixé : un objet exact existe, sans vote

Noter $r_K(y)=\sqrt{D_K(y)}$ et $L_K(r^2)=\lbrace y:r_K(y)\leq r\rbrace$. Pour deux rayons, les composantes de ces ensembles sont disjointes ou emboîtées. Leurs intersections avec X le sont aussi. Le point x entre à $r_K(x)$ dans la composante qui contient effectivement x : c'est **core**.

On peut compléter chaque coupe par les singletons des points encore extérieurs à L_K. On obtient des partitions emboîtées de tous les points. Cette complétion ne doit pas antidater leur masse dans la condensation : les dates d'entrée sont des marques de l'arbre, à conserver. Les branches spatiales sans point peuvent être contractées dans le consommateur de points ; leurs cols et leurs dates de fusion restent utiles.

Pour les points distincts, le premier rayon de composante commune définit une ultramétrique $u_K(i,j)$. Il comprend les entrées des deux points et la date de fusion de leurs composantes ; on fixe la diagonale à zéro et conserve séparément les entrées. Remplacer un col du continuum par le minimum des densités des seules feuilles modifierait l'objet.

### 1.2 Couverture et multi-K : deux obstructions distinctes

- **Couverture à un même K.** Sur X={0,2,4}, K=2, r=1, les composantes sont les centres {1} et {3}, leurs couvertures sont {0,2} et {2,4}. Une partition exclusive ne peut conserver simultanément ces deux blocs. Cover choisit une première branche puis la prolonge : c'est une projection laminaire particulière, pas toute la relation de couverture.
- **Plusieurs K.** Notre fixture indépendante X={0,1,4,7}, vérifiée par le binaire et par Γ, donne {0,1} à K1, a=1/4, et {1,4} à K4, a=36. Ces groupes se croisent. Les verticales au même a n'éliminent pas ce croisement entre coupes incomparables.

Preuves et sondes : [TOUR_ET_POINTS.md](audit_independant_20260929/TOUR_ET_POINTS.md), [tower_point_crossing.py](../receipts/audit_independant_20260929/probes/tower_point_crossing.py). L'autre auditeur a produit des fixtures complémentaires et la preuve de stabilité ci-dessous dans [AUDIT_LAMINARITE_POINTS_20260929.md](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md). Son travail n'est pas un résultat de nos propres sondes.

### 1.3 Une vraie garantie de robustesse est disponible pour core

Si mêmes cardinal/IDs et chaque point est déplacé d'au plus ε, la hiérarchie core vérifie $|u_K^X(i,j)-u_K^Y(i,j)|\leq2\varepsilon$, en **rayon**. En effet $L_K^X(r^2)\subseteq L_K^Y((r+\varepsilon)^2)$ ; les segments reliant les deux positions des extrémités restent dans la seconde filtration à r+2ε. Échanger X et Y donne la borne. Elle vaut à tout K fixé, sans hypothèse i.i.d., et inclut les contacts. Le facteur 2 est atteint pour deux points à K2.

Elle contrôle les hauteurs de fusion, pas l'identité combinatoire de toutes les petites branches, le nombre d'amas sélectionnés, l'ajout/suppression de points ou un changement de K. Elle ne se transporte pas telle quelle aux rayons carrés ni à une échelle inverse r^(−z).

Cover a une obstruction plus forte : {0,999,2000} et {0,1001,2000}, K2, changent une fusion de 499,5 à 1000 après déplacement de 2 unités. L'autre auditeur conserve cette preuve native. Le problème existe avant EOM ; supprimer de petites branches peut en atténuer l'effet pratique, mais ne donne pas une garantie générale de projection.

### 1.4 Fidélité statistique : préciser l'estimateur et le régime

Pour une densité euclidienne ambiante de dimension d, $\widehat f_{K,n}(y)=K/(nv_dD_K(y)^{d/2})$ a exactement les L_K comme superniveaux après changement d'échelle. FULL à K fixé puis core donnent donc l'arbre plug-in de cet estimateur, restreint aux observations.

Une route de preuve existe : consistance uniforme de l'estimateur → contrôle uniforme des hauteurs de fusion. Les théorèmes k-NN exigent un K croissant, notamment K/log(n)→∞ et K/n→0 sous leurs hypothèses ; **K≤10 ne satisfait pas ce régime asymptotique**. Voir [Devroye–Wagner 1977](https://luc.devroye.org/devroye_wagner_1977_the_strong_uniform_consistency_of_nearest_neighbor_density_estimates.pdf) et la note vérifiée [sources statistiques](audit_independant_20260929/tower_statistical_sources.md).

La mesure des erreurs de fusion est plus adaptée que le seul ARI d'une partition : une bonne coupe ne juge pas toute la hiérarchie. [Eldridge–Belkin–Wang 2015](https://proceedings.mlr.press/v40/Eldridge15.pdf) donne ce cadre et distingue séparation, minimalité et stabilité. Notre note explicite le transfert par inclusions de superniveaux, avec conservation des dates du continuum.

Une trame LiDAR quantifiée n'est pas automatiquement un échantillon i.i.d. d'une densité volumique : support surfacique, dépendance capteur, échelle de quantification et doublons sont des choix de modèle. Remplacer d=3 par une dimension intrinsèque supposée ne prouve pas que les chemins ambiants calculés par FULL sont des chemins sur la surface. EOM avec z réglé, cover et remplissage n'héritent pas sans preuve de cette consistance.

### 1.5 Proposition au développeur

1. Conserver les dates exactes et toutes les incidences point→composantes couvertes ; garder core comme comparateur de stabilité. La première couverture exclusive ne représente pas à elle seule toute cette relation.
2. Mesurer la récupération des observations avant fusion parasite, puis hauteurs de fusion, marges mode–col et sensibilité à perturbations/rééchantillonnages. La couverture et la masse normalisée doivent être publiées séparément, avant EOM.
3. Qualifier les masses frontière avant condensation. Pour l'arbre strict de points, imposer une affectation suivie par les ancêtres. Attacher au LCA est conservateur **si la bonne branche appartient à la liste** ; mesurer la masse perdue ou différée dans les descendants. Les votes du §9.1 donnent une partition après sélection, sans garantir une hiérarchie de points à toutes les coupes.
4. Utiliser les autres K pour corroborer des branches avant de fusionner des arbres. Une chaîne de coupes ordonnées par inclusion reste laminaire. Un consensus ultramétrique est un autre modèle : notre note donne une médiane normalisée suivie de fermeture min–max, avec garantie conditionnelle de majorité proche d'une même cible. Ni cette hypothèse ni un gain ne sont acquis ; aucune matrice n² n'est proposée comme nouveau chemin produit.

## 2. Défauts concrets et ordre de correction

P1 désigne une frontière de sûreté, d'intégrité ou de preuve à fermer ; P2 un comportement ou coût circonscrit. Les sondes sont des reproductions indépendantes de la base figée, pas des régressions déjà intégrées.

| ID | Priorité | Constat reproduit | Portée et correction utile |
| --- | --- | --- | --- |
| I1 | P1 | Les trois CLI ignorent une fin u32le partielle et rendent `status=ok` sur le préfixe | lecteur commun, erreur de lecture et reste 1..11 octets refusés avant calcul ; intégrité d'une trame entière |
| P1 | P1 | `bad_alloc` dans un worker du pool provoque SIGABRT | capture/annulation par job, TLS et restitution RAII, fermeture puis attente avant remontée/conversion en statut |
| H1 | P1 | `validate` accepte un `point_rank` hors tableau ; ASan prouve la lecture hors bornes dans la tête | borner les rangs, vérifier les parents et les deux sens du CSR, niveaux finis |
| E1 | P1 | `decide.py` accepte une scène sur 960 et `complete=false` ; merge accepte une unité hors plan | vérifier le produit exact manifeste × méthodes, unicité et appartenance avant décision |
| G1 | P1 API | Sur quatre sites u18, un centre circonscrit hors enveloppe fait perdre une coquille et fausse `nearest` | contrat de centre MEB vérifiable ou repli exact hors domaine ; aucun appel produit courant exposé trouvé |
| H2/H3 | P2 | `allow_single` absorbe aussi des points sortis trop tôt ; z non fini/non positif accepté | fermer le domaine et la sémantique de racine, comparer à sklearn sur une petite fixture |
| H4 | P2 coût | Trois remontées répétées dans la tête ; Θ(C²) sur un peigne valide | transmettre l'ancêtre sélectionné en une passe ; juger labels et travail discret |
| I2 | P2 | `--repeat=0` et `--no-points --dump` provoquent SIGSEGV | validation de répétition et export des seules attaches présentes |

Preuves détaillées : [FRONTIER_CHECKS.json](../receipts/audit_independant_20260929/FRONTIER_CHECKS.json), [TETE_BANCS_PREUVES.md](audit_independant_20260929/TETE_BANCS_PREUVES.md), [GEOMETRIE_CATALOGUE.md](audit_independant_20260929/GEOMETRIE_CATALOGUE.md).

**Mémoire.** `MemoryBudget` couvre les Buffer du nuage, pas les grands vecteurs du catalogue, de la tour et des scratches ; les paramètres publics de ces couches ne prennent pas de budget. Le commentaire « tout grand tableau » n'est donc pas un contrat de mémoire livré. Cette dette est liée à la fermeture d'exception du pool et au futur catalogue hors mémoire. Ne pas transformer le ratio RSS/boule mesuré en garantie de non-échec. Préparer un budget logique et un refus déterministe avant d'annoncer cette propriété à l'échelle.

**Niveaux de la tête.** FULL conserve les rangs rationnels exacts. `point_dendrogram` regroupe volontairement des niveaux indistinguables ou inversés en double. La condensation est exacte sur cet arbre publié ; le choix EOM n'est pas démontré identique à un calcul distinguant toutes les dates rationnelles. Conserver le rang exact séparé de la valeur de calcul, et déclarer la politique de plateau du consommateur.

## 3. Tests et preuves réellement relus ou exécutés

La copie figée a été construite dans des répertoires neufs : Release GCC13 et ASan/UBSan Clang18. L'inventaire initial est [SOURCE_BEFORE.json](../receipts/audit_independant_20260929/SOURCE_BEFORE.json). Aucun test d'échelle n'a été lancé par cet audit.

| Contrôle additionnel | Résultat et limite |
| --- | --- |
| Sept CTests livrés, hors catalogue/tour exhaustifs | 7/7 PASS ; 400,62 s mur local, [CTEST_BOUNDED.xml](../receipts/audit_independant_20260929/CTEST_BOUNDED.xml). Plusieurs tests ont dépassé 60 s : leurs propriétés TIMEOUT explicites priment sur l'option CLI. Aucune répétition longue lancée. |
| Catalogue avec Fraction, extrêmes et poids | 2 831 boules comparées, aucun désaccord. Deux expirations avec feuilles inférieures à K conservées ; les mêmes cas au paramètre sûr passent. |
| Tous ordres sur 22 nuages n≤8, core et cover, W1/W4 | 3 264 coupes Γ, 1 846 verticales, 9 518 K-parties couvrantes, 911 entrées α : PASS. |
| MEB et repli exact directement sondé | 120 sphères contre oracle brute Fraction : PASS. Aucun repli effectif du chemin MEB rapide obtenu dans ce lot ; ne pas confondre les deux couvertures. |
| Frontières d'entrée et échecs | neuf fichiers incomplets acceptés à tort ; deux crashes d'options ; exception worker SIGABRT. |
| Sûreté de tête | reproduction ASan ciblée positive d'une lecture hors bornes après validation. |
| Lot C archivé | 30 720 couples attendus, zéro manquant/doublon/refus. Le défaut E1 ne réfute donc pas le lot C publié. |
| Empreintes de reçus | lots A/C et G4 4/5 contrôlés par l'audit tête, aucun écart relevé. Les limites de certains autres reçus sont signalées par l'autre auditeur. |

Le juge vertical livré manque les composantes sans point déjà attaché : 623 naissances core et 292 cover dans notre lot. Notre juge les vérifie via leurs K-parties ; aucune image fausse trouvée. C'est un complément de qualification utile à porter dans les portes. Les sondes adverses de l'autre audit ont aussi montré qu'un mutant de verticale pouvait survivre au gate courant ; son résultat lui appartient.

## 4. Contrats et affirmations à borner

- Le domaine u18/1 mm prioritaire pour les chronos sans sol est autorisé par les décisions datées ; ce n'est pas une substitution non autorisée au float32.
- Les mesures natives `--no-points` construisent tous les ordres 1..K et les verticales ; les commandes de clustering ne construisent que l'ordre choisi. Le backend G4 publié est CPU, pas GPU. Une sonde CUDA ajoutée pendant l'audit ne qualifie pas le moteur GPU.
- Les trois trames 08/000000, 08/000100, 08/000200 appartiennent à une seule séquence. Le protocole adopté sur 30 trames de dix séquences reste ouvert. Les secteurs capteur servent au diagnostic, pas à sa qualification.
- Les sommes catalogue+tour sont les passes chaudes, hors préparation ; les médianes G4 4 vont de 0,2143 à 0,2603 s à K5 et de 0,8698 à 1,1246 s à K10. Lecture/segmentation/quantification sont encore d'autres périmètres.
- Les ratios ≈460 boules/site et le coût mémoire par boule sont empiriques ; ni borne de sortie indépendante de la géométrie, ni capacité générale LiDAR acquise. L'autre audit a corrigé les unités et le ratio de pic ; ses chiffres ne sont pas repris comme nos mesures.
- Multiplicités dans FULL, domaine K au-delà de 10, juges d'échelle de la tour, GPU et robustesse statistique restent ouverts. La règle pondérée conservatrice du catalogue suit SPEC mais pas la conception GEN plus serrée : pas de perte observée, dette de raccord explicite.

## 5. Coordination et état de fermeture

Le développeur a lu les constats intermédiaires et accepté un plan de correction dans [NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md](NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md), commit `0bce6cc00`. Les corrections de portée de passation/errata sont commises en `2aacfa2e5`. Ces réponses sont prises en compte ; les défauts produit ci-dessus restent ceux de la base figée, jusqu'à relecture des correctifs et de leurs reçus.

La fermeture des constats est mise à jour dans [le suivi courant](SUIVI_AUDIT_INDEPENDANT.md), sans multiplier les addenda. L'autre auditeur possède [audit_continu_20260929/](audit_continu_20260929/) ; ses fichiers sont préservés. Les preuves et les notes intermédiaires remplacées sont conservées dans [receipts/audit_independant_20260929/](../receipts/audit_independant_20260929/).

**Ordre utile de la suite :** petites corrections de sûreté/intégrité et juges ; incidences complètes et masses frontière ; comparateur core et diagnostics de récupération/robustesse ; projection laminaire explicite ; sélection de branches ; comparaison multi-K après définition de sa cible. Les chronos et le score empirique ne doivent pas servir de preuve à une étape différente.
