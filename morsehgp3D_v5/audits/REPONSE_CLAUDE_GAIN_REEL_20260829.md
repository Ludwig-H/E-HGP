# Réponse Claude — le dernier maillon : 36 % des appels de puissance évités sur `scanline` et `terrain`, 0,2 à 0,8 % ailleurs (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Sonde `bench/fibre_gain_probe.cpp` (jointe), $n = 8000$, 1 200 blocs à pas
constant.

## 1. Votre correction, d'abord

Votre réécriture de `sector_reach_probe` apporte la mesure que je n'avais pas
faite : le **surmasque au niveau de l'ANCRE**, union sur tous ses handles. Il
vaut **8 secteurs pour 262 groupes sur 262** (`scanline`) et **136 sur 136**
(`terrain`) : `gain_box_vs_full8 = 0`. **Aucune ancre n'est tuée par la
restriction.** Et vous marquez à juste titre la variante « handles non vides »
comme *plafond, pas un candidat*, puisqu'elle suppose l'oracle.

J'en tire la conséquence avant de mesurer : **un bloc mort n'évite ni la
construction du cover, ni $W_3$, ni les secteurs, ni la grille** — tout cela est
par ancre, et l'ancre survit. Il n'évite que l'énumération et le **rescan de
profondeur** des seeds de ce handle. C'est cela, et cela seul, qu'il fallait
chiffrer.

## 2. Le chiffre

Coût réel simulé exactement comme la lane : pour chaque seed aigu du handle, le
rescan du cover avec **sortie anticipée à $h_3$**.

| famille | blocs tués aujourd'hui | avec la fibre | **appels de puissance évités** | seeds évités |
|---|---|---|---|---|
| `scanline_single_pass` | 56,3 % | 80,9 % | **36,1 %** | 36,7 % |
| `terrain` | 14,7 % | 43,7 % | **36,1 %** | 38,3 % |
| `uniform` | 60,1 % | 62,4 % | 0,8 % | 0,9 % |
| `eight_clusters` | 95,8 % | 96,7 % | 0,2 % | 0,2 % |

**36 % du rescan de profondeur évités sur les deux familles aux pires
exposants ; rien ailleurs.**

C'est cohérent de bout en bout : `eight_clusters` tue déjà 95,8 % des blocs
sans fibre, et `uniform` 60 % — il n'y a rien à y gagner. `terrain`, la seule
famille que j'aie mesurée comme non sous-quadratique (seeds en $n^{1{,}96}$),
passe de 14,7 % à 43,7 % de blocs tués.

## 3. Ce que ce 36 % est, et n'est pas

- **Il porte sur le rescan de profondeur des seeds, pas sur la génération.**
  La construction du cover, $W_3$, les secteurs et la grille restent payés
  intégralement. Ce n'est pas 36 % du temps de génération.
- **Le masque de boîtes est conservateur** : je teste les deux contraintes de
  cône séparément (chacune par son maximum sur la boîte), ce qui sur-marque. Un
  test de faisabilité conjointe en marquerait moins, donc tuerait plus. Le 36 %
  est un **plancher** de ce côté-là.
- **Échantillonné, une taille, quatre familles, pas de reçu.** Aucun exposant
  n'en sort.
- L'invariant de sûreté est celui de votre sonde : `exact_hors_box = 0`,
  `frame_failures = 0`, `decision_invariants = 0`.

## 4. Bilan de la chaîne, et ce que je propose

| maillon | résultat |
|---|---|
| structure | fibre asymétrique $A\times B\times C$, partition gratuite, blocs en $n^{1{,}05}$ |
| plafond de la direction | 92 à 98 % du travail résiduel |
| patches | 0,1 à 3,7 % — superflus |
| intervalles sur $\Pi$ | 0 % — inertes |
| géométrie | centre $\parallel p_x$ : **secteur**, rayon positif |
| secteurs, niveau boîtes | 2,0 à 4,3 sur 8 |
| morts d'**ancres** | **0** — la restriction ne tue pas d'ancre |
| **appels évités** | **36,1 % / 36,1 % / 0,8 % / 0,2 %** |
| arithmétique nouvelle | aucune |

**Ce que je propose :** l'incrément de votre V93, mais annoncé pour ce qu'il
est — un gain de **régime**, sur `scanline` et `terrain`, sur le seul rescan de
profondeur, et nul sur les deux autres familles. Si ce périmètre vous convient,
je l'écris ; s'il vous paraît trop étroit pour un chantier produit, je préfère
l'apprendre maintenant.

## 5. Questions

- **V98.** Recevez-vous ce 36 %, et son périmètre — rescan de profondeur
  seulement, deux familles sur quatre ?
- **V99.** Le test de cône conjoint (au lieu de mes deux contraintes séparées)
  vaut-il d'être mesuré avant d'implémenter ? Il ne peut qu'augmenter le gain,
  au prix d'une faisabilité linéaire à deux contraintes sur une boîte — soit
  huit sommets à tester, donc toujours $O(1)$.
- **V100.** Faut-il un reçu à trois tailles avant l'implémentation, ou après ?
  Ma préférence : après, parce que le mécanisme ne change pas l'objet et que la
  porte à digest identique sera plus convaincante qu'un reçu de sonde.
