# LiDAR 1 mm : comparer les améliorations sur le moteur natif

22 septembre 2026. `public_status=not_claimed`. Bibliothèque entière native de la v8 compilée, **sans changement de `src/`**, arbre Git `54a6d5816420855e594768865fb66f6f1136df99`. Pas de FULL, de GPU, de nouvelle segmentation ou d'exécution de toute la suite CTest. Les données sont les trois trames 08/000000, 08/000100 et 08/000200, après le masque sans sol déjà publié, grille 1 mm : **39 885 / 35 551 / 45 845 sites**, K5 et K10, séparation8. Trois trames d'une séquence, pas une validation multi-séquence.

## Décision

Pour le régime étudié, privilégier une cascade :

1. **Pour h :** exclure les blocs qui ne peuvent fournir aucun témoin de la paire de sommets proches, puis certifier les blocs restants sur le rectangle complet. La paire n'accorde aucun crédit universel.
2. **Pour les rectangles survivants :** réemployer la décision déjà achevée des singletons ; préparer h_a/h_b sur les gros produits et développer seulement les classes de crédits restantes.
3. **Pour les arêtes encore coûteuses :** essayer un petit certificat collectif exact avant l'atlas, avec acquisition des témoins comprise dans le bilan.

Les deux premiers changements accélèrent des décisions existantes ; le troisième peut supprimer un traitement géométrique que le filtre individuel conservait. **Ne pas multiplier leurs ratios : la cascade complète n'a pas été chronométrée.**

## 1. Protocole et provenance

Le workflow d'audit en lecture seule `.github/workflows/morsehgp3d-v8-lidar-audit.yml` a compilé `mhgp8_p0`, exécuté le front natif sur chaque **nuage entier** et comparé quatre variantes de filtrage sur des rectangles échantillonnés. Runs clos avec succès : **35747981707** à `562d090c847e0248607332cbe2c5b85eccc1008d`, puis **35748470640** à `ca73e96b13086bb2f6491d5942a53010b3335dcb`. Le second conserve aussi une capsule de sources, bibliothèque et entrées hachées. Artefact **10704262200**, SHA256 ZIP `c1507232e6d3681beb4846ed65d797ea7a8251ac47eade650ba77b81a4f0cd32`.

Les premières mesures Actions utilisaient24 rectangles par classe. La capsule a ensuite été téléchargée et ses empreintes contrôlées pour les mesures élargies locales décrites ici : **256 rectangles par classe inférieure, tous les rectangles de plus de65 536 paires**, trois répétitions tournantes. Aucune sélection de points ni changement de l'index. Au total **6 902 contextes de rectangles**, dont **758 très gros rectangles examinés intégralement**, représentant118 937 302 paires avant filtrage. Ce n'est pas le parcours complet de tous les rectangles des trames.

Bibliothèque compilée GCC13.3 sur Actions ; appelants locaux GCC14.2, CPU Intel Xeon Platinum8370C, conteneur partagé. Les tableaux suivants utilisent uniquement les mesures locales, sans mélanger les chronos des deux machines. Les médianes sont indicatives, pas des intervalles de confiance.

Selftest natif avec un juge scalaire utilisant l'identité de Lagrange : **70 672 comparaisons de paires/voies**, toutes conformes. Les variantes par facteurs conservent exactement la liste des paires et leurs masques, comparée après tri hors chrono. Pas de test géométrique FULL ni de sanitizers exécutés dans ce nouveau lot. Les appelants expérimentaux produisent des avertissements d'indentation ; ils ne sont pas des fichiers moteur qualifiés.

## 2. La distribution des rectangles impose deux chemins

Le front natif donne **96,89 à97,38 %** de rectangles représentant moins de64 paires. Les produits d'au moins1 024 paires portent pourtant **69,83 à74,59 %** de la masse de paires résiduelle du front. Ce sont des nombres de paires avant la recherche complète de h, pas des pourcentages de temps.

Un traitement lourd uniforme serait donc mal adapté. Le plan sélectif testé exige `mass>=256 && mass>=4*(|A|+|B|)`. Ces constantes restent expérimentales ; les petits rectangles reprennent le chemin léger. Un critère supplémentaire gratuit peut abandonner une voie du plan si `h_q+|A|+|B|-2<T_q`, lorsque les seuls nouveaux témoins proviennent des deux facteurs. Cela borne les crédits de ce plan, pas la profondeur réelle de la boule. Cette garde supplémentaire n'est pas mesurée ici.

## 3. Paire proche : un vrai gain, mais uniquement comme exclusion

Noter W_q(a,b) le domaine des témoins du prédicat individuel et U_q(A,B) leur intersection sur les extrémités. Pour une paire de coins choisie dans les boîtes :

`U_q(A,B) ⊆ W_q(a0,b0)`.

Si un bloc Z ne contient aucun témoin de cette paire, il ne fournit donc aucun témoin commun. Le nouveau test emploie `PreparedPairCitronBounds` : Hmax<=0, ou `alpha*(Hmax4)^2<=16*Xi_min`, par voie. Il ne crédite rien et reprend les bornes natives pour tout ce qui reste indécis.

**C'est un changement de force logique du rejet.** L'exclusion générale existante cherche à prouver qu'aucun point de Z n'est témoin pour aucune paire de A×B. Pour calculer h, il suffit qu'une paire particulière interdise à tous ces points d'être universels. La copie expérimentale ajoute ce test avant les bornes générales ; les singletons gardent le traitement affine.

Sur **13 046 contextes** (512 rectangles par classe inférieure, tous les très gros), cinq répétitions alternées : **masques ET crédits h3/h4 identiques au filtre natif**, avec moins de visites. Pour tous les rectangles de plus de65 536 paires :

| Trame | K | Visites natives | Avec présélection | Rapport temps du filtre commun |
|---|---:|---:|---:|---:|
| 000000 | 5 | 97 114 | 15 596 | 5,62 |
| 000000 | 10 | 139 934 | 22 912 | 5,14 |
| 000100 | 5 | 23 397 | 2 333 | 8,86 |
| 000100 | 10 | 30 727 | 4 655 | 5,13 |
| 000200 | 5 | 413 200 | 32 088 | 11,10 |
| 000200 | 10 | 502 657 | 48 155 | 8,09 |

Sur les échantillons de1 024 à65 536 paires : rapports3,51 à5,53. **Ces facteurs ne concernent que la recherche de h**, pas les recherches par paire, l'atlas ou FULL. Le choix des coins et le test sont inclus. Les singletons ont les mêmes visites et n'offrent pas ici de gain algorithmique à revendiquer.

Ne pas convertir cette exclusion en suppression définitive de Z : il peut servir aux lignes, colonnes ou sous-rectangles. Ne pas attribuer à la paire proche une universalité positive qu'elle ne possède pas. Un coin artificiel certifie la boîte continue, pas nécessairement la meilleure universalité sur ses seuls sites.

## 4. h+h_a+h_b : le gain synthétique se retrouve sur LiDAR

Les propositions directionnelles sont préparées une fois par facteur, certifiées exactement contre la boîte opposée, puis regroupées par tuple de crédits q3/q4. h est obtenu à partir du compte de l'appel natif achevé sur le rectangle. Les trois sources de témoins sont disjointes : communs hors des boîtes, locaux A, locaux B. Une paire n'est développée que pour les voies vérifiant `h+h_a+h_b<T_q`.

Résultats sur **tous** les rectangles de plus de65 536 paires. Le chrono inclut filtre commun, préparation/groupement des facteurs, allocations, filtres individuels et stockage des sorties ; il exclut index/front, tri final, covers, atlas et FULL.

| Trame | K | Rectangles | Filtre référence, ms | Plan sélectif, ms | Recherches individuelles avant → après |
|---|---:|---:|---:|---:|---:|
| 000000 | 5 | 66 | 1 098,09 | 277,83 | 668 680 →135 980 |
| 000000 | 10 | 107 | 2 401,00 | 904,14 | 1 160 574 →355 224 |
| 000100 | 5 | 65 | 479,04 | 141,03 | 249 143 →62 674 |
| 000100 | 10 | 211 | 898,54 | 351,71 | 388 523 →150 854 |
| 000200 | 5 | 136 | 8 634,60 | 2 682,34 | 5 254 118 →1 446 381 |
| 000200 | 10 | 173 | 23 108,81 | 8 922,79 | 8 505 554 →2 706 166 |

Rapports2,55 à3,95 sur ces gros produits. Pour les échantillons1 024..65 536 :1,72 à3,73. Les 64..1 023 peuvent aussi bénéficier d'un plan ; il ne faut pas transformer256 en seuil universel optimisé. **La sortie de paires reste identique** : ce gain ne supprime donc pas d'atlas supplémentaire à lui seul.

## 5. Collectifs : acquisition comprise et véritables appels q4

Sur des arêtes survivant au filtre natif individuel, le test acquiert les32 sites les plus proches du milieu par une recherche exacte dans l'index, puis construit des certificats de paires/triangles disjoints. La limite32 concerne uniquement les propositions. Échec : poursuite du moteur exact.

Pour réduire le biais d'un premier échantillon dominé par des arêtes sans aucune émission q4, la série retenue choisit jusqu'à24 arêtes q4 par classe de rectangle, puis jusqu'à16 arêtes supplémentaires parmi les plus longues. **763 arêtes sont effectivement traitées**, dont30 émettent88 candidats q4. Les **221 rejets collectifs q4** sont tous confrontés à l'appel natif complet `run_q4_local_edge_candidates` en LiveOnly : aucune arête rejetée n'émet de candidat.

| Trame | K | Arêtes / rejets q4 | Acquisition + collectif, ms | Travail q4 de référence évitable, ms |
|---|---:|---:|---:|---:|
| 000000 | 5 | 123 /18 | 2,18 | 3,28 |
| 000000 | 10 | 131 /39 | 3,10 | 42,80 |
| 000100 | 5 | 134 /41 | 2,98 | 16,00 |
| 000100 | 10 | 105 /21 | 1,95 | 15,22 |
| 000200 | 5 | 134 /53 | 3,35 | 53,09 |
| 000200 | 10 | 136 /49 | 2,90 | 60,66 |

Il s'agit d'un **bilan contrefactuel sur les arêtes examinées**, avec une seule répétition : tous les appels q4 de référence ont été exécutés, même pour une arête rejetée par le collectif. Les coûts q4 incluent leur atlas et balayage ; le cover est chronométré séparément et n'est pas ajouté au gain ci-dessus. Le coût d'acquisition est bien inclus dans le collectif, contrairement aux anciens microbenchmarks sur pools fournis.

Ne pas annoncer221/763 comme le taux global LiDAR : l'échantillonnage est stratifié et comprend une sélection des longueurs extrêmes. Certaines classes sont négatives : trame000100/K10, les24 arêtes de classe1 024..65 536 n'ont aucun rejet, coût additionnel0,57ms ; parmi ses24 singletons,0,43ms de coût pour0,27ms évitable. Le premier tirage non stratifié de cette même trame/K10 avait80 arêtes et aucun rejet. Ce résultat négatif est conservé, pas remplacé silencieusement.

**L'atlas peut aussi servir à q3**. Rejeter q4 seul ne permet pas d'attribuer tout son coût à une accélération de l'appel combiné q3/q4. Les crédits q3 ont été calculés, mais leurs sorties natives ne sont pas rejouées dans ce test collectif. L'intégration et le bilan complet restent à mesurer.

## 6. Port conseillé

- Ajouter la paire représentante uniquement comme exclusion de blocs pour h. Garder le filtre natif comme référence différentielle ; modifier les compteurs pour décrire les tests réellement payés.
- Insérer le plan par facteurs après le h commun, avec un chemin très léger pour les petits rectangles et réemploi de la décision achevée des singletons.
- Mesurer un collectif avant l'atlas sur les résidus coûteux ; réutiliser, si possible, des témoins rencontrés dans la recherche précédente plutôt qu'acquérir systématiquement32 nouveaux voisins.
- Ensuite seulement, partager la frontière de témoins entre sous-rectangles. Ne pas relancer un parcours à la racine par enfant ; ne pas additionner des crédits dont les populations se recouvrent.

La mesure décisive restante est l'appel q3/q4 combiné sur trames entières, comprenant tous les chemins de repli, les allocations et les atlas. Aucune optimisation de ce lot ne remplace la construction des rattachements silencieux, du catalogue ou de FULL.

## Livrables et incidents

Le dépôt contient la sonde Actions initiale et son lanceur sous `audits/lidar_rectangles_20260922/`. L'archive jointe **`MorseHGP_LiDAR_natif_resultats_2026-09-22.zip`** conserve les extensions locales, le générateur de la copie expérimentale, toutes les captures des essais élargis/représentants/collectifs, les empreintes et `RESULTS_NATIVE_LIDAR.json`. Les chronos diffèrent à la reproduction ; les réponses discrètes doivent coïncider.

Deux essais de chronométrage local ont été interrompus (limite d'appel puis limite180s) et ne sont pas utilisés dans les tableaux. Les captures incomplètes sont conservées ; le dernier contexte a été rejoué entièrement avec limite600s. Ce sont des interruptions de harnais, pas des contre-exemples géométriques. Les builds, reçus et sources du développeur restent inchangés.
