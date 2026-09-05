# Revue sémantique du producteur FULL relatif

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Aucun défaut concret identifié sur les octets `e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170`.** Le raccord des portails, des parents stricts et du certificat compact est cohérent avec l'autorité relative annoncée. Cette revue est statique : aucun build ni exécution du produit ou de ses tests. Les sources encore évolutives sont épinglées, et les textes du composant et du contrat effectivement retenus sont capturés dans [semantic_review.json](semantic_review.json).

Sources principales : [full_gabriel.hpp](../../src/forest/full_gabriel.hpp) et [CONTRAT_PRODUCTEUR_FULL_GABRIEL.md](../../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md). Les lignes citées visent le composant épinglé. Le contrat a reçu un addendum de couverture pendant la lecture : premier hash `519590ba3db0048705f243852d61fbc2ed548de2ca0d183cd59b95d4db8a8320`, version finale relue `bc619feea3c209d2fb2403b8c9f72efb67f3b08b965800c44cea157672779827`. Le composant est resté inchangé.

## 1. Autorité d'entrée et domaine mathématique

Le module contrôle un index authentique non vide, sans positions répétées, de taille au plus `INT32_MAX`, et `1<=K<=min(10,n)` (99–114). Les points sont résolus par une table locale PointId–indice géométrique ; ils ne sont pas confondus avec les rangs Morton (116–120, 209–217). Les tableaux internes publics d'un `CloudIndex` arbitrairement corrompu ne sont pas une entrée acceptée par le contrat.

La grammaire des records est contrôlée avant utilisation de leurs champs : cardinal K ou K+1, support de taille 2 à 4, identités distinctes connues, masque exactement plein, fraction positive ; les labels répétés sont refusés (219–236, avec `fold_event_ok`). Les arrays fixes sont donc utilisés avec au plus dix points par facette et onze par coface. `facet_without` est appelé avec un élément effectivement présent une seule fois. Le tri des records par label, puis des permutations chronologiques par niveau rationnel et label, rend le parcours indépendant de l'ordre physique des catalogues.

La validation n'établit pas la vérité géométrique des supports, des intérieurs ou des niveaux fournis, ni l'exhaustivité des deux catalogues. C'est conforme au statut littéral `kCompleteRelative`. Une entrée incomplète peut ne déclencher aucun portail manquant et néanmoins produire une forêt incorrecte vis-à-vis de Gamma ; son acceptation relative n'est pas une contradiction avec le contrat. Les octets de `ForestEvent` inutilisés ne constituent pas un format binaire canonique qualifié.

### Raccord de la paire de catalogues à l'inertie des blocs omis

La prémisse « tous les Gabriel de cardinal K et K+1 sont fournis, exacts et réguliers » suffit au raccord suivant ; elle n'impose pas la régularité géométrique de toutes les autres boules.

Soit B la MEB d'une coface non Gabriel Q de cardinal K+1. Posons I l'ensemble des intérieurs stricts globaux, E sa coquille, U un support minimal positif, $p=|I|$ et $s=|U|$. Son saturé contient au moins K+2 points. Si $p+s\leq K+1$, on peut compléter U∪I par des points de E pour obtenir G de cardinal K+1. G conserve B comme MEB et contient tous ses intérieurs : G est Gabriel. Mais le saturé possède plus de K+1 points ; sa coquille est donc strictement plus grande que U, et ne peut être un support essentiel indépendant. G contredirait la régularité du catalogue complet. Ainsi $p+s\geq K+2$.

Le [théorème d'inertie saturée 4.2](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#531-inertie-h_0-exacte-des-blocs-saturés-au-dessus-de-la-fenêtre-de-rang) s'applique : les K-facettes strictes du saturé forment déjà un graphe connexe couvrant tous ses points. Les contacts égaux entre blocs identifient leur MEB, les autres contacts sont pré-lot. Une directe régulière ne partage pas de facette égale avec un bloc non direct distinct, puisque son saturé n'a que K+1 points. Ces blocs ne cachent donc ni fusion ni point que le quotient des directes aurait omis.

Pour une facette non Gabriel F, le même remplissage au cardinal K exclut une boule irrégulière avec $p+s\leq K$. Si $p+s=K+1$, une extra-shell donnerait une Gabriel irrégulière au cardinal K+1 ; sans extra-shell, F a un seul intrus et rejoint sa directe de même niveau. Le reste est couvert par l'inertie précédente. Avec les minima Gabriel K réguliers, cela ferme les naissances FULL et leur couverture. Cette déduction complète la [preuve FULL](../receipts_gabriel_20260905/full_proof_review.md) sous l'autorité exacte de la paire de catalogues ; elle ne transforme pas une simple liste déclarée complète en preuve de complétude.

Une MEB réellement visitée peut encore être irrégulière dans un bloc inerte de haut rang. Le producteur la refuse localement ; l'inertie de son bloc ne certifie pas ce chemin par héritage. Aucun transfert automatique du domaine accepté E/F n'est utilisé.

## 2. Portails : géométrie et autorité terminale

`locate` consulte d'abord les alias historiques, normalise leur successeur et exige une racine d'identifiant strictement inférieur à `prior_count` (320–325). Les alias autorisés proviennent soit d'un minimum déjà actif, soit d'une directe traitée, soit d'un portail déjà abouti. Leur simple présence n'est donc pas une preuve par recouvrement ponctuel.

Une facette stricte inconnue déclenche un portail chargé avant le travail. Sa MEB est recalculée et comparée strictement au niveau cible ; le census d'intrus est achevé. Zéro ou un intrus donne le refus `full_gabriel_missing_prior_alias` (327–341), car le minimum ou la directe antérieure aurait dû installer la facette. À K1, une telle absence est directement refusée : toutes les racines ponctuelles ont été installées à zéro.

Le premier pas utilise correctement les deux témoins z,w (343–362). La clé devient Q=F+z, mais la boule reste B(F) et `next_intruder` vaut w. Le support est exprimé en indices géométriques stables ; son premier essentiel appartient encore à Q après tri. Le remplacer par w forme exactement R=(F privé de cet essentiel)+z+w. Les deux intrus sont stricts, distincts et étrangers à F ; le cardinal reste K+1. L'essentialité implique β(R)<β(F), puis le code vérifie cette décroissance sur la nouvelle MEB. Il n'utilise pas par erreur les anciens `sites` pour calculer R : `resolve` les remplit avant l'appel.

Le helper [silent_incidence.hpp](../../src/forest/silent_incidence.hpp) vérifie la coquille locale dans `miniball` (162–291), puis toute extra-shell étrangère dans `intruders` (294–335). Deux témoins suffisent pour une décision, mais le contrôle du bord ne s'arrête pas à leur découverte. `count=2` est saturé, non un census total. Après l'entrée du portail, un unique intrus est expressément autorisé : seule l'absence d'intrus hors catalogue échoue (376–378).

Le premier Q n'exige effectivement ni seconde MEB ni seconde requête spatiale. Toute nouvelle R exige sa propre MEB et, si elle n'est pas terminale, son propre contrôle de bord. Les cofaces successives partagent une K-facette et précèdent strictement le lot cible : leur terminal appartient à la composante recherchée de F. La stricte décroissance exclut un cycle sur le nombre fini de labels ; elle ne garantit pas la réussite dans un plafond donné.

Le terminal est recherché par le **label complet**, puis contrôlé par niveau exact, date strictement antérieure et ancre déjà installée (363–374). Il n'est pas confondu avec une autre boule de même rayon. Sa MEB vient d'être recalculée ; le census global terminal est volontairement remplacé par l'autorité du record exact et régulier. Son jeton historique est normalisé, puis sa racine est de nouveau contrôlée `<prior_count`. Le module conserve les tokens des directes même lorsqu'elles ne produisent aucun nœud : une continuation antérieure reste une autorité terminale utilisable.

## 3. Lots atomiques, minima et ordre terminal

Les niveaux sémantiquement égaux sont regroupés entre les deux catalogues (138–154). Dans `lot`, `prior_count` est pris avant le lot. La première passe ne relie que les anciennes racines des retraits essentiels dans une DSU locale (382–416). Les portails peuvent ajouter des alias vers ces anciennes racines et comprimer leurs chemins, mais aucun successeur global de fusion n'est encore créé : les composantes pré-lot restent fixes.

Le quotient local associe chaque ancienne racine à un seul groupe. Les groupes d'au moins deux racines deviennent les parents des multifusions ; leurs listes et les multifusions sont triées canoniquement (417–435). Une naissance du lot ne peut servir de parent : les feuilles sont installées seulement après ce calcul. Elles reçoivent leurs IDs avant les nouveaux nœuds internes, conformément au certificat compact.

Toutes les unions globales sont ensuite installées, puis seulement les ancres des directes et tous leurs alias sont normalisés vers l'état fermé du lot (436–456). Une directe dont tous les retraits aboutissent à une seule ancienne composante ne crée pas de nœud ; elle conserve néanmoins son ancre et ses facettes. `no_op_connections` compte de telles **directes**, pas des lots ou des composantes distinctes.

Deux sorties de points identiques restent distinctes si leurs ensembles de minima le sont : ni DSU, ni alias, ni successeur ne compare les couvertures ponctuelles. La couverture finale est reconstruite par le certificat sur les feuilles descendantes. La lecture structurelle déjà fermée n'est pas une deuxième décision géométrique.

À K1, les points du domaine sont installés dans un lot zéro avant toute connexion positive (126–135). Le certificat conserve la différence entre zéro ouvert et fermé. À K=n≤10, le seul label de minimum possible est X, et aucune coface valide de cardinal n+1 ne peut être résolue dans le domaine. La feuille terminale est donc conservée sans connexion. Les ordres K>n ou K>10 sont refusés, et aucune promesse sur une tour plus large n'est implicite.

Les permutations physiques des points ou des catalogues ne fournissent aucune identité algorithmique. Un renommage non monotone des PointId peut changer les IDs denses et le choix d'un essentiel ; sur un parcours accepté il conserve la forêt abstraite attendue, après transport des labels et des parents. Il ne promet pas les mêmes compteurs ou le même domaine de réussite si un autre chemin rencontre une dégénérescence hors fenêtre.

## 4. Compteurs, limites et exceptions

`charge` teste `count>=cap` avant l'incrément. Les sommes des records, les nœuds supplémentaires et les références parentales utilisent l'addition prospective bornée du certificat (112–114, 199–207, 431–437). Les plafonds valent zéro par défaut. Les itérations géométriques portent sur au plus onze sites ; les conversions vers les indices `i32` sont protégées par la borne d'index de l'entrée.

Le helper géométrique est un membre unique du Builder de l'ordre, avec son résultat et ses limites persistants (87–97, 173–182). `geometry_caps` met à zéro les capacités de cœur, chaîne historique et cofaces ajoutées ; le module appelle exclusivement ses méthodes `miniball` et `intruders`, jamais son `run()`. Le plafond supplémentaire `max_meb_calls` est chargé avant chaque invocation ; le compteur physique interne augmente immédiatement à l'entrée. Supports et nœuds de census ne repartent jamais de zéro entre portails.

Les compteurs sans garde propre ont une borne par des opérations déjà chargées : `alias_hits` par les visites strictes ; `terminal_direct` par les demandes de portail ; `normalized_anchors` par les lectures de successeurs ; les compteurs de feuilles/plages du helper par ses nœuds de requête. Chaque longueur locale est au plus le compteur global de pas. Aucune addition de deux compteurs `u64` n'est utilisée pour autoriser une dépense et aucun débordement atteignable n'a été identifié dans cette lecture.

Une normalisation de profondeur d facture d+1 lectures pour trouver la racine, puis d lectures et d écritures de compression : 3d+1 opérations (246–268). Une coupure pendant la compression peut modifier une partie privée du chemin, mais chaque raccourci pointe vers la même racine ; l'appel échoue ensuite sans forêt. Ce plafond ne couvre pas les opérations de DSU locale ou les créations de successeurs, conformément au contrat. Les successeurs de fusion ont toujours un ID plus grand ; les raccourcis restent valides et ne créent aucun cycle.

Les compteurs décrivent des opérations chargées, y compris certaines tentatives interrompues par une allocation. Par exemple, `aliases` est incrémenté avant `emplace` ; en cas de `bad_alloc`, il peut dépasser le nombre d'alias effectivement installés. `normalized_anchors` peut compter la marche initiale réussie d'une normalisation dont la compression est ensuite refusée. Ces valeurs restent bornées et ne sont pas des tailles de payload publiées après un refus.

Les limites de certificat portent sur les records retenus, pas sur toute allocation temporaire. Une liste de naissances ou les groupes du lot peuvent être préparés avant le contrôle de place dans les successeurs ; le nombre de lots de sortie est vérifié avant sa conservation, après le travail du lot. Entrées, index locaux, alias, DSU et lots coexistent avec la validation finale. Cela respecte le contrat de records annoncé, mais n'est ni un budget de RAM ni une échéance CPU.

Le résultat public reçoit sa forêt seulement après succès de `build_full_certificate` (156–169). Toute autre sortie conserve ou remet une valeur invalide. Le destructeur du Builder recopie les statistiques géométriques avant destruction de ses membres, y compris lors d'une exception (97). Les `bad_alloc` et `length_error` internes ou issus de la validation finale deviennent des refus nommés (158–164, 469–480) ; déplacer ou vider le certificat ne demande pas d'allocation. Les événements sources restent référencés en lecture seule pendant l'appel et aucun pointeur vers eux n'est conservé dans la forêt retournée.

## 5. Décision de qualification

Le code lu réalise le calendrier mathématique annoncé et préserve les refus nécessaires aux portails. La revue ne remplace pas le pont indépendant préparé par l'auditeur principal sur des catalogues rationnels : ce pont doit vérifier les partitions de minima étiquetés, les couvertures, les parents et les deux côtés des coupes, et rendre non vacues les branches de portail et de refus qu'il annonce.

Le contrat constructeur indique désormais que ses positifs actuels sont globalement réguliers et qu'ils n'imposent pas encore un deuxième pas nommé ni un intermédiaire à un seul intrus. Ces limites ont été lues, sans exécuter ou auditer sa porte ici. Elles concernent la couverture dynamique, pas une erreur trouvée dans les transitions 376–378. Une qualification ultérieure doit distinguer ses nouvelles branches des résultats déjà observés, sans redemander les preuves fermées des primitives.

L'identité d'entrée, l'horizon autorisé, la couture verticale, l'archive, la CLI et les coûts d'une tour complète restent hors de cette API horizontale relative. Aucun résultat massif ou gain de performance n'en découle. Écritures limitées à cette note et son JSON sous `audits/` ; aucun build, Git ou GCP.
