# MEB privée à deux budgets : qualification indépendante

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le certificat local, la charge prospective et le raccord au Builder privé sont qualifiés sur leurs octets capturés.** Ces reçus ne qualifient ni intégration CLI/archive ni producteur FULL, et ne donnent aucun gain de tour. Cette consolidation ne relance aucun moteur. GCP non utilisé.

La preuve du support unique, l'ordinal, le carré non régulier et la réfutation du budget ordinal seul sont exposés dans [PROPOSITION_MEB_ET_BUDGETS.md](../docs/PROPOSITION_MEB_ET_BUDGETS.md). Ses mentions « futur » décrivent sa préparation initiale. Les [résultats constructeur à deux budgets](../docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) et [résultats de coût](../docs/RESULTATS_COUT_MEB_20260905.md) ont leurs autorités propres. Cette note conserve les raccords et preuves indépendantes supplémentaires.

## Domaine et vérification du code capturé

Le [prototype](receipts_meb_dual_20260905/inputs/dual_pivot.hpp) `0645aa00` consomme le [helper historique](receipts_meb_dual_20260905/inputs/legacy_pivot.hpp) `d6dbba19` et F `f75a136a`, sur 2–11 sites distincts u16, indices valides, formes internes authentiques et observateurs passifs. Ces préconditions ne constituent pas une API validant des candidats arbitraires.

`form` trie les positions dans `sites[]` avant les primitives. La positivité et la contenance donnent l'unique MEB ; le shell final égal au support impose le premier accepté de F. Chaque pivot réussi augmente strictement le rayon. Le cap 16 est une borne de travail, pas une garantie de convergence. `q4_form` canonise le signe du déterminant : `det<=0` ne rejette aucune orientation supplémentaire. La matérialisation conserve le support entier, notamment `support[0]`, et le niveau q4 brut depuis le même tuple, sans reconstruction depuis la clé réduite.

La récurrence `choose` reste dans n≤11, q≤4 : valeur≤330, produit avant division≤3630 ; l'ordinal vaut 1..550. Le certificat local suffit donc à conserver le terminal et les champs littéraux, indépendamment de l'ordre de proposition.

## Charges et durée de vie

La [revue budgétaire](receipts_meb_dual_20260905/budget/review.json) distingue c legacy, p proposé et A réellement essayé dans F. Les gardes `p>=P` puis incrément avant forme, et `R>L-c` avant addition, sont sûrs jusqu'à MAX. Un certificat peut être refusé par L ; il laisse alors la boule sentinelle intacte et charge le préfixe virtuel restant. Une proposition échouée conserve p et retrouve F sans nouvel échec scientifique.

Exactement, $\Delta c=A+\sum\min(R,L-c_{\mathrm{avant}})$ sur les branches certifiées, tandis que $\Delta p$ compte les formes proposées. Ainsi $A+\Delta p\leq\Delta c+\Delta p$. Les plafonds restent deux u64 séparés : leur somme mathématique peut dépasser u64. Depuis un état cohérent A≤c, le miroir F conserve cette inégalité même sur exception. Les compteurs auxiliaires doivent garder leur marge ; injecter arbitrairement MAX dans un compteur déjà plein est hors contrat.

Un seul `Work` appartient à la tentative de l'ordre, sans réinitialisation par MEB ou repli. Le port conserve P=0 par défaut, une référence sans proposition et `reference_ordinal_plus_proposal_v1`. Les consommateurs publics doivent qualifier ce schéma séparément. Le [juge arithmétique](receipts_meb_dual_20260905/budget/budget_transition_probe.py) offre un `--check-only` portable ; ses transitions Python ne sont pas des exécutions C++.

## 7. Reçus privés et obligations de qualification actualisées

L'ancre historique conserve son contenu d'attribution. La [contrelecture constructeur normal](receipts_meb_dual_20260905/geometry_constructor/captured_normal.json) et [optimisée](receipts_meb_dual_20260905/geometry_constructor/captured_optimized.json) ferme 9 339 comparaisons, 1 507 ordinaux et le mutant prospectif : 46 437 violations, code 4. Les anciennes limites du reçu triangle ne restent pas des demandes ouvertes globales.

Les frontières rapides q2 à c=MAX−1 et q3 à c=MAX−4/MAX−1 sont appelées à P401. Leur branche rapide est raccordée par non-interférence : `propose` ne lit pas c et les mêmes scènes exigent un certificat dans `named_fast`. Aucun compteur rapide par frontière n'est inventé. La vraie scène q4 de rang 550 est appelée aux caps 549/550/551 ; ses deux pivots essaient seize formes. La combinaison q4, c=MAX−550, L=MAX n'est pas mesurée.

## Qualification locale indépendante

Le [bridge](meb_dual_bridge.cpp), le [juge rationnel](meb_dual_oracle.py) et le [reçu compilé](receipts_meb_dual_20260905/geometry/run.json) conservent deux builds C++20 stricts O2 et O1/UBSan. Par build : 89 nuages, 178 ordres, 3 430 MEB, 1 507 ordinaux ; 416/310/64 succès rapides q2/q3/q4, 1 650 refus legacy et 40 refus de coquille. Les [rejeux normal](receipts_meb_dual_20260905/geometry/normal.json) et [optimisé](receipts_meb_dual_20260905/geometry/optimized.json) jugent les mêmes sorties.

Le Gram rationnel, les coordonnées barycentriques, la contenance, le shell et l'énumération lexicographique Python fixent indépendamment support, R et caps. Pour q4, le déterminant rationnel fournit aussi les trois limbs bruts attendus. Trois copies fautives sont réfutées : shell omis, ordinal +1, numérateur/dénominateur q4 doublés. La dernière conserve le rayon : son rejet contrôle réellement la représentation brute. L'[erreur initiale de classement du juge](receipts_meb_dual_20260905/geometry/initial_judge_rejection.json) reste conservée.

Cette campagne part de Work frais. Son maximum rapide observé est R=465, quatre pivots ; 44 succès q4 utilisent le limb supérieur et 114 succès ont au moins deux pivots. Les [dépendances](receipts_meb_dual_20260905/geometry/dependency_review.json) sont liées aux captures, sans prétention de build hermétique.

## Builder persistant et exceptions

Le [dossier Builder](receipts_meb_builder_20260905/README.md) qualifie l'overlay `6e517c57`/helper `33255ebc`, distinct de F. La [revue sémantique](receipts_meb_builder_20260905/semantic_review.md) vérifie le corps F littéral, les callers inchangés et les miroirs ; l'identité locale fournit l'induction sur le même parcours silencieux.

Les [trois builds nominaux](receipts_meb_builder_20260905/compiled/run.json) passent chacun 3 444 appels locaux et 60 wrappers. Six séquences exécutent quatorze étapes sur le même Builder ; les 23 [attendus dérivés](receipts_meb_builder_20260905/budget_cases.json) ne sont pas tous exécutés. CHAIN5 produit huit MEB de rang un et une coface silencieuse déjà présente avant le troisième appel ; à P2, p=2 et A=6.

Huit injections contrôlent charges et exceptions. `bad_alloc` purge réellement une sortie non vide ; `runtime_error` se propage, sans rendre observable le résultat interne détruit du wrapper. Ses miroirs restent testés à l'appel local. Quatre copies fautives ciblent reset Work, miroirs p/A et charge après forme ; les [jugements normal](receipts_meb_builder_20260905/compiled/normal.json) et [optimisé](receipts_meb_builder_20260905/compiled/optimized.json) exigent leurs motifs causaux précis.

## Autorité native et coût

La [qualification native v2](receipts_meb_native_20260905/README.md) ferme `NoObserver` : 9 351 états confrontés complètement à F/Trace avant et après mesure, captures consommées sous chrono. La [revue indépendante du coût](receipts_meb_native_20260905/cost_review.md) distingue 10 722 candidats physiques contre 67 884 pour les 1 152 appels principaux P401, ralentissement du n=2 répété et contrôle P0 sensible à AB/BA. Ces observations ne fixent aucun seuil produit. Les reçus ultérieurs de coût gardent leur qualification propre ; aucune nouvelle campagne n'est demandée ici.

## Réduction démontrée des formes de pivot

**Le proposeur natif `0645aa00` peut éliminer des candidats impossibles avant de former leurs boules.** Ceci est une preuve pour un futur delta privé, pas sa qualification C++ ni son intégration FULL. Le helper actuellement qualifié reste inchangé.

Soit Q la base positive courante, de rayon r, et z un point **strictement extérieur** à sa boule. Par unicité de la MEB, celle de T=Q∪{z} a un rayon strictement supérieur à r. Toute base positive B⊆Q définit au contraire sa propre MEB de rayon au plus r. Sa boule ne peut donc contenir T : **tous les supports acceptables du pivot contiennent z**, même si T a des points supplémentaires sur sa coquille. Garder l’ordre relatif des candidats restants conserve le premier accepté.

L’initialisation native cherche exactement le diamètre global D de tous les sites, puis forme sa boule de rayon D/2. Les pivots augmentent strictement le rayon. Dès le premier pivot, aucune paire de ces sites, de rayon au plus D/2, ne peut contenir T : **supprimer aussi tous les q2 des pivots**. Cette deuxième preuve dépend de la paire globale maximale et de la positivité courante ; elle ne s’applique ni au q2 d’initialisation ni à une petite MEB arbitraire.

| Cardinal de Q | Maximum actuel | Imposer z | Imposer z et supprimer q2 |
| --- | ---: | ---: | ---: |
| 2 | 4 | 3 | 1 |
| 3 | 11 | 7 | 4 |
| 4 | 25 | 14 | 10 |

Dans l’appel actuel, z occupe la dernière position du sous-ensemble. Il suffit donc d’énumérer les couples puis triples d’anciens sites complétés par z, en conservant le tri canonique de `form`. Seize pivots demanderaient au plus 161 formes, initialisation incluse, contre 401 ; en tenant compte des deux premières arités, la borne est `1+1+4+14×10=146`. Ce sont des bornes de formes, sans garantie de convergence ni de latence. Les recherches de paire et les puissances restent du travail ; le petit cas n=2 défavorable n’est pas accéléré par ces filtres.

Le [juge rationnel](meb_pivot_filter_review.py) et son [reçu](meb_pivot_filter_review.json) passent normalement et sous `-O`, avec sorties identiques : cinq scènes, 62 permutations des bases anciennes, q2→q3, q3→q4, q4→q3, remplacement d’un essentiel q4 et coquille supplémentaire. Trois généralisations fausses sont réfutées : z intérieur, z seulement sur la coquille, paire initiale non maximale. Pour cette dernière, Q={(0,0,0),(2,0,0)} et z=(5,0,0) exigent une nouvelle base q2. La fixture q4→q3 satisfait la borne locale r≥D(T)/2 ; elle n’est pas présentée comme une trajectoire native effectivement exécutée.

Filtrer avant `charged_form`, sans facturer P pour une forme non essayée. Ce calendrier change : conserver le témoin ancien, Work persistant, charges prospectives et repli F. À P non limitant, le premier accepté et la trajectoire restent identiques ; à P limité, le proposeur peut avancer davantage, donc ses compteurs et sa route peuvent changer. Garder le shell final, l’ordinal legacy **sur tous les sites de la demande**, le support entier dont `support[0]` et le niveau brut q4. Un rang dans le petit pivot ne remplace jamais cet ordinal.

L’ordre stable — q3 avant q4, puis couples et triples historiques complétés par z — conserve le calendrier déclaré des essais restants. Conserver la contenance sur T entier, le départage strict du diamètre global, le premier violateur strict dans l’ordre `sites[]`, les slots canoniques par position, les temporaires sur échec, ainsi que le plafond 16 et l’emplacement de l’incrément des pivots. L’exigence shell=q reste finale, sur tous les sites. Le filtre q2 appartient à cette trajectoire native ; il ne devient pas une règle d’un `small_ball` générique.

**Correction du 6 septembre : notre ancien motif « plusieurs bases positives intermédiaires » était faux dans le domaine d’un vrai pivot.** Si Q est une base positive affinement indépendante et z strictement extérieur, la nouvelle base positive est unique. L’ordre reste observable dans P et ses refus, mais son inversion ne peut changer cette base à budget non limitant. « P non limitant » signifie une marge suffisante dans les deux bras sur toute la séquence du même Work, avec observateurs passifs et sans exception ; un même plafond numérique ne suffit pas.

Soient c et b les centres avant/après, r et R leurs rayons. L’unicité de la MEB de Q et l’intrusion stricte imposent R>r. Tout support nouveau contient z. Les anciens points de la nouvelle coquille, S⊆Q, sont affinement indépendants et appartiennent au plan radical des deux sphères :

$$2(b-c)\cdot x=\lVert b\rVert^{2}-\lVert c\rVert^{2}+r^{2}-R^{2}.$$

Au point b, le membre gauche moins le membre droit vaut :

$$\lVert b-c\rVert^{2}+R^{2}-r^{2}>0.$$

Ainsi b n’appartient pas à ce plan. Comme b appartient à l’enveloppe convexe de S∪{z}, z ne peut appartenir à l’enveloppe affine de S. Toute la nouvelle coquille est donc affinement indépendante ; les coordonnées barycentriques de b y sont uniques, et leurs coefficients strictement positifs donnent l’unique base. Certains coefficients peuvent être nuls : une coquille supplémentaire reste possible sans ambiguïté de base. La borne de demi-diamètre n’est pas nécessaire à cette preuve.

La [preuve et les fixtures permanentes](receipts_followup_20260906/multiple_bases_proof.md) distinguent un violateur strict et un point seulement sur la coquille, où deux bases peuvent effectivement exister. Le tétraèdre régulier fournit une sentinelle admissible pour l’ordre des essais : même base q4, mais quatre formes contre une au second pivot, donc admissions P différentes. Ce sont des calculs rationnels, sans nouvelle qualification C++.

Les [bornes sur les appels FULL réels](MONO_FULL_COURANT.md#borne-des-supports-meb-q4-sur-les-six-passages-singleton) motivent ce suivi sans en choisir le dispatch. Un histogramme par K, type d’appel, n et ordinal R suffirait à compter arités et poids d’énumération ; comparer le proposeur sur cette distribution exige aussi les sites des demandes, et non les seuls agrégats.

La [préparation privée capturée](receipts_followup_20260906/filtered_static_review.json), `484a89bc`, traduit ces invariants sans défaut statique trouvé : sous-séquence q3 puis q4 contenant z, q2 initial conservé, mêmes contrôles et matérialisation. Ce nouvel avis n’est ni une compilation ni une qualification C++ du proposeur filtré.

## Réutiliser une certification terminale déjà acquise

Le [refus 32k/K9](MONO_FULL_COURANT.md#refus-courant--appels-meb-puis-piste-de-réutilisation) impose de distinguer coût interne et nombre d’appels. Une piste démontrée sous les prémisses du Builder vise uniquement les **retours répétés sur un même label direct déjà certifié par une descente**. Lors de sa première arrivée, conserver toute la route actuelle : MEB locale, coquille essentielle, descente stricte, niveau du record égal au niveau recalculé, coupe antérieure et ancre valide. Marquer le record seulement après cette terminaison réussie.

Le label complet, ses coordonnées et le record géométrique restent immuables durant l’ordre. L’unicité de la MEB permet donc de réutiliser sa certification locale à une arrivée suivante sur ce même label. Vérifier encore le niveau enregistré strictement inférieur au niveau précédent et à la coupe, ainsi que la disponibilité du token ; **normaliser le token courant**, jamais une ancienne racine mémorisée. La branche terminale retourne immédiatement après insertion facultative de l’alias : elle ne consomme plus le support ou la boule locale. Aucun nouvel objet géométrique n’est à matérialiser sur cette réutilisation.

Cette première version n’amorce le mémo que par les terminaisons de chaîne déjà recalculées. Les raccourcis J=1 et F+z restent ceux du code actuel. Un record jamais contrôlé suit toujours l’ancienne route, préservant notamment le premier rejet `full_gabriel_terminal_level_mismatch`. La clé du certificat est le label entier, pas son niveau ni l’ensemble des points couverts par sa composante. Un indicateur sur les records directs déjà indexés suffit ; son coût réel en taille de record et résidence doit être mesuré. Aucune coface silencieuse ni table globale de MEB n’est ajoutée par cet argument.

À réussite commune et calendrier déclaré, noter R le nombre de réutilisations : `meb_calls=P+C−R`, tandis que P, C, first-C et les normalisations restent identiques. Placer la réutilisation avant la charge de la nouvelle invocation ; la placer dans `geometry.miniball` laisserait inchangé le plafond externe d’appels. Conserver les dépenses antérieures, distinguer demandes, appels réellement effectués et réutilisations, puis qualifier les nouvelles frontières de refus. Les recherches de terminal et les compteurs de supports changent aussi ; les anciennes identités ne sont pas reprises silencieusement.

La rentabilité se décide en comptant T terminaisons de chaîne et U labels terminaux distincts, soit R=T−U pour cette politique. Les captures 32k donnent T=826 460 mais pas U ; la borne inférieure de R reste zéro. Les premières fixtures utiles sont un terminal répété après fusion de son ancienne racine, un faux niveau à la première visite, deux labels de même niveau, C=0 et C saturé, puis les caps exacts/cap−1 dans la nouvelle unité. Ceci est une proposition de contrat prouvée et à qualifier, sans implémentation ni gain de temps annoncé.
