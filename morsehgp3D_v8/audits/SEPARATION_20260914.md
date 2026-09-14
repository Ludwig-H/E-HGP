# Séparation s ∈ {8, 10, 12} du front v8 : même objet, coûts voisins, s = 8 confirmé

14 septembre 2026, après **ba11e3ab** (port Pool terminal). Auditeur
indépendant B. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Le domaine de la séparation est s ≥ 8 ; une valeur inférieure n'a pas
de sens pour l'objet et n'est pas mesurée ici. Le constructeur compare
s = 8, 10 et 12 dans ses reçus sans en désigner un optimum, et désigne
maintenant le front et les petits rectangles comme le coût dominant.
Cette note mesure, sur la sonde produit des sources ba11e3ab, ce que
s change réellement dans ce domaine, avec et sans le filtre Pool
terminal. Reçu rejouable :
[separation_20260914/](separation_20260914/SEPARATION_CHECKS.json).

## 1. Ce que s ne change pas

Sonde `mhgp8_wspd_q2_census_probe`, Kmax 10, graine 3, MidpointSamples /
SharedBlocks / frère / Complement / ancres individuelles, quatre
familles à 8k avec s ∈ {8, 10, 12} et Pool désactivé ou au seuil 64,
puis uniforme et amas à 32k avec les mêmes s et Pool 64 : 30 exécutions.
Pour chaque entrée, le condensé canonique des supports (nombre, IDs
intérieurs, coquilles, somme et xor) est **identique** pour les trois
valeurs de s et les deux réglages de Pool. La séparation est un contrat
du front sur la forme des rectangles ; ni le census ni le filtre n'en
dépendent pour l'objet produit. C'est attendu, et c'est vérifié.

## 2. Ce que s change : rectangles contre candidates

| Entrée | s | Rectangles | Candidates q2, Pool 64 | Candidates q2, sans Pool | Sites de facteurs émis | Plus grand facteur |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Uniforme 8k | 8 | 1 966 080 | 3 194 249 | 3 194 249 | 4 937 516 | 9 |
| Uniforme 8k | 10 | 2 194 971 | 3 058 164 | 3 058 164 | 5 138 918 | 8 |
| Uniforme 8k | 12 | 2 356 825 | 2 986 552 | 2 986 552 | 5 280 415 | 7 |
| Amas 8k | 8 | 1 210 070 | 1 743 978 | 29 728 389 | 2 927 863 | 1 056 |
| Amas 8k | 10 | 1 327 196 | 1 695 946 | 29 680 357 | 3 033 288 | 1 056 |
| Amas 8k | 12 | 1 406 182 | 1 670 797 | 29 655 208 | 3 103 651 | 1 056 |
| Terrain 8k | 8 | 432 884 | 935 699 | 935 699 | 1 230 915 | 13 |
| Terrain 8k | 10 | 499 914 | 906 409 | 906 409 | 1 315 469 | 13 |
| Terrain 8k | 12 | 553 917 | 888 983 | 888 983 | 1 380 084 | 9 |
| Rangées 8k | 8 | 69 677 | 16 091 188 | 16 091 188 | 162 930 | 4 000 |
| Rangées 8k | 10 | 81 363 | 16 091 188 | 16 091 188 | 178 320 | 4 000 |
| Rangées 8k | 12 | 83 977 | 16 083 976 | 16 083 976 | 175 952 | 4 000 |
| Uniforme 32k | 8 | 10 180 690 | 17 325 368 | — | 26 155 316 | 10 |
| Uniforme 32k | 10 | 11 457 301 | 16 517 539 | — | 27 270 245 | 7 |
| Uniforme 32k | 12 | 12 374 250 | 16 077 165 | — | 28 059 743 | 7 |
| Amas 32k | 8 | 7 579 029 | 12 184 055 | — | 19 124 581 | 4 048 |
| Amas 32k | 10 | 8 460 940 | 11 704 558 | — | 19 902 382 | 4 048 |
| Amas 32k | 12 | 9 083 933 | 11 443 047 | — | 20 441 962 | 4 048 |

De s = 8 à s = 12, les rectangles augmentent et les candidates du
census diminuent, dans des proportions qui se compensent presque. Sans
Pool, les amas gardent environ 29,7 millions de candidates quel que
soit s (les 28 produits inter-amas dominent) ; avec Pool 64 ces produits
sont filtrés et le reste suit la pente de l'uniforme. Sur l'uniforme,
le terrain et les rangées, Pool 64 ne sélectionne aucun rectangle dans
ce domaine de s (plus grand facteur bien sous 64 pour l'uniforme et le
terrain ; rangées sans réduction) : son coût y est nul et son effet
aussi.

## 3. Ce que s coûte en temps

| Entrée | s | Temps englobant, Pool 64 (s) | Temps englobant, sans Pool (s) | Front + comptage, Pool 64 (s) |
| --- | ---: | ---: | ---: | ---: |
| Uniforme 8k | 8 | 5.12 | 5.20 | 4.71 |
| Uniforme 8k | 10 | 5.29 | 5.28 | 4.87 |
| Uniforme 8k | 12 | 5.24 | 5.26 | 4.82 |
| Amas 8k | 8 | 2.80 | 12.61 | 2.47 |
| Amas 8k | 10 | 2.94 | 12.49 | 2.60 |
| Amas 8k | 12 | 2.89 | 12.59 | 2.55 |
| Terrain 8k | 8 | 0.96 | 0.99 | 0.80 |
| Terrain 8k | 10 | 0.99 | 1.01 | 0.82 |
| Terrain 8k | 12 | 0.96 | 0.98 | 0.81 |
| Rangées 8k | 8 | 0.23 | 0.23 | 0.14 |
| Rangées 8k | 10 | 0.26 | 0.24 | 0.16 |
| Rangées 8k | 12 | 0.24 | 0.24 | 0.16 |
| Uniforme 32k | 8 | 33.51 | — | 31.41 |
| Uniforme 32k | 10 | 30.18 | — | 28.25 |
| Uniforme 32k | 12 | 29.88 | — | 27.99 |
| Amas 32k | 8 | 19.94 | — | 18.30 |
| Amas 32k | 10 | 20.11 | — | 18.48 |
| Amas 32k | 12 | 20.36 | — | 18.73 |

Dans ce domaine, le temps englobant ne dépend presque pas de s : les
écarts entre 8, 10 et 12 sont de l'ordre du bruit d'un hôte partagé
(à 8k, moins de 5 % ; à 32k uniforme, s = 8 sort à 33,5 s ici contre
27,7 s dans une exécution précédente du même binaire, ce qui borne le
bruit à environ 20 %). Le filtre Pool, lui, divise par 4,5 le temps des
amas à 8k quel que soit s et ne coûte rien ailleurs.
Le choix s = 8 du constructeur est donc un point de fonctionnement
aussi bon que les autres, et la séparation n'est pas un levier de coût.
La voie de progrès reste le coût par rectangle du front lui-même
(recherche de témoins depuis la racine, mesurée dans
[PROPAGATION_TEMOINS_20260914.md](PROPAGATION_TEMOINS_20260914.md)) et
le census des petits produits, comme le constructeur et l'auditeur A
l'ont écrit.

## 4. Reproduction

```bash
mkdir -p /tmp/pinned && git archive ba11e3ab morsehgp3D_v8/src morsehgp3D_v8/bench morsehgp3D_v8/CMakeLists.txt morsehgp3D_v8/cmake | tar -x -C /tmp/pinned
cmake -S /tmp/pinned/morsehgp3D_v8 -B /tmp/pinned_build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && cmake --build /tmp/pinned_build --target mhgp8_wspd_q2_census_probe
python3 -B morsehgp3D_v8/audits/separation_20260914/run_separation.py --probe /tmp/pinned_build/mhgp8_wspd_q2_census_probe --src-root /tmp/pinned/morsehgp3D_v8 --output /tmp/separation.json
```

Les temps sont ceux d'un hôte partagé, un fil, une répétition : ils ne
sont pas un contrat, seuls les comptes et les condensés le sont.
