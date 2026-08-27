# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex, avec relecture critique des audits concurrents
- **Pin CPU fonctionnel exécuté :** `6e8a6aba69b76dda936332abb7f8b1ef1b72f79f`
- **Pin de la sonde secteurs exécuté :** `d76f502c7ece561fc04e1d4f4a003b604465ced9`
- **Tip d'audit précédent relu :** `86272a2f9cb2de6d196eb2ea41e31892c7779fac`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_adaptatif`](../receipts/campagne_g4_v5_20260827_adaptatif/RECU.txt), source `8f95df2effd07ffa7a8aa7cf7fe79be1be9c7b2c`, publié par `a0d134205b5b4364ada1e6c12995f979f59698b4`
- **Worktree observé hors verdict :** intégration produit de `sector_kill`, non commitée au-dessus de `d76f502c` ; le probe racine `.codex_fold_contract_probe.cpp` appartient à un autre auditeur et n'a pas été touché
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; aucune VM interrogée ou mutée

## Verdict

La v5 reste **orange, avec des progrès réels**. Le pin CPU `6e8a6aba` passe
les **161/161** portes Release, dont 7 oracles. Onze portes ciblées passent
aussi sous ASan+UBSan en Debug sans diagnostic. Le préflight des lots, le repli
vers le corps hôte, les assertions de routage et le durcissement Q4 ferment une
partie importante de l'audit précédent. Ils ne soldent toutefois ni les
domaines d'index et additions vérifiées, ni l'exhaustivité du validateur Q4,
ni l'autorité des overrides, ni la qualification CUDA courante.

La sonde `d76f502c` met en évidence une piste utile : sur certaines familles
denses, un certificat sectoriel pourrait éviter beaucoup de seeds sans
construire de mosaïque d'ordre supérieur. Son théorème de suffisance n'a pas
révélé de faute sur la variante commitée. En revanche, les sorties brutes ne
constituent pas un reçu décisionnel, leur synthèse contient plusieurs chiffres
faux, et le « zéro faux positif » compare au producteur courant plutôt qu'à un
oracle indépendant. La variante actuellement intégrée au produit est en outre
différente de celle mesurée.

Au snapshot de worktree relu, les conformités nominales `uniform` et
`eight_clusters` à 1 200 points passent. Le mutant non strict était invisible
sur ces deux gros nuages et rendait le code 3 ; Claude l'a remplacé par une
fixture de frontière déterministe. Après renforcement de sa population, la
porte nominale et le mutant attendu au code 4 passent. Cette fixture qualifie
la stricte frontière Q3 visée, pas encore le théorème complet ni Q4.

## Résultats établis

| Périmètre | Résultat observé | Portée exacte |
|---|---|---|
| CPU `6e8a6aba` | Release `gate` : **161/161**, dont 7 portes `oracle` | build local canonique, pas un reçu pérenne |
| ASan+UBSan Debug `6e8a6aba` | **11/11** portes API, batch, exception, routes et replis ciblés | aucune généralisation à toute la suite ni à CUDA |
| ASan+UBSan RelWithDebInfo | compilation refusée avant test dans `cloud_index.hpp` par `-Werror=array-bounds` sous GCC 13 | échec de build optimisé à traiter séparément ; ce n'est pas un diagnostic sanitizer d'exécution |
| Replis oversized CPU | Q3 et Q4 non vides, lots plafonnés, zéro désaccord | probes manuels ; les CTests restent vacuables sur `anchors_oversized` |
| Protocole local | selftest : `violations=0` ; docs au contrôle final : 210 Markdown ; registre : 20 phases | les contre-tests adversariaux ci-dessous restent acceptés |
| Sonde `d76f502c` | cible Release compilée ; deux smokes code 0 | aucun des 176 CTests du pin ne lance la sonde |
| Intégration secteurs non commitée | 2/2 conformités nominales ; fixture ciblée nominale et mutant non strict : **2/2** | résultat sur worktree mouvant ; la fixture cible une frontière Q3, pas l'exhaustivité Q3/Q4 |
| G4 source `8f95df2e` | quatre couples CPU/GPU à 50 k, deux digests appariés, petites lanes non vides, mutant Q3 code 4 | égalité bornée observée ; campagne partielle et non terminale |

Les probes oversized manuels ont observé :

- Q3 : ancres device/hôte/oversized `22472/20794/19809`, seeds
  device/hôte `210290/2098076`, maximum de lot 50 seeds, zéro désaccord ;
- Q4 : ancres device/hôte/oversized `17407/8857/7876`, seeds
  device/hôte `155155/781669`, maximum 50 seeds et 5 750 paires, zéro
  désaccord.

Ces observations prouvent que le repli a été emprunté pendant l'audit. Les
CTest `ancre_trop_grande` ne l'imposent toujours pas : une régression qui rend
`anchors_oversized=0` peut rester verte.

## Requalification du reçu G4

La formulation autorisée reste **égalité bornée observée au pin `8f95df2e`**,
pas « lane exacte » en général. Les quatre contrats `--gpu` terminent au code 0
avec `digest_balls` et `digest_all` identiques au CPU ; la porte lane brute
présente quatre cas Q3 et quatre cas Q4 non vides, et le mutant Q3 device est
tué au code 4.

La campagne reste partielle : 24 runs sur 25, adaptatif
`scanline_single_pass` absent, journal de session perdu, trap non exécuté et
aucun verdict final du validateur. Elle ne qualifie ni le précomptage Q4, ni le
layout SoA réduit, ni le routage hôte direct de `10c46c87`, tous postérieurs à
sa source. Le reçu raconte l'arrêt ciblé mais ne conserve pas sa sortie de
certification.

### Temps de bout en bout à 50 000 points

| Famille | CPU 48 fils | GPU 48 fils | Surcoût GPU | Pic RSS CPU / GPU | Digests appariés |
|---|---:|---:|---:|---:|---|
| `uniform` | 78 s | 89 s | +14 % | 19,3 / 19,0 Go | `balls` et `all` |
| `terrain` | 23 s | 44 s | +91 % | 3,68 / 5,31 Go | `balls` et `all` |
| `scanline_single_pass` | 38 s | 96 s | +153 % | 3,10 / 7,25 Go | `balls` et `all` |
| `eight_clusters` | 246 s | 718 s | +192 % | 17,6 / 17,5 Go | `balls` et `all` |

L'adaptatif `eight_clusters` à `min_sites=256` prend 713 s et 18,2 Go. Les
rapports recalculés depuis ses compteurs sont :

- Q3 : `(anchors_device + anchors_host) / rect_alive = 4,6926` et
  `anchors_device / rect_alive = 3,3173` ;
- Q4 : `(anchors_device + anchors_host) / rect_alive = 1,6272` et
  `anchors_device / rect_alive = 0,5139`.

Ces populations hôte et device n'ont pas la même sémantique : les rapports ne
mesurent ni les « ancres avec seed » ni le partage d'un cover. Les comptes
`generation` bruts donnent environ 9,84 ancres par rectangle vivant Q3 et
13,62 en Q4. Parmi les seeds, 99,1 % en Q3 et 91,3 % en Q4 partent au device.

Le diagnostic causal reste probabiliste. Le pin mesuré fabrique encore sur
CPU covers, seeds, formes et lots, puis copie SoA et résultats ;
`eight_clusters` traverse 18,22 milliards de seeds Q3 et 1,49 milliard Q4. La
génération passe de 189 s CPU à 659 s GPU, tandis que `kernel_ms=111196,5` est
un cumul d'événements de 48 exécuteurs, pas un mur GPU. Écrire
« matérialisation et orchestration probablement dominantes » jusqu'à disposer
de murs séparés préparation/H2D/kernel/D2H.

## P1 — fermetures encore requises sur `6e8a6aba`

### Capacité et routage

- Q3 teste encore `size() + ajout > cap`; Q4 fait de même pour plusieurs
  buffers. L'addition peut déborder. Comparer à `cap - courant` après avoir
  vérifié `courant <= cap`.
- Q4 caste `i` en `u32` dans `lens_idx` avant le rejet `nc > UINT32_MAX`.
  Borner avant tout cast les domaines cumulés sites, lentilles, ancres, seeds,
  paires et émissions.
- Ajouter aux portes oversized un plancher explicite sur le compteur de repli,
  idéalement une fixture distincte pour sites, seeds et paires.
- Les statistiques mélangent encore ancres routées, ancres matérialisées,
  ancres sans seed et morts W4. Séparer ces populations avant de comparer des
  ratios.
- `--gpu-min-sites` passe par `std::atoll` : `1x` est accepté et l'overflow
  n'est pas diagnostiqué strictement. Tester zéro, négatif, suffixe et
  dépassement au code 2.

### Validateur Q4

- `validate_q4_batch_view` déréférence des tableaux structuraux avant leurs
  gardes nulles ; `validate_q4_results_view`, appelée seule, suppose l'ancre du
  seed valide.
- Un seed vivant ayant une complétion admissible mais `stages={}` et
  `emits={}` passe encore. Recalculer exactement le nombre attendu de
  complétions et d'émissions ; la fixture « lost emission » actuelle laisse
  `st.emit=1` et ne falsifie pas la suppression cohérente de tout le flux.
- L'option publique `emit_eq=false` ne doit pas être un bypass du validateur
  d'autorité. Employer une fonction de mutant séparée.
- La recherche de chaque `y` dans toute la lentille est en
  `O(n_emits * lens_count)`. Verrouiller l'ordre des indices puis employer une
  recherche logarithmique ou un parcours fusionné.

### CUDA et autorité de résultat

CMake enregistre encore `mhgp5_q3_lane_device_route_mutant` avec
`--inject=route-ignore-threshold`, mais la source CUDA Q3 commitée ne parse pas
`--inject` et n'appelle pas `mutants_enable`. Si CUDA était disponible, cette
porte rendrait 2 plutôt que le 4 attendu. `nvcc` est absent de la machine de
l'auditeur : aucune compilation CUDA courante n'est revendiquée.

La décision sur les overrides reste : conserver le statut transactionnel
`complete_regular`, mais ajouter une autorité machine-readable orthogonale,
par exemple `cpu_reference` ou `experimental_override`, et la propager aux
callbacks et au protocole. Un override vide peut terminer expérimentalement ;
il ne peut pas devenir référence par le seul statut terminal.

## P1 — protocole de campagne

Le selftest courant passe localement, mais des mutations adversariales montrent
que le contrat est encore trop permissif :

- un run `min_sites=1` entièrement hôte en pratique a été accepté ; exiger du
  travail device non nul séparément en Q3 et Q4 pour ce régime ;
- un adaptatif avec `anchors=0/0`, seeds positifs et statut complet a été
  accepté ; imposer ancres **et** seeds non vides des deux côtés ;
- comparer la conservation appariée : les sommes hôte+device des ancres et des
  seeds doivent être identiques entre tout-device et adaptatif pour chaque
  lane ; le reçu 8f satisfait la conservation des seeds ;
- le validateur ne lie pas chaque corps à la famille, `n`, `s`, `smax`, seed et
  threads annoncés. Un faux pilote peut réutiliser les mêmes digests pour
  plusieurs familles ;
- la phase GPU lance encore six contrats coûteux après l'échec d'un contrat CPU
  de phase 2. La conditionner aux quatre codes CPU nuls et aux deux digests ;
- le selftest ne falsifie que `digest_all` et `remote_rc`. Une version privée
  du validateur sans contrôle `digest_balls` ni `scp_rc` reste verte. Ajouter
  des scénarios balls-only, transport, backend absent et route device vide ;
- ajouter `timeout --kill-after` pour qu'un processus ignorant `TERM` ne dépasse
  pas la borne transactionnelle.

## P1 — sonde et certificat sectoriel

### Ce que la mesure montre réellement

Les fichiers bruts de [`mesures_secteurs_20260827`](../receipts/mesures_secteurs_20260827/LISEZMOI.txt)
sont intéressants, mais leur synthèse et le message du commit ne sont pas
fidèles aux sorties :

- `eight_clusters`, Q3, n=4000 contient 46,7218 s pour le corps et 7,1188 s
  pour le test, non 86,4 s et 15,1 s ;
- Q4, n=4000 contient 31,3773 s et 0,9614 s, non 34,3 s et 1,1 s ;
- les ratios cover/rectangle valent 0,50–0,51 en Q3 uniforme mais 0,28 en Q4,
  6,92–10 en Q3 clusters mais 1,23–1,31 en Q4, et 2,21 en Q3 scanline mais
  0,98 en Q4. Le résumé `scanline 2,2–3,5` n'est soutenu par aucun fichier.

Le partage potentiel est donc très dépendant de la lane et de la famille : un
candidat de rectangle naïf coûte environ deux fois la somme des covers Q3
uniformes et plus de trois fois les covers Q4 uniformes, tandis qu'il devient
favorable sur Q3 clusters. Ce résultat déconseille une représentation unique
non routée ; il ne mesure pas encore la lane par rectangle.

La comparaison temporelle n'est pas un gain attendu. `t_prod_killed_ns`, qui
devrait isoler le coût de production des seules ancres éliminées, est calculé
après le test secteur, contient ce test, multiplie la soustraction par zéro,
puis n'est jamais imprimé. Le temps « corps » exclut histogrammes, handles,
cover et morts W4. En Q4, `rect_sum_covers` exclut les covers déjà payés des
ancres W4-mortes ; les dénominateurs deviennent asymétriques.

Il manque encore lentilles, `seeds * cover`, `seeds * lens`, complétions,
visites, octets H2D/D2H, RSS et coût conditionnel des ancres tuées. La sonde
stocke neuf `u64` par rectangle/ancre, trie plusieurs fois, réalloue le vecteur
de candidats par ancre et additionne sans garde. À grande taille, elle peut
elle-même devenir le principal coût ou OOM.

### Pourquoi ce n'est pas encore une preuve

- `wrong > 0` est seulement imprimé ; `main` rend toujours 0. Un cas
  `uniform n=40 q3` tue zéro ancre, annonce zéro faux positif et réussit.
- `wrong` fusionne K4 et K8 et peut compter deux fois la même ancre.
- La référence est `scan_anchor_q3` ou `process_anchor_q4` avec le même cover,
  pas un oracle exhaustif indépendant.
- La cible est compilée par `mhgp5_executable` avec `MHGP5_TESTING=1` ; une
  mesure produit doit employer `mhgp5_product_executable`.
- Le parsing accepte `n=0`, `n=-1` et `n=3junk` au code 0. La ligne d'en-tête
  imprime la cardinalité demandée, pas nécessairement celle générée.
- La provenance omet commande exacte, hash de source, toolchain, `coord`, seed,
  `s`, `smax`, coefficient, filtre flottant, RSS, code de sortie et état du
  worktree. `HEAD 312034ce + sonde` n'est pas un pin reproductible.

La revue mathématique de la variante commitée n'a pas trouvé de défaut de
sûreté : les rayons carrés `D2/12` en Q3 et `D2/8` en Q4, la stricte inégalité
et les largeurs sous u16 sont cohérents. Le « K8 octogone » est toutefois un
rectangle avec quatre points colinéaires supplémentaires : c'est un éventail à
huit secteurs, pas un octogone ni un raffinement monotone de K4. Le zéro faux
positif observé reste une porte de réfutation, jamais une promotion.

### Intégration produit observée hors verdict

Le worktree remplace la construction orthogonale mesurée par une construction
à deux produits `d x axe`. Les pourcentages et temps de `d76f502c` ne sont donc
pas des mesures de ce code. Au snapshot relu :

- le builder batched teste le secteur, puis les routes hôte rappellent
  `scan_anchor_q3` ou `process_anchor_q4`, qui retestent le même secteur ; les
  ancres hôte non tuées paient deux fois ce coût ; Q4 répète aussi W4 ;
- les huit compteurs sont des `u32` incrémentés jusqu'à la taille du cover alors
  que seul le seuil `h <= 10` importe. Les saturer à `h` évite un domaine
  d'overflow inutile ;
- les conformités nominales `uniform` et `eight_clusters` n=1200 passent, mais
  le mutant non strict n'y change aucun digest : les deux portes ont rendu 3,
  pas 4 ;
- la première version de la fixture ne donnait que trois témoins par secteur ;
  sa population renforcée rend désormais les codes attendus 0 et 4. Elle
  vérifie la disparition d'une boule Q3 analytique sous le mutant non strict,
  sans devenir un oracle exhaustif du certificat.

Porte minimale avant activation :

1. un gate Q3 et Q4 qui énumère indépendamment tous les supports owner et leur
   profondeur exacte sur de petits nuages ;
2. égalité filtre activé/désactivé sur candidats bruts, post-RLE et digests,
   avec planchers de morts sectorielles non nuls ;
3. cas équilatéral Q3, tétraèdre régulier Q4, extrêmes u16 et égalités strictes,
   plus mutants de rayon, secteur oublié et non-strict ;
4. codes 1/3 pour échec/inefficacité et 4 seulement lorsque la contradiction
   ciblée est réellement observée ;
5. mesure de la variante effectivement intégrée, en mode produit, avec le coût
   secteur total comparé au coût évité sur les seules ancres tuées.

Pour réparer la fixture non stricte actuelle, une construction simple est
d'énumérer les 83 triplets relatifs entiers satisfaisant
`x*x + y*y + z*z == 625`, `z <= 0`, hors extrémités. Sous le mutant, les huit
secteurs courants reçoivent `9/23/46/60/60/46/23/9` témoins. Au centre de la
boule `abx`, les points `z=0` restent sur la coque et ceux avec `z<0` restent
dehors : le seed nominal reste donc un contre-témoin.

## P2 — documentation et nettoyage

- Rafraîchir entièrement [`GPU.md`](../docs/GPU.md). Il qualifie encore les
  lots 8f de « bornés », affirme trop directement que le kernel n'est pas la
  cause, sous-estime les payloads, conserve `K=1..10 exact`, décrit l'ancien
  cap post-ancre et annonce encore Q4 « en attente de G4 ».
- Distinguer les primitives terminales déjà `MHGP5_HD` du dataflow rectangle
  encore hôte : handles, covers, histogrammes, formes et bornes.
- Mettre [`MATHEMATIQUES.md`](../docs/MATHEMATIQUES.md),
  [`PLAN_DE_TESTS.md`](../docs/PLAN_DE_TESTS.md) et
  [`PROVENANCE.md`](../docs/PROVENANCE.md) au niveau des portes réellement
  présentes, sans promouvoir un payload de rendu public absent.
- Le pin différentiel `receipts/conformite_v4/familles_v4.txt` nomme encore un
  programme compilé hors dépôt sans source, commande, toolchain ni hash
  binaire. Le régénérer de façon rejouable avant de renforcer son autorité.

## Ordre de fermeture conseillé à Claude

1. Garder `sector_kill` hors autorité produit jusqu'à un oracle exhaustif borné
   Q3/Q4 et une mesure de la variante intégrée ; la fixture de stricte frontière
   est maintenant verte. Supprimer aussi le double scan des routes hôte.
2. Réparer l'activation du mutant CUDA Q3, les domaines `u32`, les additions
   vérifiées et les planchers des replis oversized.
3. Fermer l'exhaustivité du validateur Q4 et son bypass `emit_eq`.
4. Sceller l'autorité des overrides/callbacks et durcir le protocole sur les
   routes, la conservation, les deux digests et les préconditions de phase.
5. Corriger les reçus et `GPU.md`, puis seulement décider entre lane par
   rectangle, certificat sectoriel et routage hybride à partir de mesures
   appariées.

## Reproduction et limites

Résultats locaux au pin CPU `6e8a6aba` :

```text
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release : code 0
cmake --build build/v5 --parallel 4 : code 0
ctest --test-dir build/v5 --output-on-failure --parallel 4 -L gate : 161/161, 7 oracle, 261,66 s
bash gcp-migration/selftest_campagne_v5.sh : violations=0, PROTOCOLE CONFORME
python tools/check_docs.py : 210 Markdown actifs validés au contrôle final
python tools/check_implementation_status.py : 20 phases validées
```

Le build ASan+UBSan Debug ciblé passe 11/11 en 573,41 s réelles. Le build
RelWithDebInfo sanitizer s'arrête avant tests sur le warning GCC 13 promu en
erreur à `cloud_index.hpp:130-131`. Les journaux résident dans des builds
locaux non versionnés ; leurs durées ont donc une provenance plus faible que
les résultats fonctionnels.

Au pin sonde `d76f502c`, `mhgp5_rect_probe` compile en Release. Le smoke
`uniform n=40 q3` rend 0 avec zéro mort sectorielle ; `eight_clusters n=200 q3`
rend 0 avec 2 536 morts K4 et 2 605 K8. `ctest -N` compte 176 tests, dont 161
labelisés `gate`, et zéro test `mhgp5_rect*`.

Sur le worktree d'intégration observé, les deux conformités nominales n=1200
passent. Les deux premières portes mutantes opportunistes rendaient 3 au lieu
de 4. Après leur remplacement et le renforcement de la fixture ciblée, le
nominal et le mutant non strict passent aux codes attendus 0 et 4. Ces résultats
ne valent que pour le snapshot non commité et devront être rejoués au pin
publié.

Le validateur épinglé du reçu G4 rend toujours
`campaign_status=partial_or_failed` sur l'artefact adaptatif
`scanline_single_pass` absent. Les hashes du payload et du manifeste se
recomposent. `nvcc` est absent. GCP n'a pas été utilisé ; l'arrêt raconté par
le reçu n'a pas été recertifié. Le probe concurrent
`.codex_fold_contract_probe.cpp` n'a été ni ouvert, ni modifié, ni inclus.
