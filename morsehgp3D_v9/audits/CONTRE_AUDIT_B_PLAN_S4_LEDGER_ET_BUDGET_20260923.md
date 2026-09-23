# Contrelecture B du plan S4 : unités du ledger et budget réellement transféré

23 septembre 2026. Cadre : `exploration_v9_hors_registre`, conception en
lecture seule, `public_status=not_claimed`. Cette note ne qualifie ni un port
CUDA S4 ni un gain G4. Elle complète
[`AUDIT_S4_RESIDENCE_ORDINALS_20260923.md`](AUDIT_S4_RESIDENCE_ORDINALS_20260923.md)
sans reprendre ses points de raccord GPU–CPU et d'ordinal S2.

## 1. Une graine q4 peut produire plusieurs émissions

Le [plan S4, §2](../docs/s4_conception_20260923/PLAN_S4.md) propose dans
`check_lanes_batch` l'identité « graines = émises + rejets ».
Elle n'est pas valide pour q4. Le balayage Window30 appelle `group` pour
`lower`, chaque groupe `inner`, puis `upper`
([`q4_window.cpp`](../src/gen/lanes/q4_window.cpp)). Le balayage Local28
fait pareil, groupe par groupe
([`q4_local.cpp`](../src/gen/lanes/q4_local.cpp)). Chacun peut émettre
un support pour **la même graine**. Ce n'est pas un doublon à dédupliquer.

Fixture entière admissible, K=3, IDs dans l'ordre affiché :

```text
a=(0,0,2)   b=(4,0,2)   x=(2,3,2)
y+=(2,2,4)  y−=(2,2,0)
```

Pour la graine `x`, les supports `{a,b,x,y+}` et `{a,b,x,y−}` sont deux
tétraèdres distincts, de profondeur zéro et coquille de taille quatre.
Le calcul exact donne, après translation recentrant l'arête,
`c±=(0,5/6,±1/6)`, `rayon²=85/18`, et des poids barycentriques
`(25/72,25/72,2/9,1/12)>0`. Le site `y` opposé est strictement extérieur.
Le [micro-test reproductible](s4_q4_multigroup_probe.cpp) appelle
**directement les deux voies v9** sur la seule graine `x=2` ; il obtient
`seed_queries=1`, `groups=2`, `emitted=2`, en Window30 et en Local28
avec son domaine `Positive` par défaut. Les sorties sont les supports
`0,1,2,4` et `0,1,2,3`, tous deux à profondeur zéro et coquille quatre.
Compilation locale : `g++ -std=c++20 -O0 -I morsehgp3D_v9/src/gen
morsehgp3D_v9/audits/s4_q4_multigroup_probe.cpp
<libmhgp9_gen.a_v9> -pthread -o <probe>` puis exécution code zéro.
La bibliothèque v9 locale avait l'empreinte SHA-256
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a` ;
ce binaire mutable n'est pas un artefact épinglé de qualification.
Ainsi `1 ≠ 2 + 0` : la porte projetée rejetterait une sortie exacte ou
forcerait un compte fictif.

**Correction de conception.** Pour q3, un ledger par graine peut
partitionner les décisions. Pour q4, garder séparément (i) graines
visitées/écartées, (ii) groupes de racines examinés, rejetés en profondeur,
sans support, ou ayant émis, et (iii) émissions. Une graine peut contribuer
à plusieurs groupes. Le juge doit comparer le multiensemble exact des
supports, clés et incidences, pas simplement une somme de compteurs.
Ajouter cette fixture en porte positive CPU/port hôte/GPU avant de figer
le schéma des compteurs.

## 2. L'enregistrement ne tient pas dans les 112 octets annoncés

Les champs énumérés dans le [plan, §2](../docs/s4_conception_20260923/PLAN_S4.md)
totalisent **au moins 129 octets**, même avec deux ordinaux de 32 bits et
sans alignement : arité 1, quatre IDs 16, profondeur 4, taille de coquille
4, deux empreintes u64 16, clé de cinq i128 80, deux ordinaux 8. Une
structure native alignée peut être plus grande. Le plan chiffre pourtant
« environ 112 o », 95 Mo à K5 et 520 Mo à K10 ;
[`ANALYSE_ARCHITECTURE_GPU.md`, §4](../docs/s4_conception_20260923/ANALYSE_ARCHITECTURE_GPU.md)
utilise 104 o en omettant empreinte et ordinaux. À 850 k / 4,63 M
enregistrements, le seul minimum de 129 o représente 109,65 / 597,27 Mo
décimaux, avant alignement, arène, tri, copies et sorties CPU. Ce n'est pas
encore une borne de pic GPU : mesurer `sizeof`, sérialisation, capacité et
pic `cudaMemGetInfo` avec les sorties maximales et le repli exact.

De même, les 1,6 / 5,6 Go annoncés pour les plages S4.0 ne sont pas des
bornes : ils multiplient le nombre d'arêtes ouvertes par une **moyenne de
covers de toutes les arêtes**, non par leur somme réellement ouverte.
Publier `Σ retained_ranges`, `Σ cover_sites` des seules arêtes retenues,
leurs maxima et le pic d'arène en u64, puis une vérification de capacité
avant allocation. Un `atomicAdd` non gardé ne suffit pas à assurer la
publication atomique par arête en cas de dépassement : la réservation doit
être bornée, ne publier aucun préfixe, et reporter l'arête entière au CPU.

## 3. Portes de coût et d'identité avant le débit GPU

- Le [routage des arêtes lourdes](../docs/s4_conception_20260923/PLAN_S4.md)
  suppose que S3 connaît `cover_sites × graines`. S3 connaît les tailles
  de cover et masques, pas le nombre de graines q3/q4 ; il faut un prépassage
  facturé, ou un prédicteur prudent qui ne devient pas un certificat.
- Le débit d'un balayage q3 du cover ne borne pas le coût q4 : deuxième
  passage, comparaisons de racines, ex æquo et émissions multiples par graine
  doivent être mesurés séparément. Compter `Σ_{graine admise} cover_sites`,
  pas une fraction de graines non pondérée. Mesurer le maximum par arête et
  la traîne des replis dans le **mur complet**.
- Les empreintes somme/xor des IDs de coquille ne sont pas injectives.
  Elles comparent un résumé, pas la liste exacte. Soit la porte GPU expose
  les IDs de coquille en mode diagnostic, soit sa revendication se limite
  explicitement aux champs de `Presentation`, avec recensus indépendant.
  Un condensé de tour égal ne garantit pas l'égalité des incidences.
- L'ordinal d'émission doit provenir de l'ordre canonique graine puis
  groupe de racines, jamais de l'ordre de fin des warps ou d'un compteur
  atomique. Le radix final n'efface pas une origine non déterministe.
- « pgcd binaire puis division par soustractions successives » n'est
  acceptable que si la division est **bornée par les bits** (décalages et
  soustractions), non une soustraction répétée jusqu'au quotient. Une gate
  sur les extrêmes u18 doit mesurer l'itération maximale.

Le [reçu R14](../receipts/g4_tower_r14_20260923/README.md), publié après
le plan S4 fondé sur R13, change les données de départ. Sur 08/000000/K5,
la chaîne est 1,976 s et le poste « survivants » 0,733 s ; même son
ablation fictive totale laisserait **1,243 s** à autres phases figées.
Les autres trames K5 laissent aussi 1,025 s (000100) et 1,305 s
(000200). S4 ne peut donc, seul et à coûts invariants, satisfaire 1 s
sur ces trois sous-nuages. Refaire les projections avec R14, puis juger
les effets simultanés sur q2, front, filtres, certificats, recensement et
tour. Aucun de ces calculs ne qualifie les trames brutes, les séquences
supplémentaires, s10/s12 ou les dizaines de millions de points.

**Priorité proposée au développeur :** corriger le ledger q4 et le contrat
mémoire/identité avant S4.0 ; intégrer la fixture de double émission ;
mesurer un prépassage q3/q4 borné et le mur FULL. Une nouvelle piste de
certificat de groupes de gardes existe
([preuve](CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md)), mais son shadow
pré-cœur et son coût net restent entièrement à établir : ne pas le
compter comme gain S4.
