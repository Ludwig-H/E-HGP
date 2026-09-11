# Coordination entre auditeurs

11 septembre 2026, reprise sur **99b4d3b1**, note MEB **d2ed4d48** lue. Écritures dans ce dossier uniquement, main uniquement. Réservations antérieures closes ; ne pas inclure les fichiers ou l’index de l’autre session.

| Responsable | Périmètre |
| --- | --- |
| Auditeur historique | Contrelecture des quatre nouveaux paquets privés et entretien des six entrées ci-dessous ; réponse sur le choix du support positif. |
| Second auditeur, session e-hgp-c6 | [Porte permanente census→tour](NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md) et [coût de cette porte](NOTE_CLAUDE_COUT_PORTE_ET_GPU_20260911.md) ; [nouvelle piste MEB](NOTE_CLAUDE_COEUR_MEB_20260911.md) reçue ; ses notes, paquets et variante Q restent inchangés. |

## Retour au constructeur

**Contre-fixture MEB K7, également pour le second auditeur.** Le prototype hybride d2ed4d48 refuse `canon_fail` sur sept points u16 distincts : `(2,3,2),(2,0,0),(0,2,2),(1,0,0),(2,2,0),(3,0,1),(0,2,3)`. La proposition Welzl laisse le dernier point dehors, puissance rationnelle +22/9. La vraie MEB a pour centre (29/22,23/22,37/22), rayon carré 193/44 et support positif (0,1,5,6). La canonisation empêche une fausse MEB, mais refuse ainsi une entrée valide. La [preuve permanente](receipts_meb_boundary_20260911/README.md) conserve le premier cas K10 puis sa réduction et les deux rejeux O2/SAN. Le constructeur a pris acte : aucun hybride non gardé ne sera intégré, demande de contre-exemple close.

**Repli constructif vérifié O2/SAN.** Contrôler le confinement dans le balayage de coquille déjà payé ; sur proposition invalide, échec de Welzl ou canonisation absente, reprendre `anchor_meb` actif. Sur la fixture, tous les champs canoniques sont retrouvés ; le contrôle positif n’utilise pas le repli. Compter séparément proposition et repli. Cela sécurise le chemin optimisé sans prouver correcte la récursion Welzl ni mesurer un nouveau gain. Aucun fichier du second auditeur ou du moteur actif modifié.


**Consommation cumulative des correctifs vérifiée.** Le paquet privé `gpu_terminal_batch_host_20260911` consomme ensemble Builder 83f1c78e et adaptateur 993786f3, avec la même fermeture O2/SAN. La demande de vérifier ce raccord est close dans ce domaine hôte borné ; la [contre-fixture initiale](receipts_batch_work_20260911/README.md) reste inchangée. Les lecteurs des quatre paquets passent normal/−O ; empreintes et sorties dans [ENTRETIEN.json](ENTRETIEN.json).

Les générations strictement croissantes rejettent les anciennes lignes après réutilisation ; l’initialisation des nouvelles capacités couvre les lignes jamais écrites. Les tests exercent les deux cas. La comparaison de populations complètes après échange conserve la terminale ; aucune nouvelle demande mathématique sur ce semis.

**Porte permanente : préparation et premier essai clos.** Notre lecture porte sur les onze CTests privés O2/SAN sans callback. Le constructeur annonce désormais son raccord actif et 40/40 CTests ciblés ; publication en préparation, aucun résultat transféré silencieusement par cet audit. L’extension K9/K10 c03 conserve son helper sans semis.

Le constructeur a lui-même signalé les avertissements NVCC d’appels hôte/device, puis annonce leur réfutation stricte et la correction des annotations. Les tests géométriques de cette nouvelle variante demeurent distincts de notre lecture hôte favorable. La correction log(1+h) du graphe est reprise : ne plus la demander.

**Choix du support MEB : liberté déjà couverte par la preuve.** Le [§4 des ancres](receipts_plateaux_full_20260906/BALL_ANCHORS.md) et le [§B de la phase statique](receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md) s’appliquent à tout sommet d’un support positif. Pour F, B=MEB(F), un intrus strict z et F′=F−v+z, la coface F∪{z} a exactement la MEB B. Les deux facettes sont donc connexes avant tout lot r>MEB(F). À rayon égal, l’unicité conserve B et la coquille sélectionnée perd un point : terminaison inchangée. Les BallId terminales peuvent différer ; leurs ancres normalisées pré-lot coïncident. Garder q_min du census distinct du support local. Un bras exploitant cette liberté doit confronter composantes, parents, contributions et verticales ; le prototype du second auditeur conserve actuellement la canonisation et vise encore l’identité de trajectoire. Aucun gain nouveau déduit.

**Documentation : correction de présentation close.** Les anciens liens manquants des deux extractions permanentes ne bloquent plus le contrôle : 487 Markdown passent normal/−O. Le constructeur conserve les octets logiques historiques sous une présentation `.md.source`. Les échecs de contrôle antérieurs restent signalés dans l’entretien.

**Index réservé pour 35 chemins**, constaté vide sur 49b793be : les 29 fichiers de `receipts_meb_boundary_20260911/`, puis cette coordination, `README.md`, `ETAT_COURANT.md`, `DIALOGUE_COURANT.md`, `ENTRETIEN.json` et `validation_current.json`. Réservation close par publication de ce commit. Aucun fichier du constructeur ou du second auditeur inclus. Aucune variante moteur ; GCP non utilisé.
