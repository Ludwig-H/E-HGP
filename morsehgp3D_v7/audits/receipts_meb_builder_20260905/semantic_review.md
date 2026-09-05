# Raccord privé du Builder : conservation de F et charges persistantes

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**La lecture ferme le raccord sémantique nominal au Builder sous le domaine ci-dessous. Aucun défaut concret n'a été trouvé.** Le repli est réellement séparé de la proposition, Work survit à tous les appels de l'ordre, et les deux miroirs conservent les charges déjà acquises lors du déroulement de pile. Cela établit une propriété des sources privées ; cette contrelecture n'a lancé ni compilation, ni moteur, ni benchmark. Les reçus compilés préparés séparément ont leur propre autorité.

## 1. Sources effectivement relues

Les SHA256 ont été recalculés sur les fichiers privés indiqués par le constructeur. Leurs copies d'audit ont les mêmes octets ; les liens ci-dessous visent ces copies conservées.

| Objet | SHA256 |
| --- | --- |
| [silent_incidence.hpp](inputs/overlay/silent_incidence.hpp), initialement `build/v7_meb_dual_product_overlay/silent_incidence.hpp` | `6e517c5705ca5d21dfe8fb920510ee50d61af7c53a465cfe3175959ff45a0b15` |
| [meb_proposal.hpp](inputs/overlay/meb_proposal.hpp) | `33255ebcb92864acd6322424618ebdc2d4e1253e917004c1c4d76a3798ecf352` |
| [builder_from_F.patch](inputs/documents/builder_from_F.patch.txt) | `42c495bcce7b9a9a22bd13a59f271e96981ac8a3dc5420c7c8da36cb20a07e51` |
| [BUILDER_PORT.md](inputs/documents/BUILDER_PORT.md.txt) | `4ef6316cb8edc3ca589c07ae74400c691b35ef69e529117184102edc0c606b71` |
| [référence F entière](../receipts_resolver_20260905/qualification/snapshots/source/silent_incidence.hpp), également `reference_F.hpp` dans l'overlay privé | `f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76` |
| [README du port du helper](inputs/documents/README.md.txt) | `de2cbf85c8ead15bdcd0e2218fa68165b591ae54983d7917e21c96b3f38489a7` |
| [port_from_0645.patch](inputs/documents/port_from_0645.patch.txt) | `52e5ec65fa89eeb6f954d24467af318349744293d5806169e3c8a4d57c1bf30e` |

La comparaison textuelle indépendante confirme que le corps F, lignes 162–292, est littéral dans l'overlay, lignes 234–364, après le seul renommage `miniball` en `miniball_reference`. Le suffixe allant du commentaire d'`intruders` à la fin du fichier est identique après retrait de l'unique membre Work ajouté. La [capture des sources](inputs/source_review.json) documente séparément le patch entier et la fermeture des includes ; sa [table d'inclusion](inputs/include_map.json) permet de reconstruire les chemins relatifs sans lire le produit vivant. Les lignes citées ensuite sont celles des copies ci-dessus.

## 2. Domaine et lemme géométrique consommé

On conserve le domaine interne de F : index u16 valide à positions distinctes, `sites[0:n]` valides sans répétition, n entre 2 et 11, objets de sortie vivants et sans alias perturbant les entrées, mutations de test désactivées. Les sources/caps restent valides pendant le Builder, sans mutation concurrente. `run` valide son catalogue régulier comme auparavant ; K=1 revient avant les deux sites d'appels MEB, lignes 463,481,493. Le garde n2..11 de la proposition ne constitue pas un validateur du repli F pour des arguments arbitraires.

Le [théorème local déjà fermé](../MEB_DOUBLE_BUDGET_COURANT.md), §§ 3–5, fournit le lemme utilisé : un certificat positif contenant tous les sites et dont la coquille complète compte exactement q a le même unique support que F ; les slots canoniques donnent son premier ordinal R, entre 1 et 550, puis les mêmes champs de boule, y compris le niveau brut q4 et les slots inutilisés nuls. La relecture du port `33255e` retrouve les formes et ordinaux historiques et les algorithmes budgétés `0645`, avec le changement de namespace annoncé et `materialize<LocalBall>` initialisé par valeur. Aucun nouveau calcul géométrique ou changement d'échelle n'est introduit par le dispatcher.

Les compteurs commencent à zéro dans le wrapper, lignes 545–547. Une injection de test doit respecter A initial≤c initial, pivots≤p, certified≤p et certified+fallback≤meb_calls, cette dernière somme étant mathématique. Il faut conserver la marge nécessaire aux incréments de meb_calls et aux observations terminales. Les champs Stats ne sont pas une API permettant de réinitialiser Work entre les appels. Des caps abaissés sous les charges déjà acquises sont traités sans diminuer ces charges ; ils ne permettent plus d'affirmer que le compteur historique est inférieur au nouveau cap.

## 3. Un seul état par ordre, sans récursion du repli

Le constructeur, lignes 142–146, initialise Work une fois depuis les quatre nouveaux diagnostics correspondants. L'ordre des membres est correct : `out` précède Work, lignes 534–535. Aucun rechargement de Work depuis Stats ni nouveau Builder n'apparaît dans `miniball`. Le wrapper construit exactement un Builder pour sa tentative ; le K commun est celui du catalogue validé. Les MEB des facettes du cœur et celles des étapes de chaîne utilisent donc le même p, aux lignes 481 et 493.

Le graphe d'appel de repli est `miniball → miniball_reference_counted → miniball_reference`. Le dernier n'appelle ni `miniball`, ni la proposition, ni un autre Builder. Le piège identifié dans le prototype qui créait un Builder F frais est ainsi effectivement levé dans cette source. La proposition n'écrit que ses temporaires et Work ; avant son échec elle ne modifie aucun des 13 champs legacy, ni la boule, ni les événements. Le repli retrouve donc exactement l'état d'entrée F, avec en plus la trace des formes proposées déjà payées.

Cette propriété vaut pour une tentative/ordre. Elle n'instaure pas un budget global partagé entre plusieurs ordres, ni un protocole de reprise après reconstruction arbitraire de Builder.

## 4. Équivalence locale, puis conservation de la descente

Écrivons c=`meb_supports`, L son plafond, p=`meb_proposal_supports`, P son plafond et A=`meb_fallback_supports`.

| Branche du dispatcher | Effet legacy et boule | Nouveaux diagnostics |
| --- | --- | --- |
| P=0, lignes 184–189 | Appel au corps F littéral, même si c≥L ; un meb_calls dans F | Aucun p/pivot nouveau ; A reçoit le delta F ; fallback augmente seulement si c<L |
| P>0 et c≥L, lignes 191–193 | Un meb_calls ; refus `silent_meb_support_budget` ; c et boule intacts | Aucun travail ni terminal proposé nouveau |
| P>0, c<L, p≥P ou échec de proposition, lignes 198–202 | F au même état initial ; ses succès/refus et ses éventuelles écritures de boule sont conservés | p/pivots payés conservés ; fallback augmente une fois ; A reçoit le delta F |
| Certificat, R≤L−c, lignes 204–214 | Un meb_calls ; c devient c+R ; même boule littérale que F | certified augmente ; A inchangé |
| Certificat, R>L−c, lignes 204–210 | Un meb_calls ; c devient L ; refus budget ; boule sentinelle intacte | certified augmente malgré le refus ; p conservé et A inchangé |

Sur n≥2, la branche c≥L de P>0 a les mêmes effets que la première tentative de paire F. Dans la branche certifiée, le lemme précédent exclut tout candidat accepté avant R ; le préfixe de référence se termine donc exactement comme le tableau l'indique. L'égalité R=L−c autorise le succès. Le test par soustraction précède l'addition. Dans un repli à coquille non essentielle, l'écriture de boule effectuée par F avant son refus scientifique est conservée : le dispatcher ne restaure pas une sentinelle à tort.

La conservation est celle du booléen, du statut/raison, des 13 champs legacy et de tous les champs de boule, depuis un même état initial. Elle n'efface pas un statut artificiellement préchargé lors d'un succès. Les cinq champs ajoutés sont une sortie nouvelle, à juger séparément ; une comparaison qui ne regarderait que les 13 premiers ne les qualifierait pas.

Par induction sur les appels locaux, cette équivalence conserve le contrôle de `run` sur le domaine nominal et en l'absence d'exception. Le code d'intrusion, de catalogue, d'événement et de descente est littéral ; les niveaux et `support[0]` sont identiques. Le sommet retiré ligne 512, les cofaces suivantes, les terminaux directs/cachés, les autres budgets, les refus puis la purge du wrapper suivent donc la même trace logique que F. Cet argument ferme le raccord de la primitive à ce Builder ; il ne transfère aucun reçu binaire ou certificat horizontal à une nouvelle variante non encore qualifiée.

## 5. Charges physiques, ordinaux et miroirs

La charge p est faite dans le vrai `charged_form` du helper, lignes 144–149, avant l'observateur et la forme. Le dispatcher instancie `propose<false>` avec `NoObserver`, lignes 195–200. P=0 et p≥P court-circuitent avant toute sélection de paire. Le cap de 16 pivots et la borne de 401 formes par proposition restent ceux du helper déjà étudié.

Autour de F, `ReferenceWorkMirror` mémorise c initial puis ajoute c final−c initial à A, lignes 218–228. Aucun autre code de ce segment ne charge c : chaque unité ajoutée correspond donc à un candidat F essayé, y compris un candidat rejeté ou une exécution arrêtée au budget. Comme F ne diminue pas c et n'incrémente qu'après le garde, la soustraction est définie. Depuis A initial≤c initial, on obtient $A_{\mathrm{final}}=A_{\mathrm{initial}}+(c_{\mathrm{final}}-c_{\mathrm{initial}})\leq c_{\mathrm{final}}\leq\mathrm{UINT64\_MAX}$. Une branche accélérée ne modifie pas A.

Pour une suite d'appels, $\Delta c=\Delta A+\sum_j\min(R_j,L-c_j)$, où j parcourt les certificats atteints avec c_j<L, y compris ceux qui finissent en refus legacy. A compte ainsi les formes du repli ; p compte les formes proposées ; c demeure une charge de référence, pas leur total physique. Depuis zéro et à caps fixes, $A+p\leq c+p\leq L+P$. Le total A+p, la borne L+P et les agrégats de tour doivent être représentés par une paire ou une arithmétique élargie. L'overlay ne les additionne pas en u64.

Le miroir extérieur, lignes 174–183, republie p/pivots/certified/fallback sur toute sortie ; il ne remet aucun compteur à zéro. Chaque proposition commencée paie sa première forme ; cette charge couvre aussi le dernier pivot qui rencontre ensuite P épuisé. L'invariant pivots≤p est donc conservé. Un certificat consomme au moins une nouvelle charge, donc certified≤p est conservé. Aux retours normaux, un appel ajoute exactement un meb_calls et au plus une observation parmi certified/fallback. Ces preuves ne rendent pas admissible l'injection de MAX dans un compteur d'observation qu'on s'apprête à incrémenter.

Deux nuances comptables sont substantielles. `certified` signifie certificat géométrique trouvé avant l'admission L ; il n'est pas un compteur de succès. `fallback` compte la décision de repli avec marge legacy, conformément à `0645` ; à P0 et c≥L, le corps F est bien appelé, mais fallback et A n'augmentent pas. Le traiter comme le nombre brut d'entrées F créerait une fausse divergence de test.

## 6. Exceptions : ce qui est effectivement conservé

Les deux destructeurs de garde sont `noexcept` et ne font que des opérations scalaires bornées dans le domaine admis. Le miroir intérieur s'exécute avant l'extérieur lorsque F déroule la pile. Il publie le travail F payé avant l'exception, puis le miroir extérieur publie les charges de proposition antérieures. Les sorties ordinaires empruntent les mêmes destructeurs. Aucun catch ni rollback n'est ajouté au dispatcher.

Une exception injectée après une charge p et avant toute décision de certificat/repli laisse c, A, meb_calls et les observations terminales à leur état précédent ; p payé reste visible à la sortie. Une exception injectée dans F après une charge c conserve ce delta dans A ; meb_calls et la décision de repli ont déjà été comptés. Ces attentes sont différentes et permettent de tester les deux miroirs causalement. Une exception d'observation doit être injectée dans une couture d'audit explicitement pinnée ; le `NoObserver` nominal est `noexcept`.

Le wrapper historique, lignes 542–555, conserve son seul catch `std::bad_alloc`, sa purge des événements et le motif `silent_allocation_failure`. Les autres exceptions continuent de se propager. Une erreur ultérieure, par exemple pendant un `push_back` de `run`, retrouve les diagnostics MEB déjà publiés. Le proposeur et sa matérialisation actuels n'allouent pas ; les injections décrites ne sont pas des pannes nominales observées. Aucun argument ne prétend que F et la proposition ont le même calendrier d'exception ou que meb_calls est incrémenté avant chaque instruction d'une proposition interrompue.

Les champs Stats miroirs ne sont frais qu'à la sortie de `miniball`. Pour mesurer la causalité au vrai `before_form`, l'observateur doit lire Work fourni par le helper. Lire seulement le miroir Stats au milieu de l'appel ferait échouer à tort la voie nominale. Modifier ces champs entre deux appels puis attendre qu'ils remplacent Work serait également une utilisation hors contrat.

## 7. Décision constructive et portée

Le raccord proposé satisfait statiquement les obligations de durée de vie, de repli sans récursion, de conservation F, de séparation A/p/c et de publication exceptionnelle. Il est cohérent avec `BUILDER_PORT.md`. La prochaine qualification utile porte sur **ce dispatcher consommé**, son même Builder sur plusieurs appels, ses cinq nouveaux diagnostics, ses miroirs et la causalité de ses appels au helper. Elle ne demande pas une nouvelle preuve q2/q3/q4. Les comparaisons compilées et injections préparées séparément doivent identifier leurs adaptations de sources et leurs propres terminaux ; cette note n'en revendique aucun résultat anticipé.

La couture actuelle est réellement `NoObserver` nominale, mais ne sélectionne aucun mutant de charge au niveau Builder. Une couture d'audit vers `propose<true>` doit traverser cet appel pour qualifier la causalité du raccord. Les anciens mutants et compteurs `MHGP7_TESTING` placés uniquement dans la référence peuvent être contournés par la voie certifiée ; leur couverture P0/repli ne se transfère pas à P>0. Cette limite est déclarée dans le document constructeur, pas dissimulée par le port.

L'overlay ajoute un plafond et cinq champs C++ internes ; il ne raccorde ni API, ni CLI, ni archive, ni schéma public. La conservation P0 des champs legacy ne signifie pas une identité de temps ou d'instructions : le nouveau dispatcher et les miroirs existent aussi à P0. Les mesures du helper privé antérieur ne mesurent pas ce coût, et aucun gain de tour ne découle de cette lecture. Le stockage ajouté par le raccord est scalaire, sans catalogue global ou mosaïque nouvelle.

Fichiers écrits par cette sous-tâche : la présente note et son manifeste de pins. Aucune mutation produit, aucun Git, aucun build ou benchmark. GCP non utilisé.
