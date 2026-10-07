# `JUG-EMST` : juge d'échelle de l'ordre un

7 octobre 2026. Juge **hors produit**, indépendant du catalogue, de la tour, du code de `src/` et de la v11 :
l'ordre un de la tour FULL est l'arbre de fusion du lien simple sur l'arbre couvrant euclidien minimal (EMST) exact
des sites, avec plateaux, au niveau $d^{2}/4$. C'est le point 4 des portes du
[contrat de la tour](../../docs/CONTRAT_TOUR.md) (§ 9) et la ligne `JUG-EMST` (ancien J2) de
[`OBJET_ET_CONTRAT_MATHEMATIQUE.md`](../../docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md) ; il répond à la part « juge EMST
jamais implanté » du constat `CST-0013` (l'état du registre reste à l'auditeur).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (juge hors produit)
quantification=quantized_u21_input_only (le juge couvre aussi u24 et u32 : coordonnées u32 quelconques)
public_status=not_claimed
```

## 1. Objet et preuve courte

Soit $X$ un ensemble fini de sites **distincts** de $\mathbb{Z}^{3}$ (doublons refusés, décision D8). À l'ordre un,
$D_{1}(y)=\min_{x\in X}\left\Vert y-x\right\Vert^{2}$ et $L_{1}(a)=\bigcup_{x\in X}\bar{B}(x,\sqrt{a})$.

1. **Composantes.** $L_{1}(a)$ est une réunion finie de boules fermées, donc de convexes : ses composantes sont les
   réunions des boules sur les composantes du graphe d'intersection $G(a)$, d'arêtes $\lbrace x,x'\rbrace$ telles que
   $\left\Vert x-x'\right\Vert^{2}\leq 4a$. C'est le modèle fini $\Gamma_{1}(a)$ : une 1-partie est un site, au
   niveau $\beta=0$ ; une 2-partie relie deux sites au niveau $\beta=d^{2}/4$, rayon carré de la boule diamétrale,
   qui est leur plus petite boule englobante. La coupe ouverte au niveau $a$ est la même chose avec une inégalité
   stricte.
2. **Réduction à un arbre couvrant minimal.** Soit $T$ un arbre couvrant minimal du graphe complet pondéré par
   $w=d^{2}$. Pour toute arête $e\notin T$, le chemin de $T$ entre ses extrémités n'a que des arêtes de poids au plus
   $w(e)$ (propriété de cycle). Donc, pour tout seuil $t$, les composantes de $\lbrace e : w(e)\leq t\rbrace$ sont
   celles de $\lbrace e\in T : w(e)\leq t\rbrace$, et de même pour $w(e)<t$. Les coupes fermées et ouvertes de
   l'ordre un se lisent sur $T$ seul, **quel que soit** l'arbre couvrant minimal choisi.
3. **Plateaux.** Au niveau $a=w/4$, les arêtes de $T$ de poids exactement $w$ relient des composantes de la coupe
   ouverte et forment une **forêt** sur ces composantes : deux de ces arêtes entre les mêmes composantes, ou un cycle
   de composantes, fermeraient un cycle de $T$ avec des chemins de poids $<w$ intérieurs aux composantes. Chaque arbre
   de cette forêt, à $m\geq 1$ arêtes, réunit $m+1$ composantes de la coupe ouverte en **une** composante de la coupe
   fermée : c'est une fusion à $m+1$ enfants, jamais une chaîne de fusions binaires. Les autres composantes
   continuent. C'est le plateau atomique de l'objet (`TOW-P`).
4. **Numérotation canonique** (celle de l'oracle borné et des vidages `MHGP11FUL1`) : naissances par (niveau 0,
   position $(x,y,z)$ lexicographique), fusions par (niveau, plus petite naissance du sous-arbre), enfants triés.
   L'ordre est total : deux fusions de même niveau ont des sous-arbres disjoints. $\square$

L'arbre ne dépend donc pas du départage des arêtes de même longueur. Le juge fixe tout de même un ordre **strict**
(distance carrée, plus petit site, plus grand site), le site étant son numéro de naissance : l'EMST est alors unique,
Borůvka le rend sans cycle quel que soit l'ordre de visite (chaque composante choisit son arête sortante minimale,
qui est dans l'arbre par la propriété de coupe), et son empreinte `sha256_emst` est déterministe. Le théorème 2 du
manuscrit (identité de $L_{k}$ et $\Gamma_{k}$) est invoqué, pas re-vérifié ; à l'ordre un il se réduit au point 1.

## 2. Calcul

| Étape | Méthode | Exactitude |
| --- | --- | --- |
| lecture | `.u32le` (12 octets par point : $x,y,z$ en u32 petit-boutiste, format des trames de la v11 et de la v12), `.ids.u32le` facultatif ; tri lexicographique ; refus des doublons | au plus $2^{31}-1$ sites (au plus $2^{32}-2$ nœuds) |
| EMST | Borůvka sur un arbre k-d équilibré (coupe médiane de l'axe le plus étendu, feuilles de 16 sites) ; élagages exacts : sous-arbre entièrement dans la composante ; boîte strictement plus loin que la meilleure arête courante de la composante (visitée à égalité : une arête de même longueur peut gagner au départage) ; voisin exact mémorisé tant qu'il reste extérieur (l'ensemble des sites extérieurs ne fait que décroître) ; minorant mémorisé d'une requête bornée restée vide | distances carrées entières : `u64` si toutes les coordonnées sont $<2^{31}$ ($3\cdot 2^{62}<2^{64}$), `u128` sinon (profil 32 compris) ; aucun flottant |
| plateaux | arêtes triées par (distance carrée, plus petit site, plus grand site), groupées par distance ; racines union-find prises à la coupe ouverte, puis une fusion par groupe connexe, rangée par plus petite naissance | niveaux réduits $d^{2}/4$ |
| comparaison | lecture stricte de l'en-tête, des sites et de l'ordre un d'un vidage `MHGP11FUL1` ; ordres supérieurs non lus | niveaux comparés par $4\,\mathrm{num}=d^{2}\,\mathrm{den}$ en entiers de précision arbitraire (formes non réduites et mots de rembourrage admis) ; centres par $\mathrm{num}_{i}=x_{i}\,\mathrm{den}$ |

Le vidage est comparé **champ par champ** : sites dans l'ordre de Morton exact (comparateur sans clé formée, le bit
$b$ de l'axe $a$ en position $3b+a$, comme la v11), poids unitaires, `PointId` (contre `--ids`, sinon distincts),
puis pour chaque nœud de l'ordre un parent, début et cardinal de la CSR, niveau exact, centre des naissances, et enfin
les enfants. Égalité de tous les champs : l'arbre attendu, donc un ordre un cohérent. La première différence de nœud
est publiée, suivie des écarts d'en-tête (nombre de nœuds, racine).

La comparaison exacte des rationnels n'est pas un luxe : la v11 écrit le niveau d'un rang dans la forme de la première
boule de ce rang. Sur la suite rapide de l'oracle, ses fusions d'ordre un portent les dénominateurs 4, 324, 576 et
16 384 (`circle25_pair` : niveau 25 écrit $409600/16384$, forme du triangle de même rang).

## 3. Usage, sorties, codes

```text
mhgp12_jug_emst <nuage.u32le> [--ids <nuage.ids.u32le>] [--vidage <FULL MHGP11FUL1>] [--bits 21|24|32]
                [--arbre <sortie texte>] [--emst <sortie texte>] [--force-u128]
```

Sortie : un objet JSON sur une ligne : `sites`, `arithmetique` (`u64` ou `u128`), `naissances`, `fusions`,
`multifusions` (au moins trois enfants), `arite_max`, `niveaux_distincts`, `niveaux_a_plusieurs_fusions`,
`aretes_emst`, `aretes_a_egalite`, `racine`, `sha256_arbre`, `sha256_fusions` (sans les positions : invariante par
translation), `sha256_emst`, compteurs de Borůvka (`tours`, `requetes`, `voisins_gardes`, `minorants`, `distances`,
`profondeur_kd`), verdict du vidage, temps par étape. `--arbre` écrit la forme texte canonique (`N n`, puis
`B x y z` par naissance, puis `F num den enfants...` par fusion) ; `--emst` les arêtes (`d2 a b`) ; `--force-u128`
prend la voie `u128` même sous $2^{31}$ (contrôle croisé des deux arithmétiques).

| Code | Sens |
| --- | --- |
| 0 | arbre calculé ; vidage identique s'il est donné |
| 1 | écart avec le vidage (première différence sur la sortie d'erreur et dans `vidage.message`) |
| 2 | usage |
| 3 | entrée invalide : nuage illisible ou vide, doublon (D8), coordonnée hors de `--bits`, identifiants en nombre différent, vidage illisible, tronqué, de signature, de profil, de domaine ou de poids invalides |

Empreintes : `sha256_arbre` = SHA-256 de `ehgp.v12.jug_emst.ordre1.v1\0`, du nombre de naissances, des positions
$(x,y,z)$ des naissances (mots u64 petit-boutistes), du nombre de fusions et, par fusion, du niveau réduit
(numérateur et dénominateur en `natural()` de la v11 : longueur sur un mot puis octets petit-boutistes), du nombre
d'enfants et des enfants. `sha256_fusions` omet les positions ; `sha256_emst` hache les arêtes triées. Les portes
recalculent ces empreintes en Python (`tests/commun.py`) ; le SHA-256 natif est écrit ici (aucune dépendance) et
jugé sur les vecteurs de la norme.

## 4. Portes

```bash
cmake -S morsehgp3D_v12/juges/emst -B <build> -DCMAKE_BUILD_TYPE=Release
cmake --build <build> -j3 && ctest --test-dir <build> --output-on-failure -j3
# porte LiDAR : MHGP12_DATA_DIR=<trames> MHGP12_EMST_VIDAGES=<vidages de la v11> ctest --test-dir <build> -L lidar
```

| Porte | Ce qu'elle juge | Code attendu |
| --- | --- | --- |
| `mhgp12_jug_emst_autotest` | SHA-256 (vecteurs FIPS 180-4), entiers de précision arbitraire contre `u128`, comparateur de Morton contre la clé formée (200 000 paires), Borůvka contre Kruskal sur toutes les paires puis plateaux contre un lien simple calculé sur toutes les paires (300 petits nuages : grilles, droites, génériques, coins u32, grille $2^{28}$ ; 18 920 arêtes à égalité ; voies `u64` et `u128`), Borůvka contre Prim dense sur trois nuages d'environ 4 000 sites (grilles à égalités, uniforme) | 0 |
| `mhgp12_jug_emst_temoins` et `_O` | 11 témoins gravés (§ 5), 13 refus, 17 vidages fabriqués à l'ordre un (dont les deux PointId dupliqués de `CST-0232`) ; 141 contrôles, nombre exact | 0 |
| `mhgp12_jug_emst_oracle_rapide` et `_O` | suite rapide de l'oracle borné [`reference/`](../../reference/README.md) (`hgp12_ref`, 342 nuages) contre l'ordre un de `Definition` : 34 refus de doublons, 308 arbres identiques, 308 translations de $2^{31}$ (voie `u128`, `sha256_fusions` inchangée), 308 vidages écrits depuis l'oracle déclarés identiques ; compteurs exacts (1 846 naissances, 1 312 fusions dont 158 à au moins trois enfants, arité maximale 8) | 0 |
| `mhgp12_jug_emst_mutant_binaire` | mutant causal : une fusion binaire par arête au lieu du plateau | 4 (tué) |
| `mhgp12_jug_emst_mutant_departage` | mutant causal : égalités départagées par les plus grands sites | 4 (tué) |
| `mhgp12_jug_emst_vidages_v11` (label `lidar`) | vidages `MHGP11FUL1` de la v11 gelée aux empreintes de [`MESURE.md`](../../docs/MESURE.md) § 4 (K5 exigés, K10 s'ils sont là) : identité de l'ordre un et comptes gravés ; par nuage, homothéties vers les profils 24 et 32 (mêmes enfants, niveaux multipliés par $4^{j}$, voie `u128` au profil 32), translation de $2^{31}$, permutation renversée des points et des identifiants ; sautée (77) sans `MHGP12_DATA_DIR` et `MHGP12_EMST_VIDAGES` | 0 |

Toutes les portes Python tournent sous `python3 -S` (et `-S -O` pour les jumelles), bibliothèque standard seule, sans
`assert`. Les 8 portes passent aussi sous ASan et UBSan (`-fsanitize=address,undefined -fno-sanitize-recover=all`).
`tests/echelle.py` est un banc (pas une porte) : nuages uniformes à graine fixe de plusieurs millions de sites.

## 5. Témoins gravés (`tests/temoins.py`)

| Témoin | Sites | Attendu exact |
| --- | --- | --- |
| `WIT-TRI-EQ` | $(0,0,0),(1,1,0),(1,0,1)$ | une fusion ternaire au niveau $1/2$ |
| `alignes4` | quatre points alignés à pas 1 | une fusion quaternaire au niveau $1/4$ |
| `WIT-SIX` | deux triangles de la thèse (§ 6.1), pont de 2000 | deux fusions ternaires simultanées au niveau 999 956, puis une binaire au niveau $10^{6}$ |
| `u32_coins` | origine, trois coins d'axe et coin opposé de $[0,2^{32})^{3}$ | fusion quaternaire au niveau $M^{2}/4$, puis binaire au niveau $M^{2}/2$, $M=2^{32}-1$ ; $d^{2}=2M^{2}>2^{64}$ |
| `u32_diagonale` | coins opposés de $[0,2^{32})^{3}$ | une fusion au niveau $3M^{2}/4$ |
| `seuil_u64`, `seuil_u128` | coordonnées $2^{31}-1$, puis $2^{31}$ | même forme, voie `u64` puis `u128` |
| `cube_unite` | $\lbrace 0,1\rbrace^{3}$ | une fusion à huit enfants au niveau $1/4$ |
| `chaine`, `deux_paires`, `un_point` | pas croissants ; deux paires au même niveau ; un site | fusions binaires ; rang par plus petite naissance ; aucune fusion |

Chaque témoin fixe aussi l'EMST sous l'ordre strict et les trois empreintes recalculées en Python, puis rejoue la voie
`u128` forcée.

## 6. Mutants

| Mutant | Effet attendu | Verdict |
| --- | --- | --- |
| `binaire` (`MHGP12_MUTANT_BINAIRE`, `src/plateaux.hpp`) | une fusion binaire par arête, en chaîne, au lieu de la fusion N-aire du plateau | tué : 26 contrôles en échec (arbres et vidages) ; 536 désaccords sur la suite rapide de l'oracle |
| `departage` (`MHGP12_MUTANT_DEPARTAGE`, `src/emst.hpp`) | départage inversé des arêtes de même longueur | tué par l'EMST gravé (12 contrôles) ; **l'arbre à plateaux est inchangé** sur tous les témoins, comme le veut le point 2 de la preuve |

## 7. Validation sur les vidages de la v11 gelée (7 octobre 2026)

v11 construite depuis l'archive épinglée (`microbancs/outils/source_v11.py`, `libmhgp11.a` `050532a9…`), vidages
reproduits **à l'octet** aux empreintes de [`MESURE.md`](../../docs/MESURE.md) § 4 (K5 et K10), puis jugés :
**identité de l'ordre un partout** ; l'ordre un des vidages K10 est celui des vidages K5.

| Cas | K | sites | fusions | multifusions | niveaux distincts | niveaux à plusieurs fusions | `sha256_arbre` |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| uniforme 8 000 | 5 | 8 000 | 7 999 | 0 | 7 999 | 0 | `ae5aaf27cc31f675537240b63f8c2db8dfdb45c2b27a3c7bd4c2b9238c88e39b` |
| uniforme 16 000 | 5 | 16 000 | 15 999 | 0 | 15 996 | 3 | `65ded228df429c71799e3c65721c4ab8925d3243566e02ad520ce586d11e6d28` |
| uniforme 32 000 | 5 | 32 000 | 31 998 | 1 | 31 991 | 7 | `f46e4cb477d11088531a203479cd772d94ff479c5ec0549ceffd9c6e62d8db31` |
| ng00 | 5 et 10 | 39 885 | 39 796 | 87 | 22 511 | 7 350 | `fbe9969e6b5d6ae9716cbeef526c44a59b8d95b2ee8df9325c85de184e92dd0d` |
| ng01 | 5 et 10 | 35 551 | 35 461 | 87 | 23 344 | 5 907 | `674989e0ea6dd2f318abc4c8c49b5928f66773030cda65e9a1eb82c16db35db1` |
| ng02 | 5 et 10 | 45 845 | 45 563 | 272 | 21 528 | 7 069 | `0e0ed701e5e88ece5493468702fa798525e01fda37b62903310f7aa11623ac4d` |

Sur les trames, la grille de 1 mm produit beaucoup d'égalités : 24 733, 18 131 et 31 393 arêtes de l'EMST partagent
leur longueur avec une autre (ng00, ng01, ng02), d'où les multifusions (arité maximale 4). La v11 a aussi été jouée
sur les 308 nuages sans doublon de la suite rapide de l'oracle : 308 ordres un identiques. Un seul numérateur de
niveau augmenté de 1 dans le vidage K5 de ng00 est trouvé : `ordre 1, noeud 40885 : niveau 532/4, attendu 531/4`.

## 8. Coût mesuré

Codespace de 8 cœurs partagé avec d'autres agents, un fil, Release, GCC 13.3 ; le temps local ne prédit pas G4.

| Entrée | Voie | Tours | Requêtes | EMST | Plateaux | Empreintes | Total | Pic mémoire |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| uniformes 8 000, 16 000, 32 000 (u18) | `u64` | 7, 7, 8 | 35 144, 70 476, 153 371 | 19 à 96 ms | 0,4 à 3,7 ms | 6 à 24 ms | 39 à 179 ms (lecture du vidage comprise) | — |
| ng00, ng01, ng02 | `u64` | 9, 8, 9 | 276 150, 230 417, 326 035 | 117 à 165 ms | 3 à 6 ms | 25 à 32 ms | 203 à 282 ms (lecture du vidage comprise) | 10,7 Mo (ng00) |
| 1 000 000 uniformes, $[0,2^{21})^{3}$ | `u64` | 10 | 4 757 151 | 3,7 à 3,8 s | 0,2 à 0,3 s | 0,8 s | 4,8 à 5,1 s | — |
| 4 000 000 uniformes, $[0,2^{21})^{3}$ | `u64` | 10 | 18 815 198 | 16,4 s | 1,7 s | 3,3 s | 21,9 s | — |
| 1 000 000 uniformes, $[0,2^{32})^{3}$ | `u128` | 10 | 4 823 101 | 4,6 s | 0,3 s | 0,8 s | 5,8 s | — |
| 4 000 000 uniformes, $[0,2^{32})^{3}$ | `u128` | 11 | 19 352 945 | 20,3 à 24,4 s | 1,9 s | 3,3 s | 26,0 à 30,8 s | 808 Mo |

De 1 à 4 millions de sites, l'EMST croît de 4,4 fois (pente $n\log n$ : 4,4), les requêtes de 3,96 fois ; la voie
`u128` coûte environ 20 % de plus que la voie `u64`. Rien n'en est conclu de définitif : un seul fil, une machine
chargée, des nuages uniformes et non des scènes LiDAR.

## 9. Limites

- **Ordre un seulement**, par construction : ni les ordres supérieurs, ni les verticales, ni les coupes. Le juge ne
  lit pas les ordres supérieurs d'un vidage : ce n'est pas le lecteur strict (`full_semantic.py` de la v11), il ne
  remplace ni ses contrôles ni l'empreinte de `MES-M0`.
- **Doublons refusés** (décision D8, code 3) ; aucune option « sites distincts ».
- Au plus $2^{31}-1$ sites ; un seul fil (environ 4 s par million de sites en local) ; environ 200 octets par site.
- L'invariance par translation n'est jugée que par translation explicite (portes de l'oracle et LiDAR), jamais par
  l'empreinte `sha256_arbre`, qui contient les positions.
- Le temps local ne prédit pas G4 ; aucune décision de vitesse du produit ne se prend sur ce juge.
- Ce dossier n'est pas couvert par `tools/check_style.py` (il nomme le format de la v11, comme les microbancs) ; il en
  suit les règles : C++ en ASCII, commentaires sans accents, deux espaces, aucun `assert`.
- Hors registre : rien ici ne change `docs/implementation_status.toml` ni un statut public.
