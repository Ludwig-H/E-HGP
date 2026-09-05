# Cache facultatif des rattachements FULL

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Objet et état du delta

Ce delta du [producteur FULL](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) sépare les
autorités nécessaires du cache dérivé des facettes. Il vise le verrou
d'installation systématique des alias observé dans les
[premières mesures mono](RESULTATS_MONO_FULL_20260905.md). Il n'ajoute ni
Gamma exhaustif, ni mosaïque de Delaunay d'ordre supérieur, ni cofaces de
descente stockées. La sortie reste le certificat horizontal des minima
Gabriel et des vraies multifusions avec leurs parents.

La [preuve indépendante antérieure au delta](../audits/receipts_full_producer_20260905/lazy_alias_next_step_review.md)
ferme la suffisance mathématique de cette séparation sur le domaine régulier
déclaré. Le
[précontrôle de développement](../receipts/full_lazy_development_20260905/README.md)
conserve les deux refus d'une fixture non globalement régulière, sans
relâcher cette hypothèse. Après correction, la qualification fraîche
ferme 14/14 CTests Release et 14/14 ASan/UBSan, LeakSanitizer actif ; les
[reçus propres au delta](../receipts/full_gabriel_lazy_20260905/README.md)
en détaillent les pins et limites. Les
[mesures appariées et paliers](RESULTATS_MONO_FULL_LAZY_20260905.md)
rapportent séparément le gain de résidence, l'absence de gain de temps
à 8k, le succès à 16k et le refus à 32k. L'ancienne
qualification `e02d163c…` reste un reçu de la politique historique.
Le [delta courant des lots unitaires](CONTRAT_LOT_UNITAIRE_FULL.md)
conserve ce contrat de cache et repasse les portes lazy dans ses
17 CTests ciblés par build. Ses balayages frais observent 209 allocations
lazy sur les mêmes six cellules ; les 434 citées plus bas appartiennent
au header historique `13c6cc72…`.

## Deux API et deux contrats distincts

`build_full_gabriel_order` conserve l'installation de toutes les facettes
incidentes et le plafond cumulatif `FullGabrielLimits::max_aliases`. Sa
politique porte `eager_all_incident_facets_v1` ; elle reste le défaut de
l'API historique, sans changement de la CLI F.

`build_full_gabriel_order_lazy` reçoit les mêmes index, catalogues et limites
de travail, plus `FullGabrielCacheLimits{max_entries}`. Il exige explicitement
`max_aliases=0` ; sinon il refuse avec `kInvalidInput` et
`full_gabriel_lazy_alias_budget_conflict`. Ce zéro n'est donc pas une
désactivation cachée de l'ancien contrat : le nouvel appel choisit une
autre politique, `lazy_first_c_strict_resolutions_v1`, visible dans le
résultat même en cas d'échec.

Le cache retient les premières C facettes strictes non minima résolues,
sans éviction. Une capacité nulle ou pleine déclenche une résolution sans
insertion. Aucun plafond n'est relevé, aucune dépense passée n'est remise à
zéro et aucune composante n'est perdue. Cette politique ne promet pas le
même calendrier d'admission que l'ancienne : les recherches supplémentaires
peuvent atteindre les plafonds de MEB ou rencontrer une coquille refusée.

## Autorités permanentes et état atomique

Les minima conservent leurs identités de naissance dans l'index de leur
catalogue. À K1, ces identités sont les offsets des PointId triés, tous nés
à zéro. Chaque directe conserve son ancre **après fermeture du lot entier**,
y compris si aucune multifusion n'a été publiée. Les successeurs historiques
restent disponibles pour normaliser ces tokens. Aucune de ces trois
autorités n'est une entrée du cache facultatif.

Les demandes strictes seules entrent dans le dispatcher : retirer un
essentiel de la directe au niveau a donne une facette F de niveau inférieur.
Les racines sont lues avant toute fusion globale du lot a. Le regroupement
local (DSU générale, tableau dédupliqué pour une seule directe)
réunit ensuite ces anciennes racines ; minima et multifusions reçoivent
leurs IDs, puis toutes les directes sont ancrées dans l'état fermé. La
seconde passe d'installation des K+1 alias par directe est supprimée dans
la seule politique lazy.

## Résolution d'une facette stricte

Le catalogue des minima est cherché d'abord. Une correspondance doit avoir
un niveau strictement antérieur et un token installé. Sa normalisation
fournit le parent sans MEB ni census. Ensuite seulement le cache est consulté ;
un hit normalise lui aussi son ancien token vers la coupe stricte courante.

Lors d'un miss, une requête de portail est facturée, puis la MEB de F et
son census global sont calculés. Le niveau doit rester strictement inférieur
au lot et le contrôle de coquille doit terminer. Le helper conserve au
plus deux intrus : zéro et un sont des comptes exacts, deux signifie au
moins deux.

| Intrus étrangers | Résolution après un miss |
| --- | --- |
| J=0 | Le minimum F aurait dû être trouvé ; refus `full_gabriel_minimum_missing`, jamais naissance tardive |
| J=1, intrus z | Chercher la directe exacte F+z, vérifier son niveau égal à celui de F, son ancre installée et son antériorité ; normaliser vers le parent pré-lot |
| J≥2 | Descente stricte historique jusqu'à une directe antérieure, avec ses contrôles locaux et budgets persistants |

Pour J=1, le census achevé certifie que F+z a exactement la même MEB et
le même support que F. Aucun nouvel appel MEB, census ou pas de descente
n'est nécessaire pour F+z. Le label complet reste indispensable : l'égalité
du seul rayon ne permet pas de choisir une ancre. Un terminal absent,
mal daté ou non encore installé refuse transactionnellement.

Un rattachement certifié peut enfin être inséré s'il reste une place. Il
n'est pas nécessaire de le mémoriser pour rendre le parent à son appelant.
Les alias des facettes égales ne sont pas installés à leur naissance ; une
consommation ultérieure les résout notamment par la nouvelle branche J=1.

## Charges, résidence et limites

Tous les plafonds géométriques et de successeurs du constructeur restent
persistants sur l'ordre. `portal_requests` compte chaque demande passant
par MEB+census, y compris J=1 et les recalculs faute de place. Sur succès,
`meb_calls = portal_requests + chain_steps`. `terminal_direct` inclut les
terminaux J=1 et ceux de la descente ; `singleton_intruder_resolutions`
distingue les premiers.

Les nouveaux compteurs séparent `minimum_lookups`, `minimum_hits`,
`cache_lookups`, `cache_hits`, `cache_inserts`, `cache_skips` et
`direct_lookups`. `aliases` et `alias_hits` restent nuls en lazy : ils
continuent de désigner l'ancien calendrier, pas le nouveau stockage.
Les recherches de catalogue sont comptées par demande, pas par comparaison
de clés ; leur coût logarithmique ne devient pas un plafond de temps CPU.

`cache_inserts` compte prospectivement les admissions avant allocation.
Sur succès, il égale le nombre résident d'entrées, au plus C ; sur panne,
la dernière admission peut ne pas avoir abouti. `cache_skips` compte les
résolutions rendues sans insertion faute de capacité. `bad_alloc` reste
un refus global avec forêt invalide et arènes vides, pas un fallback caché.
Les dépenses déjà faites restent visibles et les sources sont immuables.

Sur succès seulement, l'absence d'éviction et d'insertion sautée avant
saturation impose `cache_inserts = min(max_entries, portal_requests)` et
`cache_skips = portal_requests - cache_inserts`. La
[contrelecture indépendante du juge](../audits/receipts_full_lazy_20260905/digest_probe_review.md)
isole cette porte supplémentaire : les simples bornes et la somme des
deux compteurs n'interdisaient pas un skip prématuré. Cette identité ne
s'applique pas au milieu d'un refus transactionnel.

Avec D directes, il reste au plus 4D demandes de parents stricts. Le cache
est borné par C, mais les catalogues, minima, ancres et nœuds de sortie ne
le sont pas. Il n'y a ni borne linéaire générale en n, ni garantie de RSS,
ni streaming massif acquis. Les gains de résidence et de temps demandent
des mesures appariées ; supprimer des alias peut ajouter des MEB/census.

Les ancres inter-K ne sont pas encore exportées. La verticale, l'archive,
la CLI FULL et le supplément pondéré restent distincts. Aucun résultat
50k/1 s, 100 ms ou dizaines de millions sur G4 ne découle de ce delta.

## Qualification exécutée

Les [portes lazy](../tests/full_gabriel_lazy_gate.cpp) exercent six nuages,
27 couples nuage/ordre et trois capacités par couple : 81 exécutions
paresseuses, 3 192 coupes, neuf permutations et 18 cas K=n. Les minima
isolés, J=1, la descente à deux pas et le hit partagé entre directes d'un
même lot possèdent des sentinelles nommées. Le mode rejets ajoute 127
refus et vérifie les budgets à zéro, à la charge exacte et juste dessous.

Le premier candidat partagé reste un négatif permanent : la boule CV de
niveau 17/2 a la coquille ABCV et le support CV. Le remplacer par une
fixture globalement régulière ne supprime pas cette contradiction.
La [porte mémoire](../tests/full_gabriel_lazy_allocation_gate.cpp) balaie
434 fautes persistantes sur E5 et J1, chacune aux capacités 0, 1 et un
million : aucune exception échappée, aucune forêt partielle et dépenses
déjà faites conservées. Les sept portes historiques eager/lecteur sont
réexécutées, pas héritées de leurs anciens reçus.

Le [digest sémantique](CONTRAT_DIGEST_FULL.md) reçoit sa qualification
arithmétique indépendante, incluse dans ces 14 tests. Ces résultats
ciblés ne réattribuent pas la suite F complète au nouveau code et ne
prolongent pas l'oracle géométrique borné n≤8 jusqu'à K9/K10.
