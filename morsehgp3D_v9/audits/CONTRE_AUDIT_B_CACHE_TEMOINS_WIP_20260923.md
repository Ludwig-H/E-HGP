# Contre-audit B — cache de nœuds témoins q3/q4 en chantier

23 septembre 2026. Lecture **WIP non commitée** du worktree développeur
sur `e0ae05a7` : `q34_witness_search.cpp` SHA-256 `bfeada0bf3a7…`,
son en-tête `64e47df3f7e2…`, `wspd_q34.cpp` `cb83bf12a168…` et
`tower_chain.hpp` `d67493a9db41…`. Aucun chrono, reçu, G4 ni exposant
de croissance de cette version n'est acquis. Cette note complète la
[lecture de la preuve conjointe](CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md).

## Ce que le cache peut réellement économiser

La recherche complète sur une paire `(a,b)` peut conserver jusqu'à
`(K−1)+(K−2)=2K−3≤17` nœuds qu'elle a admis comme témoins stricts.
Pour la prochaine paire ayant la même extrémité `a`, ces nœuds sont
**réévalués exactement** avec les nouvelles bornes ; un nœud admis
crédite seulement les voies qu'il portait. Dans le parcours interne,
les nœuds tracés forment une antichaîne **par voie** : dès l'admission,
le bit de la voie est retiré avant la descente. Sous cet invariant, le
cache ne peut rejeter qu'une voie que la recherche Affine complète
aurait elle aussi rejetée. La lecture n'a pas trouvé de fausse
exclusion sur ce chemin interne.

Le cache s'exécute dans `Engine::edge`, **après**
`expanded_pairs++`, avant la recherche complète et le cover. S'il
rejette toutes les voies, il saute leur recherche et le cover ; mais
**le filtre Affine existant aurait sauté le même cover**. Par rapport
à la baseline actuelle, les masques finaux, `rejected_pairs`,
`cover_builds`, `cover_sites`, `dead_form_sites`, sorties et tour doivent
donc rester identiques cache on/off. Seul le coût des recherches de
témoin par paire peut baisser. Cette variante n'élimine ni les 12–33 M
paires développées ni le chargement de 1,8–9,3 Md formes observés dans
R3 ; ne pas la présenter comme fermeture du verrou sous-quadratique.

## Précondition non imposée à l'API publique

La fonction publique `q34_cached_witness_rejections` accepte un
`span<Q34WitnessNode>` arbitraire. Elle contrôle l'index du nœud et les
bits de voie, **pas** la disjonction des plages par voie annoncée dans
son commentaire. En donnant deux fois le même nœud feuille contenant
un témoin strict unique à K=3, voie q3 (`T3=2`), le code crédite deux
sites et peut renvoyer un faux rejet. Contre-fixture **exécutée
localement** sur une compilation temporaire du WIP, hors dépôt puis
nettoyée : `a=(0,0,0)`, `b=(10,0,0)`, témoin `z=(5,0,0)` isolé en
feuille 3. À K=3, voie q3, la recherche complète laisse q3 ouverte
(`full_open=2`) ; une entrée `{3,2}` donne rejet `0`, deux copies de
cette entrée donnent rejet `2`. Au milieu, `4H_min=100>0` et `Xi=0`,
mais il n'y a qu'**un** site distinct. Le chemin de production
ne forge normalement pas ce span ; le problème est le contrat de
l'interface exposée. Préférer une trace opaque, possédée et liée à
l'identité de l'index, ou valider antichaîne et unicité par voie avant
crédit. Si l'API reste publique, ajouter un refus causal de doublon,
d'ancêtre/descendant et de trace d'un autre index. Cette exécution
temporaire n'est pas un test archivé du produit.

## Porte de mesure avant le prochain G4

`ChainOptions::q34_witness_cache` devient `true` par défaut dans le
WIP, mais `bench/tower_probe.cpp` n'a ni bascule CLI, ni champ d'option
JSON, ni compteurs du cache. `ChainLedger` ne transporte pas
`witness_cache.{queries,node_tests,q3_rejections,q4_rejections,
full_rejections}` ni `witness.cache_rejected_pairs` ; le worker G4 v5
ne connaît pas cette variante. Une future campagne se retrouverait
incapable de l'identifier ou de l'ablater proprement. **Compléter
CLI→JSON→ledger→validation hôte avant toute nouvelle dépense G4.**

Une ablation appariée cache on/off à mêmes `s,K,W`, mode Affine et
certificat de voies mortes doit comparer toute la chaîne et les masses
invariantes citées plus haut, puis publier recherches évitées,
`pairs.node_visits`, nœuds retestés, CPU/mur, RSS et tailles de trace.
Une trace interne fait au plus 17 entrées à K10 ; son empreinte est à
mesurer pour clôture comptable, mais le verrou de fond reste ailleurs.
Les portes WIP actuelles utilisent des petits oracles et des mutants de
bornes ; elles ne sont pas encore cette ablation ni une preuve de gain.

Le mutant WIP `witness_cache_endpoint_admitted` est **équivalent** sur
entrée valide : remplacer `h.minimum4<=0` par `<0` ne fait entrer que
`h.minimum4=0`; alors `h4_squared=0`, tandis que `Xi.high≥0` est une
somme de carrés, si bien que le test suivant
`alpha*h4_squared<=16*Xi.high` continue toujours sans crédit. Il ne peut
être tué par une réponse géométrique fausse. Rejeu du binaire mutant
WIP : code `0`, `status=PASS`, 23 757 checks, alors que
`mutants.json` attend `1`. Le remplacer par une
mutation qui force réellement un crédit à `H_min=0`, et archiver une
fixture causale, avant de compter cette porte parmi les preuves.

## Reprise WIP v6 après cette alerte

Le développeur a ensuite remplacé ce mutant et ajouté une porte dédiée
`q34_witness_cache_gate.cpp` SHA-256 `b9509ac1cd46…`. Rejeu local :
24 804 comparaisons avec la même paire, 24 804 avec une autre paire,
7 556 rejets croisés et 4 787 traces portant séparément les deux voies.
Les **trois mutants désormais présents** sont tués par réponse
géométrique erronée. Le survivant équivalent ci-dessus est donc un
épisode WIP historique, non le verdict sur la nouvelle porte.

La sonde a désormais `--lever=NAME=0|1`, le JSON `options.levers` en
schéma v6, et trois compteurs cache propagés jusqu'au ledger ; le worker
et le plan sont passés au schéma v6/v4. Cela résout l'absence de levier
exposée plus haut, **pas** le besoin d'une ablation cache seule à
trame/K/s/W identiques : la porte chaîne compare encore tous les leviers
ON à tous OFF en changeant aussi W2→W1. La voie publique à span forgé
reste inchangée et la contre-fixture de doublon reste valable.

Le protocole v6 WIP n'est pas encore qualifié : son faux producteur
`tower_selftest_v9.py:76` émet littéralement
`mhgp9_tower_probe_v5` tandis que le worker exige `v6`. Le test ciblé
`Protocol.test_probe_and_time_validation` échoue immédiatement sur
`probe schema/status` ; le remplacement v5→v6 **en mémoire** le fait
passer, sans constituer une porte complète. En outre,
`tower_session_v9.py` n'a pas encore corrigé les quatre défauts de
réception du [contre-audit v5](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md).
Pas de nouvelle dépense G4 avant correction, selftests normaux/`-O`,
relecture du snapshot et ablation cache seule.

## État après publication sur `main`

Le produit est publié sous `7f64a279`, puis le faux producteur a été
corrigé sous `6345a985`. Rejeu indépendant du test ciblé sur ce dernier
commit : **1/1 PASS** en 0,026 s ; l'échec de schéma ci-dessus est donc
historique. Il ne vaut ni les 20 mutants du selftest complet ni une
réception G4. La nouvelle source de cache SHA-256 `87655330ec4d…`
réécrit algébriquement son inégalité stricte, sans vérification de
l'antichaîne : la contre-fixture du span dupliqué reste applicable.

Le statut courant reste **non qualifié pour une nouvelle dépense G4** :
les défauts de réception hôte v5 sont inchangés, aucune ablation isolée
cache on/off à mêmes trame/K/s/W n'est archivée, et les gains locaux
70/61/67 % de recherches évitées annoncés par le développeur n'ont pas
encore de reçu apparié versionné. Le cache est une optimisation du
filtre de paire, pas une réduction démontrée des covers ou formes.

## Correctif API publié `a1d7a9bc`

La révision relue localement sous `23074987`, puis publiée sur `main`
sous `a1d7a9bc`, refuse désormais, **avant tout crédit**, deux nœuds dont les
plages se chevauchent sur une même voie. Le contre-exemple de la feuille
dupliquée et sa variante ancêtre/descendant renvoient maintenant
`invalid_argument`, tandis que la porte des traces valides demeure verte.
Une trace d'un autre index, si ses indices et plages recalculés dans
l'index courant restent disjoints par voie, n'introduit pas de faux
crédit : ses boîtes et populations sont elles aussi relues dans cet index.
Le commentaire « doit provenir d'un seul appel tracé » est désormais
plus restrictif que la condition mathématique réellement vérifiée.

La validation ajoute un coût `O(m²)` **à chaque paire** retestée ; pour
une trace interne `m≤17`, jusqu'à 136 comparaisons de plages, non
comptées dans `witness_cache.node_tests`. Sur des millions de paires,
mesurer ce coût dans l'ablation cache seule. Une trace opaque certifiée
une fois à sa production éviterait cette revalidation répétée tout en
gardant l'API fail-closed. Le correctif de réception v6 séparé est
[contrelu ici](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) ; il ne ferme
pas encore la réception de tous les champs de garde archivés.

## Coût caché quantifié sur le reçu G4 R5

Sur les six premières répétitions du
[reçu R5](../receipts/g4_tower_r5_20260923/README.md), le cache est
interrogé **123,228 millions** de fois, pour **554,501 millions** de
`node_tests` géométriques. La double boucle de validation précède ces
derniers. Pour une trace interne de taille `m≤2K−3` (7 à K5, 17 à K10),
elle exécute `m(m−1)/2` tests de recouvrement de **masques** à chaque
appel, puis compare les plages seulement si les voies se chevauchent.
Ne pas appeler la borne suivante « comparaisons de plages » : leur
nombre est conditionnel au partage de voie. Comme `node_tests_i≤m_i`,
le total des **tours de
boucle** a pour minorant entier
`Q·C(floor(T/Q),2)+(T mod Q)·floor(T/Q)`, où `Q=queries` et
`T=node_tests` ; son majorant interne est `Q·C(2K−3,2)`.

| R5, première répétition | Requêtes Q | Tests géométriques T | Tours de validation : minorant–majorant |
| --- | ---: | ---: | ---: |
| 000000/K5 | 21,609 M | 61,564 M | 58,300–453,793 M |
| 000100/K5 | 10,373 M | 30,473 M | 29,828–217,825 M |
| 000200/K5 | 20,411 M | 62,839 M | 66,050–428,636 M |
| 000000/K10 | 27,214 M | 148,659 M | 335,086 M–3,701 Md |
| 000100/K10 | 14,694 M | 81,159 M | 185,385 M–1,998 Md |
| 000200/K10 | 28,927 M | 169,806 M | 415,130 M–3,934 Md |

Les six cas agrégés impliquent **au moins 1,090 milliard** de tours
de validation, possiblement jusqu'à 10,734 milliards, **non comptés**
dans `node_tests`. En outre, chaque entrée géométriquement testée porte
au moins une des deux voies ; parmi `t` telles entrées, au moins
`C(ceil(t/2),2)+C(floor(t/2),2)` couples partagent une voie. La même
minimisation entière sur `T/Q` donne ainsi **au moins 424,330 millions**
de comparaisons d'intervalles sur les six cas (majorant interne
**5,005 milliards**, avec au plus 4/3 nœuds par voie à K5 et 9/8 à
K10). Ce sont des bornes combinatoires, **pas** un temps mesuré : les
conditions de
masque, la hiérarchie des plages, les caches CPU et le compilateur
peuvent rendre un tour peu coûteux. Instrumenter séparément
`validation_pair_iterations`, `interval_compares`, histogramme de
longueurs, constructions de traces, puis ablater au même
trame/K/s/W avec RSS et sortie FULL.

La voie publique acceptant un span arbitraire doit continuer de
vérifier les doublons, ancêtres et indices hors domaine **avant tout
crédit**. Une voie interne pourrait recevoir un ticket opaque immuable,
fabriqué uniquement par une recherche tracée réussie (ou une factory
validant l'antichaîne une fois), lié par propriété à l'index utilisé :
elle retesterait la géométrie sur chaque paire sans refaire le carré
de validation. Prouver impossibilité de forger/modifier/croiser l'index
et comparer ticket/public/oracle sur mêmes et autres paires. Cela reste
une réduction de contrôle, **pas** des 12–33 millions de paires
développées ni des milliards de formes ; le certificat de groupe avant
expansion garde la priorité architecturale.
