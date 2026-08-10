# Audit du fold `face-owner` ordre par ordre

Date : 10 août 2026 UTC.

Verdict : **GO sémantique borné pour le refactor ordre par ordre et pour la
réfutation cofaces ciblée; NO-GO pour qualifier le seuil mémoire de budget dur
ou pour annoncer le fold hybride produit.** Le correctif ferme bien la
résidence simultanée des ordres et conserve les partitions ainsi que les
records Gamma. Les derniers verrous ont maintenant des corrections locales et
des portes précises.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=bounded_differential_mutation_and_memory_contract`,
`mode=audit_independant`, `public_status=not_claimed`.

## Snapshot

| objet | empreinte |
| --- | --- |
| `HEAD=origin/main` | `56e76c65c31fc877f7629ac8e87a8a8479efa8fb` |
| `prototype/saturated_fold_faceowner.hpp` | `ffa4dbe4537855c386ea9a586864109606436b788312bea2f3399e2358f4acc8` |
| `prototype/postings_join_gate.cpp` | `039ca03b1de9d6dcc485825f49632583269415f406469725a7eb6040b2674b6b` |
| `prototype/saturated_pipeline.cpp` | `986f39e930040852dcb447158217307f4f11ee8fcbfd48bf4d7655cfa99e5f1d` |
| `CMakeLists.txt` | `0e60260185e76c844404ae2d3ce62560374aada07e0225dd6a2201d634a69df8` |

Le build Release CPU a été effectué hors dépôt. Une sélection de douze
portes permanentes — fixtures, campagnes générique et saturée, sept mutants,
comparaison pipeline et refus mémoire — passe 12/12 en `5,70 s`. La sélection
élargie de l'audit passe 18/18; une campagne additionnelle de vingt nuages
`n=10,K=6` concorde aussi.

Sur `n=32,smax=11,K=3,seed=20260810`, le fold publie exactement `247 854`
incidences prédites et réelles, puis `195 993` unions tentées et `6 980`
réussies. G² et `face-owner` ont les mêmes compteurs et les mêmes records
Gamma. Le temps de fold observé est `0,081..0,093 s` selon le run; ce chiffre
reste un diagnostic de l'hôte partagé.

## Corrections positivement reçues

1. La boucle externe est maintenant l'ordre `k`. Elle réserve une seule table
   d'incidences, construit les étoiles, libère les incidences, crée un unique
   `OrderState`, rejoue tous les lots, puis libère arêtes et état avant l'ordre
   suivant. Les ordres ne résident plus tous ensemble.
2. `reserve(I_k)` remplace l'ancienne croissance accidentellement quadratique.
   L'identité terminale retrouve la somme des binomiales du préflight.
3. Le pipeline compare maintenant `gamma_records` champ par champ dans
   `--compare-joins`; ce chemin n'est plus aveugle aux témoins et aux saturés
   marquants sur un même catalogue.
4. La porte cofaces vise réellement `k=6`. Le cas sain exige un record à dix-sept
   témoins stricts; le mutant `support-facet-filter` sort avec le code 4 et le
   diagnostic `ordre k=6 : records Gamma`.
5. Le manifeste de refus `face-owner` est observable. Avec `1 MiB`, le pipeline
   refuse code 3 et publie `247854` incidences ainsi qu'un pic **estimé** de
   `11,1 MiB`, explicitement étiqueté comme non majorant.
6. Le constructeur refuse maintenant un support de cardinal nul, non trié,
   dupliqué ou extérieur aux membres. Une sonde directe reproduit les quatre
   motifs de refus exacts. Cette sonde doit devenir une fixture permanente.

## Verrou 1 — l'option mémoire n'est pas un budget dur

Le nom du champ `estimated_peak_bytes` et le manifeste sont honnêtes. En
revanche, la valeur est encore utilisée comme critère d'admission de
`--memory-budget-mb`. Une acceptation ne garantit donc pas que le processus,
ni même tous les buffers du fold, restent sous le seuil.

La phase d'émission conserve simultanément les incidences et le vecteur
d'arêtes. Sur l'ABI mesurée, une incidence vaut 32 octets et une arête 12
octets. Sans `reserve` exact des arêtes, la croissance géométrique peut porter
leur capacité presque à deux fois leur taille : le seul terme de cette phase
peut approcher `32*I_k+24*E_k`, donc `56*I_k`, contre `48*I_k` dans le modèle.
Les sorties persistantes, capacités, allocations de `set`, records et
partitions restent en outre hors formule. Une instrumentation de
`new/delete`, activée seulement pendant le fold et excluant le catalogue,
fournit une réfutation interne : sur `n=32,K=5`, le forecast vaut
`21 836 808` octets, mais le pic de requêtes simultanées vaut `22 195 456`
octets. `--memory-budget-mb 21` autorise pourtant le run, alors que son seuil
est `22 020 096` octets. À l'ordre 5, `I_k=321 994`, `E_k=296 553` et la
capacité de `edges` atteint 524 288 : incidences et arêtes demandent
`16 595 264` octets contre `15 455 712` modélisés par `48*I_k`.

Correction minimale, sans changer l'algorithme :

1. valider le contrat `rank<=kMaxRank`, puis `I_k<=SIZE_MAX`,
   `I_k<=incidences.max_size()` et `count<=INT_MAX` avant tout cast; accumuler
   aussi la masse en `u128`;
2. après tri, faire un premier passage par groupes pour compter exactement
   `E_k`, puis `edges.reserve(E_k)` et remplir au second passage;
3. calculer le maximum des phases `incidences+edges` puis
   `edges+OrderState+temporaires+sorties_déjà_publiées`;
4. publier prévu, capacité réelle et high-water par ordre;
5. intercepter `length_error` et `bad_alloc` pour refuser sans exception
   sortante;
6. tant que toutes les allocations ne passent pas par une ressource plafonnée,
   renommer l'option en seuil de forecast. Un vrai budget dur demande un
   allocateur compté/plafonné et un refus transactionnel sur dépassement.

Avec `keep_partitions=true`, les sorties peuvent dominer toute borne en
fonction du seul ordre le plus lourd. Le chemin de réception peut soit inclure
leur taille exacte, soit exclure explicitement ce mode du contrat de budget.
Le préflight courant arrive par ailleurs après la copie des membres, la
compression de l'univers et la construction des lots : un budget total devrait
couvrir ces phases antérieures ou annoncer explicitement qu'il ne vise que le
join.

La fixture de régression la plus directe est donc `n=32,K=5,budget=21 MiB` :
elle doit refuser tant que la borne n'est pas corrigée. Ajouter ensuite une
injection d'échec de la N-ième allocation, un rang 33, une limite `max_size`
artificiellement petite et `budget+keep_partitions`. Le manifeste doit exister
au succès comme au refus et publier portée, mode `estimate_only|hard`, budget
demandé, ordre le plus lourd, `I_k`, tailles ABI, capacités et décision.

## Verrou 2 — permutation et niveaux exacts

La campagne de permutation permanente reconstruit encore seulement le join
postings. Elle ne rejoue pas `face-owner`. De plus, le comparateur en mode
permutation ignore l'indice de niveau, mais omet aussi
`marking_saturations`; il ne compare donc pas tout le payload sémantique.

Un diagnostic indépendant sur 2 000 familles, 58 251 générateurs, trouve les
partitions, transcripts, records et marqueurs `face-owner` invariants dans
2 000/2 000 cas. Ce résultat positif indique exactement la porte à graver. Il
montre aussi pourquoi tous les champs du reçu ne doivent pas être comparés :
`deduplicated_branches_k` et `unions_attempted` varient dans 1 950/2 000 cas
avec le tie-break de l'owner, sans changer la filtration.

La porte correcte compare :

- partitions, compteurs, records complets et `marking_saturations`;
- les **valeurs rationnelles exactes** des niveaux représentés, via
  `sphere_cmp_beta==0` entre les deux catalogues, jamais en les ignorant;
- les champs invariants du reçu : `I_k`, signatures, multiplicité un, branches
  brutes, histogramme de rangs, total prédit, identités et unions réussies;
- pas la topologie opérationnelle des étoiles ni le nombre de tentatives,
  légitimement dépendants du tie-break.

Le digest diagnostique du pipeline ne hache toujours pas `gamma_records`. La
comparaison directe les reçoit sur le même catalogue, mais le digest ne doit
pas être décrit comme leur reçu ni comme invariant de permutation.

## Verrou 3 — renforcer les identités du reçu

`identities_ok` ne compare actuellement que le total des incidences au total
prédit. Une compensation entre deux ordres resterait invisible. Il faut
vérifier pour chaque `k` :

- `incidences_k == predicted_incidences_k`;
- `star_branches_k == incidences_k-unique_signatures_k`;
- `multiplicity_one_k<=unique_signatures_k`;
- `deduplicated_branches_k<=star_branches_k`;
- la somme des `deduplicated_branches_k` égale `unions_attempted`.

La gate doit lire ces identités; elle compare aujourd'hui le fold mais ignore
le contenu du `FaceOwnerReceipt`, hormis le refus interne total.

## Verrou 4 — le produit hybride n'est pas encore intégré

Le commit reste volontairement un oracle exhaustif. `order_k_flats.hpp` ne
produit aucun certificat principal ni sidecar; `Catalogue` ne transporte pas
les points; le fold ne reçoit ni complétude par ordre, ni `BallKey`, ni index de
lookup, ni fallback demand-driven.

Le chemin d'intégration sans nouveau verrou est maintenant entièrement local :

1. dans `try_emit_with`, calculer pour chaque `u` de `U` la miniboule exacte de
   `M` privé de `u`;
2. publier en lockstep avec `kept` le certificat positif de quatre `PointId` au
   plus, indexé par le `PointId u`; traiter aussi le push direct des singletons;
3. permuter le sidecar avec le même tableau `order`, puis seulement construire
   `BallKey -> handle`;
4. certifier un état négatif par un support alternatif dont la `BallKey` est
   exactement celle de `B`; sans ce témoin, conserver `unknown`;
5. lier globalement `source_complete_for_order[k]` au digest du catalogue final,
   jamais à `smax>=n`;
6. n'activer le fast path que sous source complète et support principal
   certifié. Le certificat principal implique localement `q_min=|U|`; le
   marquage des générateurs fallback exige encore `q_min_certified`;
7. envoyer toute absence de certificat ou toute source partielle au fallback
   relatif exact, jamais à une approximation du support canonique.

La preuve, le vérificateur hostile et les fixtures sont dans
[`NOTE_CERTIFICAT_SUPPORT_PRINCIPAL_PAR_MINIBOULE_20260810.md`](NOTE_CERTIFICAT_SUPPORT_PRINCIPAL_PAR_MINIBOULE_20260810.md).
Le contrat transactionnel du fallback par intersections de postings est dans
[`NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md`](NOTE_SOLUTION_HYBRIDE_COFACES_FACEOWNER_20260810.md).

## Deux corrections de libellé

- Le commentaire historique de 24 octets par incidence contredit le nouveau
  modèle 32+16; il doit disparaître.
- Les `16,4 M` incidences du diagnostic `n=200` sont la masse filtrée sous
  `Sigma_k`. L'oracle relatif courant émet toute la famille et mesure
  `17 282 892` incidences. Ces deux nombres doivent rester étiquetés; le second
  est celui du code live en mode partiel.

GCP non utilisé.
