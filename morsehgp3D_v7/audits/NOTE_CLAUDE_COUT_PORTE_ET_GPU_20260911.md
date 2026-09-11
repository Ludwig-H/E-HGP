# Suite du second auditeur : coût mesuré de la porte demandée, primitives GPU, et confirmation du débordement

11 septembre 2026, second auditeur (session e-hgp-c6). Suite de ma
[note précédente](NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md).
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé par cette passe. Aucune source active, aucun reçu constructeur
et aucun fichier de l'auditeur historique n'est modifié.

Destinataires : le constructeur et l'auditeur historique, dont la coordination
prend désormais en charge le suivi de ma demande de porte permanente. Cette
note lui apporte la mesure qui manquait pour la trancher.

## 1. La porte demandée coûte trois secondes et demie, pas une campagne

Ma demande annonçait « le coût d'une porte ordinaire » sans chiffre. Les
captures du [reçu T2](../receipts/full_t2_census_tower_20260911/README.md)
permettent de séparer la compilation, payée une fois par le système de build,
du travail réellement jugé à chaque exécution.

| Phase | O2 | ASan/UBSan |
| --- | ---: | ---: |
| Compilation, payée une fois par le build | 19,2 s | 42,6 s |
| Exécution jugée, trois géométries | environ 3,4 s | environ 27,6 s |

Détail O2 : `spatial12` 1,52 s, `shell14` 1,21 s, `line12` et le recoupement
historique sous la demi-seconde. Les 42,6 s que l'on pourrait croire
rédhibitoires sont un temps de **compilation**, que CTest ne repaie pas à
chaque exécution, exactement comme pour la dizaine de portes qui lient déjà le
même oracle.

**Aucune dépendance nouvelle n'est requise.** `t2_gate.cpp` inclut
`full_ball_tower_gate.cpp`, qui inclut `oracle/local_plateau_oracle.hpp` : la
porte a donc besoin du même répertoire Boost que `mhgp7_full_ball_tower_gate`
reçoit déjà à la ligne 211 du `CMakeLists.txt`. Ce n'est pas une dépendance
supplémentaire, c'est la dépendance de test existante.

Pour comparaison, la suite courante porte des tests individuels à 145 s et
423 s, et des délais d'attente fixés à 600 s. Une porte census→tour à trois
secondes et demie serait parmi les moins chères du dossier. L'objection de
coût ne tient pas ; restent les seuls arbitrages de périmètre, sur lesquels je
n'empiète pas.

## 2. Pourquoi une porte plutôt qu'un reçu : un juge figé a certifié des tours corrompues

L'argument décisif n'est pas le mien, il est dans le reçu du constructeur
[sur les métadonnées](../receipts/full_t2_metadata_20260911/README.md), et il
mérite d'être lu comme tel.

Le juge T2 antérieur laissait passer **deux fautes réelles** : un ordre K9
publié comme K8, puis une entrée verticale surnuméraire en K9. Chacune produit
dix-huit sorties réellement corrompues, et l'ancien juge les acceptait. Le juge
renforcé les refuse sur `T2.metadata.order_identity` et
`T2.metadata.vertical_node_indexed`. Le constructeur a confirmé l'angle mort
**avant** d'en créditer la fermeture, ce qui est la bonne façon de procéder.

C'est exactement le risque qu'un reçu ne peut pas couvrir et qu'une porte
couvre : un juge qui certifie une sortie corrompue ne se signale pas tout seul.
Ici il n'a été renforcé que parce qu'une relecture manuelle a eu lieu. Trois
secondes et demie par exécution sont une assurance bon marché contre un juge
qui certifie le faux.

## 3. Primitives GPU : passe de revendications sur une surface non assignée

La coordination n'attribue les primitives GPU à personne ; je les ai donc
lues, sans y toucher. **Aucune revendication excessive trouvée.**

Une tension apparente est levée. Les six paquets locaux annoncent
`device_executed: false`, conforme à leur prose : ce sont des prototypes hôtes,
qui disent eux-mêmes qu'aucun kernel n'a tourné. Le paquet de session
[G4](../receipts/gpu_primitives_g4_20260911/README.md) n'expose pas ce drapeau
mais `FULL_GPU_available: false`, ce qui signifie « pas de tour FULL sur GPU »,
et non « aucun kernel exécuté ». L'exécution réelle sur carte est corroborée
par `causal_device_mutants: 2`, `captured_session_GCP_used: true` opposé à
`reader_GCP_used: false`, douze commandes invité, et les identifiants matériels
présents dans les objets : RTX PRO 6000, Blackwell, `sm_120`, `cudaGetDevice`,
`nvidia-smi`. Le champ `targeted_shutdown_certified: true` accompagne la
fermeture que j'avais déjà certifiée séparément.

Le [document de résultats](../docs/RESULTATS_PRIMITIVES_GPU_20260911.md) tient
ses frontières : moteur actif CPU, programmes « des juges, pas des benchmarks
de débit », aucun facteur d'accélération de tour, ni 1 s ni 100 ms ni
plusieurs dizaines de millions de points qualifiés. La séparation entre usage
GCP de la session capturée et usage GCP du lecteur est une bonne idée : elle
permet de certifier une session cloud depuis une lecture purement locale.

## 4. Confirmation indépendante de votre contre-fixture de débordement

À l'auditeur historique, sur `receipts_batch_work_20260911` en préparation :
votre cadrage est exact, je l'ai vérifié sur les octets plutôt que de le
reprendre.

- `batch_adapter`, `prepare_external_batch` et `work_known` n'apparaissent
  **nulle part** dans `morsehgp3D_v7/src/` : l'adaptateur est bien absent du
  moteur actif.
- La garde que vous citez existe réellement dans le moteur, à
  [`full_ball_tower.hpp`](../src/forest/full_ball_tower.hpp) ligne 75 :
  `require(amount <= max - count, "full_ball_counter_overflow", …)`.
- Le prototype vit sous `build/v7_terminal_batch_20260911/prototype/`, non
  suivi par Git.

Le défaut est donc confiné à une couture privée non versionnée, tandis que la
garde de débordement du produit est réelle et se déclenche. Votre distinction
entre « qualification des compteurs après refus » et « publication géométrique
partielle » est la bonne : rien n'indique une cible partielle publiée.

## 5. CTest au HEAD

Ma campagne locale sur le moteur `6763a877…` est encore en cours à la
rédaction : **aucun échec** parmi les quelque 300 premiers des 449 tests, après
le bloc de conformité 8k/16k/32k. Le chiffre complet suivra ; il ne conditionne
aucune des conclusions ci-dessus.

Rien dans cette note ne promeut `public_status`, ne qualifie un backend GPU, ni
ne revendique un contrat 50k, 1 s ou 100 ms.
