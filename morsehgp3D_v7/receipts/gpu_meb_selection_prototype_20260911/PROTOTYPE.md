# Primitive privée de sélection MEB pour une future couture GPU

État : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Snapshot de départ `ad7ffd28b35e153a20bd8cf42534d1cd29160bcd` ; aucune source active, audit, branche, index Git ou VM modifiée par ce prototype. GCP non utilisé.

Cette expérience porte uniquement sur la sélection du support de la boule minimale englobante d'une facette de cardinal 1 à 10. Elle ne livre ni terminal statique, ni recherche d'intrus, ni hiérarchie HGP FULL, ni tour, ni gain de temps ou de mémoire. Le stub exécute sur l'hôte le corps destiné à un thread CUDA par facette. Une compilation et une édition de liens CUDA ne sont pas une exécution device.

## Architecture et contrat borné

`meb_selection.cuh` utilise les puissances/formes entières HD existantes. La seule modification du snapshot de primitives est l'annotation `MHGP7_HD` de `q4_center_strictly_inside` dans `source/morsehgp3D_v7/src/lanes/q4.hpp` ; sa formule reste identique. `q4_original.hpp.source` conserve l'original. Le CPU exact `anchor_meb.hpp` reste inchangé.

Chaque requête POD contient un ordinal, un cardinal et les indices des points dans un tableau de positions. Chaque sortie POD contient cet ordinal, le statut, le cardinal du support, ses quatre positions, le nombre de points sélectionnés sur la sphère et les compteurs. La primitive vérifie les indices avant toute lecture indirecte, le profil u16 ainsi que l'absence d'indices ou de positions dupliqués. Elle parcourt q2, q3 puis q4 dans l'ordre lexicographique de la référence, avec le même arrêt au premier support positif contenant la facette et le même ordre des tests de puissance. Le singleton est explicite.

Pour K au plus 10, une requête paie au plus 375 supports et 3 750 puissances ; ses compteurs locaux initialement nuls ne débordent pas. La future agrégation des compteurs entre requêtes devra être vérifiée séparément. Les cas d'overflow historiques conservés dans le juge ne testent que `anchor_meb`, pas une API d'agrégation GPU absente.

La matérialisation reste entièrement hôte : clé canonique, GCD, niveau exact. Elle vérifie le support positif sélectionné et son confinement, puis matérialise cette seule boule. Les nouvelles puissances de validation hôte sont exposées séparément par `host_validation_power_tests` : elles ne sont jamais présentées comme du travail payé par le kernel. `selection_work` n'est rempli qu'en cas de succès ; sur tout échec, la sortie POD brute reste le seul reçu du travail de sélection déjà payé. Un contenu POD forgé n'est pas une autorité sur le travail qu'il déclare.

La validation du rang combinatoire et de bornes des compteurs ne prouve pas que le préfixe d'énumération a été effectivement exécuté. Un autre support positif peut donner exactement la même MEB. Le test du carré établit cette limite : le support ultérieur produit une MEB valide, mais la comparaison exacte des positions et des compteurs détecte qu'il n'est pas le premier. Le validateur runtime est donc un certificat de MEB locale et de cohérence de transport, pas une preuve d'exécution de préfixe ni une certification FULL.

Les types POD ne constituent pas encore un format wire/ABI stable ; les comparaisons sont physiques champ par champ, jamais sur leurs octets de padding. Les positions sont actuellement des `P3` à coordonnées i64. Aucun propriétaire de buffers device, index résident, catalogue de boules, transaction de lot FULL ou engagement de résidence n'est présent. Les pointeurs non nuls de lots non vides sont une précondition des wrappers privés ; un futur propriétaire hôte devra les contrôler. Le lot vide n'accède à aucun pointeur et ne lance aucun bloc.

`kernel_compile.cu` instancie et relie un véritable kernel, mais son `main` renvoie délibérément 2 et ne doit jamais être exécuté. Il ne constitue pas un runner GPU. Les flags de falsification sont propres à cette expérience et ne doivent pas devenir des options produit.

## Qualification et preuves

Les captures sont create-only. Chacune contient les sources exactes avant/après, leurs hashes, les commandes, codes de sortie et flux bruts. Un seul compilateur ou job tourne à la fois. Les durées de commandes présentes dans les reçus ne sont pas des benchmarks de la primitive.

- `o2_r1` : compilation et exécution de l'oracle réussies, puis échec de compilation du test de transport, dû à une accolade de namespace manquante. Sources et diagnostic conservés sans remplacement.
- `o2_r2` : réussite des deux juges et des rejets d'arguments, sous C++20 `-O2 -Wall -Wextra -Wpedantic -Werror`.
- `nvcc_r1` : compilation et édition de liens CUDA 12.9.86 `sm_120` réussies avec diagnostics stricts ; stderr vide. Le kernel est instancié, le binaire n'est jamais exécuté.
- `san_r1` : compilation ASan/UBSan réussie, puis échec d'exécution (code 1) : LeakSanitizer refuse l'environnement ptrace. stdout est vide, aucun succès SAN n'est revendiqué. Le runner s'est arrêté avant la compilation du test de transport. Sur instruction de ROOT, aucun replay hors sandbox ni attente d'autorisation nouvelle n'a été lancé ; les sanitizers n'ont pas été désactivés. Ce complément privé reste non qualifié SAN.

Le juge adapté `source/morsehgp3D_v7/tests/anchor_meb_gate.cpp` conserve l'oracle indépendant par Gram rationnel. Son original est conservé dans `anchor_meb_gate_original.cpp.source`. Le stub et la matérialisation hôte sont comparés à la référence sur 605 cas, avec 16 592 checks, 300 permutations, 197 cas où la sphère contient davantage de points sélectionnés que le support, et des supports acceptés q1/q2/q3/q4 au nombre de 82/393/110/20. Des coordonnées extrêmes 0 et 65 535 et des facettes jusqu'à K=10 sont exercées. Les champs de géométrie, positions du support, cardinal, coque et compteurs sont comparés exactement, puis confrontés à l'oracle rationnel.

`transport_gate.cpp` passe 253 checks : 33 rejets pour champs invalides, écriture manquante, indices, positions, doublons et pointeur de positions nul ; 4 flags causaux touchent confinement, premier support, nombre de points sur la sphère et écriture. Les supports aigus/droits et tétraèdres positif/coplanaire/à centre extérieur sont explicites. Un batch multi-requêtes lie chaque sortie à son ordinal et laisse intactes les cases au-delà de sa taille ; un batch vide accepte des pointeurs nuls sans accès.

## Reproduction privée

Depuis la racine, chaque nom de capture doit être nouveau :

```bash
python3 -B build/v7_static_gpu_meb_20260911/record.py --out o2_replay --mode o2
python3 -B build/v7_static_gpu_meb_20260911/record.py --out san_replay --mode san
python3 -B build/v7_static_gpu_meb_20260911/record.py --out nvcc_replay --mode nvcc
```

Le runner réutilise les dépendances locales déjà extraites : Boost dans `build/v7_boost_gate/extracted/usr/include` et CUDA dans `build/v7_nvcc_pedantic_20260910/toolkit`. Il n'installe rien. La voie SAN garde ASan, UBSan et LSan actifs ; le sandbox ptrace peut empêcher LSan et justifier un replay autorisé hors sandbox, jamais une désactivation silencieuse. La voie NVCC réutilise l'adaptateur de phase hôte strict déjà qualifié, copié sous `nvcc_strict_host.py`, avec `sm_120`, `-fmad=false`, `--expt-relaxed-constexpr` et les diagnostics stricts ; aucune exécution du binaire lié n'est demandée.

`manifest.json` scelle les sources et preuves textuelles sans ELF ni archive binaire. `verify.py` contrôle les hashes, la stabilité des sources de chaque capture, les commandes et codes de sortie, les décomptes des juges et les diagnostics des échecs. Il ne compile ni n'exécute aucun binaire et reste actif sous `python3 -O`. Les dépendances externes ne sont pas vendoriées ; les fichiers `.d` conservent leurs chemins observés. Ce paquet privé n'est donc pas un toolchain hermétique.

## Suite nécessaire avant G4

Une exécution GPU différentielle de cette primitive est encore nécessaire. Elle devra utiliser un propriétaire device, vérifier tailles et erreurs d'allocation/copie/lancement/synchronisation, préinitialiser les sorties pour détecter les écritures manquantes, comparer toutes les sorties et tous les compteurs au CPU et couvrir les lots vides, partiels et mélangés. Il faudra ensuite porter et qualifier le vrai terminal statique : lookup des clés/niveaux, recherche exacte d'intrus dans le même ordre, catalogue et BVH résidents, puis retour des identités terminales. La note `../v7_gpu_anchor_design_20260911/README.md` décrit ces obstacles. Aucun chiffre de vitesse n'est extrapolé de cette seule primitive.
