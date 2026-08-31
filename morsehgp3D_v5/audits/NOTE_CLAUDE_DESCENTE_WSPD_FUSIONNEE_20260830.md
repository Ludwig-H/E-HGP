# La descente WSPD est payée trois fois — mesure, porte de correction, et ce que la fusion coûte

Note Claude, 30 août 2026, ancrée au pin `119b80b0`. Sonde
`bench/wspd_fusion_probe.cpp`, cible `mhgp5_wspd_fusion_probe`, counter-only,
un fil, `s = 8`, `smax` effectif du pipeline, graine 3.

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Cette note ne revendique aucune borne et ne change pas l'objet.** Elle chiffre
une redondance d'exécution et fournit la porte qui la rendrait sûre à retirer.
Elle ne touche pas au verdict P0 d'`ETAT_COURANT.md` sur la sémantique de forêt,
qui reste le blocage prioritaire.

**Statut après contre-audit : hypothèse prometteuse, porte produit non reçue.**
La sonde prouvait l'identité de ses deux transcriptions locales, mais son bras A
n'appelait pas `alive_rectangles`. `collinear_seven` montrait déjà qu'elles
peuvent être identiques entre elles et différer du produit. Les phrases
ci-dessous qui présentaient la mémoire comme mesurée sont également rétractées :
la sonde ne publie qu'une charge utile minimale `size*sizeof(T)`, pas un HWM.

> [!NOTE]
> **Le premier de ces deux reproches est fermé.** La sonde porte désormais un
> **bras PRODUIT** qui appelle `generate_detail::alive_rectangles` telle quelle,
> et les bras A et B sont l'un comme l'autre jugés **contre lui**, pas l'un
> contre l'autre. Elle dérive en outre ses seuils de
> `smax_effective = min(smax, nombre de points rendus)` comme `run.hpp:225`, et
> imprime les deux effectifs. `collinear_seven` rend bien neuf points malgré
> `--n=600` et publie **`30/30/29` à `smax_effectif = 9`**, la valeur du produit,
> au lieu du `30/30/30` de l'ancienne sonde à `smax` forcé. `A==produit=OUI` et
> `B==produit=OUI` sur les six familles. Les autres réserves — HWM, mur,
> tailles 16000/32000, `scanline_overlap_multiecho`, plusieurs graines, un mutant
> qui force réellement le code 3 — restent ouvertes.

## 1. Le fait de structure

`generate_candidates` appelle `alive_rectangles` **trois fois**, une par lane
(`src/pipeline/generate.hpp`, trois sites). Or les décisions de la descente sont
rigoureusement indépendantes de la lane :

- le prédicat `separated(va, vb, s, 1)` ne lit ni la lane ni son seuil ;
- `box_w2` et le choix du facteur scindé (`w2a >= w2b`) non plus ;
- l'ordre `(enfant, gardé)` poussé dans la vague suivante non plus.

Seul le test de mort `fc.c[lane_idx] >= h_q` dépend de la lane.

Et `count_universal_witnesses` **accepte déjà un masque trois bits** et rend
trois compteurs (`FusedCounts::c[3]`). Sa descente partage entre lanes :
l'élagage commun `hmax4_boxes`, la marche d'arbre, le crédit de sous-arbre, et
`corner64_universal_34` qui décide q3 **et** q4 en un seul appel. La machinerie
de fusion existe donc au niveau du nœud ; c'est la descente WSPD qui la sollicite
trois fois avec un masque à un seul bit.

Le poste n'est pas mineur. Sur les reçus `masses_q3_seed3_20260829`, la phase
WSPD occupe **68 à 70 %** du temps de génération sur `uniform` aux deux tailles
mesurées, 35 à 42 % sur `eight_clusters`, 16 à 49 % sur `terrain` et `scanline`.

## 2. La sonde et sa porte de correction

`bench/wspd_fusion_probe.cpp` joue **trois** bras sur le même nuage, à un fil :

- **bras P (produit)** — `generate_detail::alive_rectangles` appelée telle
  quelle, trois fois, à la configuration **effective** du pipeline. C'est
  l'autorité des listes ;
- **bras A** — la même descente transcrite et instrumentée, jouée trois fois
  avec les masques `0b001`, `0b010`, `0b100`. Il existe parce que
  `alive_rectangles` n'expose ni nœuds visités ni évaluations de coins ; sa
  fidélité est exactement ce que le bras P vérifie ;
- **bras B** — **une** descente, chaque rectangle portant un masque des lanes
  encore indécises. Une lane sort du masque dès qu'elle est morte ; le rectangle
  n'est scindé que si le masque reste non vide ; à un terminal, un seul appel
  avec autorité de coins sert toutes les lanes survivantes.

**La porte compare les deux bras instrumentés au bras produit**, et non l'un à
l'autre. Elle exige que les trois listes de rectangles vivants des bras A **et**
B soient **identiques** à celles de `alive_rectangles` — même cardinal, même
ordre, mêmes `(a, b)`, même `core`. Code de sortie 3 sinon, 2 en refus avant
calcul, 0 si conforme. Six portes CTest sont enregistrées à `n = 600`, sur les
quatre familles de mesure **et sur les deux contre-familles** `two_lines` et
`collinear_seven`.

La configuration effective est celle du produit : la sonde dérive ses seuils de
`smax_effective = min(smax, nombre de points rendus)` comme `run.hpp:225`, et
imprime `n_demande`, `n_rendu` et `smax_effectif`. Le cas gravé est
`collinear_seven`, qui rend neuf points malgré `--n=600` : la sonde publie
désormais **`30/30/29` à `smax_effectif = 9`**, la valeur du produit, là où une
version antérieure à `smax` forcé publiait `30/30/30`. Sur les six familles,
`A == produit` et `B == produit`.

L'argument d'identité est direct : les décisions de scission étant indépendantes
de la lane, l'arbre de rectangles est le même dans les deux bras ; le bras A pour
la lane `q` visite exactement les rectangles où `q` est vivante, et le bras B ne
laisse `q` émettre que là. La sonde ne remplace pas cet argument, elle le grave.

## 3. Ce que la fusion économise — compteurs exacts

`A == produit` et `B == produit` sur les six familles et les trois tailles.

`n = 2000`, économie du bras B sur le bras A :

| famille | rect. visités | appels témoins | nœuds d'arbre | coins |
|---|---:|---:|---:|---:|
| `uniform` | 55,3 % | 54,5 % | 42,5 % | 29,7 % |
| `terrain` | 58,0 % | 57,6 % | 47,1 % | 33,3 % |
| `eight_clusters` | 58,8 % | 57,5 % | 45,6 % | 35,0 % |
| `scanline_single_pass` | 58,4 % | 58,1 % | 47,8 % | 35,4 % |

`n = 8000` — taille d'intérêt. Les `rect_vivants` du bras produit reproduisent
les cardinalités `rect_alive` des reçus `masses_q3_seed3_20260829`
(`uniform` 259609/665954/735759, `terrain` 129392/207772/215015), ce qui est
attendu puisqu'il s'agit du même appel. L'autorité de la porte ne vient pas de
cet accord de cardinalité mais de l'égalité **de listes** exigée entre les trois
bras.

| famille | rect. visités | appels témoins | nœuds d'arbre | coins |
|---|---:|---:|---:|---:|
| `uniform` | 53,3 % | 52,7 % | 41,6 % | 28,6 % |
| `terrain` | 57,1 % | 56,8 % | 46,5 % | 31,6 % |
| `eight_clusters` | 56,0 % | 55,0 % | 43,6 % | 31,1 % |
| `scanline_single_pass` | 57,6 % | 57,5 % | 47,5 % | 32,7 % |

`n = 600`, mêmes colonnes, avec les contre-familles :

| famille | rect. visités | appels témoins | nœuds | coins |
|---|---:|---:|---:|---:|
| `uniform` | 58,0 % | 57,0 % | 44,6 % | 32,0 % |
| `terrain` | 59,3 % | 58,8 % | 47,9 % | 34,5 % |
| `eight_clusters` | 60,8 % | 59,6 % | 48,1 % | 38,8 % |
| `scanline_single_pass` | 59,9 % | 59,4 % | 48,6 % | 37,1 % |
| `two_lines` | 65,1 % | 65,0 % | 59,7 % | 47,7 % |
| `collinear_seven` | 66,7 % | 66,7 % | 61,9 % | 50,0 % |

Le profil est stable : environ **58 % des visites de rectangles et des appels au
compteur de témoins**, et **43 à 48 % des nœuds d'arbre**, sont aujourd'hui
refaits à l'identique. L'économie sur les coins est plus faible (30 à 39 %),
ce qui est attendu : `corner64_universal_34` est déjà partagé entre q3 et q4, et
seule la part q2 et les recouvrements entre lanes s'y ajoutent.

Ces quatre compteurs sont **déterministes** : ils ne dépendent ni de la machine
ni de la charge.

## 4. Ce que la sonde ne mesure pas correctement : le mur

La sonde imprime aussi un mur (27 à 35 % d'économie aux deux tailles). **Il ne
doit pas être reçu.** Les runs ont été joués sur une machine à quatre cœurs
partagée avec d'autres travaux, un seul passage par bras, sans alternance AB/BA.
C'est exactement le protocole que l'audit refuse depuis le
`+34 %` post-séparation, et c'est le pattern d'erreur n° 5 de
`PISTES_FERMEES.md` — un statut déclaré au lieu d'une mesure.

Le mur recevable exige un banc apparié contrebalancé intra-processus, médiane
des rapports par paire, sur machine au repos, avec sortie brute, pin et hash de
binaire versionnés dans `receipts/`.

## 5. Ce que la fusion coûterait

Trois postes, nommés pour ne pas être découverts en réception.

**Charge utile de la vague — comptée, mémoire non mesurée.** Le bras B porte
l'**union** des lanes encore indécises, plus un octet de masque par rectangle.
On pouvait craindre un pic de vague jusqu'à trois fois celui du bras A, qui vaut
le maximum sur les trois lanes jouées séquentiellement. La mesure dit le
contraire, `n = 2000` :

| famille | pic A (rect.) | pic B (rect.) | écart | pic A (o) | pic B (o) |
|---|---:|---:|---:|---:|---:|
| `uniform` | 93 506 | 93 574 | **+0,07 %** | 748 048 | 1 122 888 |
| `terrain` | 17 832 | 17 846 | +0,08 % | 142 656 | 214 152 |
| `eight_clusters` | 52 866 | 52 880 | +0,03 % | 422 928 | 634 560 |
| `scanline_single_pass` | 12 750 | 12 762 | +0,09 % | 102 000 | 153 144 |

Le pic logique **en rectangles** bouge peu dans ces quatre runs. C'est le même
fait que l'économie de
58 % vu de l'autre côté : les trois lanes descendent presque exactement les
mêmes rectangles, donc leur union est presque leur maximum. Seuls les **octets**
de charge utile calculée croissent de 50 %, par l'octet de masque et son
alignement (8 → 12 o). `size*sizeof(T)` ignore toutefois `capacity`, la
coexistence de `vague` et `suivante`, les buffers parallèles et les copies de
concaténation : ce tableau n'est pas un pic mémoire. Un éventuel empaquetage
dans `NodeRef` reste une hypothèse ; le signe encode déjà feuille/nœud et aucun
contrat de bits hauts libres n'est établi.

**Résidence des listes de vivants — un poste visible, pas le seul.** Aujourd'hui
`generate_candidates` recycle un unique `std::vector<AliveRect> alive` d'une lane
à l'autre ; le corps de q2 est consommé avant que q3 ne descende. Fusionner
impose soit de garder les trois listes simultanément, soit de restructurer
l'ordonnancement en lançant les trois corps depuis la même descente. Mesuré à
`n = 2000` : `uniform` 158 496 rectangles résidents (2,54 Mo) contre 363 056
(5,81 Mo), soit **+129 %** ; `terrain` 49 596 contre 129 594. Le facteur est
d'environ 2,3 sur les quatre familles.

Le reçu déjà versionné à `uniform n=32000` donne
`1094102/2908394/3233183` rectangles vivants. À 16 octets par `AliveRect`, les
trois listes représentent donc une charge utile minimale d'environ 115,8 Mo
décimaux, contre 51,7 Mo pour le maximum séquentiel — pas 26 Mo contre 12 Mo.
Cela ne remplace toujours pas un HWM. Le poste contredit la doctrine de
streaming par ordre et doit être **arbitré, pas supposé**. La seconde option
(trois corps depuis la même descente) peut réduire cette résidence, mais doit
être mesurée avec les autres buffers simultanés.

**Sémantique des compteurs.** `rect_visited[lane]` et `workers_wspd[lane]`
perdent leur sens si une visite sert trois lanes. Il faut soit les redéfinir
explicitement (visites totales, et une ventilation par lane encore ouverte),
soit conserver un compteur de visites imputées. Un compteur renommé en silence
est le pattern d'erreur n° 5 ; la réception doit dire lequel des deux.

## 6. Ce que cette piste n'est pas

- **Ce n'est pas un gain de complexité.** La fusion divise un facteur constant.
  La borne `O(s^3 n)` reste `ouverte` pour la variante radix-Morton
  (`docs/MATHEMATIQUES.md` § 5) et cette note ne la rapproche pas.
- **Ce n'est pas un gain sur les seeds.** Le nombre de seeds q3 est intrinsèque
  au nuage : le balayage `s = 2..10` le laisse invariant à l'unité près. La
  fusion ne touche qu'à l'amont.
- **Ce n'est pas une piste fermée.** Aucune entrée de `PISTES_FERMEES.md` ne la
  couvre : ni le cap de population dans le critère terminal, ni la scission du
  facteur le plus peuplé, ni les deux arbres coexistants. Le critère terminal,
  le choix du facteur scindé et le prédicat de séparation sont **inchangés** ;
  seul l'ordonnancement des trois descentes change.
- **Ce n'est pas une livraison.** La sonde et ses portes sont commitées ;
  `alive_rectangles` n'est pas modifiée et le chemin produit est intact.

## 6 bis. Contre-lecture adversariale — trois corrections à ma charge

Un panel de réfutation lancé sur six lentilles a retrouvé cette piste
indépendamment, avec sa propre sonde et des compteurs qui recoupent les miens à
la décimale. Il a aussi trouvé trois choses que je n'avais pas vues. Je les ai
revérifiées dans le code avant de les écrire.

**a) La sonde ne passe pas par `postsep_refine`, donc son ledger est vide par
vacuité.** Mes deux bras poussent directement dans la liste de sortie. À `L = 0`
c'est une transcription fidèle — `postsep_refine` s'y réduit à `emit(r, core)` —
et l'identité des listes reste donc valide. Mais la production alimente aussi,
au passage, `led->base`, `led->emitted` et `led->parents`. Une intégration qui
copierait le bras B tel quel obtiendrait un ledger **vide** qui passerait toute
comparaison d'égalité de ledger : c'est le vert par vacuité, pattern d'erreur
n° 7. La réception doit donc exiger que le bras fusionné **appelle
`postsep_refine` par lane survivante** et que le ledger soit **non vide** à
`L = 0`, en plus des listes identiques.

**b) Une part du gain mesuré est de l'allocation, pas de l'algorithme.**
`count_universal_witnesses` construit un `std::vector<Entry> stack` **à chaque
appel** (`src/spindle/witness_count.hpp:72`), soit deux fois par rectangle et
par lane. Supprimer des appels supprime donc des allocations, et le panel
chiffre ce seul poste à 4,9–5,6 % du mur de la descente. Ce confondeur doit être
retiré **avant** d'attribuer quoi que ce soit à la fusion : hisser le tampon en
variable d'ouvrier est un changement à surface d'objet nulle, mesurable seul, et
qui doit passer en premier.

**c) Le gain bout-en-bout est plus petit que ce que la part de la phase suggère.**
Rapporter −28 % de la descente à ses 68 % de la génération donne un ordre de
grandeur trompeur : le panel mesure **−2,7 % à −9,5 % de bout en bout**, pas les
−18 à −20 % qu'une simple règle de trois laisse espérer. La raison est que la
descente et les corps se disputent les mêmes lignes de cache et que les gardes
d'ancre en aval ne bougent pas. Ne rien annoncer au-delà de cette fourchette
tant qu'un banc apparié ABBA n'a pas tranché.

**Deux points d'intégration que je n'avais pas nommés.** `alive_rectangles` a
trois appelants hors `generate_candidates` sur le chemin device ou batch :
`src/gpu/q3_lane_batched.hpp:438`, `src/gpu/q4_lane_batched.hpp:527` et
`src/gpu/device_witness.cu:169`. Un changement de signature doit les porter,
sinon deux définitions de la descente divergent en silence. Et treize sondes de
`bench/` et portes de `tests/` l'appellent aussi : la signature actuelle doit
survivre, ou toutes doivent être portées dans le même commit.

**Instrumentation gratuite, à faire avant la mesure.** `GenerateStats` calcule
déjà `rect_visited[3]`, et la WSPD calcule `wave_peak` ; **ni l'un ni l'autre
n'est imprimé par `print_run`** (zéro occurrence dans `src/pipeline/run.hpp`).
Les exposer coûte deux lignes et donne les deux compteurs dont la réception a
besoin, sans sonde.

## 7. Réception proposée

1. **FAIT.** Le bras A est jugé contre un bras PRODUIT appelant directement `alive_rectangles` avec
   `smax_effective`, puis imprimer effectifs demandés/réels et configuration
   effective. Graver notamment le cas `collinear_seven` à neuf points.
2. Ajouter `scanline_overlap_multiecho`, des planchers sur sorties et compteurs,
   plusieurs graines et un mutant qui force réellement le code 3.
3. Étendre la mesure à 16000 et 32000 avec capacités simultanément résidentes
   ou HWM attribuable ; `size*sizeof(T)` ne suffit pas.
4. Banc apparié contrebalancé du mur, machine au repos, reçu versionné.
5. Seulement ensuite, un raccord dans `alive_rectangles` derrière une porte
   d'égalité des trois listes **et** des digests par lane, avec le mutant
   `wspd-fusion-mask-leak` (une lane morte qui reste dans le masque et émet un
   rectangle qu'elle ne devrait pas) et le mutant `wspd-fusion-mask-early`
   (une lane retirée du masque avant que son compte n'atteigne son seuil).
6. Le raccord à `postsep_refine_levels > 0` demande sa propre porte : la sonde
   ne couvre que `L = 0`.

## 8. Reproduction

```bash
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --target mhgp5_wspd_fusion_probe --parallel
./build/v5/mhgp5_wspd_fusion_probe --family=terrain --n=2000
ctest --test-dir build/v5 -R '^mhgp5_wspd_fusion_'
```

Chaque chiffre des tableaux ci-dessus est un comptage d'un seul run, jamais une
extrapolation. Aucun reçu versionné n'est encore attaché : cette note est un
diagnostic, pas un reçu. GCP non utilisé.
