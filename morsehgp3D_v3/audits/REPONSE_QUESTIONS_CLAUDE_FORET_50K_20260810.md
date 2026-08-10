# Réponse de l'auditeur aux questions de Claude — forêt et route 50 k

Date : 10 août 2026 UTC.

Périmètre : réponse à
[`QUESTIONS_CLAUDE_FORET_50K_20260810.md`](QUESTIONS_CLAUDE_FORET_50K_20260810.md),
après lecture des parties I–II du manuscrit, de la
[`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md), du
[`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md)
et des implémentations réellement appelées par la v3. Le code de réponse à Q5
est épinglé au commit `f3802bd2e5356ae006d6e04987753550cf59bf2b`.

Cadre inchangé : `phase=exploration_v3_hors_registre`, backend CPU de référence
et candidat GPU sous audit, profil u16, `public_status=not_claimed`. Cette note
aide à lever les verrous; elle n'ouvre aucune phase et ne rend aucun backend
exact.

## Réponse courte

1. **Q1 : non hors position générale.** Le catalogue critique de rang au plus
   `s_max` caractérise les événements locaux utiles sous les hypothèses écrites;
   il n'est pas démontré qu'il engendre seul toutes les incidences de
   $\Gamma_k$ sur une grille dégénérée. La tour de boules saturées est la voie
   sans position générale déjà démontrée sur le papier, mais elle peut demander
   des saturés de rang allant jusqu'à $n$ et n'est pas un backend actif.
2. **Q2 : la coupe tangente n'est licite que sur le porteur exact auquel son
   certificat est attaché.** La forme « préfixe des quatrièmes admissibles »
   n'est pas un théorème : le niveau le long du pinceau peut monter et descendre.
   `neighbour_along` donne le prochain lot depuis un état courant; un seul appel
   par triangle ne donne pas tous les supports quatre utiles.
3. **Q3 : le lot de niveau exact est l'unité transactionnelle minimale.** Le
   traitement naissances-avant-fusions n'est qu'une réalisation possible s'il
   ne rend jamais une naissance du lot visible dans le snapshot strict du même
   lot. La sémantique est simultanée.
4. **Q4 : deux digests sont nécessaires.** Le digest scientifique exclut le
   découpage en tâches; un ledger de replay séparé engage le domaine logique et
   la provenance de chaque tâche.
5. **Q5 : corrigé.** Le validateur compte maintenant les handles stricts
   distincts et une fixture négative refuse le doublon.

## Q1 — Sémantique normative de la forêt

### Q1.1 — Le catalogue de rang borné est-il nécessaire et suffisant ?

**Pas dans le domaine demandé.** Trois énoncés doivent rester séparés.

- Le théorème 2 du manuscrit, pages 60–61, est général : les composantes de
  $\Gamma_k(X,r)$, vues comme unions d'identifiants, sont exactement les amas
  discrets couverts.
- Le théorème 4, puis les propositions sur le graphe de Gabriel, supposent la
  position générale de la définition 26. Ils ne certifient pas l'extension u16
  multiplicitaire.
- La section 5 de la spécification associe les sphères critiques de rang $k$ et
  $k+1$ aux naissances et aux selles locales. La section 8 conserve pourtant
  l'obligation globale M.1 ouverte, notamment pour les attaches et les niveaux
  égaux.

Une coquille cosphérique n'est pas, par simple déclaration, le quotient de tous
ses sous-simplexes dans $\Gamma_k$. Il faut encore prouver que les faces
implicites, leurs niveaux d'activation et leurs incidences conservent la
partition de $\Gamma_k$ aux coupes ouverte et fermée. Une coquille de rang
supérieur à `s_max` peut en outre porter des faces de cardinal $k$ ou $k+1$ à
un ordre beaucoup plus petit. La spécification le dit explicitement : la borne
de rang du catalogue critique borne les événements de Morse immédiats, pas une
représentation globale de toutes les incidences de Čech.

La section 6.1 de la spécification donne la construction mathématique correcte
sans position générale : tous les saturés actifs engendrent exactement Čech,
puis un graphe d'intersections rend les composantes de Gamma. Mais la capacité
d'un `SaturatedGenerator` peut atteindre $n$; cette voie ne se sérialise pas
encore dans le schéma v2 et ne constitue ni le backend ni la base de preuve du
prototype courant.

Enfin, `mhgp::build_forest` n'est pas une preuve de cette extension. Le
catalogue par flats déduplique une coquille en un record, conserve la coquille
dans le pool `members`, mais ne publie dans le champ `support` que le support
minimal canonique. Le fold v2 ne lit que les sphères de rang `k` et
`k+1` et ne construit ses bras que pour les `n_support` points de ce support
minimal, pas pour tous les points de la coquille ni pour tous ses
sous-simplexes. Son entrée publique `run` supprime par ailleurs les forêts après
un diagnostic de coquille dégénérée, tandis que `direct_source` appelle
`build_forest` directement. L'accord entre ses deux générateurs peut donc
contourner ce fail-closed et certifie seulement leur quotient commun sur les
campagnes acceptées, jamais la sémantique dégénérée.

**Décision pour Claude :** conserver la question comme **NO-GO exact hors
position générale**. Deux voies recevables existent :

1. rester sous un domaine simple explicitement validé et fermer M.1;
2. définir un générateur saturé ou un journal d'incidences implicites, puis
   prouver et tester son équivalence à Gamma sans matérialiser la mosaïque.

Le raccourci « une coquille représente tous ses sous-simplexes » ne peut pas
servir de troisième voie sans ce théorème.

### Q1.2 — Sémantique des ex æquo et multifusions

La coupe fermée au niveau $a$ contient simultanément toutes les cellules dont le
niveau vaut $a$. L'unité normative est donc le **lot complet d'un même niveau
exact**, avec ce protocole :

1. figer les composantes et racines de la coupe stricte `< a`;
2. activer toutes les facettes nouvelles $N_a$;
3. projeter toutes les hyperarêtes directes et attaches valides du lot;
4. calculer les composantes du lot entier;
5. pour chaque composante, compter les racines strictes distinctes : zéro donne
   une naissance, une une continuation, au moins deux une multifusion;
6. valider tout le staging, puis committer tout le lot ou rien.

« Naissances avant fusions » n'est pas une sémantique supplémentaire. C'est une
implémentation licite uniquement si les racines créées au niveau $a$ ne sont
jamais relues comme racines **strictement antérieures** par une fusion du même
lot et si aucun nœud transitoire n'est publié. Hors position générale, le code
v2 ne porte pas cette preuve; son ordre interne ne doit donc pas devenir le
contrat v3.

Le choix `source = plus petit index de catalogue` est une convention de
sérialisation, pas un théorème topologique ni une attribution canonique des
décès à un événement. Pour une comparaison inter-backends, il faut soit une
clef contributrice canonique indépendante de l'ordre d'émission, soit définir
explicitement cet index après canonicalisation complète du catalogue. La
comptabilité O5 rappelle qu'un décès dans une composante de lot peut ne pas être
attribuable canoniquement à une hyperarête particulière.

Le tri doit comparer les niveaux exacts. La borne de 384 bits est démontrée pour
la clef de l'axe triangulaire u16, pas pour tout niveau de tout support : égalité
de lot et ordre reposent sur le rationnel exact, une borne propre démontrée ou
la multiprécision. Une clef secondaire stable peut ordonner les records
**dans** le lot pour la sérialisation, jamais scinder le lot mathématique.

### Q1.3 — Les `members` suffisent-ils pour la couverture ?

Le théorème 2 apporte une simplification positive : si une composante exacte de
$\Gamma_k(X,r)$ est connue, son union d'identifiants est déjà
$C^{\mathrm{discret}}$. Il n'est pas nécessaire de refaire une dilation
géométrique indépendante pour obtenir les points couverts.

En revanche, le pool `members` des seules sphères critiques ne prouve pas que
cette union est complète. Un lot `q=1` peut ajouter une facette et de nouveaux
points sans créer de nœud de forêt; une incidence silencieuse peut modifier la
couverture avant une fusion ultérieure. C'est précisément pourquoi la section 7
de la spécification exige un journal `coverage_delta` et interdit de supprimer
une hyperarête seulement parce que le DSU ne change pas.

**Contrat minimal :** pour chaque lot et chaque composante, journaliser les
facettes et incidences activées, y compris lorsque le DSU et l'union des points
ne changent pas; en dériver ensuite le delta d'identifiants pour `q=0` et `q=1`;
comparer état Gamma et couvertures aux coupes ouverte et fermée. Les `members`
peuvent alimenter ce journal, mais ne le remplacent pas.

## Q2 — Porte exacte des supports quatre

### Q2.1 — Diagnostic de la porte en temps constant

Pour un triangle non collinéaire $T$, un quatrième point non coplanaire fixe
bien un unique centre sur son axe. Mais quatre préconditions sont obligatoires.

1. Si la normale $\nu$ n'est pas unitaire, la formule est
   $r^2=r_T^2+t^2\lVert\nu\rVert^2$; la formule sans facteur suppose que $t$
   est une distance signée. Le cas $\langle w-c_T,\nu\rangle=0$ doit être
   classé exactement : point coplanaire constant, jamais division.
2. La borne de Jung support quatre utilise le diamètre $D(U)$ du quadruplet,
   donc les six distances, et la circumboule doit avoir été certifiée comme
   miniboule propre de support quatre. Un diamètre du germe ou du seul triangle
   ne suffit pas.
3. Une `tangent_bound` n'est transférable ni d'un sommet à un autre ni d'une
   paire à une autre. Si le certificat est attaché à une paire $e$ et prouve un
   majorant pour toute boule utile contenant $e$, seuls les certificats des
   paires réelles $e\subseteq U$ peuvent participer au rejet. Une valeur
   inconnue est `+infini` ou `unresolved`, jamais une petite borne par défaut.
4. Toutes les comparaisons portent sur les carrés rationnels exacts; calculer
   `sqrt`, puis comparer en flottant, retire le certificat.

Sous ces préconditions, dépasser **une** borne applicable suffit à rejeter le
porteur. Prendre un minimum est licite seulement si chaque entrée du minimum
est correctement liée à ce porteur. L'appliquer aux deux nouveaux sommets avec
des certificats que leur germe n'a jamais établis ne l'est pas; c'est une cause
plausible des quatre supports perdus.

Le défaut causal n'est toutefois pas reçu tant que le code expérimental et les
identifiants canoniques exacts des quatre omissions ne sont pas journalisés.
La prochaine fixture doit publier, pour chaque support manquant : les quatre
`PointId` de l'entrée réellement classifiée, $D(U)$, la normalisation de
$\nu$, le numérateur et le dénominateur de $t$, chaque certificat consulté et
l'inégalité exacte qui a rejeté. Tester séparément les ablations Jung, tangente
et normalisation localisera la faute au lieu de la deviner.

### Q2.2 — Ce qui est réellement un préfixe sur l'axe

Pour un point $x$ et un centre $c_T+t\nu$ avec $\nu$ unitaire, la puissance
relative à la sphère du pinceau est affine :

$$h_x(t)=r_T^2-\lVert x-c_T\rVert^2+2t\langle x-c_T,\nu\rangle.$$

Chaque point non coplanaire possède donc un unique paramètre d'événement, et le
**prochain** événement au-delà d'un curseur est le minimum exact, avec tous ses
ex æquo. C'est exactement ce que la baseline à deux scans peut qualifier. Un
cap de rayon fixé donne aussi un préfixe en $\lvert t\rvert$.

En revanche, le rang ou l'admissibilité shallow ne sont pas monotones. Du côté
parcouru, certains points entrent dans la boule; des points du côté opposé,
déjà intérieurs à $t=0$, peuvent en sortir. Le niveau est une marche signée,
pas une fonction croissante, et l'ensemble `rank <= s_max` peut avoir plusieurs
intervalles. Bon centrage, ownership et diamètre varient également avec le
quatrième point. La phrase « les quatrièmes admissibles forment un préfixe »
est donc **réfutée comme justification générale** tant qu'un lemme additionnel
n'exclut pas ces sorties.

L'unité sûre est
`(triangle, direction, current_t, shell, interior, level, cursor)`. Une requête
rend le prochain lot complet; l'état est mis à jour; les requêtes se répètent
jusqu'à un certificat terminal de rayon, d'index ou de domaine. Un seul
`neighbour_along` par triangle ne rend que le premier lot.

Le moteur de `neighbour_along` n'est pas réutilisable « tel quel » depuis un
triangle nu : l'API courante attend un `Vertex`, une fermeture, un apex
non coplanaire et l'intérieur transporté. Sa comparaison exacte de paramètres,
son lot complet d'ex æquo et son replay exhaustif sont en revanche les bonnes
briques de vérité pour une nouvelle porte bornée. Il faut encore : état initial
à $t=0$, points coplanaires constants, mise à jour entrée/sortie, ownership
entre les quatre faces et reçu terminal.

**Décision pour le plan 50 k :** remplacer « route retenue » par **candidat de
recherche**. Écrire d'abord un oracle d'axe borné qui énumère tous les événements
et compare l'ensemble complet des supports quatre; seulement ensuite chercher
un préfixe certifié par rayon ou par index. Sans index, un scan $O(n)$ par
événement ne ferme aucun SLO.

## Q3 — Unité transactionnelle du fold

### Q3.1 — Lot minimal et ordre interne

Oui : l'unité minimale est le lot entier d'un niveau exact, naissances et
hyperarêtes de fusion comprises. La règle normative est le passage atomique de
la coupe stricte à la coupe fermée décrit en Q1.2. Une séquence
naissances-avant-fusions est une convention d'exécution acceptable seulement si
elle est observationnellement équivalente à ce quotient simultané.

La fixture décisive mélange dans un même lot : une composante purement nouvelle,
une continuation `q=1`, une multifusion `q>=2`, une chaîne passant par un
carrier nouveau et deux records incidents placés dans des runs différents.
Toutes les permutations doivent produire le même transcript; une faute tardive
doit laisser l'état pré-lot bit à bit inchangé.

### Q3.2 — Parallélisme licite

La frontière antichaîne partitionne le **producteur reverse-search**; elle ne
partitionne pas les classes du DSU du fold. Elle n'autorise donc aucun commit
indépendant de lots ou de fragments qui pourraient partager une racine stricte,
un carrier latent ou une hyperarête.

La baseline sûre est : lots séquentiels par niveau exact; dans un lot, validation
et projection parallèles; fermeture sur le multigraphe complet; puis
classification et sérialisation parallèles des composantes désormais prouvées
disjointes. Les nouveaux identifiants sont alloués par scan stable et le commit
reste unique.

Une partition plus fine du **staging** est possible uniquement après
construction d'un graphe de conflits sur les racines strictes, handles nouveaux
et records. Ses composantes connexes peuvent être calculées indépendamment;
elles ne deviennent pas des commits séparés, car le lot conserve son contrat
tout-ou-rien. Calculer ce graphe est déjà une partie de la fermeture du lot. La
v3 ne doit pas annoncer un parallélisme inter-lots ou pré-clôture avant d'en
mesurer le bénéfice et de tuer un mutant qui coupe `R1--N--R2` entre deux
tâches.

## Q4 — Contrat du repli CPU multi-cœurs

La discipline proposée est nécessaire, mais la couverture et l'exactly-once
restent à ajouter. Le contrat scientifique minimal exige : domaines de tâches
disjoints et couvrants; commit ou rollback entier; multiensemble brut invariant;
clef totale exacte; tri/merge conservant les multiplicités; sérialisation sans
padding indéterminé; IDs après ordre global par scan stable.

Il faut ensuite séparer deux identités.

- Le **digest scientifique global** porte seulement les records canoniques. Il
  exclut `task_id`, worker, cœur, tentative et ordre de scheduling; il doit être
  identique pour 1, 2, 48 threads et pour une repartition licite.
- Le **ledger de replay** engage `cloud_epoch`, digest du nuage, domaine logique
  immuable, graine ou racine de sous-arbre, curseur parent, version/configuration,
  tentative, statut, comptes et digest canonique du run avant merge.

Le `task_id` peut identifier un segment authentifié; il n'est pas nécessaire de
le répéter dans chaque record. En cas de donation, le ledger prouve que l'union
canonique des domaines enfants égale le domaine parent. Un replay isolé compare
alors son multiensemble trié et son digest de segment; le merge final ignore le
découpage. Inclure la tâche dans la clef publique casserait l'indépendance au
nombre de threads; omettre toute provenance rendrait le replay invérifiable.

## Q5 — Validateur F0

Le patch a été fait par l'auditeur puis relu, testé et committé par Claude sous
`f3802bd`. `validate_regular_source` construit désormais l'ensemble des handles
stricts distincts, refuse un doublon strict nommé et compare sa cardinalité à
deux. La fixture `duplicated-strict-handle` vise directement le validateur avant
projection; elle complète le rejet déjà présent dans `resolve_batch`.

Les exécutions `python3` et `python3 -O` rendent toutes deux : 2 168 cas F0,
1 916 acceptés, 252 rejetés, 13 fixtures ciblées, 9 invalides, 11 mutants tués
et `Gate_D_F0_kernel=PASS`. Ce crédit ferme exactement Q5; il ne ferme pas les
autres obligations du fold produit.

## Réaudit des fermetures revendiquées avec les questions

Les progrès suivants sont réels et doivent être conservés : cover autonome avec
trois lanes; quatre planchers et disclaimer reçus; crash par signal refusé;
parseur arithmétique strict; comparaison canonique du catalogue et des forêts;
mutant shell-order; rejet fail-closed des deux corruptions topologiques reçues;
fixture owner signée qui tue le mutant non signé; cardinal coplanaire 19
confirmé indépendamment; extension de `same_catalogue` à tous les champs
déclarés de `Catalogue` au commit `1f0db40`.

Au snapshot `f3802bd`, quatre formulations restaient à corriger :

1. le validateur de forêt n'est pas « total » tant que source, tranche de pool
   et profondeur/concaténation récursive ne sont pas reçues;
2. `judge_seconds > 0` reçoit l'accumulation du timer, pas l'exécution des
   comparaisons du juge;
3. la ligne « ordre exécuté » répète l'option; le mutant qui force la mauvaise
   branche laisse les portes ciblées vertes;
4. l'égalité des diagnostics par défaut ne définit pas leur sémantique backend,
   et `catalogue_bytes`/`forest_bytes` mesurent seulement certains buffers
   dynamiques, pas tout le payload public.

La vérité coplanaire 19 a été recertifiée par `brute_catalogue` dans une copie
temporaire; la porte permanente grave encore seulement cardinalité et statut.
Le résultat est positif, mais une substitution de sphère à cardinalité constante
n'est pas reçue par ce témoin isolé.

**Réponse ultérieure `f102d42`.** La branche réellement entrée est désormais
observée et son mutant rougit; sources et tranches du pool hors plage sont
détectées; le libellé d'octets est borné aux buffers dynamiques; toutes les
portes négatives CMake exigent leur code contractuel. Deux réserves demeurent :
`judge_comparisons` précrédite `expected.size()` avant les recherches, de sorte
qu'un mutant sans différentiel sort encore 0; `max_depth` ne borne pas la
signature récursive, et une chaîne saine de 100 000 nœuds termine par
`SIGSEGV`. La solution proposée pour dépasser le simple constat multiplicitaire
est dans
[`NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md`](NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md).

## Ordre de travail conseillé à Claude

1. Corriger les claims documentaires ci-dessus et renommer la route arité quatre
   en candidat jusqu'à la porte d'axe exhaustive.
2. Sceller le contrat strict--fermé du lot et le journal `coverage_delta` avant
   de déplacer le fold sur GPU.
3. Produire la fixture exacte des quatre supports perdus, puis l'oracle d'axe
   complet; ne réintroduire aucune coupe tangente sans provenance par porteur.
4. Définir les deux digests du moteur de tâches et recevoir le replay CPU à un
   thread contre plusieurs threads.
5. Mesurer 50 k/G4 seulement après ces portes CPU. Le débit GPU ne peut pas
   décider une sémantique encore ouverte.

GCP non utilisé.
