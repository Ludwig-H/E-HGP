# Harnais réel du consommateur q34 : périmètres chronométrés

27 septembre 2026, base `70168cc3b`, audit CPU isolé, grille 1 mm,
`not_claimed`. Aucun changement du moteur, de l'arène ou du consommateur
gelés ; pas de GCP. Pas de nouvelle représentation ou variante.

## Entrée et ordre de comparaison

Lire ou générer une entrée, préparer un index natif et collecter une fois
le front `MidpointSamples`, K5/s8 pour les grands cas. Le même vecteur de
rectangles, ordre et masques compris, est ensuite transmis aux deux bras.
Son hash est publié. Le propriétaire conserve tous les points, sans quota
ni échantillonnage ; le fichier sans-sol est un masque de trame entière
déjà figé avant les coupes capteur, pas une nouvelle segmentation.

Ordre fixe : **candidat puis natif**, une observation. Préparation arène,
consommateur et natif sont tous à un worker. Q4096 est une capacité mémoire,
pas une configuration GPU ou un paramètre de recherche. Cet ordre peut
favoriser le cache du second bras ; un ratio n'est pas une preuve de gain
stable. Pas de moyenner des familles ou des scènes différentes.

## Phases

1. Entrée, index et front : trois chronos partagés, plus un vrai intervalle
   mur d'entrée au départ du candidat, qui inclut hashes et instrumentation.
2. `Prepared::build` : copie des rectangles, filtre rectangle, préparation
   Pool/arène et segments fallback. Tout est payé dans le candidat.
3. Construction du `Cursor`, vagues jusqu'à EOF, compaction et allocations.
4. `finish()` : tri des seules S survivantes et conversion native, ensemble
   car le consommateur est gelé. Aucun tri/copie de la référence n'y entre.
5. Destruction effective du curseur et de `Prepared`. Le résultat S et ses
   masques rectangles sont conservés pour la comparaison et une remise à
   la suite native. Aucun autre shared_ptr ne retient `Prepared`.
6. Référence `run_q34_filter_batch_cpu` : chrono propre, incluant son propre
   filtre rectangle et ses temporaires internes. Front/index sont partagés
   et exclus des deux chronos opérateur.
7. Comparaison exacte hors chronos opérateur ; destruction des deux sorties,
   du vecteur front, de l'index, du propriétaire et de l'entrée, mesurée à part.

`candidate_total` est l'intervalle réel des phases 2–5, y compris lectures
RSS, stockage des compteurs et scan diagnostique de S3/S4. La différence
avec la somme des quatre sous-phases est publiée comme
`candidate_observation_overhead`, pas soustraite silencieusement.
`candidate_input_to_S2` est un vrai chrono continu depuis l'entrée, pas la
somme de durées choisies. `experiment_total` contient aussi le natif, le
juge et le nettoyage final ; il ne représente pas le coût du candidat.
L'écriture JSON finale est hors ce chrono ; les petites maps de résultats
et leur destruction ne sont pas un poste algorithmique qualifié.

## Travail et mémoire

Publier Praw avant filtre rectangle, P/P3/P4 après ce filtre, E/E3/E4 après
Pool, S/S3/S4, requêtes physiques, rejets par voie, visites, F, classes,
bandes, segments fallback, Q, vagues, chunks et comparaisons de tri.
Vérifier notamment `S3=P3−rejets_natif3=E3−rejets_ponctuels3`, et q4 de
même ; les unions ne s'obtiennent jamais en ajoutant les deux voies.

Capacités distinctes : entrée, index/nuage partagé, vecteur front appelant,
`Prepared` retenu (qui inclut l'arène), arène seule pour ventilation, pic
des tableaux du curseur, sortie native retenue. Ne pas additionner arène
et `Prepared`, ni sortie native et pic curseur comme s'ils étaient disjoints.
Le pic curseur inclut réallocation ancienne+nouvelle et coexistence
keyedS/nativeS ; il ne mesure pas le pic de construction de `Prepared`.

RSS courant à plusieurs frontières et `ru_maxrss` monotone du **processus
entier**, Linux uniquement. Le second bras voit la sortie du premier encore
vivante. Le high-water inclut démarrage/allocateur/runtime et les phases
antérieures : ni son delta ni le RSS à une frontière ne donnent un pic
isolé de phase. Le nettoyage peut ne pas rendre les pages immédiatement au
système ; cela ne signifie pas que les objets sont encore possédés.

## Qualification puis mesures séparées

`run.py` compile deux nouveaux binaires et exécute de nouveau la gate de 175 lots
gelée, ses trois mutations, puis trois petits appels du nouveau harnais
(uniforme/terrain/amas 64), en Release et Clang ASan/UBSan/LSan. L'archive
générateur est réutilisée immuable et hachée ; aucune qualification n'est
héritée du seul fait de la réutiliser.

`measure.py` refuse de démarrer avant relecture LIVE PASS de cette capture.
Il réutilise le binaire Release qualifié sans le recompiler. Première
campagne : ng00 entière, puis uniforme 8k/16k/32k. Les six demi/quarts ng00
forment une deuxième capture, lancée après décision de coût et désormais
close PASS comme la première. Le lecteur
vérifie les sept partitions exactes par rapport au masque entier figé et
les IDs originaux ; n observé est publié et les ratios ne supposent pas
des morceaux de taille exactement moitié/quart.

Ni segmentation, catalogue, hiérarchies FULL, GPU, ni contrat 100 ms ne sont
mesurés par ce harnais. Il doit donner un verdict sur le coût total de S2
et le travail résiduel, pas requalifier toute la tour.
