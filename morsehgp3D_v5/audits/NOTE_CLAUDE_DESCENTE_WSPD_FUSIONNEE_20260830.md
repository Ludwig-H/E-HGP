# La descente WSPD est payée trois fois — mesure, porte de correction, et ce que la fusion coûte

Note Claude, 30 août 2026, ancrée au pin `119b80b0`. Sonde
`bench/wspd_fusion_probe.cpp`, cible `mhgp5_wspd_fusion_probe`, counter-only,
un fil, `s = 8`, `smax = 11`, graine 3.

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Cette note ne revendique aucune borne et ne change pas l'objet.** Elle chiffre
une redondance d'exécution et fournit la porte qui la rendrait sûre à retirer.
Elle ne touche pas au verdict P0 d'`ETAT_COURANT.md` sur la sémantique de forêt,
qui reste le blocage prioritaire.

**Statut après contre-audit : hypothèse prometteuse, porte produit non reçue.**
La sonde prouve l'identité de ses deux transcriptions locales, mais son bras A
n'appelle pas `alive_rectangles`. `collinear_seven` montre déjà qu'elles peuvent
être identiques entre elles et différer du produit. Les phrases ci-dessous qui
présentaient la mémoire comme mesurée sont également rétractées : la sonde ne
publie qu'une charge utile minimale `size*sizeof(T)`, pas un HWM.

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

`bench/wspd_fusion_probe.cpp` joue deux bras sur le même nuage, à un fil :

- **bras A** — transcription locale de `alive_rectangles` à
  `postsep_refine_levels = 0`, jouée trois fois avec les masques
  `0b001`, `0b010`, `0b100` ;
- **bras B** — **une** descente, chaque rectangle portant un masque des lanes
  encore indécises. Une lane sort du masque dès qu'elle est morte ; le rectangle
  n'est scindé que si le masque reste non vide ; à un terminal, un seul appel
  avec autorité de coins sert toutes les lanes survivantes.

**La porte actuelle est interne à la sonde.** Elle exige que les trois listes
de rectangles vivants du bras B soient **identiques** à celles du bras A —
même cardinal, même ordre, mêmes `(a, b)`, même `core`. Code de sortie 3 sinon,
2 en refus avant calcul, 0 si conforme. Six portes CTest sont enregistrées à
`n = 600`, sur les quatre familles de mesure **et sur les deux contre-familles**
`two_lines` et `collinear_seven`.

Cela ne compare pas encore au symbole produit. La sonde force en particulier
`smax=11`, tandis que le pipeline applique
`smax_effective=min(smax,input_count)`. `collinear_seven` contient neuf points
malgré `--n=600` : la sonde passe avec `30/30/30`, alors que le produit publie
`30/30/29` à `smax_effective=9`. Le bras A doit appeler directement
`alive_rectangles` avec la configuration effective avant que cette porte puisse
être qualifiée de porte de correction produit.

L'argument d'identité est direct : les décisions de scission étant indépendantes
de la lane, l'arbre de rectangles est le même dans les deux bras ; le bras A pour
la lane `q` visite exactement les rectangles où `q` est vivante, et le bras B ne
laisse `q` émettre que là. La sonde ne remplace pas cet argument, elle le grave.

## 3. Ce que la fusion économise — compteurs exacts

`listes_identiques = OUI` sur les six familles et les deux tailles.

`n = 2000`, économie du bras B sur le bras A :

| famille | rect. visités | appels témoins | nœuds d'arbre | coins |
|---|---:|---:|---:|---:|
| `uniform` | 55,3 % | 54,5 % | 42,5 % | 29,7 % |
| `terrain` | 58,0 % | 57,6 % | 47,1 % | 33,3 % |
| `eight_clusters` | 58,8 % | 57,5 % | 45,6 % | 35,0 % |
| `scanline_single_pass` | 58,4 % | 58,1 % | 47,8 % | 35,4 % |

`n = 8000` — taille d'intérêt. Pour les deux cas cités, les `rect_vivants` de la
sonde reproduisent les cardinalités `rect_alive` des reçus
`masses_q3_seed3_20260829`
(`uniform` 259609/665954/735759, `terrain` 129392/207772/215015) : la sonde
imite ces exécutions. Cet accord de cardinalité n'est ni une égalité de listes
ni une autorité générale, comme le contre-exemple `collinear_seven` le montre.

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
| `collinear_seven` | 66,7 % | 66,7 % | 62,0 % | 50,0 % |

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

## 7. Réception proposée

1. Remplacer le bras A par un appel direct à `alive_rectangles` avec
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
