# Réponse Claude — le maillon central : exiger 2,4 secteurs au lieu de 8 triple le taux de mort sur `terrain` (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Sonde `bench/sector_reach_probe.cpp`, $n = 8000$, 1 500 blocs à pas constant.

## 1. Le maillon qui manquait

Toute la chaîne reposait sur un proxy : « moins de secteurs requis ⟹ plus de
morts ». Je ne l'avais jamais mesuré. `anchor_sector_kill` exposant déjà
`sector_counts`, la mesure est directe — même arithmétique, même cover, seule
la condition change :

- **aujourd'hui** : l'ancre meurt si le minimum sur les **huit** secteurs
  atteint $h_3$ ;
- **avec la fibre** : le bloc meurt si le minimum sur les seuls secteurs
  **atteignables depuis $\mathrm{Box}(C)$** atteint $h_3$.

| famille | morts aujourd'hui | **avec la fibre** | **gagnés** |
|---|---|---|---|
| `scanline_single_pass` | 51,5 % | **82,4 %** | **+ 30,9 points** |
| `terrain` | 17,0 % | **52,2 %** | **+ 35,2 points** |
| `uniform` | 55,9 % | 62,4 % | + 6,5 points |
| `eight_clusters` | 95,8 % | 96,9 % | + 1,0 point |

**Sur `terrain`, le taux de mort par secteurs triple : 17 % → 52 %.** Sur
`scanline` il passe de la moitié aux quatre cinquièmes.

## 2. Pourquoi c'est le bon endroit

Le gain se concentre **exactement** sur les deux familles dont j'ai mesuré les
pires exposants :

- `terrain` : seeds en $n^{1{,}96}$, complétions en $n^{1{,}93}$ — la seule
  famille qui n'est pas sous-quadratique — et c'est elle qui gagne le plus ;
- `scanline` : ancres en $n^{1{,}43}$ à $n^{1{,}56}$ — deuxième plus gros gain.

Et il est **négligeable** là où le mécanisme actuel suffit déjà :
`eight_clusters` tue 95,8 % par secteurs sans aucune fibre, donc + 1,0 point.
C'est cohérent, et c'est rassurant : un mécanisme qui aurait gagné partout
également aurait signalé un artefact.

## 3. V95 — le plafond du split de $C$, mesuré avant de le proposer

| famille | secteurs parent | enfants | enfants échappant au cas « origine contenue » |
|---|---|---|---|
| `scanline` | 2,29 | **1,73** | 48,2 % |
| `terrain` | 3,40 | **2,50** | 45,1 % |
| `uniform` | 3,73 | **2,92** | 44,5 % |
| `eight_clusters` | 2,16 | **1,77** | 47,7 % |

Un niveau de split réduit les secteurs requis de 18 à 26 %, et **environ 46 %
des enfants échappent au cas où le rectangle contient l'origine** — celui qui
ne rapporte rien. C'est un plafond réel.

**Je ne le propose toujours pas.** Un split double le nombre de blocs, donc le
coût du certificat, et je n'ai pas mesuré ce côté-là. J'ai déjà payé deux fois
un split dont le gain apparent ne survivait pas au coût.

## 4. Ce que cette mesure n'est pas

- Ce sont des morts **de blocs par le test de secteurs seul**. En production,
  une ancre qui survit aux secteurs passe encore par la grille de cellules puis
  par le scan de profondeur : ces morts ne se convertissent pas une pour une en
  appels de puissance évités.
- Une taille, quatre familles, un échantillon, pas de reçu.
- Le calcul des secteurs atteignables emploie ici un `atan2` de mesure ; une
  implémentation doit passer par les produits vectoriels entiers avec les
  sommets de l'octogone, le produit **nul** marquant les deux secteurs
  adjacents (V96).

## 5. L'état de la chaîne, complet

| maillon | résultat |
|---|---|
| plafond de la direction | 92 à 98 % du travail résiduel |
| patches | 0,1 à 3,7 % — superflus |
| intervalles sur $\Pi$ | 0 % — inertes |
| structure des centres | secteur, rayon positif, $v \parallel p_x$ |
| secteurs atteints, niveau boîtes | **2,0 à 4,3 sur 8** |
| **morts supplémentaires** | **+ 1,0 à + 35,2 points**, concentrées sur les pires familles |
| arithmétique nouvelle | **aucune** |
| plafond du split de $C$ | −18 à −26 % de secteurs, 46 % d'échappées |

## 6. Question

- **V97.** La chaîne est complète et chaque maillon est chiffré. Recevez-vous
  l'ensemble, et l'incrément à écrire est-il bien celui de votre V93 — calcul
  entier des secteurs atteignables, restriction du seuil, porte à digest
  identique et compteur d'appels de puissance évités ? Si oui, je le commence ;
  sinon je voudrais savoir quel maillon vous ne recevez pas avant d'écrire une
  ligne dans le chemin produit.
