# Contrelecture bornée : resolver géométrique statique et raccord temporel

11 septembre 2026. Agent `static_resolver_review`, à la demande de ROOT.
Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucun code actif, fichier d'auditeur, index Git ou
état GCP modifié. Aucune compilation ni mesure de performance par cette note.

## Verdict de conception

**Favorable, sous census complet et avec raccord temporel inchangé.** Le delta
minimal ne construit ni Gamma, ni toutes les facettes, ni un nouveau merge-tree :
il remplace la valeur de sortie du resolver `jeton de composante` par un
`BallId terminal admis`, prépare ces réponses hors calendrier, puis ne fait que
normaliser `root(anchors[terminal], prior_count)` avant chaque fermeture de lot.
La preuve ne donne aucune garantie universelle sous-quadratique ni un gain déjà
mesuré ; les chaînes exactes, le census et la sortie restent à payer.

Sources lues intégralement : header FULL, `anchor_meb.hpp`, note de séparation du
second auditeur, note et README du reçu statique. Parties utiles de
`local_plateau.hpp`, `parallel/pool.hpp`, porte FULL et oracle statique également
inspectées. Les deux lecteurs publics du reçu statique passent, Python normal
et `-O` : 32 commandes conservées, 7 mutants, 75 captures inchangées, 80 fichiers
du manifeste. Cette lecture de reçus ne constitue pas une réexécution de
l'oracle ni une qualification de l'implémentation future.

## 1. Delta minimal proposé

1. Garder `validate_catalogue()` et son tri canonique `(niveau exact, BallKey)`.
   Il certifie notamment `arity == q_min` dans les coquilles supplémentaires.
   Après cette validation, le prédicat statique utilise directement les champs
   validés : `p + arity - 1 <= K && K <= p + u`.
2. Extraire la création des représentants de `prepare_block()` dans une fonction
   pure : même quotient local, mêmes contributions, mêmes représentants stricts.
   Aucune requête `root`, aucun `prior_count`, aucune ancre. Les `ShellTable`
   restent immuables ; `rank()` est `const` et possède ses temporaires.
3. Extraire `meb`/`intruder`/descente dans un resolver ayant seulement des
   références constantes vers l'index, le census, l'index de clés, et K. Pile,
   chaîne, cache facultatif et compteurs sont propres au worker.
4. Résoudre chaque représentant vers un `BallId` admis ; associer les résultats
   à leur bloc par indices fixes, indépendamment de l'ordre d'exécution. Ne pas
   émettre un jeton de composante, une racine finale ou un simple hash.
5. Après jointure complète des workers, rejouer le calendrier actuel. Pour
   chaque lot, normaliser tous ses terminaux via les ancres pré-lot, trier et
   dédupliquer les **racines obtenues**, puis appeler `close_lot()` inchangé.
   Installer toutes les ancres du lot uniquement après fermeture entière.
6. Garder le calcul vertical et la construction du certificat existants. Pour
   un premier delta, K1 peut rester intégralement nominal : sa géométrie ne fait
   déjà aucune MEB. Cette asymétrie évite un identifiant terminal ambigu.

Le test de terminal est exactement : recherche BallKey complète, vérification
du niveau associé, puis admission à K. La seule présence dans le catalogue
commun n'est pas suffisante. Sinon, continuer l'échange strict d'intrus ; une
absence d'intrus sans terminal admis reste `missing_weak_terminal`, pas une
naissance et pas un saut silencieux du représentant.

## 2. Les gardes sémantiques à conserver

- **Avant le consommateur, sur toutes les routes.** Vérifier le niveau terminal
  strictement inférieur à celui du bloc consommateur, y compris lors d'un hit
  de cache et d'un hit de semis. Le nominal n'avait pas besoin de ce contrôle
  sur cache car il ne semait qu'après fermeture ; le semis statique peut voir
  tout le futur. Chaque MEB de descente doit garder le contrôle strict existant.
- **Sémantique de cache.** La valeur actuelle est un jeton temporel `u64` ;
  elle ne se convertit pas en `BallId` même lorsque les valeurs numériques
  coïncident. Un cache statique vérifie la facette entière et K (explicitement
  ou par instance liée à K), et stocke uniquement un terminal admis.
- **Semis.** Le semis `I union U` est valable uniquement si `K == p + u`,
  puisque c'est alors une facette K et sa MEB certifiée est B. Une seed-table
  statique peut être construite avant tout calendrier ; sa consommation reste
  soumise au contrôle strict du niveau et à la normalisation temporelle.
- **Niveaux égaux.** Une étape interne de descente peut garder le même niveau
  et la même clé avec coquille sélectionnée diminuée d'un. Ne pas remplacer la
  règle `next.level <= previous.level` par une diminution strictement imposée.
  En revanche aucune arête terminal-consommateur n'est de niveau égal.
- **Identités.** Plusieurs terminaux distincts peuvent déjà avoir le même
  parent pré-lot. Dédupliquer des BallIds avant normalisation peut réduire du
  travail, mais ne remplace jamais la déduplication des racines normalisées.
  Des ensembles de points égaux ne sont pas des identités de composantes.
- **Blocs sans représentants.** Ils existent et doivent rester dans le
  calendrier, avec activation et contribution. K=n peut n'avoir aucune arête
  mais porte une vraie naissance au niveau de la boule totale, pas à zéro.
- **Portails silencieux.** Conserver l'ancre des continuations inertes et la
  contribution datée des croissances unaires. L'absence de nœud public nouveau
  n'autorise pas à éliminer le bloc du calendrier.
- **Verticale.** Une naissance supérieure utilise la même boule à K-1, à la
  coupe fermée du niveau de naissance. Une fusion normalise les images de
  tous ses parents à la coupe fermée de fusion. Ni coupe ouverte ni racine
  finale ne conviennent.
- **Permutation.** Permuter les jobs ou les threads doit conserver l'objet
  canonique. Permuter les PointIds peut changer le représentant choisi par
  le quotient local ; changer la règle d'intrus peut changer le terminal.
  Comparer les coupes/parents/couvertures/verticale après remappage, pas les
  terminaux ni compteurs de travail comme s'ils étaient des invariants.

## 3. Architecture mémoire et parallélisme

Noter A le nombre de blocs programmés et R le nombre de représentants, dans la
fenêtre effectivement préparée. Une représentation CSR suffit : un descripteur
par bloc `(BallId, offset, count, contribution, interior)` et R BallIds terminaux.
La date d'une arête est implicite dans son consommateur ; ne pas recopier un
`ExactLevel` par représentant. Les programmes existants donnent l'ordre exact.
Les offsets doivent couvrir les tailles représentables avec contrôles de
débordement ; le type BallId ne sert pas à représenter un offset R.

On peut traiter un bloc par tâche et construire ses facettes dans une petite
chaîne locale ; cela évite de garder R tableaux de K indices simultanément.
Pour réserver les résultats sans course : compter les représentants par bloc,
faire un prefix-sum contrôlé, puis remplir des plages disjointes. Les plateaux
peuvent conserver leur petit plan local plutôt que refaire deux fois le même
quotient. Autre premier delta acceptable : un vecteur de terminaux par bloc,
plus simple mais avec davantage d'allocations et d'en-têtes ; le coût reste
O(A+R), mais cette notation ne suffit pas à le qualifier industriel.

Les résultats peuvent être préparés ordre par ordre, ou par fenêtres entières
de lots, puis relâchés après consommation. Une limite de taille de batch est un
paramètre d'ordonnancement, pas un plafond de validité : un grand lot peut être
résolu en sous-batches, mais ne doit être fermé qu'après toutes ses réponses.
Ne pas construire simultanément toute la tour si les buffers des ordres déjà
consommés sont morts. Le futur chemin GPU a exactement le même contrat de
terminal, mais devra inclure transferts, compaction des chaînes et latence hôte.

**Cache et travailleurs.** Répliquer le cache actuel `next_pow2(16*n)` dans
chaque worker coûte O(T*n), pas O(n). Préférer une seed-table immuable partagée
et une mémoire totale explicite pour les caches évictifs privés ; ou une route
sans cache lors de la qualification initiale. La capacité et les hits sont
des mesures, jamais une hypothèse de validité. Un cache partagé mutable exige
une publication atomique/cohérente de la clé complète et de sa valeur : une
simple écriture atomique du BallId ne rend pas les tableaux de sites sans course.

La mémoire supplémentaire est donc O(A+R+n+T*(K+H+S)) pour plans, terminaux,
cache total et scratch, H étant la pile spatiale réelle et S le petit quotient
local. Ce n'est O(A+R) **au-delà de l'index/census/cache/scratch déclaré**, pas
une disparition magique de ces postes. En régime régulier, une boule fournit
au plus deux blocs et quatre représentants dans la tour ; les plateaux ont un
coût de quotient supplémentaire. A et R ne sont pas garantis O(n).

Chaque worker possède ses compteurs. Les sommes doivent être réduites avec
contrôles de débordement, `max_chain_steps` par maximum, et la capacité physique
par comptage des allocations réelles. Ne pas partager `FullBallStats`, `stack`
ou le cache mutable actuel entre threads. Les stats ne portent pas l'empreinte
sémantique ; les hits et chaînes peuvent dépendre du scheduling.

## 4. Ressource, exceptions et non-publication

Le pool existant joint tous les threads avant de relancer une exception.
Cependant un lancement partiel de `std::thread` lève `std::system_error`, que
le wrapper `build_full_ball_tower()` actuel ne capture pas. La route parallèle
doit transformer ce cas précisément en échec ressource avec `orders` vide,
sans `std::terminate`, sans résultat partiel et sans worker restant vivant.
Ne pas attribuer indistinctement toute `system_error` à une allocation mémoire.

Les erreurs d'un worker invalident toute la tour, y compris les ordres déjà
préparés. Le contrat existant stocke `orders` uniquement après retour complet,
ce qui fournit la transaction extérieure ; conserver cette propriété. Tous
les workers sont joints avant destruction des références immuables. Un défaut
de ressource du cache facultatif doit désactiver le cache ; un défaut de
ressource du tableau de terminaux doit donner un échec ressource, pas une
réduction silencieuse du nombre de représentants.

## 5. Fixtures et mutants causaux prioritaires

| Piège | Fixture et observation indépendante | Mutant ciblé |
| --- | --- | --- |
| Présence globale sans K | Huit points `equal_radius_admitted_window`, Kmax6 ; cercle 4225 non admis à K4, consommateur 70058125/11236 | Enlever le test d'admission K ; exige un refus ou divergence nommée |
| Futur dans la géométrie | Construire les mêmes demandes avant tout calendrier, puis en ordre inversé | Consulter `anchors` pendant résolution ; terminal dépendant du planning doit être refusé |
| Consommateur strict, y compris cache | Facette semée dont le terminal est injecté au niveau du consommateur | Sauter la comparaison sur hits de seed/cache ; contrôle nommé, pas simple sortie plausible |
| Clé versus parent | Triangle `(0,0),(4,0),(1,3)`, niveau 9/2 | Compter les BallIds terminaux distincts sans `root` pré-lot |
| Fermeture atomique | Carré côté 2 : K1, une multifusion à quatre parents au niveau 1 | Installer une ancre ou fusionner après chaque bloc |
| Naissance sans arête | Deux points distants de 2, K2 naît au niveau 1 | Supprimer les blocs à R=0, ou activer tous les sommets à zéro |
| Verticale fermée | Carré, K3 naît au niveau 2 | Coupe inférieure ouverte ou racine finale |
| Croissance unaire | ABCZ : ajout de Z à K3 au niveau 25 ; également version doublée en lot groupé | Supprimer les contributions unaires |
| Portail inerte | `inert_ball_doubled_lot` | Ne pas installer les ancres des blocs sans action publique |
| Descente à rayon égal | Même fixture à huit points ; elle contient effectivement une étape à clé égale | Exiger une diminution stricte de rayon ou ne pas diminuer la coquille |
| Cache exact | Deux clés de facettes en collision, avec targets différents | Comparer le hash seulement ; réutiliser un token temporel comme BallId |
| Ordre des jobs | 1/2/4 threads, ordre normal/inverse/shuffle fixé | Écrire les réponses par ordre de completion plutôt que par indice de demande |
| Ressource du pool | Injection `launch_fail_after` à 0 puis après lancement d'un worker | Admettre un lancement partiel ou laisser échapper l'exception du wrapper |
| Transaction | Échec alloué/invariant au dernier représentant d'un lot et d'un ordre | Publier un préfixe de tour ou ne pas joindre un worker |

Le corpus C++ courant à 28 nuages reste nécessaire, mais sa fixture
`actual_equal_radius_descent` utilise Kmax4 : le cercle 4225 y est absent du
catalogue, et ne tue donc **pas** à elle seule le mutant « clé globale présente
mais K inadmissible ». Ajouter la variante Kmax6 de la fixture à huit points
est nécessaire pour le cas exact de la preuve. Les deux variantes PointIds et
ordre d'entrée doivent être conservées. Ajouter un cas singleton n=1 si ce
contrat n'est pas déjà porté par une autre porte.

Pour promouvoir la route statique au rôle de chemin actif : comparer les
28 nuages contre l'oracle indépendant, les deux politiques d'intrus, les
couvertures datées et la verticale, puis le corpus indépendant de l'auditeur.
Un seul digest apparié n'est pas la preuve de complétude du census. Mesurer
ensuite 8k/16k/32k avec stats de MEB, supports, intrusions, capacité et temps
complet ; distinguer le surcoût de matérialisation des terminaux du gain CPU.

## Sources épinglées au début de la contrelecture

| Fichier | SHA-256 |
| --- | --- |
| `src/forest/full_ball_tower.hpp` | `910f45baea1750b11d2b34f40c893c9d1a34f950705cdb127ffa226de60f7b2e` |
| `src/forest/anchor_meb.hpp` | `386072c8a02bbb836d0070a10e1421418a36d3f4024ade63b4c9014c5536f786` |
| `src/parallel/pool.hpp` | `5c20aabbe673e2baa1018bea893185592bc3b394d025eadd9be07542453befc6` |
| `src/forest/local_plateau.hpp` | `df56fbf33ea3088218174f88a646c65702d97a0366ec848887f1846ae7666f2e` |
| `audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md` | `9f4617298d0aac85a3540621d2b65a98821a6f040d7bec8f9386c2d015343e2d` |
| `receipts/static_anchor_graph_20260910/NOTE.md` | `417a97aba7baf8b6ea68eaad5385b1ce663c3f185a871b4acb6a84df593531a2` |

## Addendum : plan concret transmis par ROOT

Plan reçu pendant la contrelecture : option `static_threads=0` pour le chemin
nominal ; K>=2 collecte de `Request{Key, consumer BallId, occurrence u64}` via
un visiteur commun, tri et déduplication exacte des facettes, une résolution
par groupe, puis scatter de BallIds dans l'ordre initial. Semis par table triée
immuable `I union U` lorsque K=p+u. Aucun cache temporel ni cache répliqué par
worker. Le consommateur vérifie niveau strict et ancre présente, puis normalise.

**Favorable pour prototype privé.** Le visiteur commun réduit le risque de
désaccord des représentants entre les deux phases. Le tri unique supprime
également le besoin d'une table mutable concurrente. Points de contrôle :

- Une facette identique peut avoir plusieurs consommateurs ; ne pas les perdre
  pendant la déduplication. Chaque occurrence doit consommer son résultat à son
  niveau propre. Vérifier le niveau seulement pour le consommateur choisi comme
  représentant du groupe serait une porte insuffisante sur une requête malformée.
- Ne jamais trier ou regrouper seulement le hash de la facette. À K fixé, comparer
  les K indices triés ou la clé complète avec padding normalisé.
- Pour le semis, deux occurrences de la même facette doivent avoir le même
  BallId puisque leur MEB est unique. Un désaccord indique une corruption du
  census/plan, pas un choix libre entre deux valeurs.
- L'identifiant `occurrence` est l'index avant tri. Le scatter doit remplir
  exactement une case par occurrence, sans permutation accidentelle ni trou.
  Contrôler le domaine de chaque index avant écriture si l'API reçoit un plan
  externe ; si le plan reste interne, une fixture de permutation causale suffit.
- Les groupes peuvent être calculés dans n'importe quel ordre ; une sortie par
  ordre de terminaison des workers est fausse, même avec des valeurs valides.
- Compter R et les uniques séparément, puis les hits de semis et les appels MEB
  effectivement payés. Ne pas réutiliser l'identifiant de comptabilité du cache
  direct nominal pour cette route tri-unique.
- `sizeof(Request)` sera vraisemblablement 56 octets avec cette disposition,
  à mesurer plutôt qu'à déclarer. Les 72 307 407 représentants rapportés pour
  l'ensemble K1..10 du cas 50k représentent environ 4,05 Go de requests plus
  0,29 Go de scatter, avant groupes et capacités, si matérialisés simultanément.
  Le traitement et la libération par K réduisent ce pic ; ne pas annoncer ce
  total comme un pic effectivement mesuré ni comme le nombre d'un seul K.

Mutants adaptés précisément au plan : remplacer le comparator de clé par un
hash seul ; omettre K dans une table accidentellement partagée entre ordres ;
utiliser l'index après tri comme `occurrence` ; ne scatter que la première
occurrence d'un groupe ; décaler un terminal vers un consommateur de même niveau ;
supprimer le contrôle strict sur hit de seed ; omettre la normalisation des
terminaux ; exposer un préfixe après exception worker. Les gardes de non-vacuité
doivent inclure au moins une vraie facette répétée, un groupe à consommateurs
différents, un hit de semis et une permutation non triviale des indices de sortie.
