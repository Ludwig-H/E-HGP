# Terminal CUDA c03 — extension locale de la gate à K9/K10

Jalon privé v7 : `phase=exploration_v7_hors_registre`, `backend=HOST_STUB`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. La référence produit reste `c03f6be8488453486b112811071827a96303ec86`, importée depuis le paquet K2..8 publié à `dc5a36ba`. Ce jalon ne qualifie pas le nouveau terminal avec seeds ni le header actif `6763a877`.

L’export exact et les gates hôte O2 et ASan/UBSan ROOT, avec détection des fuites activée, sont fermés PASS. NVCC compile et lie les vrais kernels sm120, sans les exécuter. Les observations O2/SAN sont déterministes et identiques. **Aucun kernel de ce terminal n’a été exécuté, GCP non utilisé, aucun gain GPU ni contrat 50k ou multi-millions annoncé.**

## Corpus fixé avant calcul

Les 949 facettes historiques K2..8 sont toutes rejouées. S’y ajoutent ligne12, coquille12+centre+extérieur14 et spatial12. Les deux nuages n12 fournissent chacun toutes leurs 286 facettes K9/K10. Sur n14, pour chaque K, les quatorze rotations de deux formes de fenêtre sont fixées avant calcul, puis dédupliquées : 28 masques par K, sans utiliser une MEB ni un résultat de descente. Au total : **1 577 requêtes, dont 468 K9 et 160 K10**.

Le plan JSON des masques Morton est exécuté et épinglé avant l’export. Chaque requête déclarée est nominale ; tout refus fait échouer l’export entier, aucun cas n’est retiré après son résultat. Le catalogue indépendant T2 est limité à la fenêtre `p+qmin≤11` avant écriture dans les tableaux ; cette sélection porte sur les boules du catalogue, pas sur les requêtes. Le juge reste borné à n≤14 et ne devient pas une architecture produit.

Le modèle T2 construit les MEB par supports positifs Gram q≤4. Sur les quatorze petits nuages, ses **1 022** MEB sont reconfrontées au modèle historique. Sur les dix-sept nuages, les **2 763** lignes de la référence c03 sont reconfrontées à Gram pour la clé primitive et le niveau. Résultats : q2/q3/q4 = **1 802/691/270**, 251 coquilles supplémentaires, 1 185 descentes strictes, une descente à rayon égal, 1 186 requêtes d’intrus. Les 1 577 ordinaux dépassent 2^32.

| Domaine | Requêtes | Descentes strictes | Requêtes d’intrus | Lignes q3 | Lignes q4 |
|---|---:|---:|---:|---:|---:|
| K9 | 468 | 577 | 577 | 229 | 226 |
| K10 | 160 | 130 | 130 | 100 | 19 |

La descente à rayon égal est non vacuante sur le corpus global, pas séparément à K9/K10. Le niveau initial `before=3*65535²+1` est une coupe supérieure fixe. Ces requêtes ne sont pas uniquement celles qu’un vrai lot FULL demanderait chronologiquement.

## Qualification et limites précises

La gate portable confronte 15 539 contrôles et exige les non-vacuités K9/K10 séparément. Douze rejets historiques et huit omissions/corruptions causales de transport sont conservés. Six nouveaux rejets testent le **dernier slot** de K9 et de K10 : indice négatif, doublon et hors domaine, avec refus avant toute MEB. Les prédicats de provenance font 44 contrôles et 40 rejets, normalement et sous Python `-O`.

Le passage à trace pleine compare chaque ligne : sites, support/slots/coquille, clé, intrus, terminal et travail. Les niveaux sont égaux **en valeur rationnelle**, pas nécessairement en représentation binaire. Le passage capacité zéro compare résultat/statut/ordinal/travail/longueur/overflow pour les 1 577 requêtes, sans comparer de ligne de trace. Cette capacité ne limite jamais la descente. Les **48 descripteurs ABI** décrivent tailles, alignements et offsets, pas 48 mots de trajectoire.

Les rejets portent sur la requête explicitement fautive, pas sur une annulation transactionnelle du lot entier. Les buffers `const` ne sont pas relus après kernel : aucune immutabilité device constatée n’est revendiquée. Le wrapper synchrone du juge n’est pas le futur propriétaire industriel. Le lot vide ne lance rien et ne lit/écrit rien. Les limites de représentation héritées restent coquille≤12, intérieur≤9 et index sans doublons géométriques.

Les compteurs du juge comprennent replays et fautes : 61 lancements simulés hôte, H2D 1 867 206 octets, D2H 1 377 448, allocations et libérations certifiées 1 867 206 octets. Ces valeurs ne sont ni des mesures GPU ni un débit extrapolé. Le binaire délègue l’autorité infrastructure au contrôleur externe.

`nvcc_r1` a réellement échoué sur un **mode exécutable perdu à l’import**, avant prétraitement hôte. Il reste conservé avec son snapshot et son code 1. Le seul correctif est `chmod 700` sur l’adaptateur de compilation privé, sans changement de contenu ; `nvcc_r2` compile et lie avec diagnostics stricts et stderr vide. Voir [MODES.md](MODES.md). Aucun ELF n’est exécuté par ces captures NVCC, même pour un argument invalide.

## Sources et preuve portable

Le helper terminal `6846376ab7fa44d6880cfa48b9b8bdde4e3d53decd5f74b1e08565b9e8a550bb`, l’owner `be3c422c8ff7a09650c7a91d175eebfef21585ca160150fb40bae83dcdd13a60`, la référence CPU et l’ABI sont inchangés. Seuls le corpus, son export et les contrôles sont étendus. La fixture autonome est SHA-256 `d6d6621ef43fd94d1dd4771a481697764d0443ea4cdd0cc30b3944c3a3864154`.

Les cinq captures compilent leurs snapshots, puis vérifient leur stabilité. Les six qualifications hôte préalables sont liées par la fermeture complète des sources communes ; exporteur, référence, types, oracle T2 et helpers effectivement consommés sont identiques entre export et consommateurs. Les paquets hôte et K2..8 restent des autorités historiques distinctes : leurs résultats ne sont pas hérités par cette extension. Les originaux et patches sont conservés, ainsi que l’échec NVCC, sans ELF ni vendor.

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7_terminal_k10_neuf
```

Le lecteur vérifie les fichiers, commandes, résultats, source closure, plan antérieur à l’export, attentes et pins des quatre ELF non distribués. Il ne recalcule aucune géométrie. L’extraction restitue les fichiers logiques dédupliqués et restaure explicitement le droit d’exécution du seul adaptateur NVCC épinglé. Les READMEs présents dans les snapshots sont historiques et préparatoires ; le présent document porte la portée finale du paquet.

Pour recompiler sans Boost : `g++ -x c++ -DMHGP7_FAKE_DEVICE -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread sources/current/cuda_trial/device_gate.cu -o /tmp/mhgp7_terminal_k10_stub`, puis `--selftest` ; CLI inconnue ou absente →2. Seul l’exporteur nécessite Boost. Les flags NVCC exacts et chemins historiques sont conservés dans les reçus ; adapter toolkit/ccbin à l’environnement et restaurer le mode exécutable après contrôle de hash. Toute future exécution G4 demeure une session gardée distincte, sans transfert automatique au moteur actif ni preuve de vitesse.
