# Shadow exploratoire : moments sur rectangles WSPD lourds

24 septembre 2026, auditeur B, hors moteur et hors registre.
[Source](shadow.cpp) SHA-256
`7a74a0946ae3dbc7089061c4abe25c4d8eb3495e330ddd73f717748ef398f1d3`.
Entrée : quart physique sans sol de 08/000200, 11 461 sites, fichier
`../s4a_cpu_scene02_physical_panel_20260924/inputs/quarter_full.u32le`,
SHA-256 `825d005ea9c3e1b8509e26bd5adda746a80779f1e12e8a7e05c1b01bc9f7e7ae`.
Bibliothèque locale du générateur v9 SHA-256
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`.
Le source a été recopié dans cet audit et compilé à nouveau avec
`g++ -std=c++20 -O2 -Wall -Wextra -Werror`, puis exécuté par B ; aucune
source moteur n'a été modifiée. Rejeu local (adapter le chemin de la
bibliothèque reconstruite au même code) :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I morsehgp3D_v9/src -I morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/moments_rectangle_shadow_20260924/shadow.cpp \
  build/v9-dev/libmhgp9_gen.a -pthread -o /tmp/mhgp9_moments_shadow
timeout 120s /tmp/mhgp9_moments_shadow \
  morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/inputs/quarter_full.u32le 5 1024
```

Un dernier argument facultatif fixe `s` (8 par défaut, jamais <8) ; les appels
`... 5 1024 8`, `... 5 1024 10`, `... 5 1024 12` ont été rejoués sur
le même input et le même binaire. Ils comparent **front et shadow
seulement**, pas la chaîne FULL aux trois séparations.

Le front est le vrai `MidpointSamples`, `K=5,s∈{8,10,12}`, puis le filtre de
témoin affine q3/q4 est appelé sur chaque rectangle. Seuls les produits
encore ouverts et de masse `|A||B|≥1024` sont inspectés. Un seul bloc
`G` de ≤64 sites est choisi par descente de l'index spatial vers le
milieu des deux boîtes. Ce choix **naïf**, non optimisé, est fixe pour
tout le rectangle. Les moments entiers `N,Z,Q` de G sont sommés pour
chaque rectangle ; ils ne sont pas précomputés ni hérités.

## Certificat entier et préfiltre

Pour une paire `a,b`, les quantités sont
`H=4((a+b)·Z−Q−N a·b)`, `D=|b−a|²`,
`C=(b−a)×(2Z−N(a+b))`. La voie q3 est fermée si
`A3=3H−4(K−2)D>0` et `A3²>12|C|²` ; q4 si
`A4=2H−3(K−3)D>0` et `A4²>8|C|²`. La preuve de
[concavité séparée sur les 64 coins](../CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md)
établit qu'une réussite à tous ces coins certifie le produit continu
des boîtes, donc chaque paire discrète.

Un premier étage entier O(1) calcule exactement `Hlo` par les 12
combinaisons de deux bornes par axe de
`Z_i(a_i+b_i)−N a_i b_i`, exactement `Dhi` par les écarts extrêmes,
et un majorant `Xhi≥|C|²` par produits d'intervalles des composantes de
`b−a` et `2Z−N(a+b)`. Substituer `Hlo,Dhi,Xhi` aux valeurs ponctuelles
dans les deux tests donne un certificat plus sévère mais sûr.
L'assertion `préfiltre ⊆ 64 coins` a passé sur tous les rectangles
sélectionnés ; la preuve de sûreté est l'encadrement, non ce seul test.
L'arithmétique i64/i128 de cette sonde dépend de **N≤64 et u18** ; ne
pas la réutiliser pour de grands blocs sans recalculer les bornes.

## Résultat et verdict

Rejeu B, même input et K5 :

| s | rectangles front | ouverts | lourds ≥1024 | masse lourde | certifiés uniformément | succès q3/q4 parmi 9 paires sondées par rectangle | rectangles avec ≥1 paire fermant toutes ses voies |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 520 210 | 245 818 | 120 | 897 149 | 0 | 22 / 61 sur 1 080 | 14 |
| 10 | 614 267 | 263 712 | 150 | 770 735 | 0 | 0 / 43 sur 1 350 | 8 |
| 12 | 703 786 | 278 579 | 159 | 580 251 | 0 | 0 / 14 sur 1 431 | 3 |

Ni le préfiltre ni les 64 coins ne certifient un de ces rectangles
lourds avec ce G. Le nombre de rectangles monte avec `s`, tandis que
la masse des seuls produits lourds baisse ; les masses légères, la
sélection, le cœur et l'aval ne sont pas chiffrés ici. Les fronts
diffèrent : les succès sur neuf paires sondées ne sont pas une
comparaison appariée paire par paire. Le même G peut certifier une
paire particulière sans certifier toute sa boîte : la grosse boîte
uniforme masque un gain ponctuel, sans quantifier les paires réelles
évitables.

Durées internes du seul sous-échantillon, **descriptives** sur hôte
partagé : choix/sommation de G ~85–100 µs, sondage de neuf paires par
rectangle ~115–139 µs, préfiltre ~29–34 µs, 64 coins à échec précoce
~15–17 µs pour les trois `s`. Elles ne sont pas des
temps G4 ni des chronos de chaîne. Dans ce régime, optimiser seulement
la formule des coins ne résoudrait pas le coût de sélection, et appliquer
64 coins à tous les 245 818 rectangles serait injustifié.

### Tuilage disjoint borné, même bloc G

Le [second sidecar](tiles.cpp), SHA-256
`ba994e6d86b6b9680c24ab8dc2589d9a0ea00989405ed31e5994035def8226c9`,
inclut le `shadow.cpp` voisin et conserve le même G de chaque racine.
À chaque profondeur, il divise en deux le facteur de plus grande
population par les enfants exacts de l'index. Les sous-produits sont
disjoints ; le script vérifie la conservation de la masse et la
monotonie des voies certifiées. Rejeu avec les mêmes options, en
remplaçant `shadow.cpp` par `tiles.cpp` dans la commande de compilation :

| profondeur | tuiles | masse totale | voies closes par 64 coins | masse dont toutes les voies ouvertes sont closes | voies closes par préfiltre |
| ---: | ---: | ---: | --- | ---: | ---: |
| 0 | 120 | 897 149 | aucune | 0 | 0 |
| 1 | 240 | 897 149 | q4 sur une tuile | 4 620 | 0 |
| 2 | 480 | 897 149 | q4 sur deux tuiles, q3 seule sur une autre | 4 620 | 0 |

Les 418 paires de la tuile q3 seule à profondeur 2 ne sont **pas**
retirables si q4 reste ouverte. Les 4 620 paires entièrement fermées
ne représentent que **0,515 %** de la masse lourde pré-S2. C'est
un *plafond* dans ce sous-échantillon : si ces paires ne survivent pas
jusqu'à S2, l'économie réelle est nulle. Le préfiltre d'intervalles
ne ferme aucune des **840 tuiles testées**, même lorsqu'un coin réussit ;
ici il ajoute du travail sans réduire les tests de coins. Les 2 191
évaluations effectives de coins (arrêt précoce) et les allocations
de tuiles restent à payer. Ces résultats ne justifient pas de porter
ce tuilage tel quel dans le moteur.

**Décision : ne pas porter cette sélection de G ni ce tuilage tels quels.**
Le prochain essai utile est un certificat plus local, par arête S2 ou
par tuile choisie avec un meilleur G, avec jointure à `ΣF` des arêtes
réellement évitées, sorties q3/q4 et coût sélection/preuve/repli/FULL.
Ce shadow n'exécute ni S2, ni le cœur, ni FULL ; il n'établit ni gain ni
sous-quadraticité. Une réussite sur une paire sondée ne permet jamais de
fermer le rectangle entier.
