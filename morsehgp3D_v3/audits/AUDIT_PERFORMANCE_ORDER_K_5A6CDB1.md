# Audit des mesures `order_k` et du claim G4 — commit `5a6cdb1`

> **Verdict : aucune mesure observée n'est un reçu qualifiant pour le contrat 50 k / K=10 / 1 s.** Les nombres publiés sont utiles comme diagnostic de complexité : ils montrent des centaines de sommets d'arrangement visités par point et jusqu'à 1,34 milliard de candidats dès 500 points. Ils ne mesurent ni le catalogue complet, ni les forêts, ni une entrée LiDAR réelle. De vraies exécutions G4 sont visibles dans le journal local de la session, mais le sweep principal lance onze processus CPU concurrents et ne conserve aucun reçu gardé liant résultat, binaire et charge machine. Les claims du message de commit doivent donc rester `diagnostic_only`.

## 1. Claim audité et empreintes disponibles

Le message du commit `5a6cdb1` affirme notamment :

```text
The judge returns identical numbers, and the LiDAR cloud that stopped the
traversal now completes: 674.9 vertices per point at s_max = 11, against 676.1
for a uniform cloud -- the surface profile is not the harder one here.
```

Le commit complet est `5a6cdb1af030a264ce07adddd312be2c458459b4`. Son header `order_k` a le SHA-256 `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457`.

Les artefacts scratch encore lisibles au moment de l'audit étaient :

| artefact | SHA-256 | horodatage UTC |
|---|---|---|
| sortie locale initiale `b29brrvq1.output` | `8c08af104c19aa3ce110b853179baf0143d98a08da240da16fe9de88da6944c8` | 02:41:10 |
| campagne locale puis copie de `b29`, `bzfgr8ect.output` | `32ab56cd7a37fae022a9571b70dcf7bc2f208fd9d41bd0efea818bebfd2265a6` | 02:53:44 |
| générateur `cloud.hpp` | `5a887891cad03f52c3687d8f13f3a7df466cd840bbd32983344ce347e6c4ca39` | 01:41:26 |
| header du commit, contenu de `mhgp3v.tgz` | `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457` | 03:03:42 |
| archive de sources `mhgp3v.tgz` | `dfe4d58117a52f42e5e721b55b17a99e2676b03d9962d36931688f40f7e6ce39` | 03:03:42 |
| driver de parcours G4 `measure.cpp`, extrait de `drv.tgz` | `173e6959d0669b33cdbe1aedeb1e4aeac97eb0394eb68b9733e0c07294bf1891` | archive réécrite à 03:05:06 |
| header live optimisé | `68c14c212703453eb806675f11b74b259f133c63010d7f2b97fc36c1a7c1b6f1` | après le commit |
| driver live avec census critique | `1ad4e17c706662f08817637cbd9994f63ace92a2d5f76dfc8fa0424afb10c7d8` | 03:06:58 |
| binaire local live `measure` | `d7b6982910cf53e934b4816e041c44df9508bfe8feeaaaeac26346e41f464854` | 03:07:01 |

Le journal de session local `/home/codespace/.claude/projects/-workspaces-E-HGP/b0b21e98-d1d6-4b5f-b1d4-80d8b8ff54ab.jsonl` fournit la chaîne de commandes. Son préfixe jusqu'à la ligne 2 073 avait le SHA-256 `5d0b43a4ce4a763d8015e000ed80f35da8b10cdc8a16bd6b9422bce0facac7c5`; les seules lignes probantes 1 896, 1 936, 2 043, 2 047, 2 057, 2 059, 2 061, 2 072 et 2 073 avaient ensemble le SHA-256 `ce14036a93c6b8f65ad547eb507ad9fafc9ca4a0f1fe1a102bf59376e2f3fdf2`. Cette empreinte de préfixe reste vérifiable même si Claude ajoute ensuite des lignes au journal.

La ligne 1 896 lie `b29` à une compilation **locale** `g++ -O2`, suivie de l'exécution séquentielle `$S/measure`. La ligne 1 936 lie `bzf` à une autre campagne **locale** de l'oracle suivie explicitement de `cat .../b29brrvq1.output`. Les lignes 2 047 et 2 057 lient, elles, le vrai sweep G4 au header archivé `c1548b3...`, au driver `173e6959...`, aux flags `-O3 -march=native` et à la graine 99. L'archive `drv.tgz` lisible a été réécrite après le lancement pour corriger `bfs_check.cpp`; son `measure.cpp` est resté identique, mais son hash d'archive ne doit pas être présenté comme celui du tar envoyé. Aucun hash du binaire distant n'est conservé.

Les archives de 03:03 et 03:05 sont postérieures aux sorties locales initiales de 02:41 et 02:53. Elles ne scellent donc pas rétroactivement ces binaires. En revanche, le journal établit que l'archive de sources `dfe4d5...` a été créée puis envoyée avant le sweep G4 post-commit.

Audit strictement en lecture : aucun processus concurrent n'a été arrêté, aucune VM n'a été démarrée, interrogée ou modifiée, et tous les calculs auxiliaires ont été faits sous `/tmp`.

## 2. Ce que la sortie mesure réellement

La sortie initiale contient exactement :

```text
profil=lidar n=250 s_max=11 | supports4=149793 (599.17 par point) |
  requetes=1198344 candidats=294784870 | 66.27 s
profil=uniform n=500 s_max=11 | supports4=338046 (676.09 par point) |
  requetes=2704368 candidats=1341366528 | 216.03 s
```

Le driver archivé appelle uniquement `order_k_vertices`, chronomètre cet appel, puis construit un histogramme. Il n'appelle pas `order_k_catalogue` ni `build_forest`. Le temps exclut donc :

- la récolte et la déduplication des paires et triangles ;
- le calcul de miniboule pour chaque candidat récolté ;
- le census global des membres de chaque sphère critique ;
- la normalisation du catalogue et de son pool ;
- toutes les forêts d'ordres 1 à 10.

Il passe en outre directement le plafond 11 à `order_k_vertices`, tandis que `order_k_catalogue(points, 11, ...)` appelle actuellement le parcours avec le plafond 13 afin de récolter les arités basses. Même le coût de navigation nécessaire au catalogue proposé est donc sous-mesuré par ces lignes.

Le champ nommé `supports4` vaut simplement `v.size()`. Dans le snapshot à coquille variable, ce ne sont même plus nécessairement des supports de cardinalité quatre. Ce sont tous les sommets d'arrangement visités, critiques ou non. Une mesure locale ultérieure sur le même nuage synthétique LiDAR à 250 points a compté seulement 1 430 coquilles critiques parmi 149 793 sommets, soit environ 1 %. Elle a été faite sur le delta live postérieur `68c14c2...`, pas sur le commit scellé ; elle confirme seulement la nature du compteur, pas une performance du commit.

Les identités des compteurs sont elles-mêmes informatives. Pour une coquille simple de quatre points, ses quatre triples donnent deux directions chacun, donc exactement huit requêtes. Si aucun point hors coquille n'est coplanaire avec l'un de ces triples, chaque requête compte exactement $n-4$ candidats. Pour $V$ sommets de ce type, on obtient donc exactement `pencil_queries = 8V` et `pencil_candidates = 8(n-4)V`.

La ligne uniforme vérifie l'identité au candidat près : $8(500-4)338046=1341366528$. Pour LiDAR 250, $8(250-4)149793=294792624$, soit 7 754 de plus que les 294 784 870 comptés. Le code n'incrémente pas le compteur lorsque `orient(z) == 0`; ce déficit certifie donc des incidences coplanaires dans ce nuage quantifié. Il ne compte pas non plus toute la seconde passe du commit `c1548b3...`, ni la seconde boucle d'orientations coplanaires qui subsiste dans le delta `68c14c2...`. Le nombre `pencil_candidates` est une bonne identité de la boucle principale, pas un décompte complet de tous les prédicats exécutés.

## 3. Le profil « LiDAR » est synthétique et la comparaison reste exploratoire

`cloud.hpp` ne charge aucun scan. Il génère une surface procédurale par sinus et cosinus, ajoute trois biais de recalage et du bruit gaussien, puis quantifie sur u16. L'emprise croît comme la racine de `n` avec un pas fixé à 25 unités. Ce générateur est utile pour un stress test reproductible ; il ne justifie pas le mot « nuage LiDAR réel » ni une conclusion sur la distribution produit.

Une seule graine est utilisée par taille, `99+n`, sans répétition, intervalle de confiance ou permutation. La sortie locale initiale `b29` compare certes LiDAR à 250 points et uniforme à 500 points, mais le nombre annoncé existe bien ailleurs : la ligne 2 043 du journal conserve LiDAR à 500 points sous le header `c1548b3...`, avec 337 429 sommets, soit 674,86 par point, et 213,63 secondes. Le nombre uniforme 676,09 provient d'une passe locale antérieure ; un driver G4 ultérieur sur le même header retrouve ensuite 338 046 sommets uniformes à 500 points.

Les tailles sont donc appariables a posteriori pour le **compteur de sommets**, mais pas par un benchmark qualifiant unique et isolé au moment du commit. Surtout, ce compteur n'est pas le catalogue critique : sur le même snapshot archivé et à 500 points, le driver de localité G4 compte 3 661 coquilles critiques pour le profil LiDAR contre 59 152 pour l'uniforme, malgré 337 429 contre 338 046 sommets visités. La phrase « le profil de surface n'est pas le plus difficile » ne peut porter que sur le volume de navigation observé, pas sur le volume du catalogue ou le temps produit. Elle demanderait plusieurs graines appariées, les mêmes hashes d'entrée et de binaire, et une distribution des temps et des compteurs.

## 4. Faux artefact G4, vraies passes G4 concurrentes

`bzfgr8ect.output` contient :

```text
Terminated
=== mesure ===
profil=lidar n=250 ... 66.27 s ...
profil=uniform n=500 ... 216.03 s ...
```

Les deux lignes après le séparateur sont identiques caractère pour caractère aux deux lignes de `b29brrvq1.output`, jusqu'aux centièmes de seconde. Ce n'est pas une ambiguïté : la commande locale de la ligne 1 936 exécute un oracle puis fait explicitement `cat` de `b29`. `bzfgr8ect.output` ne doit donc pas être qualifié de trace G4. Le mot `Terminated` concerne la commande locale longue ; il ne porte aucune information sur une VM.

De vraies exécutions distantes sont néanmoins liées par le journal à la cible nommée `ehgp-blackwell-spot-ai1a` en zone `europe-west4-ai1a`. Une première commande rapporte `nproc = 48` et GCC 11.4. Le sweep post-commit envoie l'archive au même nom de cible, compile avec `-O3 -march=native`, puis lance simultanément cinq tailles LiDAR au plafond 11, deux tailles uniformes au plafond 11 et quatre runs LiDAR à 2 000 points avec des plafonds différents : **onze processus `nohup` concurrents**.

Les lignes G4 post-commit observées incluent :

| profil | $n$ | plafond | sommets | candidats | temps wall du processus concurrent |
|---|---:|---:|---:|---:|---:|
| LiDAR synthétique | 500 | 11 | 337 429 | 1 338 913 620 | 75,15 s |
| LiDAR synthétique | 1 000 | 11 | 718 782 | 5 727 231 440 | 320,41 s |
| uniforme | 1 000 | 11 | 723 950 | 5 768 433 600 | 321,15 s |
| LiDAR synthétique | 2 000 | 9 | 695 555 | 11 106 574 124 | 620,68 s |

Ces lignes prouvent une exécution distante dans la chaîne de commandes observée ; elles ne constituent pas un benchmark isolé. Chaque driver reste mono-thread et sans CUDA, plusieurs processus se partagent les 48 CPU, aucune affinité, charge par cœur, répétition ou chauffe n'est enregistrée, et aucun hash de binaire distant n'accompagne les résultats. Le GPU n'est pas utilisé. Le journal montre aussi un démarrage par le script gardé `start_and_verify.sh`; l'audit n'a interrogé ni la VM ni son état et ne transforme pas les traces de sécurité de la session en reçu de performance. Il ne conclut donc rien ici sur l'arrêt final de la cible.

La comparaison correcte est ainsi : local `c1548b3...` à 500 points LiDAR, 213,63 s ; G4 `c1548b3...` à 500 points LiDAR, 75,15 s au milieu du sweep concurrent ; local live `68c14c2...` à 250 points LiDAR, 29,60 s. Ces trois temps ne forment pas un speedup contrôlé : matériel, flags, hash et concurrence changent.

## 5. Les nombres observés réfutent déjà le contrat temporel courant

À 500 points, le seul parcours uniforme paie 1 341 366 528 candidats et 216,03 secondes sur la sortie locale initiale. Le débit apparent vaut environ 6,21 millions de candidats par seconde. À 250 points LiDAR, il vaut environ 4,45 millions par seconde ; l'écart montre qu'un débit constant ne peut pas être présumé sans protocole isolé.

La passe initiale s'est en outre achevée sur un Codespace à deux vCPU Xeon Platinum 8370C et environ 7,8 Gio, pendant qu'un long `mhgp3v_oracle` et d'autres tâches d'agents étaient actifs. Il n'existe ni affinité, ni inventaire de charge, ni répétition isolée. Les 66,27 et 216,03 secondes décrivent donc les conditions observées de cette passe, pas un débit de référence machine.

Le ratio de sommets par point ne diminue pas dans les observations à plafond 11 : il atteint même environ 719 à 724 à 1 000 points. Une extrapolation **diagnostique, non une borne asymptotique**, qui conserverait 674,9 sommets visités par point à 50 000 points donnerait 33 745 000 sommets et exactement $8(50000-4)33745000=13496920160000$ candidats sous les hypothèses de l'identité simple. Au débit le plus favorable des deux sorties locales initiales, cela représente 25,16 jours sur un cœur, ou 12,58 heures sous l'hypothèse irréaliste d'une accélération parfaite par 48 cœurs. Même en injectant le débit local live de 29,60 secondes, qui appartient à un autre hash, on obtient encore 15,69 jours sur un cœur et 7,84 heures sous le même idéal à 48 cœurs. Cette extrapolation ne prédit pas le produit ; elle montre seulement que les données publiées ne soutiennent en aucun cas une seconde.

Une optimisation live postérieure a fusionné le choix du meilleur événement et le lot non coplanaire. Sur le cas LiDAR 250, le même compteur de 294 784 870 candidats et les mêmes 149 793 sommets sont passés de 66,27 à 29,60 secondes. Cette dernière ligne est **locale**, compilée en `-O2`, avec le header `68c14c2...` et le driver `1ad4e17...`; ce n'est pas une mesure G4. Le gain de constante est utile comme diagnostic, mais les runs ne sont ni répétés ni isolés. Le commentaire live « un seul balayage » est en outre littéralement faux : une deuxième boucle en $O(n)$ subsiste pour tester les points coplanaires. L'optimisation retire surtout les comparaisons `compare_t` coûteuses de cette seconde boucle ; elle ne change ni les omissions mathématiques ni le facteur global en $nV$.

## 6. « The judge returns identical numbers » n'est pas scellé

Le commit n'embarque aucun reçu de juge pour `order_k`. Le CMake courant n'enregistre aucun test avec `--subject order_k`. L'oracle `927809...` conserve en outre l'ancien domaine : il rejette toute coquille surnuméraire alors que le nouveau sujet prétend la traiter. Une campagne indépendante de 40 petits nuages sort avec dix désaccords de domaine et une identité non fermée.

Le driver scratch `bfs_check.cpp` n'est pas un remplacement : sa « vérité brute » appelle le même `bfs::in_sphere_side` que le sujet, puis rejette toute cosphéricité et toute coquille de taille différente de quatre. Il ne peut donc ni détecter une inversion partagée de prédicat, ni juger la correction qui motive précisément le commit.

Les nombres de sommets identiques avant et après un refactoring prouvent au mieux la stabilité de ce compteur sur une entrée. Ils ne certifient ni le catalogue, ni le nouveau domaine dégénéré, ni les forêts.

## 7. Reçu minimal avant toute promotion performance

Un reçu qualifiant doit être produit par un script gardé et contenir au minimum :

1. commit, état du worktree, hashes du header, du driver, du générateur, du binaire et du nuage brut ;
2. commande complète, graine, `n`, `s_max`, compilateur et flags ;
3. machine, CPU/GPU effectivement utilisés, threads, affinité, mémoire, charge concurrente et statut de chauffe ;
4. répétitions appariées par profil, médiane, dispersion, temps wall/user/sys et pic RSS ;
5. compteurs qui couvrent toutes les passes réelles, histogramme des tailles de coquille, sommets critiques, récolte, censuses et taille finale du catalogue ;
6. temps séparés du parcours, du catalogue complet et des dix forêts, puis temps total contractuel ;
7. porte d'exactitude verte sur le même hash avant que le temps puisse être qualifiant ;
8. pour GCP, reçus gardés de démarrage et d'arrêt de la cible exacte, avec les deux coupe-circuits certifiés.

Décision : conserver 599,17, 674,9 et 676,09 comme **mesures exploratoires de taille de parcours**. Ne pas les utiliser comme preuve de complétude, de représentativité LiDAR, de performance G4 isolée ou de respect du contrat 50 k.

GCP non utilisé par cet audit.
