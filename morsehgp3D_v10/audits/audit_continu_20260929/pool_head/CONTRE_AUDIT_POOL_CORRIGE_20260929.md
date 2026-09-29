# Contre-audit courant du pool corrigé

29 septembre 2026, clôture locale à 20:40 UTC. `public_status=not_claimed`.

**La copie corrigée traite les défauts observés ; une nouvelle preuve couvre maintenant la création partielle des fils. Le pool du checkout reste ancien à la clôture de cette capture.** Il ne faut donc ni laisser entendre que la copie est encore manifestement fautive sur ces cas, ni fermer le défaut intégré avant publication et qualification du correctif.

## Périmètre et coordination

Cette note complète le suivi local `audits/SUIVI_AUDIT_INDEPENDANT.md`, lu dans son état du 29 septembre à 20:32 UTC. Ses quatre cas d'exception appelant/ouvrier, directs/imbriqués, ne sont pas rejoués ici : son reçu `receipts/audit_independant_20260929/contre_pool_preintegration/receipt.json` couvre déjà quiescence, durée de vie du callback, restauration TLS et réutilisation, en Release et ASan/UBSan. Ces fichiers de l'autre auditeur existent dans le worktree mais ne sont pas encore publiés à `origin/main=c5015a570` à cette clôture ; ils restent sous son contrôle. Les nouvelles preuves ci-dessous sont conservées indépendamment dans nos propres reçus.

Copie corrigée relue : `build/v10-fixes/pool/src/morsehgp3D_v10/src/sched/pool.cpp`, SHA-256 `db59f6c763dff698e92537340e8bed1cbb18344f343ac763c97a6189900698cf`. Notre petit exécutable est compilé depuis le snapshot indépendant de même empreinte, pas depuis une source mutable pendant la compilation. Le [reçu fermé](../../../receipts/audit_continu_20260929/pool_corrected/receipt.json) conserve les commandes, retours, durées, empreintes avant/après et empreinte du nouvel exécutable. Tous les fichiers surveillés sont inchangés à la fermeture ; le [contrôle final du nouvel exécutable](../../../receipts/audit_continu_20260929/pool_corrected/binary_closure.json) confirme aussi son identité après exécution.

Le `src/sched/pool.cpp` du checkout `build/v9-open-worktree/morsehgp3D_v10` a encore l'empreinte `7eecd6757c03fa8184fbcee7a55b26ef312c1df16acac4241e6917328975d72a` dans cette capture. Ceci distingue matériellement la copie corrigée du moteur intégré ; aucun état Git partagé n'a été modifié ni utilisé comme substitut à cette vérification.

## Lecture du correctif

- `pool.cpp:13–22` restaure le drapeau TLS par garde de portée ; il ne reste pas levé après une exception.
- `pool.cpp:27–47` intercepte un échec dans la boucle de création, annonce l'arrêt, réveille et joint les fils déjà créés, puis relance l'exception. Une allocation échouant dans `reserve` précède toute création de fil.
- `pool.cpp:52–63` capture l'exception dans le travail et annule les nouvelles tranches. Les tranches déjà attribuées peuvent se terminer ; ce n'est pas une interruption forcée de leur callback.
- `pool.cpp:69–81,100–116` inscrit chaque utilisateur sous verrou, ferme l'accès au travail et attend tous les utilisateurs avant de relancer l'exception. Le callback et le descripteur local restent donc vivants pendant leur utilisation. L'unique écriture de l'exception est synchronisée avec le retour par cette jonction logique sous mutex ; aucun nouveau défaut de durée de vie n'a été identifié à la lecture.

Ces arguments valent pour le contrat d'un appel externe à la fois. Ils ne prétendent pas rendre sûrs des appels externes concurrents au même pool, ni une destruction du pool pendant son utilisation.

## Nouvelle preuve : création partielle

La [sonde indépendante](../../../receipts/audit_continu_20260929/pool_corrected/thread_create_failure.cpp) interpose `pthread_create` et `pthread_join` dans son propre exécutable. Elle force réellement un retour `EAGAIN` au premier, deuxième ou troisième appel de création d'un `Pool(4)`, sans modifier le moteur. Elle attend `std::system_error(EAGAIN)`, compte les fils effectivement créés et joints avant propagation, puis crée un nouveau `Pool(3)` et vérifie 1 000 unités de travail et le drapeau TLS.

| Échec injecté | Fils créés avant échec | Fils joints avant propagation | Réutilisation après échec | Résultat |
| --- | ---: | ---: | --- | --- |
| Premier `pthread_create` | 0 | 0 | Nouveau pool, 1 000 unités | Code 0 |
| Deuxième `pthread_create` | 1 | 1 | Nouveau pool, 1 000 unités | Code 0 |
| Troisième `pthread_create` | 2 | 2 | Nouveau pool, 1 000 unités | Code 0 |

Les sorties complètes sont conservées dans [le premier cas](../../../receipts/audit_continu_20260929/pool_corrected/thread_create_1.stdout), [le deuxième](../../../receipts/audit_continu_20260929/pool_corrected/thread_create_2.stdout) et [le troisième](../../../receipts/audit_continu_20260929/pool_corrected/thread_create_3.stdout). Aucun stderr, aucun dépassement de temps. La compilation ne porte que sur la sonde et `pool.cpp` ; les trois exécutions prennent chacune moins de 14 ms sur cette machine partagée. Ces durées ne sont pas un benchmark produit.

Le binaire existant du développeur `build/v10-fixes/pool/build/mhgp10_fault pool_construction` a aussi été exécuté : [quatre allocations, quatre échecs injectés, `fault_ok`](../../../receipts/audit_continu_20260929/pool_corrected/allocation_construction.stdout), code 0 en 56 ms. Ce contrôle d'allocation complète, mais ne remplace pas, le test `pthread_create` : le refus système de créer un fil n'est pas une panne d'allocation C++.

Cette nouvelle capture est Release seulement. Elle ne qualifie pas ces trois injections sous ASan/UBSan ou TSan. Le harnais mesure une défaillance à chaque position de création pour trois ouvriers, pas toutes les plates-formes ni toutes les défaillances possibles de synchronisation.

## Ce qui reste réellement ouvert

| Sujet | Statut borné par les preuves disponibles |
| --- | --- |
| Exceptions du callback, quiescence, TLS, réutilisation | Copie corrigée contre-vérifiée par l'autre auditeur en Release et ASan/UBSan ; aucun nouveau défaut trouvé ici. |
| Allocation pendant construction | Nouvelle exécution courte du gate existant : quatre échecs injectés passent. |
| Création partielle refusée par le système | Nouvelle preuve indépendante : refus après 0, 1 ou 2 fils, tous joints, puis nouveau pool opérationnel. |
| Conversion d'un `bad_alloc` en statut catalogue/tour | Les wrappers sont présents dans la copie corrigée (`generator.cpp:882–888`, `tower.cpp:1769–1775`). Le gate `fault_main.cpp` existe ; son balayage des consommateurs n'est pas exécuté dans ce contre-audit court. Qualification indépendante encore à fermer. |
| Autres exceptions | Le pool relance après quiescence. Les wrappers ci-dessus n'interceptent que `std::bad_alloc` ; ils ne constituent pas une promesse de conversion de toute exception système en statut. La construction du pool est une frontière distincte. |
| TSan corrigé | Aucun nouveau reçu ni processus TSan de la copie corrigée observé à ce contrôle. Ce n'est pas une preuve qu'aucun TSan n'a jamais été lancé. |
| Publication/intégration | La capture observe encore le hash ancien dans le checkout. Recontrôler sources intégrées et gates après publication ; ne pas transférer automatiquement la qualification de la copie. |

L'[observation des processus](../../../receipts/audit_continu_20260929/pool_corrected/process_observation.json) distingue les campagnes : les processus initialement observés sur `unit_old_asan pool_flag_restored` ont disparu ; une nouvelle chaîne `502711 → 502717 → 505294 → 505296` exécutait encore cette même porte sur **l'ancien cœur**, avec un délai maximal de 200 s. Il s'agit de vérifier que les nouvelles portes rejettent l'ancien défaut, pas d'une qualification TSan du correctif. Aucun de ces processus n'a été interrompu par cet audit.

Pour la tête, conserver le statut précis de l'autre auditeur dans sa note locale `audits/audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md` : H1/H2/H4 ciblés sur copie, H3 encore reproduit avec des niveaux positifs minuscules. Cette nouvelle preuve sur le pool ne ferme ni H3, ni les rangs/plateaux de la tête, ni les contrats de tour ou de performance. Aucun test GCP, aucune campagne longue, aucune source moteur ou autre audit modifié.
