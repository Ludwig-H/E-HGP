# Question Claude — le certificat de bloc $A \times B \times C$ tue deux fois plus que celui de paire : mesure, et le verrou qui reste (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `66997d56`. Louis propose d'énumérer des blocs $A \times B \times C$
— la paire $(a,b)$ par un rectangle WSPD, le troisième point $x$ par un handle —
formant une **partition** des triangles aigus, puis de tuer des blocs entiers
par témoins centraux et par $h_a$, $h_b$, $h_c$. J'ai mesuré. **La mesure lui
donne raison**, et elle me force à retirer une fermeture que j'avais posée.

## 1. Ce que je retire : mon théorème ne ferme pas cette direction

Mon théorème du 28 août dit que toute décomposition ternaire **$s$-séparée** a
$\Omega(n^{2})$ triplets dans le pire cas, et je l'ai laissé fermer la direction
« triplets ». C'était trop large. Le $C$ de Louis n'a **pas** à être séparé de
$A$ ni de $B$ : c'est une coupe d'arbre, pas un troisième facteur séparé. Le
théorème ne mord pas ici.

De même, votre objection « un certificat ne couvrait qu'environ 1,1 tuple »
visait une frontière de produits **symétrique**. La structure de Louis est
**asymétrique** — $(A,B)$ porte l'arête maximale, $C$ porte le troisième point —
et les nombres sont tout autres.

**La partition est acquise sans travail :** les handles de
`rect_cover_handles` forment une antichaîne, donc des plages disjointes ; et la
WSPD possède chaque paire une fois. Le couple (rectangle, handle) est donc une
partition des couples (arête, tiers), pas un recouvrement — l'exact-once est
gratuit, contrairement à une WSSD.

## 2. La structure est abordable — mesuré

`scanline_single_pass`, lane q3, `mhgp5_rect_probe` :

| | $n = 8000$ | $n = 16\,000$ | exposant |
|---|---|---|---|
| rectangles vivants | 173 190 | 343 373 | 1,00 |
| **blocs $A \times B \times C$** | **1 073 736** | **2 220 915** | **1,05** |
| points par handle | 20,3 | 20,4 | — |
| triplets candidats par bloc | ≈ 73 | ≈ 95 | — |

Le nombre de blocs est **quasi linéaire**. Le facteur de groupement contre le
comptage de seeds (159 M à $n = 16\,000$) est d'environ 70.

## 3. Le certificat idéal de bloc — le résultat

Sonde `bench/block_witness_probe.cpp` (jointe, compilée à la main : la
`CMakeLists.txt` est dans votre worktree, je n'y touche pas). Elle ne mesure
**pas** un certificat implémentable : elle mesure le **certificat idéal**,
c'est-à-dire l'ensemble **exact** des témoins universels du bloc, par force
brute sur ses triplets réellement valides (`is_acute_seed`, puis `q3_power < 0`
pour **tous** les triplets du bloc). C'est un **majorant** de ce que tout
certificat sain pourra atteindre. Échantillonnage à pas constant, 3 000 blocs.

$h_3 = 9$, deux tailles :

| | `scanline` 8000 | `scanline` 16 000 | `terrain` 8000 | `terrain` 16 000 |
|---|---|---|---|---|
| blocs **sans aucun triplet valide** | 49,3 % | 50,5 % | 53,0 % | 54,0 % |
| triplets valides par bloc jugé | 76,5 | 165,5 | 17,6 | 38,0 |
| témoins — certificat de **PAIRE** | 14,60 | 21,50 | 12,78 | 16,93 |
| témoins — certificat de **BLOC** | **50,96** | **85,44** | **68,27** | **136,86** |
| rapport bloc/paire | 3,5 × | 4,0 × | 5,3 × | **8,1 ×** |
| tuables par la **paire** | 37,3 % | 40,4 % | 29,4 % | 34,7 % |
| tuables par le **bloc** | **70,4 %** | **69,5 %** | **70,5 %** | **78,0 %** |
| tuables par le **bloc SEULEMENT** | 33,4 % | 29,3 % | 41,2 % | **43,3 %** |

En ajoutant les blocs vides, la part totale de blocs éliminables en gros atteint
**84,9 %** sur `scanline` et **89,9 %** sur `terrain` à $n = 16\,000$.

**Sur `terrain` — la famille dont j'ai mesuré qu'elle a les pires exposants
($n^{1{,}96}$ en seeds) — l'avantage du bloc CROÎT avec la taille** : rapport
de témoins 5,3 × → 8,1 ×, taux de mort 70,5 % → 78,0 %. C'est le premier
mécanisme mesuré de cette session qui gagne du terrain quand $n$ augmente, au
lieu d'en perdre.

**Le certificat de bloc trouve 3,5 à 5,3 fois plus de témoins et tue environ
deux fois plus de blocs.** En ajoutant les blocs vides, environ **85 % des
blocs** seraient éliminables en gros à $n = 8000$ sur les deux familles.

La raison géométrique est celle qu'attendait Louis : pour $(A,B)$ seul, les
centres remplissent un **disque** de rayon $D/(2\sqrt{3})$ ; savoir $x \in C$
impose au centre d'être aussi équidistant de $x$, donc de tomber sur une
**droite** qui ne balaie qu'une bande mince quand $x$ parcourt $C$. Le disque
devient une bande, l'intersection des boules grossit, les témoins abondent.

## 4. Ce que cette mesure ne dit pas — trois réserves que je pose moi-même

1. **C'est l'idéal, pas un algorithme.** Le certificat idéal connaît les
   triplets valides du bloc ; un certificat utilisable ne connaît que les
   boîtes. Tout l'écart entre les deux reste à payer.
2. **La colonne « paire » sous-estime la production.** Elle n'évalue que
   `in_spindle` sur une paire du rectangle. La chaîne réelle cumule histogramme
   de coins, $W_3$ exact par ancre, secteurs $K=8$ et grille de cellules, et
   tue davantage. La comparaison honnête du tableau est **à base commune**
   (témoins universels parmi les points de handles), pas « bloc contre
   production ».
3. **Le pire cas est inchangé.** Sur ma contre-famille du 28 août il y a
   vraiment $\Theta(n^{2})$ triangles à sortir ; les blocs y seront hétérogènes
   et aucun certificat ne les sauvera. C'est un gain de **régime**.

## 5. Le verrou, et c'est votre domaine

Pour la paire, `corner64_universal` est un certificat **suffisant** : ALL aux
$8 \times 8$ coins distincts de $\mathrm{Box}(A) \times \mathrm{Box}(B)$ prouve
le témoin universel, et c'est un sens consommé établi par l'audit v4 du
17 août.

**Existe-t-il l'analogue ternaire ?** C'est-à-dire : ALL aux $8 \times 8 \times 8$
coins de $\mathrm{Box}(A) \times \mathrm{Box}(B) \times \mathrm{Box}(C)$
suffit-il à prouver que $z$ est strictement intérieur à la circumboule de
**tout** $(a,b,x)$ du produit continu ? Pour la paire, la preuve tient à une
convexité en $z$ et à une bilinéarité en $(a,b)$. En ternaire, `q3_power` fait
intervenir la forme de Gram $G = DE - F^{2}$, qui n'est ni bilinéaire ni
manifestement monotone en $x$ : je ne sais pas conclure, et je ne veux pas
mesurer avec un certificat dont la validité n'est pas établie.

## 6. Questions

- **V60.** Acceptez-vous le retrait du § 1 — mon théorème de séparation ne
  ferme pas la direction $A \times B \times C$, parce que $C$ n'est pas un
  facteur séparé ?
- **V61.** Le certificat aux $8^{3}$ coins est-il valide, invalide, ou ouvert ?
  Si invalide, existe-t-il un **sur-ensemble sûr** du produit continu (par
  exemple en majorant la variation de la droite $h_x = 0$ quand $x$ parcourt
  $\mathrm{Box}(C)$, ce qui donnerait une **bande** de centres, puis
  l'intersection des boules sur cette bande) qui soit exact en entiers et en
  $O(1)$ par bloc ?
- **V62.** Budget : un bloc porte environ 95 triplets à $n = 16\,000$. Un
  certificat de bloc doit donc coûter nettement moins que 95 tests bon marché.
  Le $h_c$ analogue à $h_a$, $h_b$ coûterait $O(\lvert C \rvert^{2})$
  évaluations de coins par bloc, soit environ 400 × 2,2 M — trop cher. Je
  proposerais de **garder $h_a$ et $h_b$ tels quels** (déjà calculés une fois
  par rectangle) et de n'ajouter qu'un **compte central par bloc**, un seul
  nombre. La disjonction reste garantie : $h_a$ compte dans $A$, $h_b$ dans
  $B$, le central ailleurs. Est-ce le bon découpage ?
- **V63.** Si le verrou du § 5 se ferme, où placez-vous ce chantier par rapport
  au center-cover counter-only de blocs que vous aviez mis en tête de file ?
  Les deux visent la même chose — garder des ancres implicites — mais celui-ci
  a maintenant un chiffre : 85 % des blocs éliminables, 3,5 à 5,3 fois plus de
  témoins qu'au niveau de la paire.
