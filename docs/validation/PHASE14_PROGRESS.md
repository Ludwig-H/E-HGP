# Progression Phase 14 — socle industriel Morse H0

## État

Phase `14`, backend `reference_cpu`, profil `hgp_reduced`, mode scientifique `certified`, mode de déploiement `architecture_only`, `public_status=not_claimed`.

La porte d'architecture est satisfaite par la fermeture administrative de Phase 10 comme implémentation candidate conditionnelle. Sa fidélité globale des carriers reste une obligation de preuve; la porte de qualification finale reste en outre bloquée par la Phase 11, M.1, la voie CUDA complète et le protocole p95.

## Incrément 14A livré — planification résidente et streaming

`ExactDirectMorseIndustrialPlanResult` rejoue fraîchement 10.1 et 10.2, joint leurs lots et calcule exactement les nombres de minima, simplexes du catalogue de Gabriel complet, bras et références de clés. Il certifie des bornes conservatives de nœuds et d'enfants, puis applique un modèle d'octets fourni par l'appelant et une réserve explicite de sommets de descente.

Le profil `interactive_resident_50k` exige au plus 50 000 points et un unique chunk contenant tous les lots. Le profil `massive_external_streaming` agrège des lots consécutifs sans jamais couper un couple ordre--niveau; chaque frontière de chunk devient une frontière de checkpoint et de run externe. Un lot indivisible qui dépasse un plafond rejette atomiquement tout le plan.

Le plan conserve seulement des intervalles dans le journal 10.1 et des compteurs. Il ne copie ni clé, ni facette, ni niveau exact, ne construit aucune descente ou forêt, et ne matérialise ni cellule, ni coface, ni Gamma, ni mosaïque de Delaunay d'ordre supérieur. Son prédicat recalcule en une passe les intervalles, agrégats, bornes, réserves, octets, caps et frontières. Son acceptation ne signifie ni p95 inférieur à une seconde, ni capacité démontrée à dix millions de points.

## Incrément 14B livré — transaction locator proportionnelle aux unions

`ExactDirectSparsePositiveFacetLocator::apply_batch` ne copie plus l'arène des $H$ parents. Après toutes les requêtes pré-lot, un journal réservé à $U_b$ handles enregistre seulement les $W_b$ racines effectivement réorientées, avec $W_b\leq\min(U_b,H-1)$ pour $H>0$. La compatibilité des doublons lit cet état candidat. Un succès conserve exactement $W_b$ écritures; un rejet post-unions restaure exactement ces $W_b$ racines en ordre inverse.

Les diagnostics éphémères distinguent écritures transactionnelles, écritures de rollback et pic d'entrées du journal. Ils n'entrent ni dans les compteurs durables, ni dans le digest, de sorte que le schéma scientifique et ses vecteurs canoniques restent inchangés. Le scratch du lot devient $O(Q+KB+U_b)$ au lieu de $O(Q+KB+H)$; aucune nouvelle structure globale n'est construite.

## Incrément 14C livré — lanes de descentes par structure exacte

Le nouveau planificateur rejoue 14A sous un plafond explicite de chunks, contrôlé avant chaque rétention avec abandon du préfixe transitoire en cas de dépassement, puis classe les familles de chaque lot exact par cardinal de support deux, trois ou quatre, soit au plus trois lanes. Une lane reste dans un seul chunk et un seul lot; elle conserve seulement les intervalles candidats, les cardinaux et les comptes de graines, lancements et examens locaux. Elle n'alloue ni clés, ni facettes, ni permutation durable de bras.

Le plan impose un snapshot locator pré-lot commun, une sélection stable unique, une fermeture 10.5c et une mémoïsation partagées par lot, puis une jointure par `arm_seed_index`. La tuile de lancement est bornée avant allocation. Le pire coût initial autonome publié vaut quatre passages sur au plus 385 supports par graine à $K=10$; la fixture frontière à support deux porte donc deux graines et 3080 examens. La fermeture complète, la difficulté LBVH ou rationnelle et les temps GPU ne sont pas bornés.

`complete_architecture_plan()` est explicitement un contrôle de forme. `fresh_reconstruction_required_before_execution=true` exige le vérificateur frais avant exécution ou persistance. Le plan reste `architecture_only`, `public_status=not_claimed`, sans revendication 50 k ou 10 M+.

## Incrément 14D livré — exécuteur ancré et delta compact

Le nouvel exécuteur reconstruit et compare 14C une seule fois à l'ouverture, possède ce plan frais et avance par un curseur canonique. Pour chaque lot, il sélectionne toutes les lanes en une passe stable, reconstruit les seules facettes demandées, déduplique leurs clés complètes et lance une unique fermeture 10.5c sous un snapshot locator gelé. Les bras rejoignent ensuite les carriers résolus par leur `arm_seed_index`.

La fermeture est enfermée dans une portée transitoire. Avant que le résultat ne soit publié, ses nœuds, arêtes, projections, miniballs et tables de mémoïsation sont détruits; seuls restent les $D$ couples clé--carrier--témoin et les $A$ jointures, soit un delta logique $O(KD+A)$. Le rejeu de commit reconstruit seulement le lot courant, jamais 14A ni l'ensemble du plan 14C. Il avance le curseur d'exécution si et seulement si le delta complet et le stamp sont identiques; il ne mute ni locator, ni quotient, ni forêt.

Cette tranche est commune au résident 50 k et au streaming 10 M+, mais elle ne qualifie encore ni leur temps, ni leur volume. Le profil massif devra évacuer le delta compact vers un run durable avant de poursuivre. Aucune structure globale évitée par l'architecture n'a été réintroduite.

## Incrément 14E livré — couture canonique et incumbent exact

14D déduplique déjà les clés complètes de ses bras. Il appelle désormais 10.5c par une vue strictement croissante de ces clés, avec identités implicites, au lieu de reconstruire des records de graines, de les recopier, de les trier puis de recopier et dédupliquer une seconde fois leurs clés. L'entrée générale 10.5c demeure inchangée pour les appelants qui fournissent des identités désordonnées ou des clés répétées, et les deux entrées produisent le même résultat scientifique sur le sous-cas canonique distinct.

L'audit LP64 mesure 96 octets par record de graine et 88 octets par clé. La voie 14D supprime donc deux populations de records et une population interne de clés, soit $280D$ octets, trois allocations et deux tris redondants pour $D$ clés distinctes. Le gain vaut environ 13,4 Mio pour 50 000 clés et 280 Mio au plafond actuel de $2^{20}$ graines. Il ne borne pas les limbs rationnels, les tables de mémoïsation ou le scratch top-$K$ et n'est donc pas une qualification mémoire globale.

Le socle spatial accepte aussi des incumbents top-$K$ facultatifs. Ils sont validés, leurs distances sont recalculées exactement et imputées au budget total, puis ils ne servent qu'à initialiser la borne de la traversée exacte. Une bonne proposition peut augmenter l'élagage strict; une proposition adversariale conserve exactement la même partition. La coquille d'égalité reste complète. Le transcript par lot, les classes de difficulté observées et le producteur GPU restent à brancher; aucun compteur ou digest de proposition ne devient une décision scientifique.

## Incrément 14F livré — transcript sparse et consommation exacte

Le nouveau transcript hôte conserve un ensemble sparse de records triés par clé complète, chacun contenant au plus $K$ identifiants proposés. Il ne porte aucune distance ou décision. Les populations de records, références de clés, candidats, octets et entrées logiques sont préflightées avant copie; toute erreur conserve un payload vide. Le domaine des candidats est volontairement différé jusqu'à l'enveloppe de fermeture, qui revalide aussi le lot, le niveau exact, le stamp locator et l'inclusion des clés avant de créer un seul nœud 10.5c.

Chaque requête 14F utilise la facette source $F$ comme baseline exacte et une proposition facultative $P$. L'union $F\cup P$ contient au plus $2K$ identifiants distincts, dont les distances sont imputées une seule fois au budget. La heap reste de taille $K$, mais la coquille initiale conserve tous les co-minimiseurs. Une proposition adversariale ne peut pas produire un cutoff pire que $F$, et la traversée exacte continue avec prune strict et descente à égalité.

La fermeture scientifique et l'audit opérationnel sont deux payloads distincts. L'audit compte les lignes utilisées ou absentes, les fallbacks dynamiques, les tailles de pools, les distances, visites, expansions, bornes, élagages, scans complets et épuisements. Le graphe ne retient ni transcript, ni candidat, ni partition top-$K$, ni coquille. Les entrées historiques de 10.5c restent non amorcées.

Pris isolément, ce raccord ne modifie ni `prepare_next`, ni `commit_prepared` dans la session 14D. Il ne lance aucun kernel GPU et ne possède ni epoch, ni digest de contexte, ni scheduler par difficulté. Il ne qualifie ni le p95 50 k, ni la capacité 10 M+, ni un statut public.

La validation courte finale du 23 juillet 2026 passe les trois CTests transcript, spatial et fermeture sous GCC Release en 0,18 s et sous Clang 18 Release en 0,13 s. Le garde statique 10.5c passe également. L'installation/export puis le consumer externe passent 1/1 avec des appels réels au constructeur de transcript, au pool exact $F\cup P$ et à l'enveloppe vide de fermeture. Aucun benchmark, test massif, sanitizer, GPU ou GCP n'a été lancé.

## Incrément 14G livré — préparation propositionnelle sans autorité de commit

La nouvelle entrée de session prépare le lot courant avec un transcript 14F après les jointures, caps 14D, reconstructions de bras et clés canoniques. Elle garde donc la priorité des diagnostics structurels et de capacité. Le lot vide revalide lui aussi tout transcript explicite, bien que son delta scientifique conserve le raccourci historique sans fermeture.

Un rejet de transcript produit zéro fermeture, zéro delta et zéro avancement. Un succès retourne le delta 14D complet et l'audit de consommation dans deux champs distincts; ni le résultat ni la session ne retiennent record, candidat, partition, coquille ou graphe. Le transcript peut ensuite être détruit. `commit_prepared` demeure inchangé et sans paramètre propositionnel : il rejoue exactement le lot courant par la voie historique non amorcée, compare le delta entier et ignore par construction l'audit opérationnel. Une préparation complète ne certifie pas que ce rejeu tient sous les mêmes caps; cette dette de vivacité reste ouverte.

Sur E5, les transcripts vide, utile `AC` vers les points de `DE` et adversarial produisent le même delta champ par champ, tandis que leurs tailles de pools restent visibles dans l'audit séparé. Un mauvais indice de lot rejette avant fermeture. Le scratch de sélection reste $O(KA+KD)$, le transcript fourni par l'appelant vaut $O(KR)$ avec $R\leq D$, et le delta vaut $O(KD+A)$; ils peuvent coexister avec la fermeture transitoire, donc aucun pic mémoire global n'est qualifié. 14G ne ferme encore ni le doublon du rejeu exact, ni le pic des clés de bras.

14G reste `reference_cpu`, `hgp_reduced`, scientifique `certified`, déploiement `architecture_only` et `public_status=not_claimed`. Aucun producteur CUDA, epoch, digest, scheduler de difficulté, scratch réutilisable, protocole `warm_e2e`, run durable ou checkpoint n'est ajouté.

## Priorités de développement

1. fermer la vivacité préparation--commit sous budget sans donner d'autorité scientifique au transcript;
2. brancher ensuite un producteur GPU borné sur cette frontière;
3. réutiliser les capacités restantes de scratch sans conserver de graphe ou d'objet scientifique entre lots;
4. ajouter l'instrumentation `warm_e2e`, puis rendre les deltas, chunks et checkpoints durables avant toute campagne massive.

Ces priorités optimisent le chemin démontré. Elles ne réintroduisent ni les gateways historiques, ni un oracle combinatoire dans l'architecture produit.

## Validation

Les deux bibliothèques nouvelles compilent en Release avec les avertissements stricts. L'unique CTest partagé avec la fermeture de Phase 10 passe en 0,03 seconde et construit sur E5 les politiques résidente et streaming; une identité de chunk mutée invalide le prédicat structurel. Aucune campagne de performance n'est lancée.

Pour 14B, la cible locator compile avec les avertissements stricts et son unique CTest ciblé passe en 0,01 seconde. Les deux fixtures déjà discriminantes vérifient respectivement une écriture conservée et une écriture restaurée; le checker statique interdit le clone dense et impose la réservation puis l'écriture journalisée. Aucun benchmark, GPU ou test long n'est lancé.

Pour 14C, les deux CTests ciblés passent sous GCC Release strict en 0,16 seconde et sous Clang Release strict en 0,14 seconde. Ils couvrent le plan vide sans selle, les trois lanes du tétraèdre, les plafonds de chunks, lanes, lancements et examens juste insuffisants, l'absence de chunk source partiel après rejet, la tuile nulle, plusieurs chunks, deux lanes de supports distincts dans un même batch avec barrière commune, une lane falsifiée et le bord collinéaire $K=10$ à 3080 examens initiaux. Le CTest forêt séparé exerce la vraie fixture E5 tridimensionnelle, sa liaison `AC` vers la composante de `DE`, l'identité causale de cette racine avec la continuation exacte `ABC` et le rejeu frais. L'installation, l'export de la cible et le consumer externe passent 1/1. Aucun benchmark, GPU, sanitizer ou oracle long n'est lancé.

Pour 14D, l'unique CTest ciblé passe sous GCC Release strict en 0,02 seconde et sous Clang Release strict en 0,03 seconde. Il couvre une session à deux lots avec un seul ancrage 14C, un lot vide, le rejet inconditionnel d'un cap 10.5c hors plafond, les douze bras du tétraèdre dédupliqués en quatre clés, deux lanes de supports deux et trois partageant une fermeture, les caps insuffisants de onze bras et trois clés sans delta ni avancement, les retries, un stamp locator périmé, le retry sous le nouveau stamp, une traversée streaming de deux chunks, les mutations d'indice et de compteur du delta et un plan falsifié rejeté à l'ouverture. L'audit conserve zéro nœud après chaque fermeture. L'installation, l'export de `direct_sparse_facet_descent_batch_executor` et le consumer externe passent 1/1 avec appel réel du prédicat non-inline. Aucun benchmark, GPU, sanitizer, oracle long, test massif ou GCP n'est lancé.

Pour 14E, les trois CTests ciblés passent sous GCC Release strict en 0,24 seconde et sous Clang Release strict en 0,15 seconde sur le dernier rejeu. Ils comparent bit à bit l'entrée canonique et l'API générale 10.5c sur une vraie chaîne à arêtes strictes, rejettent les vues non croissantes, dupliquées, de cardinal mixte ou invalides, puis font traverser à 14D la descente exacte `AC` vers le terminal positif `DE` sans retenir son graphe. Le test spatial séparé compare la voie sans incumbent, une bonne proposition, une heap partiellement initialisée et une proposition adversariale, exerce le cap exact juste insuffisant, les identifiants invalides, répétés ou exclus, et une coquille à six égalités qu'aucun prune non strict ne peut tronquer. L'installation, l'export et le consumer externe passent 1/1 en appelant réellement les deux nouveaux symboles. Aucun benchmark, kernel GPU, test massif ou GCP n'est lancé.

Pour 14G, l'unique CTest ciblé passe sous GCC Release strict en 0,05 seconde et sous Clang 18 Release strict en 0,04 seconde. Il couvre la priorité du cap de clés avant transcript, la revalidation du lot vide, le rejet atomique d'un mauvais lot, les préparations E5 vide, utile et adversariale avec égalité complète du delta, la conservation arithmétique de l'audit, la destruction des payloads avant commit et le rejeu historique non amorcé sous caps généreux. Le garde statique 10.5c et le checker des statuts passent. L'installation/export puis le consumer externe passent 1/1 en 0,01 seconde; le consumer appelle le nouveau prédicat non inline sur une enveloppe vide, tandis que la méthode de préparation réelle reste couverte par le CTest unitaire. Aucun benchmark, sanitizer, GPU, oracle long, test massif ou GCP n'est lancé.

Une falsification coordonnée pourrait encore redistribuer des compteurs entre chunks 14A ou lanes 14C tout en conservant leurs agrégats. Les payloads compacts ne contiennent volontairement ni détail ni digest par lot; un vérificateur frais contre 10.1--10.2 est donc requis avant persistance ou exécution industrielle. Cette dette P2 est compatible avec `architecture_only`, mais devra être fermée avant toute qualification.

La dette d'intégration à arête stricte non nulle de 14D est fermée par la fixture `AC` vers `DE`. La dette P2 restante est l'API qui documente mais n'interdit pas encore statiquement les autorités temporaires dont les pointeurs deviendraient pendants.

## GCP

GCP non utilisé.
