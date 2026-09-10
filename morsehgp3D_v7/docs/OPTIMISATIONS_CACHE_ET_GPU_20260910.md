# Cache exact, résidence et préparation GPU

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Cette étape ne livre ni archive industrielle ni contrat 50k/1s.

## Changements intégrés au constructeur FULL

Le [cache exact de facettes](../receipts/ball_resolver_residence_20260910/README.md)
évite une nouvelle MEB lorsqu'une même facette a déjà été résolue. La clé
contient tous ses indices géométriques triés ; le hash choisit seulement une
case. Une collision évince, sans alias ni rejet géométrique. Tout ancien token
est normalisé vers sa composante pré-lot courante. Après fermeture du lot,
I union U peut être semée lorsque cette population a exactement K sites :
elle est alors l'unique facette de cardinal K de cette population fermée.

La capacité est une puissance de deux entre 16n et 32n cases, de 48 octets
chacune dans l'ABI testée. Une allocation impossible désactive ce cache
facultatif ; elle ne tronque pas le travail. Il est vidé entre ordres et libéré
avant la copie de banque. Les compteurs de capacité restent des mesures
d'allocation, pas des octets encore vivants après cette libération.

Cette table n'est pas un catalogue de toutes les facettes. Recherche et
remplacement prennent O(K) au pire, indépendamment des collisions ; initialiser
ou vider prend O(n). Cela ne borne pas le nombre de demandes géométriques.
Le compromis massif reste important : 24 Mio à 32k, 48 Mio à 50k,
mais 24 Gio à 30 millions dans ce layout. Aucun résultat massif n'en découle.

Deux changements de [résidence](../receipts/full_tower_residence_20260910/README.md)
s'ajoutent : libérer les index/histoires morts avant la banque finale, et
compter puis réserver exactement les arènes du journal. Aucun changement
des identités, dates, contributions ou cartes verticales, ni du schéma v2.
À n800, la capacité de sortie baisse de 23,1 %, mais le pic des allocations
demandées seulement de 3,0 %. Les brouillons de construction restent dominants.
Ces valeurs ne sont pas des économies de RSS à additionner.

Les quatre gates du moteur combiné passent O2 et ASan/UBSan. La tour juge
28 nuages, 112 ordres, 2 508 coupes et 45 948 images verticales, soit
170 320 contrôles. Le journal réserve moins souvent : ses 20 points
d'allocation restants sont tous injectés, contre 34 auparavant. Six mutants
causaux sont réfutés, dont croissance et ancre inerte dans les lots groupés.
L'angle mort signalé par l'auditeur est donc traité sur les nouveaux headers,
pas seulement par reprise d'un ancien reçu.

Sur n1000 apparié, MEB : 1 174 515 → 583 337 ; supports exacts :
97 376 638 → 52 168 577 ; capacités de sortie, verticale comprise :
74 448 896 → 55 758 656 octets. Le digest reste identique. Les timings ont
été pris sur une machine partagée et le pic RSS ne baisse pas dans cette
paire : seuls les gains physiques décrits sont attribués à ce delta.

## CUDA strict et frontière du port

Le [paquet NVCC local](../receipts/nvcc_strict_host_20260910/README.md)
qualifie la compilation **et le lien**, pas l'exécution GPU. Le prétraitement
de la source originale et la compilation finale conservent
`-Wall -Wextra -Wpedantic -Werror`. Seul le prétraitement intermédiaire du
fichier généré par NVCC désactive le diagnostic de ses directives de ligne.
Les inclusions du stub sont réellement développées ; le fichier brut n'est
pas traité à tort comme déjà prétraité. VLA et macro variadique GNU sont
toujours rejetées. Voir les phases documentées par
[NVIDIA](https://docs.nvidia.com/cuda/archive/12.9.1/cuda-compiler-driver-nvcc/index.html)
et les options de prétraitement de
[GCC](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html).

`bench/nvcc_strict_host.py` est explicitement lié à `/usr/bin/g++`.
CMake le copie dans le build CUDA lorsque cet hôte GNU est sélectionné et
qu'aucun hôte CUDA explicite n'est fourni. Il ne remplace pas silencieusement
un compilateur choisi par l'appelant. Le worker G4 vérifie son hash, le copie
exécutable hors du snapshot immuable et conserve le contrôleur et les deux
scripts d'arrêt/démarrage précédemment qualifiés.

Le [batch WSPD privé](../receipts/wspd_device_batch_20260910/README.md)
est une primitive distincte : chaque requête porte un rectangle, ses lanes,
les trois seuils et le choix des coins. Elle conserve les exclusions A union B,
les crédits par sous-arbre, l'ordre DFS et les compteurs du scalaire.
O2/SAN : 74 313 contrôles, 73 935 lignes et 37 rejets ; compilation CUDA
réussie, mais aucune exécution device WSPD dans ce reçu. Elle n'est pas
encore raccordée au générateur nominal.

Le front hôte optionnel traite des lots bornés de requêtes et rejoue les
décisions dans l'ordre original. Son fournisseur doit être lié à l'index
immuable du front avant toute requête : des NodeRef de même plage ne suffisent
pas. Le défaut scalaire du générateur reste inchangé. Aucun gain GPU de tour
ne peut être déduit de cette préparation. Son [reçu courant](../receipts/witness_front_20260910/README.md)
porte 431 010 contrôles, 1 728 configurations et 15 rejets O2/SAN.
La [campagne G4 terminée](RESULTATS_TOUR_CACHE_G4_20260910.md) mesure la route
census nominale, pas ce batch WSPD : environ 419 s K1..10 et 33,6 s K1..5.

## Points encore ouverts

La [gate des quatre blocs nommés](../receipts/full_ball_named_blocks_20260910/README.md)
est préparée et testée sur de petits catalogues, mais **NOT_EXECUTED_50K**.
Elle doit observer les racines pré-lot 1/2/2 et l'ancre K10 sur les clés de
boules épinglées. Une sonde 50k réussie ou un digest CPU/GPU égal ne remplace
pas ce contrôle nommé.

Les census globaux, les brouillons de journaux, la taille de sortie, la limite
de coquille du format et les identifiants de candidats restent des contraintes
du passage massif. Aucun test à plusieurs dizaines de millions n'est promu
sur la base d'une projection linéaire de quelques tailles. La
[borne de sortie](CROISSANCE_ET_BORNE_DE_SORTIE.md) interdit une garantie FULL
explicite universellement sous-quadratique ; les régimes mesurés restent nommés.

La [contrelecture de l'auditeur](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md)
établit désormais la séparation statique sous census complet : résoudre les
facettes vers une clé admise à l'ordre K, puis lire sa racine à la coupe
pré-lot. Toute cible est née strictement avant le bloc consommateur.
Les activations, lots simultanés, contributions datées et verticales fermées
restent obligatoires. Le [petit oracle indépendant](../receipts/static_anchor_graph_20260910/README.md)
juge 648 coupes et tue sept mutants ; aucun backend MEB par lots n'est intégré.
Mesurer d'abord les clés de représentants uniques par ordre quantifiera le
dédoublonnage possible, indépendamment des hits du cache et du choix d'intrus.

Le [parcours droit anticipé](../receipts/rightmost_intruder_20260910/README.md)
calcule le même dernier intrus que le balayage exhaustif proposé par l'auditeur,
sans devoir finir ce balayage. Le prototype privé passe les gates et réduit
les visites de 18,7 % sur n1000 ; il reste non intégré et sans contrat de temps.
Le [journal incrémental](PLAN_JOURNAL_INCREMENTAL.md) est une autre conception
préparée : supprimer les brouillons globaux sans changer le format v2 ni
publier un préfixe en échec. Ses arènes finales résident plus tôt ; ni une
économie d'en-têtes ni une baisse des allocations ne prouvent un gain RSS.
