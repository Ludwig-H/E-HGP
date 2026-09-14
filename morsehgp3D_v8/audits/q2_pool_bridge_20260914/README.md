# Pool terminal : raccord au census global sans copie de nuage

14 septembre 2026 — auditeur A, sur les sources publiées **e3af11a7**.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Cette expérience répond à la question constructeur sur le partage du
plan local et le census des plages restantes. Elle poursuit les mesures
de [sélectivité de B](../CREDITS_TERMINAUX_20260914.md) en payant cette fois
le census et les supports complets. Le prototype reste dans cet audit ;
le code produit et la tranche conjointe en chantier ne sont pas modifiés.

**Le raccord Pool/paires mérite d'être porté en premier :** son gain
q2 complet est confirmé sur les amas, et visible sur LiDAR à 50k.
Le partage local des survivantes est correct mais n'apporte ici qu'un
gain de travail limité après le filtre. Le coût dominant reste à traiter.

## Raccord réalisé et comparaison

[node_pool.hpp](node_pool.hpp) adapte explicitement le Pool P0 publié
aux vues des nœuds spatiaux : mêmes propositions et prédicats stricts,
départage par ID original, aucune copie de coordonnées, nouvelle factory
de nuage, validation globale, préparation Axis ou expansion A×B rejeté.
Le regroupement est stable dans l'ordre spatial, pas l'ordre des IDs.
Les candidates peuvent différer du harnais B, qui renumérote les
facteurs ; les supports finaux doivent rester identiques.

[bridge.hpp](bridge.hpp) inclut le fichier census **inchangé**, extrait
de Git, pour accéder à son moteur interne dans une seule unité de
compilation. Cette couture d'audit ne constitue pas une nouvelle API
produit. Un seul propriétaire et un seul index global servent tout le front.

Les trois bras utilisent Samples, q2 seul, Kmax10 :

- **Baseline** : parcours publié Complement/sibling, pour tous les rectangles.
- **Pool/paires** : Pool quand `max(|A|,|B|)>=64`, puis census individuel des seules survivantes ; les autres rectangles gardent la baseline.
- **Pool/partagé** : même filtre, puis arbre B local et couvertures de préfixes communes aux ancres de même crédit ; autres rectangles inchangés.

Le petit facteur reste A. Après crédits i de A et j de B, garder
i+j<K revient à **un préfixe de B par classe A**, soit au plus K bandes,
au lieu d'énumérer les K(K+1)/2 couples de classes. L'arbre local est
construit seulement jusqu'au plus long préfixe utile. Les couvertures
de ces préfixes sont calculées une fois par classe, puis réutilisées.
Le bras individuel ne construit ni cet arbre ni ces couvertures.

Les racines locales commencent toutes avec **compte zéro sur tout Z**.
En partagé, le B original différé et l'échappement restent les vrais
nœuds globaux, y compris les sites exclus du résidu. La permutation
locale sert uniquement aux requêtes. Le certificat frère est désactivé
sur cet arbre local : son lookup publié attend un nœud global. Toute
la coquille et tous les intérieurs sont collectés sur le nuage global.

## Résultats

36 appels, 33 configurations : trois scans LiDAR déjà préparés à 8k,
scan0 à 16k/32k/50k, s8 ; scan0/8k à s10/12 ; amas seed3 à
8k/16k/32k, s8. Répétition 50k avec ordre inversé des trois bras.
Un seul CPU fixé par affinité, hôte partagé, pas de warmup ni de
distribution statistique. Les temps sont des observations appariées.
Aucun nouveau téléchargement ni hypothèse d'alignement des points.

Temps **total** en secondes, premier passage à s8 :

| Entrée | Baseline | Pool/paires | Pool/partagé |
|---|---:|---:|---:|
| LiDAR scan0, 8k | 1,658 | 1,626 | 1,616 |
| LiDAR scan100, 8k | 1,339 | 1,374 | 1,392 |
| LiDAR scan200, 8k | 1,498 | 1,521 | 1,479 |
| LiDAR scan0, 16k | 3,639 | 3,465 | 3,530 |
| LiDAR scan0, 32k | 7,585 | 6,827 | 6,824 |
| LiDAR scan0, 50k | 13,175 | 10,899 | 10,929 |
| Amas, 8k | 13,249 | 3,062 | 3,134 |
| Amas, 16k | 53,761 | 8,504 | 8,449 |
| Amas, 32k | 204,690 | 21,901 | 21,973 |

La répétition LiDAR50k donne, dans le même ordre de colonnes,
13,080 / 10,548 / 10,722 s. Pool/paires retire donc **17,3 % puis
19,4 %** du temps dans ces deux captures ; ses visites géométriques
baissent de 30,54 %, à travail discret identique entre répétitions.
À 8k, les signes temporels varient et la masse ciblée est faible :
3 807 / 7 185 / 68 946 paires, soit 0,195 / 0,432 / 3,531 % du
résidu des trois scans. À 50k, elle représente 33,06 % du résidu.
Une décision fondée uniquement sur le pilote 8k aurait manqué ce régime.
À s10/12, le scan0/8k donne respectivement 1,627/1,685 s en baseline,
1,610/1,683 en Pool/paires, 1,623/1,668 en partagé ; aucun optimum
universel de s n'est établi.

Travail payé dans les rectangles sélectionnés :

| Entrée | Rectangles | S, sites de facteurs relus | Paires avant → après Pool | Préparation Pool/paires | Callback sélectionné, census et sortie inclus |
|---|---:|---:|---:|---:|---:|
| LiDAR50k | 1 536 | 215 776 | 6 552 439 → 109 063 | 17,38 ms | 90,18 ms |
| Amas8k | 28 | 56 000 | 27 995 740 → 11 329 | 3,71 ms | 26,80 ms |
| Amas16k | 28 | 112 000 | 111 997 058 → 29 688 | 8,30 ms | 64,07 ms |
| Amas32k | 28 | 224 000 | 447 996 847 → 102 336 | 15,18 ms | 190,19 ms |

Les résidus 16k/32k diffèrent légèrement de B (29 878 / 102 993),
dont le départage utilise des IDs renumérotés localement. Ici les douze
baselines retrouvent exactement le travail et les empreintes publiés,
et tous les bras appariés conservent les **mêmes supports complets**.
La comparaison de temps aux seules préparations trois voies de B
n'isolerait pas l'effet du partage du nuage.

Sur LiDAR50k, le partagé construit au total 17 813 feuilles B utiles
et 34 090 nœuds, puis 24 991 nœuds de couverture réutilisés pour
32 655 racines, au lieu de 109 063 racines individuelles. Cela coûte
1,92 ms de construction/couverture. Les visites géométriques globales
font 451 981 906 en baseline, 313 934 108 en Pool/paires et
310 629 105 en partagé. Les 2 158 281 certifications Pool, 1 643 634
comparaisons de sélection, 621 569 décalages logiques et les parcours
de facteurs/regroupement sont comptés séparément. Moins de racines
locales n'a donc pas donné un gain net supplémentaire stable sur le total.

Sur les amas, les doublements des visites du census passent de
**×4,106 / ×4,229 à ×2,958 / ×2,701** en Pool/paires. La somme des
facteurs sélectionnés double et les résidus totaux valent
1 743 978 / 4 786 146 / 12 184 055. C'est une croissance observée
améliorée, pas une borne asymptotique globale ni une fermeture de P0.
Après filtrage, le callback des gros rectangles pèse moins de 1 %
du temps enregistré, sur LiDAR50k comme amas32k : optimiser encore
leurs survivantes ne suffira pas. Front, nombreux petits rectangles
et sorties hors de ces callbacks portent désormais l'essentiel.

Campagnes closes : [pilote](campaign_pilot/COMPLETION.json),
[croissance LiDAR](campaign_growth/COMPLETION.json),
[50k](campaign_check50k/COMPLETION.json),
[répétition inversée](campaign_repeat50k/COMPLETION.json),
[amas](campaign_clusters/COMPLETION.json),
[s10/12](campaign_separation/COMPLETION.json).

## Preuve utile et objets pour les prochains jobs

Pour une classe A de crédit i, le préfixe B contient exactement les
classes j<K−i. Les classes A sont disjointes ; une couverture par nœuds
de **son propre arbre B** partitionne chaque préfixe. Les racines ainsi
obtenues couvrent exactement le résidu, sans doublon. Leurs parcours Z
partent de zéro : les mêmes témoins locaux peuvent être revus sans
double crédit. Lors d'un raffinement de requête, le compte et le
curseur Z sont transmis selon l'invariant déjà prouvé du census.

Une fixture non colinéaire rend les deux pièges concrets : IDs
0=(100,0,0), 1=(0,1,0), 2=(1,0,1), A={0}, B={1,2}. L'ordre spatial
est [1,2,0] ; le Pool trie B en [2,1]. À K1, son préfixe de longueur un
doit conserver (0,2), seule paire cross admissible. L'interpréter dans
l'ordre global sélectionne (0,1), dont le site 2 est intérieur :
H(0,1,2)=98, tandis que H(0,2,1)=−101. À K2, précharger le crédit un
puis recompter le même site supprimerait à tort (0,1).

Pour des jobs asynchrones, le prochain objet concret est un **contexte
immuable partagé** : identité d'index, seuil, nœuds A/B original, plan,
permutation B, arbre local et couvertures par classe. Chaque job porte
le contexte, le rang global d'ancre, la requête locale, la phase, le
curseur Z et le compte ; bornes et buffers de sortie restent propres
au worker. Répartir des plages du plan parent conserve ses crédits et
ne rescane pas B pour chaque job.

Le prototype actuel est synchrone : plan, couvertures et OrderContext
vivent dans le callback, et les vues de l'engine y changent temporairement.
Il ne faut pas les envoyer tels quels dans une file asynchrone. Les
entiers de nœuds ne certifient pas leur provenance ; le callback du
même index établit ici cette relation. Un futur handle doit l'encapsuler.

Avec S=Σ_r(|A_r|+|B_r|) sur les seuls rectangles filtrés, le Pool coûte
O(KS), puis son regroupement O(S+KR). Si m_r est le plus long préfixe B
utile, les arbres coûtent O(Σ_r m_r), leurs couvertures mises en cache
O(Σ_r K log(1+m_r)), et les activations au plus
O(Σ_r |A_r| log(1+m_r)). Ajouter toutes les visites du census et les
sorties. La partition des paires donne S≤n(n−1), puisque
|A|+|B|≤2|A||B|, mais **aucune garantie sous-quadratique pour S**.
Pour les 28 produits inter-amas, S=7n est une propriété de cette fixture.
La mémoire parallèle sera la somme des contextes en vol ; borner leur
file est nécessaire, sans garantir à lui seul le travail total.

## Validation, provenance et limites

[r2_BUILD.json](r2_BUILD.json) et [san_r2_BUILD.json](san_r2_BUILD.json)
capturent les mêmes sources, en Release et Clang ASan/UBSan, fuites
activées. Le [gate](gate.cpp) confronte **840 flux** sur dix nuages à
un oracle scalaire indépendant : toutes les paires, clés, intérieurs et
coquilles. Il compare 82 824 supports, dont 7 602 avec coquille
supplémentaire ; huit plans et 1 048 paires vérifient aussi directement
les minorants/préfixes. Huit rejets d'API passent. Le cache est réellement
réutilisé, et 48 appels exercent le seuil64 avec de gros facteurs.
Les deux mutants de permutation/précrédit sont des **contre-modèles
locaux**, pas des mutations du binaire produit.

Le premier build Release r1 précède l'ajout des rejets API ; seul r2
alimente les mesures. `san_r1` conserve l'échec LeakSanitizer sous
ptrace dans le sandbox ; `san_r2` le rejoue hors sandbox avec les mêmes
protections. Aucun échec n'est remplacé ni aucune fuite désactivée.

Le callback canonique et ses contrôles viennent explicitement de
l'[adaptateur LiDAR précédent](../q2_order_lidar_20260914/lidar_order_probe.cpp).
Le total paie chargement/génération, hash d'entrée, propriétaire, index,
front, Pool, couvertures, census, collecte, copies/tri/contrôles/hash des
supports et destructions. Les temps englobants se recouvrent : ne pas
additionner `selected_total_ms`, préparation Pool, construction/couverture
et payload. `query_build_ms` inclut la couverture mise en cache.

Les maxima `plan_peak_bytes` et `query_peak_bytes` sont des **capacités
vectorielles séparées**, pas un pic RSS ni sa décomposition exacte.
Ils excluent notamment les propositions transitoires, piles, métadonnées,
tampon d'entrée, buffers du census et copies du callback. Le partage
asynchrone et la résidence à plusieurs millions de points ne sont pas
qualifiés. Les égalités d'empreintes à grande taille ne remplacent pas
un oracle géométrique à cette taille. La tour FULL n'est pas calculée.

Le lecteur [verify.py](verify.py) vérifie archives, campagnes complètes,
rejeux normal/−O, anciennes baselines, répétitions et mutants de reçus.
Il réapplique la finitude récursive déjà imposée à l'ingestion, et ne
confond pas `COMPLETION=completed` avec l'appariement vérifié. Les
commandes, entrées, affinité et charge de l'hôte sont conservées.
La [clôture finale](r1_VALIDATION.json) exige les six campagnes,
36 lignes, 33 configurations, douze baselines identiques et neuf
mutants de reçus détectés. Aucune qualification FULL/G4 n'en découle.

```bash
python3 -B morsehgp3D_v8/audits/q2_pool_bridge_20260914/verify.py --name replay
```

`build.py --name nouveau` crée des sources/binaires neufs sous ce dossier.
`measure.py --plan ... --build-receipt ...` refuse d'écraser une campagne.
Les instantanés et binaires générés sont ignorés par Git ; leurs sources
et leurs hashes restent archivés dans les reçus.
