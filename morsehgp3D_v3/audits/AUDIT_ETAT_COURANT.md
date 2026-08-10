# Audit courant de MorseHGP3D v3

Date du snapshot courant : 10 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=math_locks_plus_gpu_differential`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype,
n'ouvre aucune phase et ne promeut aucun résultat public. Le snapshot committé
audité est `e406e1f`; il ajoute au palier `81f9210` une comparaison temporelle
source--référence et une quatorzième porte directe. Au pincement, `HEAD` et
`origin/main` pointaient sur ce commit; les sources, prototypes, CMake et claims
audités lui correspondent exactement. Le delta documentaire du présent audit
est hors de ce snapshot produit. Aucun artefact brut des campagnes de taille ni
de la session G4 n'est versionné avec ces commits.

| objet | empreinte SHA-256 |
| --- | --- |
| snapshot committé de code et de claims audité | `e406e1f646ef20eb222d50e8b2740e6d7d6f6aa3` |
| commit d'audit indépendant précédant immédiatement le snapshot produit | `2855b752c2499606e1fbebabf58e679eff04fd41` |
| `CMakeLists.txt` avec deltas replay/source directe et neuf CTests | `da5f569ce8b18a69d373e5fc9364a1ac22d50abb96d1f73bdc72dcffd3415b47` |
| `prototype/scale_profile.cpp` | `e6c31f544d8275b3f89affde11b52e11972dd7e76cf9b556112c96a43d96aacb` |
| `prototype/admissible_pair_probe.cpp` à `40ad152` | `8c89ccb627d7d0d531897b95ec24f56a473578744f16299d052133dd0fba6cc8` |
| `prototype/admissible_pair_probe.cpp` à `5d9159a` | `130e316ed956cc6a540642ded9fed21456f4c2c57b00ecb4e821f4c2cea86b8d` |
| `prototype/admissible_pair_probe.cpp` à `180975e` | `fa3e464c422839f0485a032016831d3727fb42cbf1a9bd5be7a9427da3fe55fd` |
| `prototype/admissible_pair_probe.cpp` à `ee5ee51` | `5c44a7399e3a4722dfe5ff1ca115ef931a875133fcf83549636bd4ce8e09a410` |
| `prototype/admissible_pair_probe.cpp`, réponse intermédiaire auditée | `5eb64c526ce78822e032653875ec34efbeca254559083b5763154cb2b05e301a` |
| `prototype/admissible_pair_probe.cpp` à `81f9210` | `a80cd2f727fb794318df54399e249b4f6cf9d3bc623c62b5f829563b8070cbb0` |
| `prototype/order_k_flats.hpp` | `b3ba750d938e3c4fa52453730011e2f8ed06e477b40ae971562c15aed07b65f5` |
| `prototype/order_k_device_core.hpp` | `8d8c34031df8a3b4108ae366c6db17074b97a8e0aab1e133f8d754fae990fd6d` |
| `prototype/flats_differential.cpp` | `e6ee12f8e4a61fda11a8d4b26eaafde3d20deab6cfd30195b4521117c85482c9` |
| `prototype/device_wavefront_job.hpp` | `351e25957117b68cb9da729b7802ce80cc0b84f61a12643670bcb40e9b592b1e` |
| `prototype/device_wavefront_kernel.cu` | `bebc6684ccacd763d28d2f336b9cfd17b356914addf37786afbe0c7440901ccc` |
| `prototype/device_wavefront_qualification.cpp` | `dfc146e841c30c151bec007dbeca6f8f307f880216d70142eb0af475a90fb401` |
| `audits/check_gate_d_fold_f0.py` | `3c23497f0227147d35505df5275a20b000a5704a5c862527d3409e9828ebfdc2` |
| `prototype/direct_source.cpp`, premier palier | `24ad3d37aedbf74c4b126fae30453a74d1f2a675eea572e6b92678b27c27258e` |
| `prototype/direct_source.cpp`, palier dispersion historique | `2b47247e9d1ecd6e1a8a573f4597bab9bb19e10a4a3d2ab4295c524d2d1ee68c` |
| `prototype/direct_source.cpp`, palier fonctionnel sans forêt | `9edf150de3f9b75cf931df405d0885f7644f05b622016b78fdb22bc3658216f0` |
| `prototype/direct_source.cpp`, palier courant avec forêt | `1c3948c3f1e46c43311fc6e6668ea78100b0adff9af2bc8549da109ccb7bbc4e` |
| `prototype/direct_source.cpp` à `81f9210` | `bb31b426adbea80625046e773a008ad00cbb2780749b46fcbb02f974e5db1705` |
| `prototype/direct_source.cpp` à `e406e1f` | `d933c3aeb6314f12769f594d30af6734c696b09ce2e67de39af23dbd0ed15ed9` |
| fragment `CMakeLists.txt`, correctif intermédiaire avec treize CTests directs | `a3c40f7e183122fabad0e2d645c9f2f43ff3fa721625d5e6a3d8c9ced0d8254f` |
| `CMakeLists.txt` à `81f9210`, treize CTests directs et gate cliques | `1d9be763ffdde3ae9fd1725949fc41b4788d6465f18e7cb09b4cede337e36326` |
| `CMakeLists.txt` à `e406e1f`, quatorze CTests directs | `739d21248a5fba575974aa3e40e8a0d7d4208b4a9c6710905ec4379cffa8fed7` |
| `README.md` de claims à `e406e1f` | `a5b558c1b7b2baaeacb7e779b83e0cc1d9f7a53a4f0961e317cd4a3f4e468ed3` |

Le palier `81f9210` passait 73/73 CTests en 351,62 s. Un configure et build
Release **frais de `e406e1f`**, sous GCC 13.3 avec GMP et Python actifs, passe
désormais 74/74 CTests, dont 70 tests v3 et quatre dépendances transitives v2 :
zéro échec en 319,87 s avec `ctest -j2`. C'est un résultat d'intégration CPU
positif; le `LastTest.log` temporaire avait pour SHA-256 rapporté
`9abfbe90c1b816b7bc9c8bde0bcc1fe9ecc57d07573609e178eb1822fa6d3eaf`,
relevé immédiatement après le run puis rendu non revérifiable par un `ctest -N`
ultérieur qui a écrasé le fichier. Ce run ne reçoit ni coût 50 k, ni payload GPU,
ni SLO.

À `e406e1f`, le binaire Release direct de SHA-256
`9f1ef706ed0a9005a8a6fa20f56f3caa813d63f267aa0031211ec4c6f6157afc`
passe les 14/14 CTests directs en 9,28 s. Les portes `generic`, `forest` et
`same_payload` passent aussi sous ASan/UBSan/LSan en 19,90 s avec le binaire
`33f7dc5d5b207f754fc7678363dc44e0888eff7ec5c6b0328f3bc9a2a9452436`.
La nouvelle campagne compare vingt forêts et 5 538 nœuds sans divergence
sémantique. Elle ne reçoit aucun invariant de chrono; ce positif ne valide donc
pas les nouveaux libellés temporels.

## Verdict

**GO fonctionnel borné pour owner, sweep, naissance F0 et la source directe
fermée relative; NO-GO maintenu pour la qualification produit, GPU/replay,
totalité bas ordre, chrono source--référence, coût/mémoire 50 k et source Gabriel
ouverte.**

Le premier `.cu` est un progrès réel : il définit un lancement CUDA optionnel
et un même corps source pour CPU/device. Le delta replay rend maintenant cinq
portes hôte vertes, force 27 refus et tue le mutant qui refuse tous les sommets.
Cette avancée ferme la vacuité, pas le contrat de replay : les refus ne
transportent aucun suffixe et créditent seulement deux masses scalaires.

Le kernel ne constitue pas encore une wavefront. `navigate_shallow` construit
et mémorise d'abord tous les sommets sur CPU; le kernel calcule ensuite seulement
un masque d'admissibilité des couples. Il ne produit ni voisin, ni parent, ni
enfant, ni tâche, ni run.

La première `direct_source.cpp` apporte un deuxième progrès architectural : sa
partie candidate énumère les supports sans sommet d'arrangement ni mosaïque.
Trois oracles indépendants valident le cover, le lemme de rayon et les
voisinages sur 33 914 listes et 15 360 supports propres sans écart. Le palier
`9edf150d...` corrige ensuite les claims CMake, sépare les modes, compare les
listes complètes de membres, reçoit l'unicité, applique les fallbacks petit
nuage/cap, agrège les `Q` effectifs et passe les masses en `u128`. Sept CTests
Release et quatre ciblés ASan/UBSan sont verts. Le palier `1c3948c3...` assemble
ensuite un catalogue source et compare 30 forêts abstraites sur six nuages :
1 647 nœuds, zéro divergence. Le correctif intégré à `81f9210` ferme les
lanes bas ordre, la borne $K+1\le s_{\max}$ et le juge CMake implicite; il ajoute
obligations candidats/unicité et mutant membre. Treize CTests Release et huit
ciblés ASan/UBSan/LSan passent. C'est un GO relatif borné réel.

La promotion reste interdite. Le cover rescane tous les points par feuille, le
CSR peut être dense, les high-waters sont partiels et le target refuse plus de
20 000 points. Le prototype compare un catalogue fermé partagé, pas la source
Gabriel ouverte streamée du produit. La signature forêt ignore volontairement
les indices publics : une sonde trouve 4 016 champs `ForestNode::source`
différents malgré 120 digests égaux. Son nouveau contrôle structurel n'est pas
total : cycle de frères et enfant hors plage produisent boucle et overflow ASan;
des fautes identiques des deux côtés restent vertes. La porte forêt n'exige que
1 000 nœuds : un mutant qui saute entièrement $k=5$ conserve 1 201 nœuds,
annonce encore cinq ordres et laisse les treize CTests directs verts. Les
agrégats `i64` oublient le facteur `clouds`.

`e406e1f` retire correctement le fold référence du chrono source et mesure les
deux folds séparément. Il ne ferme toutefois pas la comparaison : le timer
source contient encore le différentiel catalogue, les deux empreintes de forêt
et leur comparaison, tandis que le timer référence ne les contient pas. Le
payload public reste différent et seuls les quotients sémantiques concordent.
Un nuage refusé est facturé à la référence mais jamais à la source. Dix
répétitions documentées à `n=120`, cinq sur chaque thread SMT, gardent les mêmes
compteurs imprimés et zéro divergence mais rendent des rapports entre 0,93 et
1,10, de part et d'autre de 1 dans les deux campagnes : aucun croisement
localisé ne suit de cette dispersion.

Une campagne alternant `judge=0` et `judge=1`, sans forêt, confirme aussi que la
soustraction des modes n'isole pas le juge : les différences pairées source vont
de -0,185 à +0,241 s et la seconde commande affiche le chrono source inférieur
dans les cinq couples. La construction préalable de `flat_catalogue` n'existe
que dans un mode et change l'état initial avant le timer source; seule une
séparation interne des timers dans le même mode peut recevoir ce coût.

Le chrono est audité séparément dans
[`AUDIT_CHRONO_SOURCE_DIRECTE_E406E1F.md`](AUDIT_CHRONO_SOURCE_DIRECTE_E406E1F.md).

Le commit `180975e` remplace le minimum échantillonné par le complément exact du
maximum en demi-plan ouvert. Un oracle indépendant a comparé 74 613 multisets
planaires et 9 593 paires 3D, y compris les dégénérescences et la frontière u16,
sans écart. Le P0 algorithmique est fermé. Les nouvelles masses restent des
diagnostics finis : elles ne prouvent aucun `Big-O` et ne dimensionnent pas la
source Gabriel ouverte.

Le commit `ee5ee51` étend ce probe aux triangles/K4 du graphe admissible et à un
lemme de triple exact. Le comptage orienté et la nécessité mathématique sont
crédités : 28/56/70 sur le graphe complet, zéro triple vrai réfuté sur les runs
reçus. La réponse intégrée à `81f9210` corrige `N/A`, le vrai degré,
le chrono, les quatre faces et ajoute planchers plus mutant acceptant tout. La
gate ne reçoit toutefois ni le nombre après quatre faces, ni la couverture
exacte des vraies triples/quadruples. Deux mutants temporaires le confirment :
remplacer les trois faces supplémentaires par `true`, ou omettre les cliques
d'ancre zéro — dont 202 triples et 97 quadruples vrais sur la campagne — laisse
les deux CTests paires verts.
La décision d'architecture ne suit pas.
Les ratios cliques/supports d'arité quatre et sommets/toutes sphères du parcours
ne sont pas directement comparables; ni le facteur 25--35, ni la croissance
asymptotique, ni l'abandon de toute source directe ne sont établis. Voir
[`AUDIT_CLIQUES_ET_TRIPLE_EE5EE51.md`](AUDIT_CLIQUES_ET_TRIPLE_EE5EE51.md).

Le commit rapporte une compilation `nvcc` et quatre exécutions sur G4 avec zéro
écart CPU/device. C'est un résultat positif ciblé pour le préfixe borné, mais pas
un reçu qualifiant : commandes, sorties brutes, version patch du toolkit, hash
du binaire, PTX/cubin, rapport `ptxas`, digest d'entrée et répétitions ne sont pas
conservés. Surtout, les refus restent exclus de l'oracle; le texte « rejoués par
l'hôte » contredit le code.

Le correctif owner réduit désormais le déterminant par `sign_of` avant
`tangent_sign`, et F0 laisse naître le carré tout $N_a$. Les quatre CTests ciblés
owner/paires/F0 passent sur un build Release frais. Deux réserves empêchent
toute formule plus large : le validateur régulier F0 accepte encore un même
handle strict répété comme deux facettes, et ses deux CTests disparaissent sans
erreur si Python est absent. Le refus/replay du microkernel reste, lui, bloquant.

## Réaudit de `5d9159a` — conclusion générale correcte, mesure surinterprétée

Le commit intègre le probe de SHA-256 `130e316e...` et ajoute dans le README un
bloc **[mesuré]** qui contredit directement l'audit placé huit lignes plus haut.
La commande CPU Release suivante est reproductible :

```sh
mhgp3v_admissible_pair_probe --points 200 --smax 11 --repeats 1 --seed 20260809
```

Elle rend 10 706 paires `ADMIS`, 5 171 paires dites vraies, les maxima 177/85,
le cumul `128:0.990` et zéro paire dite vraie rejetée sur ce nuage. Le build
strict du target passe aussi. Ce sont deux résultats positifs : le diagnostic
committé est reproductible et son maximum 177 a une interprétation limitée mais
valide. Comme le P0 angulaire surestime le minimum, toute paire qu'il admet est
bien admissible pour le filtre exact; 177 est donc une **borne inférieure** du
maximum conforme sur cette entrée et réfute déjà tout cap inférieur qui n'aurait
aucun complément exact.

Le reste du paragraphe n'est pas reçu. Trois tailles ne démontrent pas que le
maximum « croît linéairement ». Les distributions 83/98/99,9 % portent sur un
sous-ensemble censuré de l'univers admissible. Le tri des distances départage
les ex æquo par `PointId`, et non par une classe géométrique. La classe imprimée
`128` couvre les rangs 64 à 127, car le label est la borne supérieure
**exclusive**. `rank_max_true` est mis à jour seulement à l'intérieur de la
branche admise par le filtre fautif et sa vérité partage les primitives de
`flat_catalogue`; les maxima 85/109/147 ne peuvent donc être attribués à « la
réalité ». Enfin, aucun de ces nombres n'impose une frontière par ancre : il
exclut seulement un k-NN borné employé seul, sans complément certifié.

Le bon résultat positif est mathématique et plus fort que l'histogramme. Sur
exactement 50 000 points u16, prendre `p=(0,0,0)`, `u=(65535,0,0)` et, pour
`1<=i<=24999`, les points `(0,0,i)` et `(65535,0,i)`. Chaque extra est
strictement hors de la boule diamétrale ouverte puisque sa distance carrée au
centre vaut `R^2+i^2`; ainsi `A(p,u)=2`. Chaque extrémité possède pourtant
24 999 points strictement plus proches que l'autre, donc le rang croisé vaut
25 000. Cette fixture réfute tout `k<=24999` sans complément sur cette entrée au
palier produit, mais ne sélectionne aucune architecture particulière.

À `5d9159a`, la contradiction `minimum_halfplane_count` n'était ni une fixture
source versionnée ni un CTest et le binaire n'avait aucun sidecar. `180975e`
grave désormais fixture, mutant et CTest; le sidecar de campagne et l'oracle
indépendant permanent restent absents.

## GO fonctionnel owner à `d1666f4`, avec garde de type encore partielle

`owner_rays_ok` réduit maintenant la valeur `i128` de `orient3d_exact` par
`sign_of` avant `tangent_sign`. Les frontières axiales 1290/1291 et alternées
1023/1024/1025, plus 2048 et 1626, rendent les catalogues owner et normal
identiques. La fixture directe produit 215 cas, zéro désaccord et 84 témoins
dont le signe aurait été inversé par la troncature. GCC, Clang et une exécution
ASan/UBSan ciblée sont verts; remettre l'appel `i128` brut échoue à compiler.

Le CTest `mhgp3v_flats_u16_owner` est désormais committé et passe. Le P0
fonctionnel est donc fermé sur le profil u16 audité. Deux réserves restent
documentaires et d'API : le commentaire source attribue encore `-INT_MIN` à
l'échelle 1025 alors que l'égalité arrive à 1024; les surcharges supprimées
bloquent `i128` et `long long`, mais plusieurs petits types entiers et enums
restent convertibles vers `int`. Elles tuent le mutant exact observé, sans créer
un type fort garantissant que l'argument appartient à `{-1,0,1}`. L'identité du
sommet owner signé demeure par ailleurs une porte distincte non fermée.

## Delta replay post-`8481b67` — anti-vacuité fermée, payload exact toujours NO-GO

Verdict épinglé au worktree `device_wavefront_job.hpp` de SHA-256
`351e25957117b68cb9da729b7802ce80cc0b84f61a12643670bcb40e9b592b1e`,
`device_wavefront_qualification.cpp` de SHA-256
`dfc146e841c30c151bec007dbeca6f8f307f880216d70142eb0af475a90fb401`
et `order_k_device_core.hpp` de SHA-256
`8d8c34031df8a3b4108ae366c6db17074b97a8e0aab1e133f8d754fae990fd6d`.
Ce delta n'est pas encore committé au moment de l'audit.

Le progrès est réel. Les planchers sont séparés, les refus d'admission sont
comptés, le mutant `--force-refuse-all` rougit, le job reçoit des contrôles
structurels nommés et les statuts de capacité/invariant sont séparés. Les cinq
CTest wavefront CPU passent. La campagne de refus rend 2 542 sommets présentés,
2 515 acceptés, 27 `kFlatOverflow` et 27 rejeux comptés, zéro fatal :
`15346+1403=16749` flats et `7109+224=7333` couples concordent avec les totaux de
référence. La fixture 32/35 et son masque `0x940800000009` sont sensibles.

Cela ferme la **vacuité du juge**, pas encore le replay structurel. L'oracle
`reference_vertex` est calculé pour tous les sommets avant l'admission; lors
d'un refus, le code n'émet ni ne conserve le suffixe. Il additionne seulement
`reference[index].flats`, `reference[index].couples` et `replayed++`.
`mass_identity()` compare donc deux totaux scalaires. Une permutation,
substitution, perte et duplication compensées dans les trois flats 32--34 de la
fixture restent invisibles. Les champs nommés `committed_*` et `replayed_*` sont
des compteurs, pas un commit/replay de payload.

La nouvelle `signature` ne ferme pas ce trou. C'est un FNV-1a 64 bits de la
position, des trois identifiants de base, de la **taille** de fermeture et des
deux bits; les identifiants de la fermeture ne sont jamais repliés. Un mutant
qui remplace un membre à taille, base et bits constants passe, et une empreinte
64 bits collisionnable ne peut établir une égalité exacte. L'exigence demeure
une séquence canonique complète `(closure,base,slot,verdict)` avec multiplicité,
ou une comparaison structurelle équivalente qui échoue fermée.

Enfin les statuts sont fail-open : `summarise_into` traite toute valeur autre
que les deux enums connues comme `kOk`. Une sonde `status=777, flat_count=1,
mask=1` est comptée acceptée et peut satisfaire ledger et masse. Tout statut
inconnu doit être fatal et une mutation permanente doit le prouver. `pending`
n'est alimenté par aucun chemin; le commentaire inclut les fatals dans
`refusés=rejoués+pending+fatals`, tandis que `total_refused()` et
`ledger_balances()` les excluent. Le high-water dit « tous les sommets » exclut
également les refus à l'admission.

Deux incohérences de statut complètent le NO-GO. `pending` est décrit comme un
trou, mais la porte autorise toute valeur telle que
`refused == replayed + pending`; elle n'exige jamais `pending==0`. Et
`Admission::kInteriorAboveContract`, documenté comme entrée malformée, est rangé
par la qualification parmi les refus rejouables et crédité dans la masse. Comme
`admit` teste la coquille avant l'intérieur, `shell=33, interior=31` masque même
l'invalidité intérieure sous `kShellOverflow`. Le job ne transporte enfin pas
`smax`, donc il ne peut vérifier la borne de campagne
$\lvert B(v)\rvert\leq s_{\max}-2$, seulement le plafond global 30. Il manque
aussi une identité de nombre de sommets : un sommet sans flat peut disparaître
sans modifier les deux masses comparées.

L'authentification du nuage reste elle aussi partielle. Un sixième point qui
duplique exactement une coordonnée existante, inutilisé par la racine et le
sommet, reçoit son digest correct puis `validate_job` rend `valid`. Le chemin
produit classe pourtant ce domaine `kDuplicateCoordinates`. Le digest
authentifie les octets fournis; il ne remplace pas la validation du profil.
De même, un shell trié contenant un point hors de la sphère du tétraèdre, avec
un intérieur plausible et `level=1`, est déclaré valide : cosphéricité, census
et niveau géométrique ne sont pas recertifiés. `validate_job` est donc un
validateur **structurel** sous précondition d'un payload CPU déjà certifié, pas
une authentification scientifique autonome du lot.

## P1 — ce kernel ne décide pas encore la reverse-search

`evaluate_vertex` énumère les flats et met deux bits d'admissibilité par flat.
Il n'appelle ni `neighbour_along`, ni `backward_pair_admissible`, ni
`decide_child`. Une admissibilité de retour positive ne suffit pas : un couple
antérieur peut être admissible et imposer `Reject`.

Un masque nul ne certifie même pas l'ordre. Les six premiers points de la
fixture précédente portent 20 flats et un masque nul; toute permutation des 20
flats conserve `(flat_count,mask)`. La porte actuelle ne voit donc pas une
régression de clef canonique sur ce cas.

Le batch n'est pas un découpage de tâches : il est la sortie matérialisée du
parcours CPU avec `seen`. Sur la fixture à sept points, le vrai arbre parent
possède 18 sommets et six sous-arbres racine disjoints de tailles
`1,1,2,6,1,6`; un descendant apparaît pourtant dans le batch avant son parent
canonique. L'indice du batch n'est ni un `task_id` structurel ni un ordre
topologique.

## P1 — contrat de job et enveloppe CUDA ouverts

`WavefrontJob` transporte des pointeurs et tailles bruts sans authentifier :

- le profil u16 et le digest du nuage;
- `root_size==4` et l'indépendance de la base;
- les bornes des identifiants;
- coquille/intérieur triés, uniques et disjoints;
- `level==interior_size` et les capacités;
- les multiplications de tailles avant allocation.

`point_count` n'est jamais lu par l'évaluateur. Un job
`root_size=0, root_base=nullptr` obtient encore `kOk`; une entrée malformée peut
donc devenir un accès hors limites device au lieu d'un `invalid_contract` avant
lancement. Les queues et le padding de `BoundedVertex`/`WavefrontJob` ne sont
pas initialisés avant leur copie, ce qui interdit aussi tout digest byte-stable.

Le CMake filtre maintenant `-Wall -Wextra -Werror` sur le seul C++ : c'est une
correction utile. L'enveloppe reste ouverte. `enable_language(CUDA)` précède le
fallback `CMAKE_CUDA_ARCHITECTURES=120-real`; CMake initialise normalement la
variable pendant cet appel, si bien que le fallback peut ne jamais agir. Une
surcharge arbitraire reste acceptée. Le build n'impose ni compilateur NVIDIA,
ni version patch du toolkit corrigée pour `__int128`, ni architecture exactement
120, ni politique d'avertissements CUDA. Le temps publié est kernel-only; il
exclut allocations, copies et surtout la construction CPU de tout le lot.

Le contrat v3 doit rester intrinsèque : seules les invariants, sources et reçus
du snapshot v3 peuvent qualifier ce kernel. Un commentaire d'intention ne
remplace ni la garde CMake ni le reçu device.

La compilation Clang 18 device-only du header v3 produisait 144 octets de local par
thread et une forte pression de registres virtuels. La session G4 rapportée ne
conserve aucun diagnostic `ptxas`, spill, stack, registre ou occupation; elle ne
permet donc toujours pas de relier le débit observé aux ressources du cubin.

## Audit du diagnostic G4 et de son interprétation à 100 ms

Le README rapporte quatre mesures kernel-only : 128 955 sommets en 0,224 ms,
71 084 en 0,170 ms, 19 019 en 0,323 ms et 2 542 en 2,020 ms, toutes avec zéro
écart CPU/device sur le `VertexVerdict` borné. En l'absence des sorties brutes,
elles sont conservées comme **diagnostics déclarés**, pas comme reçus
reproductibles.

Le journal local externe permet de retrouver les paramètres des deux grandes
campagnes : `clouds=3,points=120,coord=4000,smax=8,seed=5` et
`clouds=2,points=200,coord=8000,smax=6,seed=9`. Le chemin hôte reconstruit au
même snapshot reproduit exactement `128955/515820/340781` puis
`71084/284336/186885` pour sommets/flats/admissions, avec zéro écart sur `kOk`.
C'est un renforcement positif de la provenance des masses; le timing device et
le binaire restent non reçus tant que ce journal n'est pas scellé dans le dépôt.

Le transport hôte/device est un résultat positif ciblé. Son interprétation
quantitative doit toutefois distinguer les appels, les résultats admissibles et
le pipeline :

1. la première campagne a 515 820 flats et donc 1 031 640 appels directionnels;
   `0,224 ms` correspond à environ 4,61 milliards d'appels par seconde, tandis
   que les 340 781 résultats admissibles correspondent à 1,52 milliard par
   seconde. « Un milliard d'évaluations » mélange ces deux métriques;
2. le maximum 32 de la campagne de refus n'est pas une moyenne : les 15 346
   flats publiés plus les 27 préfixes de 32 donnent 16 210 flats évalués, soit
   6,38 par sommet. Le facteur 450 confond aussi de petits lancements sous-remplis,
   la charge par sommet et la divergence; aucune de ces causes n'est isolée;
3. multiplier un terrain hypothétique par 575 M sommets/s suppose que la
   distribution de coquilles/flats reste celle de la ligne la plus favorable,
   alors que la propre campagne dégénérée tombe à 1,3 M sommets/s.

Le chrono entoure seulement le kernel. Chaque `BoundedVertex` occupe 268 octets
et chaque verdict 16 octets. La ligne 128 955 transfère donc environ 34,6 Mo de
vertices et 2,1 Mo de verdicts hors de la fenêtre 0,224 ms. Un terrain
hypothétique de 50 à 150 millions demanderait environ 14,2 à 42,6 Go pour ces
deux flux, sans compter le nuage, les allocations ni la représentation CPU à
vecteurs. Le live matérialise en outre tout `seen_vertices` avant le lancement.

Le commit `444b851` corrige ensuite le budget primaire à 100 ms, mais sa nouvelle
conclusion ne découle toujours pas de la mesure. Les 1 096,8 sommets par point à
`n=300` ne sont pas une borne inférieure à `n=50 000`; ce profil avait en outre
une densité décroissante, faute que `f851374` reconnaît explicitement. Diviser
55 millions par 575 millions donne bien 95,7 ms **sous ces deux hypothèses**, pas
une mesure du terrain ni du pipeline. Aucun parcours GPU complet n'existe pour
recevoir le facteur « dix à trente » ou le verdict « quinze fois trop lent ».

Le ratio 6,5 ne ferme pas davantage une décision d'architecture. À `n=300`, il
compare les sommets visités aux sommets bien centrés; le rapport au catalogue
complet publié vaut environ 4,66 et ce catalogue est dominé par les arités deux
et trois. « Énumérer directement les sphères critiques » est une piste
constructive importante, mais aucune borne inférieure n'exclut encore
élagage, sauts, compression ou requêtes groupées du parcours. Elle devient une
obligation à prouver, pas une condition déjà démontrée.

Le chrono G4 entoure seulement un microkernel sur une entrée déjà produite et
copiée. `neighbour_along`, parent, source, census, owner, tri, fold, couverture,
verticales, copies, mémoire et sortie ne sont ni inclus ni bornés. Dire que ce
prédicat domine le pipeline avant d'avoir mesuré ces étages inverse la charge de
la preuve.

Le contrôle GCP de l'auditeur a été strictement en lecture seule. Les deux VM
labellisées `project=e-hgp`, dont `ehgp-blackwell-spot-ai1a` démarrée à l'heure
compatible avec la session, sont actuellement `TERMINATED`, de type
`g4-standard-48`, `SPOT`, action `STOP`. Cela crédite l'état final GCE observé;
le dépôt ne contient toutefois ni handoff de génération, ni log du double
coupe-circuit, ni reçu de révocation de la clé pour cette session.

## P1 — le profileur à densité fixe est utile, pas décisionnel seul

Le commit `f851374` corrige une faute de protocole réelle : le profil cube
antérieur faisait croître chaque côté comme `sqrt(n)` et diminuait donc la
densité. `scale_profile.cpp` propose maintenant un cube à volume proportionnel
à `n` et une nappe synthétique d'épaisseur bornée. En Release, la commande
`--points 100 --smax 11 --repeats 2 --seed 20260809` reproduit 805,5 sommets
par point, 159,28 sphères par point et les arités
`1,00/20,11/77,36/60,80`. C'est un nouveau diagnostic positif et reproductible
côté CPU.

Le commit `70ead99` publie ensuite quatre tailles `n=100/200/400/800`. Les
valeurs de sommets par point `805,5/1 011,5/1 171,9/1 271,9` et de catalogue par
point `159,3/219,8/266,3/299,9` sont utiles : les incréments observés diminuent
sur cette fenêtre. Elles ne prouvent toutefois aucune convergence. Les nuages
sont indépendants, une seule densité et trop peu de graines sont publiées, et
trois incréments n'identifient ni une asymptote géométrique ni même une fonction
bornée. Une loi logarithmique, une puissance lente ou une nouvelle transition
au-delà de 800 restent compatibles avec ces quatre points.

Le tableau ne suit même pas un protocole homogène. Au binaire Release identique
et à la graine `20260809`, la ligne cube `n=100` publiée correspond à
`repeats=2` (`805,5/159,28`), tandis que `n=200` correspond à `repeats=1`
(`1011,5/219,78`); avec `repeats=2`, cette dernière vaut
`1013,5/216,80`. Le premier incrément catalogue et les ratios qui en découlent
comparent donc des estimateurs différents. Commande, nombre de répétitions et
dispersion doivent apparaître par ligne avant toute régression.

Les lignes cube `n=400/800` ont été recertifiées avec une répétition et la même
graine; elles retrouvent exactement `1171,9/266,28` puis `1271,9/299,94`. La
nappe correspondante rend `1162,1/240,47` puis `1250,2/257,00`. Si les sommets
diffèrent de moins de 2 %, les sorties diffèrent de 9,7 % puis 14,3 % : une seule
densité ne permet pas d'attribuer causalement la masse au seul paramètre densité,
et les deux familles n'ont pas de limite commune prouvée.

Dans le modèle cube uniforme continu, une homothétie globale préserve d'ailleurs
la combinatoire Delaunay/order-k : la densité absolue n'agit sur ces ratios que
par quantification et effets de bord. La nappe à épaisseur `z=40` n'est pas une
homothétie lorsque `n` croît. Dire que « la densité décide » n'est donc ni une
conclusion causale de la table ni un invariant commun aux deux profils.

Les asymptotes `1 430` sommets/point et `390` sphères/point sont donc les sorties
d'un modèle choisi après observation, sans ajustement documenté, intervalle ni
validation hors échantillon. Les masses `7,1e7/1,9e7` à 50 k, les `0,124 s` et
le facteur `15--40` qui en découlent sont des scénarios conditionnels, pas un
écart « mesuré ». Le facteur `10--30` du pipeline reste lui-même sans pipeline
GPU complet ni reçu.

Il ne peut cependant être « le seul chiffre qui décide » les 100 ms :

- la densité `1e-3` est codée en dur et le profil LiDAR est une nappe uniforme
  synthétique, sans famille sanctionnée, digest d'entrée ni quantile;
- les nuages de statut non `kOk` sont retirés de la moyenne; `decided>0` permet
  donc une moyenne partielle sans ledger des refus;
- la déduplication du générateur emploie `std::find` dans un vecteur et coûte
  $O(n^2)$ hors chrono;
- le temps navigation exclut `CertifiedIndex::build`, le temps catalogue
  contient un second parcours via `flat_catalogue`, et la sortie « sans
  accélérateur » est ambiguë puisque l'index est actif;
- le catalogue emploie `use_index=true,use_owner=false`; il ne mesure donc ni le
  chemin owner actuellement faux sur u16, ni une source critique directe;
- `std::uniform_int_distribution` n'est pas une spécification de flux portable
  entre bibliothèques standard; une graine sans digest du nuage ne suffit pas à
  un reçu inter-machine;
- ni source directe, ni fold, ni forêts, ni couverture, ni verticales, ni
  octets, ni pipeline GPU ne sont mesurés.
- la cible n'a ni CTest permanent, ni plancher de nuages décidés, ni reçu
  canonique des paramètres et compteurs.

La porte propre publie toutes les graines et tous les statuts, sépare taille du
terrain, taille de sortie et travail par étage, puis mesure des quantiles sur les
familles enregistrées. Un ratio observé reste un diagnostic; il ne devient une
borne à 50 k qu'après un théorème ou une exécution effectivement à 50 k.
Sous l'hypothèse non validée de 19 millions de sorties, `100 ms / 19 M` donne
bien 5,3 ns par sortie; cette division ne transforme ni les 19 millions en borne
ni un compteur d'appels/admissions du microkernel en budget de prédicats aval.

## P0 historique de `40ad152` — fermé par le sweep exact de `180975e`

Le commit `40ad152` ajoute
`prototype/admissible_pair_probe.cpp` (`SHA-256 8c89ccb627d7d0d531897b95ec24f56a473578744f16299d052133dd0fba6cc8`)
et son branchement CMake (`SHA-256 f6650252fde309be1e2a81d15b1254383bdff7af0e8c805e6bc233c56b0d2db3`).
L'objectif est positif : mesurer la masse des paires laissées par un lemme
nécessaire exact. Son oracle angulaire et la conclusion quasi linéaire du commit
sont toutefois faux.

`minimum_halfplane_count` ne teste que les directions portées par les points,
puis compte la frontière avec `cross>=0`. Le minimum d'un demi-plan **fermé**
peut être atteint entre deux directions : en plaçant la frontière juste après un
groupe collinéaire, ce groupe appartient au demi-plan ouvert complémentaire,
alors que tous les tests live le remettent sur la frontière et le comptent.

Fixture entière, statut `kOk` et `RelevantGP` :

```text
centre=(100,100,100), rayon^2=194
support={(113,100,95),(113,100,105),(87,105,100),(87,95,100)}
extras={(114,100,100),(115,100,100),(116,100,100)}
paire testee={0,1}, s_max=4
```

Les trois extras sont dans la boule diamétrale de la paire, hors de la sphère
critique et sur le même rayon projeté. Le demi-espace `x<=113` ne compte que les
deux extrémités, donc le minimum exact vaut 2. Le live rend 5 et incrémente
`missing=1` pour une paire vraie. Le programme peut ainsi imprimer `ECHEC` contre
un lemme correct.

La fixture est auto-certifiante : les quatre vecteurs support relatifs au centre
sont `(13,0,-5),(13,0,5),(-13,5,0),(-13,-5,0)`, tous de norme carrée 194,
affinement indépendants et de moyenne nulle. Le centre est donc strictement
intérieur à leur convexe et la sphère est bien la miniboule de support quatre.
Le probe complet temporaire qui produit `status=ok`, `truth01=1`,
`live_least=5` et `missing=1` a le SHA-256 source
`d084861fe9ec13ed26674d374df630a992345b5151e026b20a0a3b1a5bd9246d`
et le binaire `9517fb9c...`.

Les campagnes aléatoires restent un diagnostic utile : à `n=20/50/100/200`,
elles publient respectivement `190/1176/3885/10706` paires admises contre
`180/812/2113/5171` paires dites vraies, avec `missing=0`; le filtre seul prend
0,91 s à `n=200`. Mais l'erreur live **surestime** le minimum et rejette trop de
paires. Le sweep corrigé ne peut qu'augmenter `ADMIS`; ces masses sont donc des
sous-estimations de l'univers conforme, jamais un crédit de parcimonie. Quatre
tailles et une graine ne distinguent pas davantage une croissance linéaire d'une
croissance quadratique à 50 k.

Les quatre lignes publiées ne partagent pas non plus le même estimateur :
`n=100` correspond à deux répétitions, les tailles suivantes à une seule.
Le « zéro sur 424 250 paires » additionne une fois chaque $\binom{n}{2}$ alors
que l'exécution a effectivement testé deux nuages à 100 points, soit 429 200
couples nuage--paire. Même après correction du sweep, ces quatre observations
ne peuvent prouver un `Big-O` ni une masse à 50 k.

La réparation de `180975e` calcule le maximum de points dans un demi-plan
**ouvert** par fenêtre circulaire et deux pointeurs, puis utilise
`minimum_closed=always_inside+m-maximum_open`. Antipodes, rayons confondus,
points de la droite, contrats ouvert/fermé/vif et contre-exemple 2/5 sont des
fixtures permanentes; le calcul par seules directions vives est conservé comme
mutant. Un oracle indépendant en multiprécision, avec une autre projection et
un calcul quadratique sans tri partagé, donne zéro écart sur 74 613 multisets
planaires et 9 593 paires 3D. Release, ASan/UBSan et le CTest permanent passent.

Résultat positif distinct : pour la source Gabriel ouverte, employer la boule
diamétrale **ouverte** $D_{pu}^{\circ}$ et
$A(p,u)=2+\min_H\lvert(X\setminus\{p,u\})\cap D_{pu}^{\circ}\cap H\rvert$.
Si `p,u` sont sur une sphère de support `q`, le demi-espace dirigé vers son
centre ne contient, dans $D_{pu}^{\circ}$, que des points strictement intérieurs.
Ainsi $A(p,u)\leq2+\lvert I\rvert\leq q+\lvert I\rvert\leq s_{\max}$, sans
hypothèse sur l'extra-shell. Une vérification indépendante confirme la preuve,
y compris le centre diamétral, les projections nulles et les points de
frontière. La micro-fixture
`p=(99,100,100),u=(101,100,100),w=(100,100,100)` avec deux extra-shells
orthogonaux rend `open_A=3`, `closed_exact_A=4` et `closed_live_A=5`
(source temporaire SHA-256
`9311a2a163e4f77b2ed15f5d6c706ff34cf96da463bbe21923f1c8cfca4014c3`).

Même corrigé, ce probe ne décide pas seul la source industrielle. Sa « vérité »
vient de `flat_catalogue(...,s_max,...,verify_census=false,use_index=true)` :
elle partage les prédicats du sujet, porte sur le catalogue de rang fermé et
n'est pas un oracle indépendant de la source Gabriel ouverte à extra-shell.
Le CTest possède maintenant des planchers `min_true` et `min_admitted`, refuse
les statuts non `kOk` et empêche un juge vide. Il n'impose toutefois ni valeur
attendue, ni borne supérieure, ni digest, ni assertion sur les rangs, les
histogrammes ou un écart ouvert/fermé en campagne. L'oracle indépendant de
l'audit n'est pas une fixture versionnée. Une graine et un digest 64 bits ne
permettent pas de rejouer un flux `std::uniform_int_distribution` sur toute
bibliothèque standard; les grandes tables n'ont ni coordonnées ni sidecar brut.

Le calcul exhaustif balaye toutes les paires et tous les points, puis jusqu'au
carré des projections; son pire cas est quartique et sa cible `n<=5000` n'est
pas une enveloppe de performance. À 50 k, le seul census point--boule ferait
`n*binom(n,2)=62 498 750 000 000` tests avant le sweep. `ball_points` ne
publie pas ce nombre de tests, seulement les points retenus dans les boules.
Ce programme reste un diagnostic CPU borné; il ne génère pas la source.

Le commit `180975e` corrige aussi les quatre défauts du diagnostic k-NN : rangs
de compétition avec ex æquo géométriques, maximum vrai calculé avant admission,
classes `[1,1]`, `[2,3]`, `[4,7]` exactes et horloge séparée pour la matrice
$n^2$ et les $n$ tris. La matrice est refusée au-delà de 2 000 points. Le CTest
n'asserte cependant aucun de ces résultats, et « rang des admises » désigne les
seules paires admises par le filtre **ouvert**.

Le libellé `secondes filtre` n'est pas encore un chrono isolé du sweep : par
défaut `--mutant 1` y ajoute le mutant quadratique. À `n=200`, `--ranks 0` et
digest identique `9105fff8aa83bbf0`, il affiche 0,70 s avec mutant contre 0,16 s
avec `--mutant 0`. Il inclut aussi les deux sweeps ouvert et fermé, le census et
le bookkeeping. Toute campagne de coût doit donc imposer au minimum
`--mutant 0 --ranks 0` et nommer ce temps diagnostic combiné. Avec le mutant
désactivé, chaque ligne imprime encore `vif=0`, valeur qui signifie « non
mesuré » seulement dans le résumé.

Surtout, aucun petit `k` ne découle mathématiquement de l'admissibilité. Pour
`K=16000` sur la grille u16, prendre
`p=(16001,0,0)`, `u=(48003,0,0)`, puis les `K` points distincts
`p-(i,0,0)` et les `K` points `u+(i,0,0)`, `1<=i<=K`. Tous les extras sont
hors de la boule diamétrale ouverte de `(p,u)`; cette boule a support deux,
zéro intérieur et `A(p,u)=2`. Pourtant chaque extrémité possède `K` points
strictement plus proches que l'autre : le rang croisé minimal vaut 16 001.
Le probe externe (source SHA-256
`e6f2fc7b9a1fc64514c64e284455ba07600e6fa3f497e9a3fef87e5f53ca5317`)
reproduit `n=32002,max_coord=64003,min_cross_rank=16001,A=2`. Un k-NN peut
rester un filtre reçu ou une heuristique de priorité; il ne remplace jamais une
frontière complète avec certificats et replay.

La construction tangente à 50 000 points donnée dans le réaudit ci-dessus porte
le même résultat au palier produit, avec rang croisé 25 000 et sans dépasser
u16.

## GO sémantique F0, deux P1 sur le validateur et la permanence de la porte

Le carré tout $N_a$ donne maintenant une naissance avec zéro racine, quatre
nœuds `N` et un record direct d'arité quatre. Refuser une composante sans carrier
strict est devenu un mutant tué. Warshall, DSU et un oracle borné qui énumère les
partitions concordent sur 2 168 hypergraphes; 13 fixtures, 11 mutants et cinq
injections de rollback passent avec la même sortie en mode normal et sous
`python3 -O`. Les deux CTests F0 passent sur le build frais. Le P0 du fold
général est fermé.

Le nouveau `validate_regular_source` ne constitue pas encore, seul, un
validateur brut complet. Il compte les occurrences de handles stricts sans
vérifier leur unicité : un unique binding strict répété deux fois dans un record
direct est accepté avec le compte 1, alors que `resolve_batch` refuse ce lot par
`duplicate raw endpoint`. Il ne centralise pas non plus les contrôles d'arité et
de kind. Il faut soit faire passer le lot par un validateur structurel commun,
soit documenter la précondition et graver les négatifs correspondants.

Enfin, `find_package(Python3 COMPONENTS Interpreter)` n'est pas `REQUIRED`.
Avec `-DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE`, la configuration réussit et
les deux tests F0 ne sont pas enregistrés. La correction sémantique reste vraie,
mais le claim « porte permanente » est fail-open jusqu'à rendre Python
obligatoire pour cette configuration ou à garantir explicitement la gate CI.

## Résultats positifs conservés

- Le build Release frais de `180975e` passe les quatre portes ciblées
  `flats_u16_owner`, `gate_d_fold_f0`, `gate_d_fold_f0_optimised` et
  `admissible_pair_sweep` en 15 s.
- Le sweep exact et sa projection étroite donnent zéro écart contre l'oracle
  indépendant sur 84 206 cas cumulés; le sens
  `A_open <= A_closed`, donc `admises_open >= admises_closed`, est confirmé.
- Le P0 owner passe ses frontières arithmétiques et 84 témoins de troncature;
  le P0 F0 passe sa naissance tout $N_a$ sous trois calculs de fermeture.
- Les cinq portes flats Release passent : 4 990 cas, 328 560 sommets admis,
  2 703 016 couples concordants et zéro désaccord sur leur petit domaine.
- Les quatre portes `device_wavefront` hôte passent en Release.
- Les mêmes quatre portes passent sous ASan/UBSan; aucune alerte n'est observée
  sur les chemins CPU exécutés.
- Le commit rapporte quatre lancements G4 `sm_120` et zéro écart bit à bit entre
  le `VertexVerdict` hôte et device; l'option CUDA échoue fermée localement en
  l'absence de compilateur.
- Le masque 64 bits ne décale jamais de 64 : les deux slots du flat 31 occupent
  les bits 62 et 63.
- Le lemme de paire diamétrale ouverte a été vérifié indépendamment, puis sur
  59 154 incidences support--paire sans écart; la micro-fixture
  `open_A/closed_exact_A/closed_live_A=3/4/5` sépare les trois contrats.
- Le théorème `center-cover + degree` a été contrôlé sur 4 105 supports propres
  aléatoires et dix oracles `RelevantGP` bornés sans contre-exemple. Il reste
  conditionnel à sa capability et non implémenté.
- L'inventaire GCE en lecture seule confirme les cibles labellisées arrêtées.
- `git diff --check` est vert sur le snapshot documenté.

Ces crédits prouvent une exécution device déclarée et une égalité du payload
borné; ils ne prouvent ni les refus, ni le parent, ni le voisin, ni le pipeline.

## Aide mathématique et ordre d'implémentation transmis à Claude

### 1. Réduire le microkernel exact

Sous u16, chaque `orient3d` est strictement inférieur à $2^{51}$ et la somme des
quatre orientations racine à $2^{53}$. Le microkernel entier tient donc en
`int64_t`; `i128` reste nécessaire à `next`, pas à ce hot path.

Les deux directions se calculent en un seul scan. Si
`o_z=orient(base,z)`, la direction moins exige tous les `o_z>=0`, puis
`o_h>0` au niveau positif ou la somme racine négative au niveau zéro; la
direction plus emploie les inégalités opposées. Un probe CPU indépendant a
reproduit les deux appels live sans désaccord sur les campagnes permanentes.

Sur une coquille sphérique authentifiée de points distincts, trois points ne
sont jamais collinéaires. La base canonique d'un flat est donc simplement ses
trois plus petits identifiants. L'ordre des flats est l'ordre de ces triples.
Pour décider le parent, chaque page réduit sa plus petite clef admissible; une
réduction lexicographique entre pages remplace le masque fixe et reste exacte
au-delà de 32 flats.

### 2. Construire le premier vrai `next` GPU exact

Baseline sans mosaïque : un bloc par `(v,closure,direction)`, premier scan de
tout le nuage pour réduire le paramètre extrême exact en `i128`, puis second
scan pour compacter **tous** les ex æquo du minimum en ordre d'identifiants.
Cette baseline est une vérité de qualification, pas encore l'architecture 50 k :
en position générale, quatre flats et deux directions donnent environ `16*n*V`
visites de points pour deux passes. Sous la seule hypothèse diagnostique
`V=50` à `150` millions à `n=50 000`, cela ferait $4\cdot10^{13}$ à
$1,2\cdot10^{14}$ visites. Il faut donc certifier un index terminal
output-sensitive, ou une fusion prouvée des requêtes, avant toute extrapolation.

Fixture permanente :

```text
0=(0,0,0) 1=(4,0,0) 2=(0,4,0) 3=(0,2,2)
4=(0,0,4) 5=(0,0,2) 6=(4,4,2)
v: shell={0,1,2,3}, flat={0,1,2}, direction=+1
```

Le point 4, rencontré d'abord, donne l'événement plus lointain `t=2`; 5 et 6
donnent ensemble le minimum `t=1`. Le voisin attendu est
`shell={0,1,2,5,6}`, `interior={3}`, `level=1`. Cette fixture tue
`first-valid-wins`, la perte d'un ex æquo, le mauvais sens et l'oubli du
transfert intérieur. Elle contient aussi une arête parent positive et une autre
arête à retour admissible mais rejetée par un parent antérieur.

### 3. Source directe : lemme formulé, premier prototype non certifié

Une voie exacte sans propriétaire shallow est maintenant démontrée sous une
capability séparée `center-cover + degree`. Pour chaque arité
$q\in\{2,3,4\}$, poser $t_q=s_{\max}-q+1$. Une partition canonique de la boîte
du nuage authentifie, dans chaque feuille fermée, $t_q$ PointId distincts dont
la distance carrée maximale à tout centre de la feuille est strictement
inférieure à un entier $Q_q$.

Si une miniboule propre $B_U$ vérifie $q+\lvert I(B_U)\rvert\leq s_{\max}$,
son rayon carré est strictement inférieur à $Q_q$; sinon les $t_q$ témoins de
la feuille de son centre seraient tous intérieurs. Dès lors, pour une ancre
$p\in U$, tout point de la boule fermée appartient au voisinage exact
$N_q(p)=\{x\neq p:\lVert x-p\rVert^2<4Q_q\}$. Énumérer une fois chaque support
par $p=\min U$, tester le bien-centrage, puis effectuer le census dans ce
voisinage est complet sans sommet d'arrangement ni mosaïque.

L'ordre non circulaire est essentiel : localiser le centre rationnel, tester
d'abord la banque de témoins; tous intérieurs donnent
`AboveInteriorWindow`. Sinon un témoin non intérieur prouve
$\mathrm{beta}<Q_q$ **avant** le census local. Le fallback racine, complété par
l'énumération directe lorsque `n<t_q`, rend la méthode mathématique totale;
$Q_q=\sum_i\mathrm{span}_i^2+1$ peut toutefois donner $N_q(p)=X$ et ne prouve
aucun SLO.

La vérification indépendante n'a trouvé aucun écart sur 4 105 supports propres
aléatoires, dont 4 085 dans la fenêtre, ni sur dix comparaisons exhaustives du
critère `RelevantGP`. Ce crédit porte sur les lemmes, pas sur une implémentation.
La porte de coût doit recevoir le cover et sa construction, le degré complet,
les CSR, les masses combinadiques, les pas du locator, le tri/groupement et les
replays. Avec $d_q(p)=\lvert N_q(p)\rvert$ et $d_q^+(p)$ le degré vers les
identifiants supérieurs, elle publie au moins
$C_q=\sum_p\binom{d_q^+(p)}{q-1}$,
$T_q=\sum_p d_q(p)\binom{d_q^+(p)}{q-1}$ et
$H_q\leq T_q+t_qC_q$.

Cette voie sépare deux univers : le catalogue fermé exige
$\lvert I\rvert+\lvert S\rvert\leq s_{\max}$; la source Gabriel ouverte exige
seulement $q+\lvert I\rvert\leq s_{\max}$ et doit grouper tous les supports
par `SphereKey` avant de développer l'extra-shell. Le profileur fermé et son
ratio de 6,5 ne dimensionnent donc pas cette source.

Le prototype `bb31b426...` valide positivement la banque, la localisation et le
voisinage sur les oracles bornés. Il compare les listes complètes de membres par
coquille, reçoit les doubles émissions, sépare jugement/mesure/cover, applique
les replis racine et publie `C_q/T_q/H_q` en `u128`. La dispersion porte sur
toutes les feuilles effectives. Treize CTests Release et huit ciblés sanitizers
passent. Bas ordre, borne $K$, juge explicite, unicité, membre et masse candidats
sont reçus. Le résultat est un accord relatif borné crédible avec les records du
catalogue fermé partagé et un quotient forestier non vacant.

La porte de coût reste ouverte. Le cover rescane `n` points dans chaque feuille,
donc devient quasi quadratique à densité fixe; le CSR peut être dense et les
high-waters omettent plusieurs buffers, la vérité et la sortie. Le target refuse
`n>20 000`. L'ordre canonique du catalogue, son pool concaténé, ses offsets et
les indices publics de la forêt divergent malgré les digests abstraits égaux;
les deux côtés partagent en outre `build_forest`. La validation structurelle
ajoutée n'est pas totale sur cycles/indices, le chrono mélange les deux folds et
certains agrégats `i64` ne sont pas bornés sur 2 000 nuages.
Enfin le prototype matérialise un catalogue **fermé**, pas la source Gabriel
ouverte streamée. Ces défauts ne réfutent pas le lemme, mais interdisent toute
promotion produit ou 50 k. Voir
[`AUDIT_SOURCE_DIRECTE_24AD3D37.md`](AUDIT_SOURCE_DIRECTE_24AD3D37.md).

Il reste une incompatibilité contractuelle explicite à résoudre avant
conformité de l'implémentation : la norme courante exige encore un shell complet
pour tout
support rencontré avec $\lvert I\rvert\leq s_{\max}-2$. Le terminal
`AboveInteriorWindow` par arité est mathématiquement suffisant pour rendre
l'antécédent utile impossible, mais il doit être versionné dans le contrat; il
ne peut pas être déclaré conforme par simple optimisation.

### 4. Fermer les tâches avant le débit

Une tâche porte snapshot/digest, racine structurelle, sommet, curseur exact et
segment de sortie. Ses slots d'adjacence sont tous classés; donation du
sous-arbre, convention d'émission de sa racine et retrait du domaine du donneur
sont un seul point de linéarisation. Le segment ne devient public qu'au commit;
sinon un replay duplique son préfixe.

Ensuite seulement viennent owner par supports, census exact cappé, runs à clef
de niveau 384 bits, fold de lots complets et reçus 50 k/G4. La construction
détaillée est dans
[`NOTE_VERROUS_MATHEMATIQUES_GPU.md`](NOTE_VERROUS_MATHEMATIQUES_GPU.md).

## Porte exigée avant la prochaine session G4 qualifiante

La session déclarée confirme que le microkernel se lance; sa porte reste
censurée. Avant une nouvelle session prétendant qualifier davantage :

1. fermer le replay des 35 flats et le mutant all-refused;
2. authentifier le job et l'enveloppe CUDA;
3. imposer des planchers acceptés/refusés/rejoués et kernels lancés;
4. comparer les signatures complètes, pas seulement compte et masque;
5. sceller commit, diff, toolkit, driver, architecture, binaire, PTX/cubin,
   ressources `ptxas`, digest d'entrée et répétitions.

Le reçu 50 k final doit en plus publier temps bout-en-bout par étage, octets et
high-waters par conteneur, ledger des tâches, drains CPU, concordance exacte des
runs et arrêt ciblé GCP certifié. Le débit kernel-only du microkernel ne peut pas
valider le contrat industriel.

Les audits historiques ont utilisé GCP uniquement en lecture seule pour vérifier
l'état final de cibles. Pour le réaudit de `180975e`, **GCP non utilisé** :
aucune VM créée, démarrée, arrêtée ou modifiée par l'auditeur.
