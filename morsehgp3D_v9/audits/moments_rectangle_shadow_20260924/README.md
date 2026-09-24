# Shadow exploratoire : moments sur rectangles WSPD lourds

24 septembre 2026, auditeur B, hors moteur et hors registre.
[Source](shadow.cpp) SHA-256
`24f08f682a4432c18e3f54424a150719764942b2649f41a7ac29c2d9d36e633e`.
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

Le front est le vrai `MidpointSamples`, `K=5,s=8`, puis le filtre de
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

Rejeu B : **520 210** rectangles de front, **245 818** ouverts, seulement
**120** avec masse ≥1024, de masse cumulée **897 149 paires**.
Ni le préfiltre ni les 64 coins n'ont certifié q3, q4 ou les deux sur
aucun des 120 rectangles. Sur neuf paires ponctuelles échantillonnées
par rectangle (premier/médian/dernier rang de chaque facteur), le **même
G** certifie q3 dans 22/1080 tests et q4 dans 61/1080 ; dans 14/120
rectangles, au moins une paire échantillonnée ferme toutes ses voies
encore ouvertes. Cela suggère que la grosse boîte uniforme masque un
gain ponctuel, sans quantifier les paires réelles évitables.

Durées internes du seul sous-échantillon, **descriptives** sur hôte
partagé : choix/sommation de G ~85 µs, sondage des 1080 paires ~139 µs,
préfiltre ~29 µs, 64 coins à échec précoce ~17 µs. Elles ne sont pas des
temps G4 ni des chronos de chaîne. Dans ce régime, optimiser seulement
la formule des coins ne résoudrait pas le coût de sélection, et appliquer
64 coins à tous les 245 818 rectangles serait injustifié.

**Décision : ne pas porter cette sélection de G telle quelle.** Tester
ensuite un tuilage *disjoint* limité des grands rectangles avec héritage
du bloc et des moments ; mesurer `ΣF` des arêtes S2 réellement évitées,
les sorties q3/q4, le coût de sélection/tuilage/repli et la chaîne FULL.
Ce shadow n'exécute ni S2, ni le cœur, ni FULL ; il n'établit ni gain ni
sous-quadraticité. Une réussite sur une paire sondée ne permet jamais de
fermer le rectangle entier.
