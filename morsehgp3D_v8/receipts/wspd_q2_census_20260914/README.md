# Huitième tranche : WSPD consommée par le census q2

14 septembre 2026. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

Périmètre : q2 de tout le nuage, avec front, comptage, collecte complète,
callback canonique et destructions. **Ni catalogue q3/q4, ni tour FULL,
ni contrat 50k/G4 acquis.** Voir le [contrat](../../docs/P0_FRONT_ET_CENSUS_Q2.md).

## Capture et qualification

Builds neufs Release GCC 13.3 et Debug Clang 18.1.3 ASan/UBSan :
`build/v8_front_census_20260914` et
`build/v8_front_census_sanitize_20260914`. Boost est utilisé uniquement
par les juges. Les builds des sept tranches précédentes ne sont pas modifiés.
Les répertoires deviennent épinglés à la fermeture de cette qualification.

Le gate intégré utilise un calcul scalaire indépendant sur tous les sites
des petits nuages. Il confronte supports, clés, intérieurs et coquilles,
et pas seulement un compteur de masse : 1 255 appels intégrés,
467 858 contrôles, 46 762 supports, 72 204 IDs intérieurs et 125 060 IDs
de coquille. Treize familles et deux permutations, K1/2/5/10,
s8/10/12, Pure/Samples × Pairwise/Shared ; 60 subdivisions après crédit,
11 rejets d'entrée, quatre exceptions de callback et huit modèles fautifs.
Les quatre diagonales du cube conservent séparément leur coquille de huit
points. Les nouveaux lecteurs normal/−O testent chacun 128 sorties,
53 mutations refusées et deux captures échouées contrôlées.

Les commandes, stdout/stderr intégraux, XML CTest, retcodes et hashes
avant/après sont conservés sous `qualification/`. Le script ayant produit
ces captures est conservé octet pour octet dans
[record.py.snapshot](qualification/record.py.snapshot), à son chemin
d'exécution déclaré `build/v8_front_census_20260914/record.py` lors du run.
Il ne reconstruit pas les binaires : il vérifie ceux du build déclaré.
Les essais échoués ne sont jamais écrasés par un nouvel essai.

Les **43 CTests passent** en [Release](qualification/release_193hnm51/RESULT.json)
et sous [Clang ASan/UBSan](qualification/sanitize_uzjl6kud/RESULT.json).
Chaque bras conserve aussi trois gates géométriques explicites, trois
rejets CLI du gate et dix rejets CLI de la sonde, tous avec code 2 attendu.
ASan/UBSan a été exécuté hors sandbox avec autorisation pour permettre
LeakSanitizer ; aucune détection n'a été désactivée. Les pins de fermeture
des sources, binaires et caches sont identiques aux ouvertures.
Les trois campagnes sont closes : 36 mesures principales, 16 comparaisons
à 8k et le pilote, soit **53 mesures et 48 configurations distinctes**.
Les sources et binaires restent inchangés pendant chaque capture.
Les [lecteurs normal/−O et le contrôle documentaire](qualification/readers_bhl76hyp/RESULT.json)
passent ; les deux résumés sont identiques octet pour octet. Les 48
configurations comparent les mêmes douze empreintes de sorties par
famille/taille/K. Les répertoires de build sont désormais épinglés.
Le contrôle `git diff --check` signale uniquement une ligne vide finale
dans les deux nouveaux fichiers Python (runner et gate). Leurs octets
qualifiés sont conservés ; aucun correctif cosmétique ne réécrit les pins.
Le contrôle avec `core.whitespace=-blank-at-eof` ne signale rien d'autre.

## Comparaison des quatre chemins à 8k, Kmax10, s8

Temps total mono en secondes, une exécution par bras de `paired_8k` :

| Famille | Pure / Pairwise | Pure / Shared | Samples / Pairwise | Samples / Shared |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 26,722 | 26,658 | 5,536 | 5,746 |
| Terrain mince | 16,647 | 15,067 | 1,219 | 1,152 |
| Huit amas | 18,513 | 16,467 | 16,486 | 14,503 |
| Deux rangées | 5,908 | 2,961 | 3,528 | 1,610 |

Les supports, populations et empreintes canoniques sont identiques entre
les quatre chemins. Les visites Z baissent avec Samples et/ou Shared,
mais Shared ne gagne pas systématiquement en temps : il reste légèrement
plus lent que Pairwise sur l'uniforme après Samples. Aucun choix adaptatif
automatique n'est installé.

Résultat positif borné : le proposeur Samples, plus lent sur le **front
seul** historique, réduit fortement le total face à Pure sur uniforme et
terrain. Résultat négatif : les amas restent lents malgré un front filtré
et un index partagé. Ces nombres ne sont pas comparables par soustraction
aux temps du front historique à trois voies : cette capture demande q2 seul.

## Croissance et limites

La matrice `shared_matrix` utilise quatre familles, n8k/16k/32k,
Kmax10 et les vraies séparations s8/10/12, Samples/Shared. Le pilote
`pilot_8k` conserve un premier essai uniforme/s8 du même binaire. Les
répétitions vérifient un travail discret identique ; les temps sont
exploratoires sur machine partagée, sans affinité ni promesse de latence.
Des suites de qualification et l'autre campagne ont tourné concurremment,
bien que les invocations de chaque campagne soient séquentielles. Les
petits écarts de temps entre variantes ne sont donc pas conclusifs.

Résultats s8 ; les temps couvrent toutes les étapes déclarées :

| Famille | Total 8k / 16k / 32k (s) | Visites Z 8k / 16k / 32k (millions) | Facteurs de doublement des visites |
| --- | --- | --- | --- |
| Uniforme | 4,682 / 11,159 / 26,125 | 200,529 / 483,362 / 1 165,527 | ×2,410 / ×2,411 |
| Terrain mince | 0,873 / 1,915 / 4,221 | 30,050 / 63,497 / 141,344 | ×2,113 / ×2,226 |
| Huit amas | 12,873 / 64,842 / 228,532 | 973,179 / 4 199,341 / 17 664,591 | ×4,315 / ×4,207 |
| Deux rangées | 1,481 / 5,412 / 13,704 | 97,085 / 378,674 / 942,638 | ×3,900 / ×2,489 |

Uniforme/terrain indiquent une croissance sous le carré sur ces tailles,
pas une preuve générale. Les rangées gardent une quasi-multiplication par
quatre des tâches B entre 8k et 16k ; la baisse suivante ne prouve pas une
borne asymptotique. Les amas échouent nettement au critère empirique demandé.
Leurs supports acceptés font seulement 245 733→520 208→1 088 696,
soit ×2,117 puis ×2,093, alors que les requêtes B font
51,841→206,350→832,411 millions. Le coût des vraies sorties ne justifie
pas à lui seul cette explosion intermédiaire. **P0 et la
sous-quadraticité générale ne sont pas résolues.**

À 32k, s8/10/12 donnent respectivement :

| Famille | Temps total (s) | Visites Z (millions) |
| --- | --- | --- |
| Uniforme | 26,125 / 26,837 / 27,010 | 1 165,527 / 1 141,078 / 1 135,331 |
| Terrain mince | 4,221 / 4,345 / 4,500 | 141,344 / 143,428 / 146,213 |
| Huit amas | 228,532 / 223,129 / 223,140 | 17 664,591 / 17 656,166 / 17 656,530 |
| Deux rangées | 13,704 / 13,864 / 13,561 | 942,638 / 943,312 / 943,153 |

Les compteurs détaillés des JSON sont l'autorité. Sur les amas, augmenter
s change les visites de moins de 0,05 %, pas leur régime. Les différences
de quelques pourcents en temps ne sont pas concluantes sur cette machine
partagée. Aucun s gagnant universel n'est choisi.

Le diagnostic suivant est désormais précis : 81,9/83,5/83,7 % des
subdivisions B des amas se produisent avant tout crédit. La contre-fixture
collinéaire et le certificat autonome du bloc frère proposés dans le
[contrat](../../docs/P0_FRONT_ET_CENSUS_Q2.md#contre-fixture-de-fragmentation-prématurée)
donnent une expérience suivante de coût constant par enfant, sans nouvelle
recherche depuis la racine. Cette variante n'est pas implémentée ici.

Le `terrain` est une nappe aléatoire mince, pas un capteur LiDAR. Les
rangées sont une famille adverse alignée, pas un cas général. Le corpus
LiDAR de l'auditeur A reste une qualification indépendante à raccorder.
Les capacités mémoire de vecteurs ne représentent pas un pic RSS/VRAM.

## Rejouer

Créer un build neuf à partir des sources épinglées dans les manifestes :

```bash
cmake -S morsehgp3D_v8 -B build/v8_replay_new -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/chemin/boost
cmake --build build/v8_replay_new --parallel 2
ctest --test-dir build/v8_replay_new --output-on-failure
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py run --probe build/v8_replay_new/mhgp8_wspd_q2_census_probe --output morsehgp3D_v8/receipts/front_q2_replay_new --sizes 8000 16000 32000 --families uniform terrain clusters rows --kmax 10 --s 8 10 12 --seeds 3 --modes samples --census-modes shared
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/front_q2_replay_new --summary
```

Pour les quatre bras à 8k, utiliser `--sizes 8000 --s 8 --modes pure samples
--census-modes pairwise shared`. Pour un nouveau build sanitizer, choisir
Clang, Debug et `-DMHGP8_SANITIZE=ON`, sans désactiver la détection de fuite.
Un reçu de sources courant ne qualifie pas rétroactivement un ancien
binaire : relecture historique avec ses octets épinglés. Les manifestes
conservent aussi le HEAD et l'état sale réel, les commits des auditeurs
pouvant avancer indépendamment pendant les captures gelées.
