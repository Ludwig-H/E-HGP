# Preuves propres — détachement du census q2

15 septembre 2026. Tranche16 qualifiée dans son périmètre CPU ; aucun résultat
historique n'est repris comme test de cette révision. GCP non utilisé.
Les mesures concernent une ancre sélectionnée, pas une tour FULL.

## Préflight conservé

Build exploratoire neuf `build/v8_census_split_preflight_20260915`, GCC13.3,
Release sans tests CMake : configuration et sonde passent. Le premier
essai amas8k/K10/s8, quantum256, W4, file8 conserve les41 061 transitions
de la référence, 1 056 candidates toutes rejetées ;72 fragments terminés,
71 dons, quatre slots ayant du travail. C'est un diagnostic ponctuel,
sans gel ni promotion de chronométrage. L'absence de support de cette
ancre ne remplace pas les fixtures à payload positif.

Le premier build Clang ASan/UBSan a échoué avant tests : la gate de
détachement gardait `constexpr unlimited` inutilisé (ligne76,
`-Werror,-Wunused-const-variable`). Suppression de cette seule constante
avant gel et reconstruction ; aucun changement du moteur. Le premier
lien Release avait encore la gate à deux scénarios d'injection mémoire ;
le build final doit contenir les trois scénarios, dont le crédit acquis1.
Le même diagnostic a été reproduit dans la première compilation TSan et
dans une relance ASan partie avant application effective du patch.

Après ce correctif, la gate de détachement ne se lie pas avec Clang TSan :
ses substitutions globales de `operator new/delete`, nécessaires à
l'injection exhaustive des pannes, entrent en conflit avec celles de TSan.
Cet échec de lien est conservé, pas contourné. La porte TSan de cette
tranche est donc celle du répartiteur, qui exerce le détachement simultané
sans substituer l'allocateur ; la gate d'injection est qualifiée séparément
en Release et ASan/UBSan. Ce ne sont pas deux gates TSan réussies.

La capture `tsan_gva7p_55` conserve ensuite un défaut de collecteur :
le binaire termine à0, sans stderr, et annonce `status=passed`, alors que
le premier script attendait `pass`. Le reçu reste en échec ; le script
est corrigé avant une nouvelle capture séparée, sans changement du binaire.

## Qualification fermée

- `qualification_u3qvrjyo` :66 CTests Release PASS, plus les deux gates C++.
- `qualification_ulbtho0b` :66 CTests Clang ASan/UBSan PASS, fuites activées,
  plus les deux gates C++. La gate d'injection finale porte trois scénarios.
- `tsan_jxjhkb5n` : gate Clang ThreadSanitizer du répartiteur PASS.
- `matrix_ocf7f2oa` :144 mesures d'une ancre sélectionnée, quatre familles,
  n8k/16k/32k, K5/10, s8/10/12, W1/4, quantum256, file8, seed3.
- `full_q2_regression` :12 nouvelles mesures de la chaîne q2 existante,
  quatre familles, n8k/16k/32k, K10/s8, W4,16 jobs/worker, Pool64.

Les builds Release/ASan/TSan `v8_census_split*20260915` sont épinglés.
Le build exploratoire préflight reste séparé. Les manifests conservent
commandes, environnement, affinité, hashes de sources/binaire/cache et
sorties brutes ; fermeture avant/après vérifiée. Le runner est un port
explicite du protocole de tranche15, avec schéma et bilans propres.

Gate de détachement :302 scénarios,1 458 paires d'oracle,85 416 examens
de sites,732 supports,1 679 détachements,149 crédits importés positifs,
150 imports en phase différée,139 donneurs Emit,1 148 détachements récursifs,
neuf allocations fautives injectées dans trois états, deux vrais threads.
Le parent reste réutilisable après chaque échec d'allocation.

Gate du répartiteur :2 304 appels contre128 références récursives et128
reprises mono,3 456 supports, coquille maximale30. Dons, callbacks sur
plusieurs slots et activité simultanée sont non vacuants. Quatre erreurs
callbacks et14 entrées invalides sont rejetées. L'échec partiel de lancement
n'est pas injecté dans ce raccord spécifique ; le lanceur partagé est testé
séparément dans la suite existante et la clôture a été contre-lue.

Les portes de lecture normal/−O testent chacune24 captures réelles,
12 appariements,1 993 mutants et21 CLI invalides. L'analyse reproductible
est dans `analyze.py`, sa capture normal/−O dans `analysis_*/`.

## Résultats et limites

Les72 appariements W1/W4 conservent36 compteurs géométriques, quatre types
de transitions, leur total et le digest des payloads. Sur les72 cas W4,
46 comportent des dons mais **seuls23 font travailler plusieurs workers**.
18 sorties sont non vides ;54 sont vides. Onze cas combinent une sortie
non vide et plusieurs workers actifs, sans preuve que plusieurs slots
émettent dans ces mesures (cette preuve relève des petites gates).

Le temps W1/W4 ne montre pas de gain général :8 comparaisons sur72 sont
ponctuellement favorables, ratio médian0,0738 et maximum1,756. Le maximum
concerne amas32k/K10/s10 avec sortie vide. Une seule répétition, référence
avant parallèle, qualifications et autres mesures concurrentes : **aucune
qualification de vitesse**. Le coût d'une nouvelle équipe domine souvent
celui de l'ancre. Le raccord futur doit réutiliser des workers persistants.

Capacités observées :6 272–6 368 octets par fragment ;64 octets de pointeurs
de file. Ce n'est pas la mémoire totale simultanée. Le trafic de paires
transférées atteint3,006 fois la masse candidate initiale, parce qu'une
branche peut être subdivisée puis ses descendants transférés.

Les12 mesures de q2 complet sont identiques à celles de tranche15 sur
les20 champs discrets du protocole et23 compteurs Pool. Le nouveau
répartiteur intérieur n'y est **pas** raccordé. Les48 ratios de croissance
des six postes principaux restent sous3 ; exemples des visites census :

| Famille | 8k →16k | 16k →32k |
| --- | ---: | ---: |
| Uniforme | ×2,406 | ×2,421 |
| Terrain | ×2,091 | ×2,223 |
| Amas | ×2,958 | ×2,701 |
| Rangées | ×2,070 | ×2,067 |

Ce constat mesuré n'est pas une borne générale. L'analyse publie aussi
tous les dépassements de×4 des racines sélectionnées :105 ratios, dont
sept sur les visites census. Exemple amas/K10/s8 :39 022→11→95 629 visites.
Le choix de racine change avec n : interpréter ce saut comme une croissance
globale ou le masquer serait incorrect. Le détachement n'ajoute qu'un
nombre d'exports borné par les divisions, sans diminuer ce travail géométrique.

Aucun contrat FULL/50k/G4/100ms ni plusieurs dizaines de millions de points
qualifié. Les contrelectures B restent indépendantes. GCP non utilisé.
