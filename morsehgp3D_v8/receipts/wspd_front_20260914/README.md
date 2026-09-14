# Premier front WSPD réel — qualification et mesures locales

14 septembre 2026. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

## Périmètre

Premier producteur v8 de rectangles WSPD, sur les nœuds de son index
spatial global. La référence `Pure` couvre toutes les paires ;
`MidpointSamples` propose au plus Kmax sites par produit et certifie les
rejets séparément pour q2/q3/q4 avant séparation complète. Ni plan local,
ni copie/scan de facteur, ni catalogue de rectangles dans le producteur.
La [preuve et le contrat](../../docs/P0_FRONT_REEL.md) précisent les
identités d'index, les frontières strictes et la portée des rejets.

Convention : `box_gap_diameter_v1`. Les s8/10/12 sont ceux de la vraie
décomposition v8 ; ils ne désignent pas le même prédicat que le s v4.
Les recettes sont nouvelles et déterministes, sans transfert des temps
ou preuves de l'auditeur. Les points du nuage sont distincts.

**Ce n'est pas un calcul de tour.** Le callback compte et hache chaque
descripteur, sans développer ses paires. Il n'exécute aucun census,
aucune collecte de support/coquille, aucun catalogue ou parent FULL.
Le temps total paie génération, copie/unicité, index, front+callback,
validation et destructions. Le checksum synchrone fait partie du coût ;
`front_and_callback_ms` ne doit pas être appelé « front seul ».

## Qualification fonctionnelle

40 CTests passent dans chacun des builds neufs : GCC 13.3 Release et
Clang 18.1 ASan/UBSan Debug, sans désactiver LeakSanitizer. Les XML sont
`build/v8_front_20260914/ctest_full.xml` et
`build/v8_front_sanitize_20260914/ctest_full.xml` ; aucun test en échec
n'a été relancé jusqu'au vert. Le premier test ciblé sous sandbox a
échoué pour l'incompatibilité LSan/ptrace : son
[extrait observé](ENVIRONMENTAL_FAILURE.json) est conservé, puis le même
test et la suite complète ont passé hors sandbox autorisée.

La nouvelle porte C++ exécute 727 parcours sur petits nuages, 498148
contrôles et 235134 évaluations d'oracle multiprécision indépendant.
Elle vérifie chaque paire et voie, pas seulement leur masse : couverture
pure unique, sûreté des absences après filtre, permutation, frontières,
exceptions et sept contre-modèles. Les tests de reçus normal/−O ont
chacun 48 sorties réelles, 40 mutants, deux échecs capturés et refusés,
et deux contrôles du domaine physique des rangées.

Les frontières du nouveau front ne ferment pas automatiquement la
lacune signalée par B dans `classify_witness_block`, que ce front
n'appelle pas. Les autres questions de couverture de l'audit restent
distinctes des 40 tests passés dans ces builds.

## Campagnes

Capture principale mono : uniforme 3D, terrain mince et huit amas,
n=8000/16000/32000, Kmax10, s8/10/12, seed3, modes Pure/Samples,
une mesure par tuple (54 essais). Les répétitions statistiques et le
p95 du contrat G4 ne sont pas revendiqués. Les compteurs et masses
permettent de distinguer baisse du temps et baisse du travail.
`terrain` est un slab volumique uniforme mince, pas une captation LiDAR,
une surface occultée ou une superposition de scans. Aucun résultat LiDAR
n'est qualifié par ces quatre familles synthétiques.

Les 54 essais principaux et 18 essais de rangées parallèles sont clos :
**72/72 mesures**, deux campagnes. Les lecteurs normal/−O passent avec
sortie strictement identique. Lire le [résumé complet](SUMMARY.json), la
[validation des reçus](VALIDATION.json) et la [qualification](QUALIFICATION.json).
Les sources, binaires, caches et XML sont épinglés ; les deux builds ne
doivent plus être reconstruits. La machine locale déclare AMD EPYC 9V74,
8 CPU logiques ; chaque sonde utilise un seul thread. Les mesures ont
commencé après la fin des suites de tests.

À **32k/Kmax10/s8**, temps total en secondes et volumes en millions :

| Famille | Temps Pure | Temps Samples | Rectangles Pure → Samples | Paires restantes Samples q2 / q3 / q4 |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 4,871 | 37,375 | 56,796 → 20,910 | 17,325 / 51,489 / 46,695 |
| Terrain mince | 0,566 | 4,055 | 6,700 → 2,746 | 4,017 / 7,842 / 6,786 |
| Huit amas | 1,816 | 20,815 | 21,772 → 13,271 | 460,079 / 474,810 / 472,736 |
| Deux rangées | 0,049 | 0,204 | 0,526 → 0,279 | 160,365 / 256,328 / 256,278 |

Le filtre réduit bien les produits émis, mais **n'est pas gagnant en temps
de front** sur ces mesures. Uniforme32k/s8 paie 954257811 pas d'index,
1908515622 distances de boîtes et 619822196 tests H. Les millions de
recherches recommencées et de certificats évalués dominent l'économie de
descripteurs. Cela ne démontre pas que Pure gagne une fois le census payé :
ce coût aval reste précisément à mesurer.

À 32k, s8→10→12 augmente le temps Samples : uniforme 37,375→43,562→48,896 s,
terrain 4,055→4,940→5,709 s, amas 20,815→24,160→26,459 s.
Le résidu diminue, mais très peu sur les amas : q2 passe de 460,079 à
459,338 millions entre s8 et s12. Aucun s optimal pour la tour n'en découle.

## Croissance : front et résidu donnent deux réponses différentes

Plages des doublements 8k→16k et 16k→32k, réunissant s8/10/12, mode Samples :

| Famille | Produits visités | Temps total | Paires restantes q2 | Paires restantes q3/q4 |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | ×2,305–2,532 | ×2,278–2,613 | ×2,299–2,358 | ×2,276–2,607 |
| Terrain | ×2,142–2,176 | ×2,119–2,331 | ×2,012–2,129 | ×2,072–2,154 |
| Huit amas | ×2,727–3,108 | ×2,784–3,223 | **×3,927–3,943** | **×3,904–3,928** |
| Rangées | ×2,001–2,002 | ×2,033–2,201 | ×2,499–3,990 | **×3,990–3,996** |

La croissance mesurée du **front** est inférieure au carré sur ces
tailles ; cela n'établit aucune borne globale. Le résidu des amas est
presque quadratique, celui des rangées q3/q4 possède une minoration m²
démontrée. Le front des rangées reste rapide parce qu'il ne développe
pas ces centaines de millions de paires. Une expansion naïve déplacerait
le problème vers le census.

Les résidus Pure valent exactement n(n−1)/2 ; leur doublement vaut donc
4,000250 puis 4,000125, sans être pour autant superquadratique. Tous les
chronomètres sont conservés, dont amas/Pure/s12 : front ×3,9961 et total
×3,9641 au premier doublement. Un seul essai ne mesure pas la variance.

À 32k, les capacités publiées sont 192000 octets pour l'entrée,
960000 pour le propriétaire et 3926016 pour l'index. Elles ne représentent
pas un pic RSS, et ne comprennent pas un catalogue de sortie inexistant.

Pour relire les bruts sans les modifier :

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_front_matrix.py check morsehgp3D_v8/receipts/wspd_front_20260914 --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_front_matrix.py check morsehgp3D_v8/receipts/wspd_front_20260914
```

Le [script de fermeture](record.py) a produit les trois preuves une fois,
en refusant l'écrasement. Les commandes effectives, provenance et sorties
brutes/base64 sont conservées dans chaque campagne. Les tests du lecteur
ne remplacent pas le juge géométrique indépendant du front.

## Ce que cela change et ce que cela ne ferme pas

L'ancien terme de préparation par facteur est absent du nouveau front.
Cependant chaque produit échantillonné repart encore de la racine pour
une descente unique : coût O(T(D+K)+R), pas une borne sous-quadratique
sur T ou sur le résidu. Les crédits partiels sont perdus lors des
subdivisions ; seuls les masques de rejets sont hérités.

Les deux rangées parallèles réfutent une résolution universelle par
témoins ponctuels W3/W4 : à ces tailles, même tous les sites laissent
m² candidates transversales. Cette masse n'est pas la taille de sortie
utile. Il faut garder les produits compacts et traiter l'aval par groupes.

Prochain raccord : census q2 global directement sur les nœuds du front,
sans recréer un plan par rectangle, puis certificats collectifs q3/q4.
Les contrats de toute la tour 50k sur G4 à 1 s/100 ms et les dizaines
de millions de points restent ouverts. Aucun gain GPU n'est mesuré.
