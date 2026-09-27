# Parents FULL parallèles et consommation q3/q4 par vagues

27 septembre 2026, suite de `fd1a2c7ee`, prototypes séparés du moteur.
Profil entier 18 bits/grille 1 mm, hors registre, `not_claimed`.
Aucun nouveau résultat GPU, aucune utilisation GCP dans cette tranche.

## Ce qui change pour le développement

Les deux formats proposés précédemment deviennent des calculs exécutables :
les parents FULL sont effectivement validés et écrits par plusieurs CPU ;
l'arène q3/q4 est consommée sans tableau global portant toutes les paires.
Ce sont des étapes vers le port GPU, pas une nouvelle tour mesurée à 100 ms.

Le point essentiel pour q3/q4 : **la sortie S reste exactement la même**.
Le Pool évite des tests ponctuels et leurs tableaux temporaires ; il ne
supprime ni les survivantes envoyées à S3/S4, ni les nœuds de la tour.
Le travail aval et la croissance résiduelle ne disparaissent donc pas.

## FULL : une vraie voie multi-CPU, avec repli exact

Le [prototype parallèle](../audits/b_full_parallel_parent_20260927/README.md)
travaille sur le draft existant. Quand aucune action n'est une continuation,
chaque action crée un nœud, et chaque parent ne peut être consommé qu'une
fois. Une réduction du premier ordinal par parent remplace le tri des
incidences ; les validations et l'écriture finale occupent des plages
disjointes. Les erreurs sont réduites dans l'ordre canonique, pas dans
l'ordre d'arrivée des threads. L'émission commence seulement après admission.

Une continuation déclenche le repli général. Le
[nouveau résultat mathématique et son gate natif](../audits/b_full_continuation_origin_20260927/README.md)
expliquent ce choix : un catalogue entièrement régulier suffit à exclure
les continuations, même sur un plateau de rayons égaux ; cette condition
n'est pas nécessaire et ne doit pas être supposée pour tout LiDAR.
Le critère exact est un groupe contribuant dont l'union des racines a
cardinal un. Le cas ABCZ ajoute une contribution sans changer la topologie :
le mutant supprimant les continuations conserve les nœuds et parents,
mais perd un objet FULL réel. La détection et le repli restent obligatoires.

Qualification parallèle : 14 commandes closes Release et ASan/UBSan/LSan,
372 entrées et 2 232 comparaisons par build, trois mutants chacun ;
quatre workers réellement actifs sur les parents. ThreadSanitizer passe
le même corpus dans une capture séparée de quatre commandes. Les lecteurs
normal et `-O` sont rejugés indépendamment. Le gate d'origine des
continuations ferme huit commandes, 32 chaînes natives/116 ordres et
le mutant de perte de contribution dans les deux builds.

Limites conservées : certaines initialisations et copies sont scalaires,
les grandes multifusions ne sont pas subdivisées intérieurement et les
threads sont recréés par phase. O(B+A+P+C) décrit les opérations logiques,
pas une borne uniforme sur les retries du CAS faible pour tous les drafts
invalides. Scratch principal 16A octets, plus auxiliaires/piles des workers
et sorties ; ce n'est pas le RSS ni un budget GPU. La factory publique de
banque avec alias mutable reste un défaut distinct, non réparé ici.

## Mesures sur de vrais drafts, pas seulement des exemples fabriqués

La [capture distincte](../receipts/full_parallel_real_drafts_20260927/README.md)
qualifie d'abord le harnais en Release/sanitizers, puis mesure quatre vraies
chaînes CPU. Chaque draft est encodé par le natif, le prototype W1 et le
prototype W4, trois fois avec rotation de leur ordre. Toutes les sorties
sont comparées à la forêt native complète. Les trois digests et les tailles
restent ceux de la capture précédente, sans continuation dans ces vingt ordres.

Sommes des médianes par K, en millisecondes — **pas un temps mur FULL** :

| Entrée K5/s8 | Natif | Prototype W1 | Prototype W4 |
|---|---:|---:|---:|
| 08/000000 sans sol, 39 885 sites | 163,973 | 168,825 | 120,793 |
| Uniforme 8 000 | 65,181 | 67,187 | 46,378 |
| Uniforme 16 000 | 147,045 | 152,448 | 109,568 |
| Uniforme 32 000 | 315,085 | 317,543 | 233,550 |

Les allocations, copies et écritures de chaque encodeur sont payées ;
les comparaisons et la destruction des résultats sont hors de ses chronos.
W4 est favorable sur ces quatre observations, mais les trois répétitions
varient fortement. Hôte local partagé, pas une preuve de gain stable ni un
chrono G4. Les copies du harnais et la coexistence de plusieurs sorties
interdisent de présenter le RSS ou la chaîne instrumentée comme coût du
futur moteur intégré. W1 ne justifie toujours pas de remplacer le natif.

Pour le passage 8k→16k→32k uniforme, les actions valent
629 404 / 1 301 794 / 2 660 312 et les contributions
372 698 / 770 002 / 1 574 290 : doublements voisins de ×2, pas ×4.
Cette observation concerne les objets d'encodage de ce régime ; elle ne
prouve pas la croissance sous-quadratique du générateur LiDAR ou des amas.
Aucune nouvelle coupe LiDAR ni scène supplémentaire n'est mesurée ici.

## q3/q4 : consommation complète à mémoire de vague

Le [consommateur qualifié](../audits/b_q34_arena_waves_20260927/README.md)
prépare l'arène une fois et garde les rectangles non préparés comme des
segments de repli complets. Les préfixes sont par bande ou rectangle,
jamais par paire. Une vague de capacité Q peut être aussi petite qu'un
slot : elle doit épuiser tout E, sans quota de recherche. Les crédits
décident séparément les voies q3/q4 ; le filtre ponctuel repart de zéro.

Seules les S survivantes sont conservées puis triées par leur ordinal
cartésien original avant S3. Le coût de ce tri reste présent. Les masses
logiques P et physiques E sont distinctes, ainsi que leurs deux voies.
Le stockage est O(R+F+C+D+Q+S), index partagé à part ; le pic des tableaux
du curseur comprend les réallocations et la coexistence keyedS/sortie S,
mais pas le pic de construction de l'arène ni le RSS du processus.

Les [15 commandes closes](../receipts/q34_arena_waves_20260927/README.md)
passent Release et ASan/UBSan/LSan : 175 lots, 1 050 consommations,
355 632 requêtes et 195 948 survivantes comparées exactement au batch
natif, ordre et masques inclus. Capacités 1/2/7/17/64/>E, K1/2/3/5/10,
s8/10/12, fallbacks et permutations sont exercés. Les 139 reprises au
point pré-commit n'exécutent pas deux fois la géométrie. Une exception
pendant le remplissage ou la finalisation engagée invalide le curseur ;
seule la reprise pré-commit est qualifiée. L'injection simule un échec à
ce point, pas une panne réelle de l'allocateur système.

Le contre-audit a fait corriger avant gel deux parcours hors limites
sur S vide et le compte du pic lors d'une réallocation. La capture inclut
216 consommations E=0 et six E>0/S=0, avec `finish()` réellement appelé.
Trois mutants par build sont tués : voies réouvertes, fallback omis et
ordinal faux. Les deux premiers échouent sur les comptes physiques,
le dernier sur un ordinal dupliqué ; ne pas les rebaptiser trois divergences
géométriques. Ce lot est une gate CPU S2, sans mesure LiDAR lourde ni FULL.

La [contrelecture indépendante](../audits/b_parallel_and_waves_review_20260927/README.md)
donne les invariants d'écriture, les limites de complexité et le raccord
GPU minimal. Elle ne transforme pas ces gates en qualification CUDA.

## Décisions de port vers G4

1. Garder l'encodeur natif comme référence et les continuations dans le
   contrat. Porter la réduction/validation/scatter sur tableaux explicites,
   puis vérifier les mêmes objets et premiers refus avant toute activation.
2. Porter les segments q34, fallbacks compris, vers un consommateur GPU
   dont le scratch est une vague Q, non un tableau de masse P ou E.
   Restaurer l'ordre natif des S survivantes avant S3 et garder P logique
   distinct des E requêtes physiques.
3. Ne pas activer naïvement « préparation CPU collective + S2 GPU » comme
   gain acquis. La préparation CPU précédente prenait environ 277 ms W4
   localement ; ce chiffre ne se transpose pas au CPU G4, mais aucun gain
   net n'est démontré. Son transfert au GPU ou sa réduction restent requis.
4. Mesurer ensuite la chaîne entière, allocations, transports et sortie
   explicite inclus, sur les trames entières et les coupes capteur prévues.
   Le résidu amas presque quadratique n'est pas corrigé par ces formats.

Les reçus historiques et les builds clos ne sont pas réécrits. Les échecs
de compilation initiaux restent séparés des captures corrigées. Aucun
registre de qualification, moteur ou protocole G4 n'est modifié ici.

Contrôle de publication : les avertissements d'espacement de `git diff
--check` sont limités aux sorties brutes de compilateurs/CMake et aux lignes
vides finales de sources déjà gelées (harnais de vrais drafts et snapshot
de l'échec r1). Ils sont conservés pour ne pas invalider les hashes des
captures ; les nouveaux textes de synthèse passent le contrôle documentaire.
