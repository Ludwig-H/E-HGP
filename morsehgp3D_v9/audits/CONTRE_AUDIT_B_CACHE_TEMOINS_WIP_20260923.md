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
