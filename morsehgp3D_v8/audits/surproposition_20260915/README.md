# Bilan net de la surproposition de témoins (fenêtre L = K, 2K, 4K)

Auditeur B, 15 septembre 2026. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Mesure propre de l'audit sur une **copie patchée** du moteur
`2741d614`, jamais dans l'arbre du constructeur ; aucune qualification de
vitesse constructeur, aucun statut public.

## Question

La note `docs/P0_SURPROPOSITION_TEMOINS_Q2.md` sépare le seuil K du nombre L
de propositions de `Front::filter` et demande le bilan complet : propositions
supplémentaires et tests stricts payés sur tous les produits visités, contre
rectangles, candidats et visites Z épargnés au census, en temps mur du
pipeline q2 complet. Le [plafond](../plafond_proposeur_20260915/README.md)
mesuré sur les rectangles émis dit ce qu'un proposeur peut au mieux retirer ;
ici on mesure ce que les fenêtres 2K et 4K retirent et ce qu'elles coûtent,
avec les deux politiques de la note (petits facteurs B0 = 16, tous produits).

## Méthode

- `front_filter.patch` (appliqué par `apply_patch.sh` sur
  `git archive 2741d614 morsehgp3D_v8/{src,bench,tests,CMakeLists.txt,cmake}`)
  étend `Front::filter` exactement comme la note le décrit : la fenêtre
  historique de K sites garde son ordre, sa formule et ses sauts ; si le masque
  survit et que le produit est éligible (B0 = 0 : tous ; sinon
  `max(|A|, |B|) <= B0`), les deux intervalles disjoints `[first_L, first)` et
  `[last, last_L)` de la fenêtre L = factor · K autour du même pivot sont
  proposés à leur tour, crédits conservés, arrêt à K succès stricts. Une
  exception garde l'inclusion de la fenêtre historique. Paramètres lus une
  fois par `Front` : `MHGP8_AUDIT_L_FACTOR` (1, 2, 4) et `MHGP8_AUDIT_B0`
  (0 ou 16), toute autre valeur refusée ; factor = 1 reproduit le moteur
  épinglé, ce que le reçu mesure (voir plus bas). Trois compteurs ajoutés à
  `WspdFrontWork` et fusionnés dans `merge_work` : produits ayant reçu au moins
  une proposition supplémentaire, propositions supplémentaires, effacements
  complets du masque obtenus par l'extension.
- `surproposition_probe.cpp` lance `run_wspd_q2_census` (SharedBlocks,
  Saturating, ComplementFirst, Individual, Pool 64, `MidpointSamples`), puis le
  front seul (masque 1, consommateur vide, caches chauds) dont les compteurs
  doivent égaler ceux du front du pipeline (`front_match`, sinon code 3) ; il
  imprime les compteurs du front (recherches, propositions, tests H, crédits,
  rejets, émissions, extension), du census (candidats, admis, visites Z, avances
  de curseur, scissions, collecte), du Pool, les temps mur (pipeline complet,
  comptage, collecte, front seul) et un condensé canonique du flux de supports
  (ids ordonnés, somme et xor de condensés FNV par support : témoin d'identité
  de flux modulo collision, pas un oracle). Compilé avec
  `-DMHGP8_AUDIT_UNPATCHED` et relié à la bibliothèque non patchée de
  `2741d614`, il fournit la référence extérieure au patch.
- `run_surproposition.py` enchaîne, par configuration, les cinq variantes
  répétées trois fois (compteurs identiques exigés, temps minimum retenu) puis
  le probe non patché, et grave `SURPROPOSITION_CHECKS.json` avec les hash des
  trois sources patchées. Contrôles exécutés : moteur non patché et variante
  L = K identiques sur tous les compteurs de front, de census et de flux ;
  même flux (supports, totaux intérieurs et coquilles, condensés) pour les
  cinq variantes ; `front_match` ; extension nulle en référence et strictement
  active pour L > 1 (plancher de non-vacuité : produits étendus et rejets de
  l'extension positifs, tous produits ≥ petits facteurs) ; identités
  propositions = recherches × min(K, n) + propositions étendues et propositions
  étendues ≤ produits étendus × (facteur − 1) × K ; monotonie (émissions et
  candidats ≤ référence, rejets du census diminués d'autant) ; supports =
  paires admises. Les temps sont rapportés, jamais contrôlés. Aucun `assert`,
  rejoué sous `python3 -O`.
- Exactitude contre la force brute : le harnais série
  `../chaine_q2_20260914/chain_verify.cpp` (paires q2 exactes, intérieurs
  stricts, coquille complète, clé) est relié à la bibliothèque patchée et
  rejoué en mode `--quick` pour L = 2K et 4K, B0 = 0 et 16 :
  `CHAINE_Q2_SURPROPOSITION_L<factor>_B<B0>_CHECKS.json`.

Reproduction :

```bash
./apply_patch.sh /workspaces/E-HGP <dest>
cmake -S <dest>/morsehgp3D_v8 -B <dest>/build -DCMAKE_BUILD_TYPE=Release
cmake --build <dest>/build --target mhgp8_p0 --parallel
g++ -std=c++20 -O2 -Wall -Wextra -I<dest>/morsehgp3D_v8/src -I<dest>/morsehgp3D_v8/bench \
    surproposition_probe.cpp <dest>/build/libmhgp8_p0.a -pthread -o surproposition_probe
# référence non patchée : <pin> = git archive 2741d614 construit tel quel
g++ -std=c++20 -O2 -Wall -Wextra -DMHGP8_AUDIT_UNPATCHED -I<pin>/morsehgp3D_v8/src -I<pin>/morsehgp3D_v8/bench \
    surproposition_probe.cpp <pin>/build/libmhgp8_p0.a -pthread -o surproposition_probe_unpatched
python3 -O run_surproposition.py --binary ./surproposition_probe \
    --reference-binary ./surproposition_probe_unpatched --patched-src <dest>/morsehgp3D_v8 --repeats 3
```

## Résultats (`SURPROPOSITION_CHECKS.json`, graine 3, un fil, Pool 64, minimum de trois répétitions)

Temps mur du pipeline complet (front + census + collecte) en part de la
référence L = K ; candidats et propositions du front en part de la référence
(politique petits facteurs ; la politique tous produits diffère de moins de
0,2 point sur les candidats).

| Famille | n | K | Référence L = K | 2K petits facteurs | 2K tous produits | 4K petits facteurs | 4K tous produits | Candidats 2K / 4K | Propositions 2K / 4K |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| uniform | 8 000 | 10 | 5,09 s (front 1,66 s), 3 194 249 candidats | 45,4 % | 44,9 % | 45,6 % | 45,2 % | 26,5 % / 19,6 % | 89,5 % / 140,5 % |
| uniform | 8 000 | 5 | 1,94 s (front 0,79 s), 1 413 285 candidats | 53,3 % | 52,2 % | 48,3 % | 47,8 % | 32,8 % / 22,3 % | 96,6 % / 142,5 % |
| terrain | 8 000 | 10 | 0,90 s (front 0,31 s), 935 699 candidats | 65,8 % | 64,6 % | 66,0 % | 65,8 % | 36,6 % / 26,4 % | 118,9 % / 191,5 % |
| terrain | 8 000 | 5 | 0,37 s (front 0,16 s), 398 032 candidats | 68,0 % | 66,9 % | 70,1 % | 68,9 % | 39,9 % / 32,3 % | 116,4 % / 196,1 % |
| clusters | 8 000 | 10 | 2,73 s (front 0,93 s), 1 743 978 candidats | 59,7 % | 59,4 % | 60,7 % | 60,5 % | 36,3 % / 27,6 % | 108,5 % / 175,9 % |
| clusters | 8 000 | 5 | 1,24 s (front 0,51 s), 944 441 candidats | 61,0 % | 61,9 % | 57,6 % | 57,1 % | 39,1 % / 27,3 % | 108,5 % / 165,3 % |
| rows | 8 000 | 10 | 0,21 s (front 0,04 s), 16 091 188 candidats | 106,1 % | 106,4 % | 119,6 % | 119,5 % | 100,0 % / 100,0 % | 194,3 % / 383,2 % |
| rows | 8 000 | 5 | 0,12 s (front 0,02 s), 16 039 970 candidats | 102,9 % | 102,6 % | 110,3 % | 110,8 % | 100,0 % / 100,0 % | 184,6 % / 353,9 % |
| uniform | 16 000 | 10 | 12,10 s (front 3,97 s), 7 347 458 candidats | 45,4 % | 45,0 % | 41,2 % | 40,9 % | 27,3 % / 17,4 % | 89,3 % / 127,2 % |
| terrain | 16 000 | 10 | 1,91 s (front 0,67 s), 1 886 724 candidats | 62,2 % | 61,5 % | 66,7 % | 67,0 % | 34,2 % / 27,8 % | 114,2 % / 195,9 % |
| clusters | 16 000 | 10 | 7,62 s (front 2,56 s), 4 786 146 candidats | 53,9 % | 53,7 % | 50,4 % | 50,5 % | 33,3 % / 21,9 % | 101,7 % / 149,5 % |
| rows | 16 000 | 10 | 0,43 s (front 0,08 s), 32 182 626 candidats | 106,0 % | 106,0 % | 118,5 % | 118,6 % | 100,0 % / 100,0 % | 194,3 % / 383,1 % |
| uniform | 32 000 | 10 | 28,58 s (front 9,17 s), 17 325 368 candidats | 42,6 % | 42,1 % | 39,2 % | 38,7 % | 25,6 % / 16,6 % | 86,8 % / 124,8 % |
| terrain | 32 000 | 10 | 4,16 s (front 1,51 s), 4 017 435 candidats | 63,7 % | 63,3 % | 64,4 % | 63,7 % | 35,1 % / 25,2 % | 116,2 % / 185,8 % |
| clusters | 32 000 | 10 | 19,64 s (front 6,50 s), 12 184 055 candidats | 49,6 % | 49,3 % | 46,6 % | 46,2 % | 30,5 % / 20,5 % | 96,1 % / 141,8 % |
| rows | 32 000 | 10 | 0,88 s (front 0,17 s), 64 365 502 candidats | 107,1 % | 105,8 % | 118,2 % | 120,2 % | 100,0 % / 100,0 % | 194,2 % / 383,0 % |

Exactitude contre la force brute (harnais série de `chaine_q2_20260914/`,
`--quick`, 56 nuages adversariaux × 8 combinaisons, 1 162 560 paires
contrôlées, 296 160 supports vivants) : **0 désaccord** pour les quatre
variantes, reçus `CHAINE_Q2_SURPROPOSITION_L{2,4}_B{0,16}_CHECKS.json`
épinglés sur le `front.cpp` patché (c4c3af4e…). Le condensé des supports et
les totaux intérieurs/coquilles sont identiques pour les cinq variantes sur
les seize configurations d'échelle.

Lecture :

- **La référence est mesurée, pas supposée** : sur les seize configurations,
  le probe non patché relié au moteur `2741d614` tel quel donne les mêmes
  compteurs de front, de census et de flux que la variante L = K de la copie
  patchée ; à uniform 8k K = 10, c'est le reçu constructeur cité par la note à
  l'unité (3 194 249 candidats, 2 914 705 rejets, 171 895 354 visites Z,
  279 544 supports).
- **Gain net sur les nuages génériques.** Temps total en part de la
  référence, fenêtre 2K : uniform 42 à 53 %, amas 49 à 62 %, terrain 61 à
  68 % ; fenêtre 4K : uniform 39 à 48 %, amas 46 à 61 %, terrain 64 à 70 %.
  Le gain croît avec n sur uniform et amas (32k : 42 % et 49 % à 2K). Les
  candidats du census tombent à 26 à 33 % (uniform), 31 à 39 % (amas), 34 à
  40 % (terrain) à 2K, et à 17 à 22 %, 21 à 28 %, 25 à 32 % à 4K ; les
  visites Z suivent.
- **Le front lui-même coûte moins avec 2K** : rejeter plus haut élague les
  descendants, si bien que les propositions totales restent à 86 à 97 % de
  la référence sur uniform, 96 à 109 % sur les amas, 114 à 119 % sur terrain
  (à 4K : 125 à 145 %, 142 à 178 %, 186 à 199 %). Le front seul passe par
  exemple de 9,17 s à 5,78 s à uniform 32k avec 2K.
- **Les deux politiques sont indiscernables** : petits facteurs (B0 = 16) et
  tous produits donnent les mêmes candidats à 0,2 point près et des temps
  égaux au bruit près. Les produits à gros facteurs que la fenêtre historique
  laisse passer ne sont pas rattrapés par l'extension ; B0 = 16 suffit.
- **Les rangées sont la régression attendue** : candidats inchangés, temps
  103 à 107 % (2K) et 110 à 120 % (4K), propositions doublées puis
  quadruplées pour rien. C'est le régime que le plafond annonçait (0 à 2 %) ;
  à K = 5 l'extension n'y rejette rien, ce que le reçu exige.
- **2K contre 4K** : 4K retire encore 6 à 12 points de candidats mais paie
  40 à 75 % de propositions en plus ; en temps, il gagne 3 à 5 points sur
  uniform et amas à 16k, 32k et à K = 5, rien à 8k K = 10, et perd 0 à 5
  points sur terrain. La variante 2K,
  petits facteurs, est le point robuste ; 4K ne se justifie que sur les
  grands nuages uniformes ou en amas.

## Limites

- Copie d'audit : le patch n'est ni une implémentation constructeur ni une
  proposition de code ; il sert à mesurer la note, pas à la remplacer. Les
  petits juges de bord (n < L, propositions dans A/B, tangences) restent à
  écrire côté constructeur.
- Les temps sont ceux de ce conteneur, un fil, une graine, minimum de trois
  répétitions, machine sans autre campagne ; les compteurs sont déterministes et
  portent la conclusion. Le front seul est mesuré après le pipeline, caches
  chauds ; le pipeline n'a pas d'horloge propre au front.
- Aucun mutant causal ni juge de bord dans ce dossier : l'exactitude vient du
  harnais contre la force brute (56 nuages, dont les petits) ; les cas de bord
  demandés par la note (n < L, tangences, épuisement sans K succès, K-ième
  témoin trouvé par la seule extension) relèvent des petits juges constructeur.
- Le Pool à 64 ne filtre rien sur ces nuages (`pool_filtered_pairs = 0`) :
  candidats = masse résiduelle du front.
- La copie patchée n'est pas rejouable avec les portes et matrices constructeur
  pour factor > 1 : leur invariant `proposed_sites <= K × witness_searches`
  (et le contrat « au plus K rangs adjacents » de `front.hpp`) devient faux par
  construction ; le probe d'audit ne l'utilise pas. Les variables d'environnement
  n'acceptent que les valeurs mesurées (1, 2, 4 et 0, 16), toute autre est refusée.
  Les trois compteurs ajoutés sont fusionnés dans `merge_work` pour les chemins
  à jobs, que le probe (front mono) n'exerce pas.
