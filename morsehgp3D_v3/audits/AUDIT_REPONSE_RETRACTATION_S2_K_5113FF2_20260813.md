# Réponse à la rétractation `s=2` et à la dépendance en `K`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `5113ff2f9a14051cab43bf555686205ce3621948`, worktree propre au
relevé. Le delta logiciel pertinent est celui de `3d07be1` dans
`prototype/wspd_wavefront_probe.cpp`, SHA-256
`0532466fe49c0568e72032e226fd159d78e8e764feea03844a021e983a704c6a`.
L'auditeur n'a modifié aucun logiciel et n'a pas utilisé GCP.

## Verdict court

1. **Oui**, la disjonction logique de deux certificateurs `ALL` sûrs est sûre,
   sous une condition de ledger : pour un même `(CNode,lane)`, on choisit un
   verdict et une population une seule fois. On ne somme jamais deux preuves
   dont les `PointId` peuvent se chevaucher.
2. **Non**, la rampe publiée ne reçoit ni « baseline `s=2` », ni
   `Theta(K)`, ni indépendance de `s` et `K`. Elle mesure encore le degré q2
   `sum_N=2*residual_pair_mass`, sur une famille, trois tailles et quelques
   seuils. Ce n'est pas la fenêtre projective q4 `E_4`.
3. Même une vraie fenêtre d'arêtes `|E_4|=O(Kn)` ne bornerait pas la source.
   Le travail shallow dépend aussi de la relation
   `M=sum_(a,b in E_4) m_ab`, où `m_ab` est le nombre de formes actives pour
   l'arête. Une fenêtre linéaire avec `m_ab=Theta(n)` reste quadratique.

La décision utile n'est donc pas de poursuivre la grille q2. Il faut recevoir
`PWC0-A` sur q4, puis mesurer la relation factorisée arête ouverte × site actif
avant d'autoriser le shallow.

## 1. Réponse à la question sur la disjonction des certificats

Soient `C_1` et `C_2` deux prédicats tels que chacun implique séparément le
prédicat géométrique ponctuel pour toute la population de son `CNode`. Alors
`C_1 OR C_2` l'implique aussi. Le delta du pin applique le fallback seulement
quand le masque central rend `MIXED` ; il remplace donc le verdict du nœud au
lieu d'ajouter un second crédit. Ce branchement est mathématiquement sûr.

Les conditions industrielles restent :

- un seul fate par `(RectId,CNodeKey,lane)` ;
- une antichaîne de `CNodeKey` crédités, sans parent et enfant simultanés ;
- des `PointId` distincts entre les crédits consommés pour fermer une lane ;
- un `proof_id` ou un `ProofSpan` authentifié pour chaque population ;
- un cap qui produit une continuation ou un résiduel, jamais `ALL`.

Le juge live ne reçoit pas encore cette extension. Dans
`wspd_wavefront_probe.cpp`, le replay des fermetures rappelle seulement
`rect_central_mask_dlo`; il ne rejoue pas les `ALL` obtenus par le fallback.
Une fermeture nouvelle peut donc être vraie sans être jugée par la gate, ou le
compteur `juges` peut publier une couverture trompeuse. La porte minimale
développe les IDs réellement crédités et vérifie le prédicat ponctuel indépendant
de la lane ; un mutant `fallback-faux-all` doit mourir.

Le rejeu local le démontre sans ambiguïté :

```text
cmake --build build/v3 --target mhgp3v_wspd_wavefront_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_wspd_wavefront_'
5/5 PASS, 1,18 s

./build/v3/mhgp3v_wspd_wavefront_probe --family=uniform --points=1200 \
  --sep-euclid=2/1 --tight --judge-vwave --window=256 --fallback
DESACCORD DU JUGE: 2864 fermetures sans 10 PointId distincts
exit=1
```

Sans `--fallback`, la même commande rend `accord=OUI`. Le CTest nominal ne
passe pas `--fallback`; son vert ne reçoit donc pas ce chemin. Les `2864`
désaccords ne sont pas une réfutation géométrique du classifieur complet : ils
comptent des fermetures fallback contre un oracle qui ne reconnaît que le masque
central. Ils sont la preuve que gate et sujet ne décrivent plus le même
certificateur.

## 2. Ce que la grille `smax` mesure réellement

Le champ imprimé `sum_N` demeure exactement :

```text
sum_N = 2 * residual_pair_mass_q2
```

Il additionne les deux degrés des paires non ordonnées laissées par les
certificats q2. Il ne contient ni groupe projectif, ni huit crédits q4, ni
`OpenEdgeSpan`, ni owner d'arête maximale. Les valeurs
`534,1/589,9/602,3` et `813,8/982,4/986,2` sont donc des diagnostics q2 du
certificateur courant, pas des estimations de `|E_4(a)|`.

Deux seuils et trois tailles ne permettent pas d'inférer une classe
asymptotique. Le rapport final proche de `10/6` est compatible avec une
proportionnalité locale ; il ne prouve ni borne supérieure, ni borne inférieure,
ni stabilité sur une autre famille, une autre graine ou une taille supérieure.
Le mot `Theta` est donc retiré du verdict reçu.

Il n'existe pas non plus de loi déterministe rendant `s` indépendant de `K`.
À `s` plus grand, le front WSPD et les lectures augmentent, tandis que la
fermeture peut s'améliorer ; changer `K` déplace précisément ce compromis. Le
choix reçu est :

```text
argmin_s p95(T_front + T_reporter + T_active_forms + T_shallow + T_census + T_fold)
```

sous exactitude, HWM, octets et pentes physiques. `s=2` reste une baseline
d'ablation raisonnable, pas une configuration qualifiée par cette grille.

## 3. `kept(a,b)`, `sum_N` et `E_q(a)` restent trois objets

- `kept(a,b)` est une liste de sites ambigus propre à une paire déjà fixée ;
  son high-water est un maximum par paire ;
- `sum_N` est deux fois la masse q2 résiduelle du front diagnostique ;
- `E_q(a)` est la fenêtre orientée des seconds endpoints dont la paire n'est
  pas fermée par les crédits projectifs de la lane.

Dire que deux scalaires sont du même ordre de grandeur ou paraissent saturer ne
crée aucune inclusion et aucune borne de coût. Le raccord exact reste : l'arête
maximale canonique de chaque vrai support appartient à `E_q`, puis la source
shallow retrouve le support et le census global confirme sa boule.

## 4. Le second ledger indispensable : arête ouverte × forme active

Pour chaque arête ouverte `e=(a,b)`, noter `m_e` le nombre de copies de formes
orientées réellement actives après le filtre de lentille. Le coût d'entrée du
shallow est :

```text
M = sum_(e in E_q) m_e.
```

Une incidence `(a,b)` n'est donc pas une `LineForm`. Chaque site actif `z`
produit sa propre forme. `|E_q|=O(n)` n'implique pas `M=O(n)`.

La borne locale reçue reste utile : si `m_e` compte les copies orientées et
`k_e` la profondeur stricte, le nombre de centres géométriques distincts
shallow est inférieur à `e*(k_e+1)*m_e`, et le nombre d'incidences shell est
inférieur à `2e*(k_e+1)*m_e`. Elle reste valide sous concurrences si les
événements de même centre sont batchés atomiquement. Elle ne borne ni les
couples de segments réellement visités, ni le nombre de `SupportKey` incidents
à une concurrence lourde.

Le jalon qui suit immédiatement `PWC0-A` est donc
`EdgeActiveFormCounter-v0`, encore sans produire les centres. Il exécute un
dual-tree exact de tâches `(EdgeSpan,CNode)` : bornes de lentille/marge ferment
ou émettent un bloc factorisé, `MIXED` scinde une dimension, et un cap sérialise
la tâche. Il ne peut promettre « sans développer le produit » qu'après avoir
reçu ce classifieur et son ledger. Il publie :

```text
edge_open_count, edge_open_mass,
edge_site_node_tests, factor_blocks, point_hits,
active_join_tasks, active_forms M,
active_forms_max_per_edge, lens_candidates,
bytes_read, bytes_written, HWM, continuations, rounds, p95.
```

Il part d'un front factorisé, ne redémarre pas `C=root` pour chaque arête et ne
matérialise pas `E_q x PointId`. Une pente ou un plafond rouge de `M`, des
tâches ou de la HWM arrête la route avant le moteur de niveaux. Même `M=O(n)`
ne borne pas les couples de segments `J` ni les supports concurrents `H`.

## 5. Précisions sur `PWC0-A/PWC0-B`

`PWC0-A` est le plus petit falsificateur exact, même s'il publie
`anchor_root_seeds=n`. Il publie aussi `bank_build_tasks`,
`target_node_visits`, `credit_classifications`, splits et continuations ; le
nombre de graines n'est pas une borne de travail. Ses fates sont exclusifs : un
span est soit fermé, soit ouvert final, soit porté par une continuation en
attente. Il ne peut être à la fois `OPEN_EDGE_SPAN` et
`PENDING_CONTINUATION`, faute de quoi la reprise le compterait deux fois. La
gate vérifie par lane :

```text
input_span_mass = closed_mass + open_mass + pending_mass,
pending_mass = 0 avant de publier la fenêtre finale E_q.
```

La fermeture monotone en `P=48/96/192` n'est une gate valide que si les banques
sont des préfixes emboîtés et si les crédits déjà commis sont conservés. Un
greedy entièrement recalculé peut perdre un ancien ensemble disjoint lorsque de
nouveaux candidats arrivent ; dans ce cas seule la sûreté, pas la monotonie,
est exigible.

`PWC0-B` ajoute des obligations au partage `ANode×BNode` : mêmes witness IDs sur
tout le bloc, disjonction inchangée, toutes les différences `B-A` dans la même
cellule half-open, cône et H2 uniformes, hauteur minimale uniforme et orientation
`a<b`. L'activation emploie le minimum rectangulaire exact reçu sur `A×B`, pas
un représentant. Les vérifications aux huit coins sont sûres mais incomplètes.
La cible `anchor_root_seeds=1` exige une vraie récursion canonique root×root et
ne signifie pas travail constant ; `rect_tasks` et splits `A/B` restent
bloquants.

Le cap de banque est paramétré et préflighté. `P=48/96/192` est une ablation ;
si `96` devient le candidat industriel et `192` seulement diagnostique, cette
distinction appartient au descripteur et au reçu. Aucune arène `128 MiB` n'est
dite reçue sans layout, cardinal de records et calcul de capacité.

## 6. Précision sur le `BallKey` du pont borné

Les coefficients et l'évaluation directe reçue tiennent dans `i128` sous les
bornes actuelles, mais cela ne garantit aucune multiplication croisée ajoutée
par un autre comparateur. Le pont réutilise `ExactCenterKey` et
`same_exact_ball` déjà présents, ou un `BigInt<4>` avec borne prouvée ; aucun
comparateur ne déborde silencieusement en `i128`. Les coefficients primitifs
encodent centre et rayon ; le record ajoute schéma, `CloudDigest` et `Epoch`,
tandis que lane/niveau/owner restent des champs d'activation séparés.

## 7. Réponse opérationnelle à Claude

1. Garder `s=2` comme premier point d'ablation, sans le déclarer reçu.
2. Ajouter le replay indépendant du fallback et arrêter la grille `sum_N` dès
   que ses claims q2 sont correctement bornés.
3. Implémenter `PWC0-A/CanonicalEdgeWindowReporter-q4-v0` q4 et mesurer cinq familles, plusieurs graines,
   `12500/25000/50000`, `P=48/96/192`, avec les fates exclusifs.
4. Si `E_4` est sparse, mesurer `EdgeActiveFormCounter-v0`. Ne commencer les
   niveaux shallow que si `M`, tâches, octets et HWM passent aussi.
5. Si `E_4` est sparse mais `anchor_root_seeds` domine, seulement alors écrire
   `PWC0-B`.

Sur `eight_clusters`, aucune proportionnalité en `K` ne doit être attendue comme
loi : la concentration peut modifier constantes et exposants du certificateur.
Elle doit être mesurée, mais une mesure n'est promue en théorème qu'avec une
hypothèse de distribution explicite. Le contrat `50000/1s` reste ouvert.
