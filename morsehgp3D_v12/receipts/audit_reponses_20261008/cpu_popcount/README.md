# Catalogue CPU : poste actuel et proposition popcount — 8 octobre 2026

**La finition parallèle est déjà commune au CPU et au GPU.** `Assembly::finish` rassemble les lots puis appelle
`finish_stage<PoolExecutor>` et la matérialisation parallèle `adopt`, depuis `8ba7d7287`. La racine historique
« finition CPU en série » de CST-0233 ne décrit donc plus le code courant. Les sources lues, épinglées en
`c9ac60f20`, sont identiques sur ce périmètre à `cffe3e0da` au contrôle ; aucun fichier produit n'est modifié.

Le correctif B de CST-0234 est également présent : paires calculées une fois sur l'hôte, prédicats de census
arrêtés après le seuil, seules les lignes H accessibles effacées. Ne pas reproposer ces correctifs comme absents.
Les réserves sur la simulation des voies CPU et les files de phases restent celles de l'audit historique.

## Où reste le temps CPU dans K

Relecture des neuf processus CPU FULL K5/W48, feuille 24 : quatre passes chaudes par processus. Valeurs en ms,
chaque colonne étant médianée depuis ses propres valeurs de passe ; aucune somme de médianes n'est présentée
comme un temps. `parcours` est le résidu de la traversée après soustraction des fenêtres count/fill : il comprend
aussi du travail hôte entre ces fenêtres, dont préparation/admission des lots. Ce n'est pas un chrono BFS pur.

| Trame | C | parcours | feuilles | émission | finition | C si finition gratuite |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 326,092 | 74,524 | 156,071 | 25,047 | 66,381 | 259,665 |
| ng01 | 274,071 | 62,781 | 126,045 | 24,120 | 58,802 | 215,381 |
| ng02 | 329,246 | 76,597 | 147,660 | 28,856 | 71,654 | 257,255 |

La dernière colonne soustrait la finition **dans chaque passe**, puis médiane : borne conditionnelle au reste
de cette exécution conservé, pas estimation d'une nouvelle architecture. Les parts médianes par passe sont
45–48 % pour les feuilles, environ 23 % pour le parcours et 20–22 % pour la finition. Un résidu non ventilé
de 2,5–4,1 ms subsiste. Les bruts sont ceux déjà admis par la contrelecture FULL, archive `5f8d64c1…`.
Le GPU de K rend C en 31–37 ms ; copier sa finition vers le CPU n'est plus une action restante.

La comparaison historique v11 `domain` à 200/163/195 ms (feuille 16, autre session) reste descriptive, sans
ablation appariée : voir [frontières v11/v12](../../audit_performance_20261007/mesures/README.md).
L'égalité des compteurs géométriques ne borne pas le coût des collectives simulées, files, tris ni instructions.
Cette lecture ne chiffre pas quelle fraction de l'écart v11 vient de chacun de ces choix.

## Appels logiciels effectivement retrouvés — build local seulement

Le build local existant nommé `build_v12_u21` déclare Release/u21/CUDA OFF et `MHGP12_MARCH` vide.
`flags.make` contient `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror`, aucun `-march` ni `-mpopcnt`.
Son objet `leaves.cpp.o` SHA `14223a07…` référence **`__popcountdi2`**. `objdump -drwC` montre **496 sites
statiques de relocation dans 24 fonctions instanciées**, dont 18 dans `count_body`. Ce ne sont ni 496 appels
dynamiques, ni leur nombre sur LiDAR, ni un temps de performance.

Témoin N32/Narrow dans `phase<2,…,WriteSink<32>>`, directement retrouvé dans l'objet :

```text
218d: or 0x180(%r8,%r15,4),%edi
2195: call ... ; R_X86_64_PLT32 __popcountdi2
21a4: cmp %eax,%r14d
21ac: jle ...
```

Le masque `dom` commence à l'offset 0x180 après P[32][3]. Cette séquence OR, population, comparaison au seuil
correspond au test G3 de la paire dans la source ; d'autres appels sont présents dans census, préparation et
comptage des items. Les empreintes de l'objet, des deux sondes locales, du cache et des flags sont contrôlées
avant/après. **Aucun de ces binaires n'a été exécuté ou recompilé.** Le build local n'est pas celui de K :
les artefacts binaires de K n'ont pas été rapatriés. Le fait constaté localement reste une hypothèse à vérifier
sur son binaire, malgré les mêmes options explicites par défaut dans les pilotes FULL/D6.

## Proposition minimale, encore sans compilation

`proposition.patch` conserve l'algorithme et ses compteurs. Sur l'hôte x86-64 sans macro de compilation
`__POPCNT__`, il remplace les builtins par un comptage SWAR inline ; CUDA conserve `__popc`, les cibles POPCNT
gardent le builtin, les autres architectures restent inchangées. La macro indique les instructions **autorisées
à la compilation**, pas toutes celles physiquement présentes dans le processeur.

Port déclaré : `morsehgp3D_v11/src/catalogue/leaf.cpp`, fonction `popcount_word`, commit gelé `ac081a06f`
(SHA complet dans la capture). Le corps u64 est repris explicitement ; le corps u32 est son adaptation à quatre
octets pour le masque N32. La v11 expliquait déjà ce choix par l'appel logiciel du profil x86-64 de base.
L'adoption produit devra inscrire ce port dans sa provenance et le requalifier.

Preuve : la première opération compte les bits par champs de deux bits ; la deuxième fusionne deux champs
en un compte sur quatre bits ; la troisième obtient les comptes de chaque octet, chacun <=8. Le produit
par `0x0101…` additionne ces comptes dans l'octet de poids fort : la somme <=32 ou <=64 tient dans cet octet.
Les opérations non signées sont définies modulo 2³²/2⁶⁴. Les valeurs des popcounts, masques G3 et compteurs
restent donc exactes. Aucun budget ou stockage supplémentaire, aucune modification du chemin numérique.

`git apply --check` puis application sur une copie temporaire de ce seul en-tête sont vérifiés ; résultat
SHA `d32fa18d…`. Aucun gain n'est acquis : un compilateur peut reconnaître l'idiome et le réécrire. Première
porte utile pour le développeur : comparer les instructions/relocations de l'objet corrigé et celles de
l'objet de référence, puis les digests/compteurs N32 et N256. Ensuite seulement, une ablation CPU à options
identiques doit mesurer feuilles **et catalogue entier**, à un fil et 48 fils, y compris émissions/rejeux.
Activer globalement `-march` serait un autre bras et ne permettrait pas d'attribuer seul un gain au popcount.

Ce complément reste rattaché à CST-0234/performance CPU ; il n'établit aucun défaut géométrique ni nouvelle
qualification K. Les propositions antérieures et reçus de mesures restent immuables.

```sh
python3 -B CHEMIN_DU_RECU/check.py --repo DEPOT --build BUILD_LOCAL_EPINGLE --archive RESULTS_K_TAR_GZ --check
python3 -B -O CHEMIN_DU_RECU/check.py --repo DEPOT --build BUILD_LOCAL_EPINGLE --archive RESULTS_K_TAR_GZ --check
```

Rejeux Python normal/−O identiques au champ `result` de `capture.json`. Ils invoquent seulement Git, nm et
objdump, lisent les JSON K et appliquent le patch en temporaire. Aucun build, test natif, microbanc ou GCP.
