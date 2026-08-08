# Diagnostic direct G4 — RNG, Jung et germination à 50 000 points

> [!IMPORTANT]
> Phase 15. Le parcours de paires emploie `backend=cuda_g4`, `profile=hgp_reduced`, `mode=device_resident_budgeted_morton_yao48_anchor_tiles_scaling_smoke`. La sonde de germination emploie `backend=reference_cpu`, `profile=hgp_reduced`, `mode=guarded_scale_probe_v1`, même lorsqu'elle est lancée sur une machine G4. Les trois artefacts sont `profile_only` ou `profiling_only`; ils ne qualifient ni le produit exact, ni le contrat sous la seconde.

## 1. Verdict immédiat

Le chemin courant ne peut pas tenir le contrat 50 k sous la seconde.

- La frontière paire device complète seule prend 2,395883 s dans son intervalle `frontier_ns`, et 3,927585 s depuis le début froid du processus. Elle précède les supports trois et quatre, leur classification exacte et toute la réduction hiérarchique.
- La germination chronométrée pendant 120 s est un prototype CPU séquentiel. Sur `uniform_latin`, elle ne parcourt que 4 547 839 des 1 249 975 000 paires graines, soit 0,363834 %, et l'arité quatre ne démarre pas.
- Sur `eight_clusters`, 191 paires seulement sont parcourues en 123,987 s. Les 164 paires retenues produisent déjà 1 315 683 tiers classifiés; une paire retenue porte en moyenne 8 022 tiers survivants.

Ces nombres n'accusent pas le silicium Blackwell. Ils révèlent deux architectures inadéquates : une frontière GPU très pilotée par le contrôle et les synchronisations, puis un générateur higher qui n'utilise pas le GPU.

## 2. Provenance et garde GCP

La carte observée est une NVIDIA RTX PRO 6000 Blackwell, 97 887 MiB, compute capability 12.0, pilote 580.173.02 et CUDA 12.9. Le binaire paire existant porte le SHA source `0fdd43351e68f1246f288796816a43d88c8be90d`; les deux sondes de germination proviennent du clone distant propre au SHA `64b6411`. Cette différence de provenance interdit toute comparaison de micro-optimisation entre les exécutables, mais ne change pas leur architecture respective.

La première cible `devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot` a été démarrée par le script gardé avec `SPOT`, `instanceTerminationAction=STOP` et `maxRunDuration=3600`. La VM est revenue `RUNNING` pendant la certification de l'échéance, au lieu de conserver l'état attendu; ce comportement est compatible avec une interruption SPOT mais sa cause n'est pas prouvée par les reçus. Le script a arrêté cette génération et a certifié `TERMINATED` avant tout benchmark. La campagne a ensuite utilisé `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, avec les mêmes gardes et un arrêt invité à 45 minutes. Cette cible a été arrêtée par le script gardé et relue `TERMINATED`. Aucune autre VM étiquetée `project=e-hgp` n'était active. La clé OS Login de session a été révoquée et ses copies temporaires supprimées.

## 3. Frontière paire : où vont les 3,928 secondes

| quantité | valeur |
|---|---:|
| univers de paires | 1 249 975 000 |
| masse candidate | 7 835 403, soit 0,626845 % |
| masse certifiée élaguée | 1 242 139 597, soit 99,373155 % |
| régions de prune | 8 025 397 |
| masse moyenne par région de prune | 154,776 paires |
| temps de frontière | 2 395,883 ms |
| temps total froid | 3 927,585 ms |
| génération du nuage | 79,118 ms |
| canonisation + LBVH | 28,406 ms |
| avances / reprises | 26 / 13 |
| lancements kernel / synchronisations | 40 / 66 |
| contrôles D2H | 103 247 records, 17,345 MB |
| arène device maximale d'une tuile | 1 408,926 MB |

Le parcours ne matérialise pas la matrice de paires et élague bien 99,37 % de sa masse. Mais le coût est gouverné par les objets et les preuves effectivement manipulés, pas par cette masse logique : environ 7,84 millions de candidats et 8,03 millions de régions de prune. Au rang fermé 11, chaque prune cherche 10 témoins dans 48 banques directionnelles. Le kernel affecte un warp à une ancre; plusieurs décisions de feuille et mises à jour de banque restent effectuées par la lane zéro, tandis que le warp coopère sur les intervalles et les témoins.

La capacité de 640 candidats par ancre remplit un chunk dans chacune des 13 tuiles. Chaque tuile est donc rendue à l'hôte puis reprise, ce qui explique 13 `candidate_segment_full`, 13 reprises et 26 avances. Chaque subdivision copie le compteur de paires encore actives puis synchronise; chaque avance copie ensuite tous ses contrôles d'ancre et synchronise à nouveau. Les 40 subdivisions donnent ainsi exactement 66 synchronisations dans cette exécution.

Les 103 247 contrôles contiennent 49 999 ancres utiles et 53 248 contrôles de continuation, exactement 13 fois 4 096. Le lanceur hôte les parcourt une première fois, puis la couche de cycle de vie les recertifie structurellement. L'arène de 1,408926 Go est allouée dans le premier `advance`, donc à l'intérieur de `frontier_ns`. Les segments de capacité sont remis à zéro pour chaque tuile : environ 17,198 Go de capacité cumulée sont ainsi initialisés sur les 13 tuiles, bien que la sortie logique soit beaucoup plus petite. La dernière tuile partielle ne peut pas réutiliser l'arène pleine parce que ses extents diffèrent et provoque un cycle de libération et réallocation. Enfin, 26 appels de télémétrie `cudaMemGetInfo` restent eux aussi dans la fenêtre de frontière.

La différence de 1 531,703 ms entre `total_ns` et `frontier_ns` n'est pas entièrement instrumentée. Les champs temporisés connus hors frontière totalisent 109,359 ms; il reste donc 1 422,344 ms dans l'enveloppe froide. Le premier appel CUDA est un `cudaMemGetInfo` placé avant les chronomètres de construction, de sorte que l'initialisation du contexte est une explication plausible et probablement dominante, mais l'artefact ne la mesure pas séparément. Il serait incorrect d'en faire un pourcentage de bottleneck sans trace Nsight. Même en retirant toute cette enveloppe froide, les 2,396 s de frontière restent au-dessus du contrat.

Deux campagnes antérieures du même composant et du même rang ont mesuré 2,434407 s puis 2,377329 s. La nouvelle valeur s'en écarte respectivement de -1,58 % et +0,78 %. Cette stabilité n'indique ni throttling, ni anomalie d'horloge, ni régression propre à la G4. En revanche, les artefacts ne publient ni puissance, ni fréquence, ni occupation, ni durée par kernel; un diagnostic Roofline demanderait une trace CUDA dédiée.

Cette sonde est en outre optimiste pour le produit : `candidate_device_to_host_count=0`, `exact_diametral_rank_evaluated=false` et `component_only=true`. Elle abandonne les leases de candidats après vérification de leur cycle de vie; elle ne classe pas exactement les 7,84 millions de paires et n'alimente aucune hiérarchie. Un passage qualifié antérieur au même rang ajoutait environ 8,447 s de recertification CPU. La copie et la décision exactes doivent donc être fusionnées au consommateur device, pas ajoutées après ce profil.

## 4. Germination : pourquoi la présence d'une G4 ne change rien

Le binaire lie `morsehgp3d::higher_support`, dont la germination se trouve dans `src/cpu/hierarchy/local_germination.cpp`. Il ne contient ni kernel CUDA, ni OpenMP, ni exécution parallèle. La construction d'une grille uniforme, la boucle des paires, le scan des tiers, les requêtes de population, le calcul exact du centre et les requêtes LBVH fermées sont exécutés sur l'hôte.

| quantité | `uniform_latin` | `eight_clusters` |
|---|---:|---:|
| durée mesurée | 120,007 s | 123,987 s |
| paires examinées | 4 547 839 | 191 |
| débit moyen du préfixe | 37 897 paires/s | 1,540 paire/s |
| paires retenues | 33 176 | 164 |
| tiers examinés dans les lentilles | 4 426 866 | 2 192 238 |
| tiers retenus et classifiés | 2 211 043 | 1 315 683 |
| requêtes de population | 68 485 364 | 13 021 860 |
| débit de classification | 18 424/s | 10 611/s |
| événements acceptés | 19 775 | 9 |

Quatre coûts se composent.

1. La borne tangente est préparée pour chaque point par 40 étapes de dichotomie et 26 directions. Chaque essai interroge la grille uniforme.
2. La boucle graine reste explicitement triangulaire sur toutes les paires. Le filtre tangent arrive en temps constant, mais seulement après avoir chargé et mesuré la paire.
3. Toute paire retenue balaie ensuite les 50 000 points pour construire sa lentille. Sur le préfixe `uniform_latin`, 33 176 paires retenues impliquent à elles seules environ 1,659 milliard d'itérations point--paire, avant les requêtes de segment et la classification.
4. Chaque tiers survivant déclenche une analyse de circoncentre en entiers exacts puis, s'il est minimal, une requête de boule fermée sur le LBVH CPU. Pour l'arité quatre, le code parcourrait ensuite toutes les paires de tiers retenus; cette boucle n'a pas été atteinte à 50 k.

La pathologie `eight_clusters` vient de la densité locale. Une paire retenue contient en moyenne 13 367 tiers dans sa lentille et en conserve 8 022. Elle déclenche environ 79 402 requêtes de population. La grille `uniform_latin` possède 10 114 cellules occupées, environ 4,94 points par cellule et au plus 9; la grille `eight_clusters` ne possède que 18 cellules occupées, environ 2 778 points par cellule et jusqu'à 6 250. `population_exceeds` cesse ses distances après le dépassement de rang, mais `UniformGrid::for_each_candidate` continue à visiter les cellules et à appeler un callback devenu inactif; le cap en $K$ ne termine donc pas physiquement la traversée. Le compteur `population_queries` ne mesure ni cellules ni slots visités et n'est pas comparable seul entre ces familles.

Le préfixe est aussi biaisé par l'ordre canonique. Les 191 paires de `eight_clusters` partent toutes du premier identifiant; 144 relient des amas séparés et créent de grandes lentilles. Il ne s'agit donc pas d'un échantillon aléatoire du coût moyen. La préparation des bornes tangentes n'interroge pas le garde, puis l'horloge n'est relue qu'avant chaque 64e germe et jamais au milieu du travail d'une paire. Les deux comptes terminent d'ailleurs à 63 modulo 64. Cela explique le dépassement du garde de 120 à 123,987 s et interdit d'extrapoler linéairement ce préfixe.

Le débit du préfixe `uniform_latin` donnerait naïvement 9,16 h pour les seules paires de l'arité trois. Ce n'est pas une projection valide de bout en bout : le coût des paires est fortement non uniforme, et un artefact historique de 2 400 s avait atteint 99,92 % de cette boucle. Les deux observations prouvent seulement que le parcours est distribution-dépendant et très au-delà de la seconde.

## 5. Complexité en $n$ et $K$

Notons $r$ le nombre de paires graines retenues, $t_e$ le nombre de tiers retenus pour une paire $e$, $T=\sum_{e}t_e$, $D$ le nombre de directions tangentes, $B$ le nombre d'étapes de dichotomie et $S$ le nombre de positions du segment. Ici $D=26$, $B=40$ et $S=16$.

Le prototype paie au minimum $\Theta(n^{2}+rn+T)$ opérations de parcours, auxquelles s'ajoutent les points visités par les requêtes de population et les classifications exactes. Si une requête de grille ou de LBVH visite $\Theta(n)$ points dans le pire cas, la préparation tangente atteint $O(DBn^{2})$, l'arité trois $O(n^{4})$ et l'arité quatre $O(n^{5})$ avec la boucle actuelle. Ces bornes ne sont pas des complexités de Delaunay d'ordre supérieur : elles proviennent directement des paires graines, des tiers par paire et des paires de tiers. Elles sont précisément ce que la nouvelle source sparse doit supprimer.

La dépendance favorable en $K$ vient des arrêts après $K+1$ témoins et, dans l'arrangement shallow proposé, de la profondeur $\kappa=K_{\mathrm{eff}}-3$. Mais le prototype de grille n'interrompt pas sa traversée physique après le cap, tandis que le frontend GPU exige $K$ témoins et réserve 48 banques de largeur proportionnelle à $K$. À $K$ fixé, Jung et la profondeur peuvent rendre la sortie locale linéaire; ils ne rendent pas spontanément le catalogue global d'ancres linéaire.

## 6. Corrections d'architecture, par ordre d'impact

Pour la frontière paire existante : conserver un processus et un contexte chauds; remplacer les retours hôte de compteur par un scheduler device persistant; remplacer les caps par ancre par un `count--scan--emit` ou un spill pool global déterministe; garder une arène persistante et employer des epochs plutôt que remettre environ 17,2 Go de capacité à zéro; sortir `cudaMemGetInfo` du chemin chaud; transformer les contrôles AoS de 168 octets en compteurs compacts; répartir les décisions aujourd'hui concentrées sur la lane zéro; et surtout fusionner proposition, classification exacte bornée et consommateur afin de ne jamais copier puis recertifier un catalogue de millions de records.

Ces optimisations ne suffisent pas à elles seules. Même en imputant tout le résidu non ventilé à l'initialisation froide et en conservant seulement 109,359 ms de postes connus hors frontière, la frontière devrait tomber sous 890,641 ms pour respecter une seconde : elle doit donc gagner au moins un facteur 2,69 avant même tout aval exact. Le gain structurel doit aussi réduire le nombre d'ancres et d'objets émis.

Pour les supports trois et quatre : remplacer la boucle CPU $\binom{n}{2}$ par une source d'ancres certifiée; construire les voisinages par `count--scan--emit` sur le LBVH device; traiter les petites ancres par warp, les moyennes par bloc et les lourdes par file persistante; puis énumérer les niveaux de profondeur au plus sept dans le disque de Jung au lieu des paires de tiers. Les filtres flottants restent propositionnels et toute ambiguïté doit retomber sur les prédicats exacts.

Le point mathématique encore ouvert est global : ni le RNG fini ni le facteur de Jung ne prouvent une source complète de $O(n\,\mathrm{poly}(K))$ ancres. Tant que cette borne ou un complément fail-open mesuré n'existe pas, le passage à des dizaines de millions de points n'est pas justifié.

## 7. Intégrité des artefacts

| fichier | SHA-256 |
|---|---|
| `pair_frontier_uniform_50000_closed_rank11.json` | `606218484fe95346043a56d6f4c260b3222b1362081bf445586f73ae47ab81e5` |
| `germination_uniform_latin_50000_closed_rank11_120s.json` | `d8bf0bc116b124386712b2792945833e5dd7d19a9d974ce7613559e905c439d0` |
| `germination_eight_clusters_50000_closed_rank11_120s.json` | `db52aa9f0190e0315319cd041f9ff91dccd1e30da4b716a37657b340b7d2061f` |

Les deux artefacts de germination sont au schéma historique v1. Leur slot arité quatre non exécuté contient le placeholder erroné `completeness_guaranteed=true`; sa base vide, son arité nulle et ses compteurs nuls le rendent non certifiant. Le schéma v2 ajoute `applicable`, `executed` et `floating_rejections_certified`. Dans l'implémentation actuelle, ce dernier champ reste faux et toute chaîne de reprise reste non scellable jusqu'à disposer d'un curseur contigu, d'une identité entre audit et payload, et de rejets flottants à intervalles extérieurs avec repli exact.

La sonde paire embarque son SHA Git. Les JSON v1 de germination n'embarquent ni SHA Git, ni digest du binaire, ni options de compilation, ni identité de machine; leur provenance G4 dépend donc de ce rapport de session et n'est pas autonome. La prochaine sonde v2 devra sérialiser ces champs ainsi que les visites physiques de cellules ou de points, actuellement absentes des compteurs.
