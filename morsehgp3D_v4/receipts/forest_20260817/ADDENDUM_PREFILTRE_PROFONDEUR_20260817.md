# Addendum — pré-filtre de profondeur : exact, jugé, et honnêtement quasi neutre

Date : 17 août 2026. Exécution du point « pré-filtre de profondeur guidé
par les compteurs » (reçu flux réels : 98 % des boules uniques meurent en
`|I_B| > 9` après avoir payé leur census).

## L'objet (`src/pipeline/ball_stream.hpp`)

`ball_depth_exceeds(ix, key, cap)` : descente COMPTANTE avant le census —
une boîte entièrement STRICTEMENT intérieure (`max P < 0` sur la boîte)
est avalée en O(1) par son compte de positions uniques, sans allocation
ni collecte. EXACT : une feuille a `min P = max P` ; à 0 elle est
coquille et n'est jamais comptée — le filtre tue exactement les boules
que le census aurait tuées. Mutant `prefilter-nonstrict` (boîtes à
`max P <= 0` comptées intérieures : des boules à plateau meurent à
tort) : TUÉ par le juge (code 4, événements manquants). Porte
`mhgp4_forest_probe_mutant_prefilter`. **78 portes CTest vertes**, 0
désaccord jugé partout.

## Mesure — le résultat est NEUTRE, et c'est la donnée qui compte

| n=400 uniform | avant | après |
|---|---|---|
| t_census | 32,0 s | 30,9 s |
| t_flux | 19,5 s | 16,6 s (bruit machine) |
| désaccords (n=120 jugé) | 0 | 0 |

Gain census ≈ 3 %. Pourquoi si peu : le census sortait DÉJÀ tôt (au
10e intérieur trouvé, feuille par feuille) et, à ces tailles, les boules
mortes ont un `|I_B|` à peine supérieur au plafond — la descente
comptante visite à peu près les mêmes nœuds que la sortie anticipée.
L'avalement de sous-arbres ne paiera que sur des boules PROFONDES
(`|I_B| >> 9`), c'est-à-dire aux n d'intérêt (8000/16000/32000) où une
boule couvre des sous-arbres entiers. Le pré-filtre est conservé : exact,
au pire neutre, potentiellement utile à l'échelle — mais il ne FERME pas
le poste de coût.

## La conclusion que les compteurs imposent

Le coût n'est pas le prix unitaire d'une boule morte (~4 µs), c'est leur
NOMBRE : 7 597 781 boules uniques à n=400, dont 6 858 491 candidats du
générateur q4, pour 104 802 événements finaux. Aucun filtre aval
(census, pré-filtre) ne peut compenser un générateur qui émet 65 fois
trop : le prochain poste est le FILTRE DE CANDIDATS Q4 À LA GÉNÉRATION,
sans census — soit un minorant de profondeur par boule certifié
(le lemme de complétude sous les seuils h ne borne que l'ancre ; les
témoins de fuseau minorent du mauvais côté), soit la re-dérivation de la
sélection axiale comme pré-clé de candidats (son économie change : elle
coûtait plus cher que la baseline énumérée comme SÉLECTION, mais chaque
candidat évité économise désormais tri + census aval). Question posée
aux auditeurs : `QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md`.
