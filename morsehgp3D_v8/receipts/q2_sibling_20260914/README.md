# Certificat frère q2 — preuves et mesures propres à la neuvième tranche

14 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
GCP non utilisé ; aucune tour FULL ni qualification GPU.

## Objet mesuré

Même front WSPD q2, mêmes candidates et mêmes supports complets, avec
ou sans certificat autonome du frère. Le temps total paie génération,
propriétaire, index, front, census, collecte et callback canonique, puis
destructions. Les compteurs séparent propositions/tests du frère et
visites Z : un rejet supplémentaire n'est pas nécessairement un gain.
Voir le [contrat et la preuve](../../docs/P0_CERTIFICAT_FRERE_Q2.md).

Le chemin par défaut reste Disabled. Le schéma sonde v1 est conservé
sans option finale ; les captures avec `none|sibling` utilisent v2.
Le runner refuse Pairwise+sibling, compare les digests et les populations
du front, et ferme ses reçus par hashes de sources et de binaire.

## Qualification locale

Builds de reprise : `build/v8_sibling_r2_20260914`, GCC 13.3 Release,
et `build/v8_sibling_sanitize_r2_20260914`, Clang 18.1.3 Debug,
AddressSanitizer/UndefinedBehaviorSanitizer et détection des fuites.
Boost 1.83 vient de `build/v7_boost_gate/extracted/usr` ; il n'est
utilisé que par les juges indépendants bornés, pas le produit.

Le nouveau juge confronte l'ensemble des supports, clés, intérieurs et
coquilles à un scalaire exhaustif indépendant, K1/2/5/10, s8/10/12,
deux fronts et deux modes. Il couvre réflexions, permutations, frontières,
défaut/Disabled, saturation après crédit et exceptions du callback.
La fixture `{0,5,10,11}` à K2 interdit que le mode autonome abaisse
silencieusement son seuil en utilisant le compte hérité. La fixture
`{(1000,0,0),(0,0,0),(0,1,0)}` exige un vrai test de frère à Hmin=0
sans rejet. Les mutants modèles ne sont pas des mutations du code produit.

Le résultat exécuté du juge r2 est 1 449 appels, 64 725 supports,
485 775 contrôles ; 3 417 tâches rejetées par frère, dont 10 après crédit.
Les lecteurs de campagne conservent les 128 appels v1 et ajoutent
64 appels v2, avec mutants de schéma, commande et compteurs.
Les **44 CTests passent en Release et Clang ASan/UBSan**. Les captures
[Release](qualification/release_qlv_ystf/RESULT.json) et
[sanitizer](qualification/sanitize_8ifrldn7/RESULT.json) conservent chacune
20 commandes, les logs bruts, le cache CMake et les hashes de fermeture.
Les [lecteurs normal/−O](qualification/readers_1fohytt0/RESULT.json)
passent avec sorties identiques, 32 mesures et 32 configurations.
Le runner de preuve ajoute six contrôles déterministes de lancement,
interruption et arrêt de session aux 41 contrôles historiques ; les
deux modes Python passent. Aucun contrôle n'est porté par `assert`.

Les snapshots `qualification/record.py.snapshot` et
`qualification/record_initial.py.snapshot` conservent les scripts utilisés,
sans réécrire les essais initiaux. Pour reproduire le pilote de qualification,
le placer au chemin `build/v8_sibling_r2_20260914/record.py`, construire
dans les répertoires neufs déclarés, puis appeler `release`, `sanitize`
ou `readers`. `--execution-context` est une déclaration enregistrée,
pas une modification de permissions. Le script ne reconstruit aucun
binaire et crée un nouveau répertoire pour chaque tentative. Ne jamais
écraser les builds maintenant épinglés pour une nouvelle révision.

## Mesures : un gain de régime, pas une solution globale

Deux campagnes séquentielles à l'intérieur de chacune :
[paired_8k](paired_8k/MANIFEST.json), quatre familles, s8/10/12,
none/sibling (24 mesures), et [growth_s8](growth_s8/MANIFEST.json),
n16k/32k, quatre familles, sibling, s8 (8 mesures). Toutes sont
Samples/Shared, Kmax10, seed3, un thread par processus. Les coordonnées
proviennent des mêmes recettes publiées ; `terrain` est un slab
synthétique, pas un scan LiDAR. La comparaison s10/12 nouvelle est
limitée à 8k ; aucun nouveau temps de baseline 16k/32k n'est revendiqué.

Les deux campagnes et certaines qualifications ont coexisté sur l'hôte
partagé ; les audits indépendants ont aussi travaillé. Les temps sont
donc exploratoires, sans intervalle statistique ni isolation CPU. Les
différences de quelques pourcents ne sont pas une attribution causale
robuste ; les compteurs et les sorties sont, eux, déterministes.

Temps total du composant en secondes, s8, nouvelle révision seulement :

| Famille | none 8k | sibling 8k | sibling 16k | sibling 32k |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 4,828 | 4,743 | 11,970 | 28,762 |
| Terrain synthétique | 0,912 | 0,979 | 2,079 | 4,431 |
| Amas | 13,248 | 13,184 | 52,845 | 214,901 |
| Deux rangées | 1,494 | 0,241 | 0,481 | 0,969 |

La comparaison appariée montre le gain net sur les rangées : environ
×6,20 en temps à 8k, et 97,09→8,66 millions de visites Z, avec
0,287 million de nouveaux tests de frère. À 16k/32k, les visites valent
18,96/40,34 millions, plus 0,701/1,530 million de tests du frère. Les
supports rendus sont 123 860/247 860/495 860 ; toutes les populations
et digests concordent entre variantes et séparations comparées.

À l'inverse, uniforme/terrain8k ne présentent aucun frère de cardinal
suffisant : zéro test géométrique, mais les propositions restent payées.
Sur amas8k, le compteur Z baisse de 973,18 à 922,28 millions, avec
4,232 millions de bornes supplémentaires et 45,61 millions de propositions.
Cela ne change pas l'ordre de croissance observé :

| Famille, sibling/s8 | Visites Z + tests frère : 8k→16k→32k | Facteurs de croissance |
| --- | --- | --- |
| Uniforme | 200,53 M → 483,36 M → 1 165,53 M | ×2,410 ; ×2,411 |
| Terrain synthétique | 30,05 M → 63,49 M → 141,34 M | ×2,113 ; ×2,226 |
| Amas | 926,51 M → 3 976,43 M → 16 764,41 M | **×4,292 ; ×4,216** |
| Deux rangées | 8,95 M → 19,67 M → 41,87 M | ×2,198 ; ×2,129 |

Cette somme est un indicateur d'évaluations géométriques, pas un nombre
d'instructions équivalentes ni tout le travail. Sur amas, les tâches
46,90→183,43→731,80 millions et les propositions
45,61→180,03→723,57 millions restent presque quadratiques. Les supports
245 733→520 208→1 088 696 sont pourtant proches du linéaire.
La collecte reste payée et le front ne change pas entre variantes.
Le résultat **réfute une qualification sous-quadratique générale**.

s10/12 ne répare pas les amas à 8k : environ 926,56/927,15 millions
d'évaluations avec le frère, contre 926,51 à s8. Sur les rangées,
8,95/9,09/9,05 millions à s8/10/12 ; l'amélioration principale vient
du certificat, pas de s. Le mode reste optionnel, Disabled par défaut.
P0, les parents FULL, q3/q4, multi-CPU/GPU, 50k/G4 et massif restent ouverts.

## Essais en échec, conservés

`qualification/release_sgb63byi` : 43/44 CTests passent, mais la gate
historique de campagne révèle la perte d'un stdout à l'interruption.
L'enfant avait écrit puis signalé son parent avant que Popen rende son
objet au collecteur. Une reproduction déterministe a confirmé cette
fenêtre ; la gate n'a pas été affaiblie. Le correctif diffère le handler
Python pendant le lancement, puis rejoue l'interruption dans le bloc
qui possède le processus et collecte ses pipes. Les handlers sont aussi
restaurés si Popen échoue. Le masque OS de l'enfant n'est pas modifié.

`qualification/sanitize_ov74mtzz` : échec LeakSanitizer sous le traçage
du sandbox, pas un passage ASan/UBSan. Reprise hors sandbox requise,
sans désactiver les fuites. Les tests du frère ont encore été renforcés
pendant ces essais initiaux : leurs pins de fermeture ne sont pas une
qualification des sources finales. Les fichiers bruts et statuts restent
conservés ; seuls les nouveaux builds et captures r2 feront autorité.

Les sources, bibliothèques et résultats des tranches précédentes restent
des témoins séparés. Les chiffres LiDAR et la variante K−c du prototype
indépendant A ne sont pas transférés à ces qualifications produit.
