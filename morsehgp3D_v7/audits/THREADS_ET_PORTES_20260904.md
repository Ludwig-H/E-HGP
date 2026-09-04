# Threads et portes du port v7

Date : 4 septembre 2026. Statut : exploration v7 hors registre,
CPU de référence, entrée u16, audit mathématique et architectural,
public_status=not_claimed. Les hashes des reçus désignent les sources vérifiées.

## Création partielle et exceptions

Les primitives parallel_items, parallel_ranges et le tri partagent désormais
une admission différée. Aucun callback ne commence tant que tous les threads
demandés ne sont pas créés. Une exception pendant leur création annule
l'admission ; un joiner RAII attend chaque thread créé avant propagation.
Ainsi le tri ne peut entrer dans une barrière avec des participants absents.

Le tri capture aussi les exceptions du comparateur, abandonne la participation
du worker fautif par arrive_and_drop, puis joint tous les workers avant de
relancer la première exception. Le tableau reste provisoire après exception.
Le bord public du pipeline traduit les erreurs de ressources OS en
resource_exhausted et invalide ses résultats provisoires.

thread_failure_gate exerce quatre créations partielles après deux workers
réellement démarrés : items, ranges, tri direct, tri par permutation. Il exige
zéro callback admis et zéro worker vivant au retour. Quatre autres cas font
lever une exception pendant le travail, dont un comparateur qui ne lève que
pendant la fusion de tranches différentes. Un tri réussi ensuite vérifie que
le mécanisme reste utilisable. Un témoin pipeline complet et sa panne de
création vérifient aussi le refus transactionnel de l'API, sans callback ni
payload conservé.

Le mutant parallel-admit-partial-launch réintroduit l'admission malgré la
panne : les deux primitives sans barrière admettent 65 568 callbacks dans le
run observé ; le juge rend 4. La valeur sert de témoin, pas de contrat de
planification. Les crochets et compteurs de panne sont absents du produit.

## Enregistrement et signatures

CMake enregistre les portes de permutation, résidence, C6a stub, threads,
census direct, incidences silencieuses et archives. L'enregistrement d'une
porte ne constitue pas son résultat d'exécution.

EXPECT_LINE exige maintenant une ligne entière. EXPECT_PREFIX est distinct
et réservé aux trois messages d'allocation dont les mesures RSS varient.
Six fixtures du vrai wrapper vérifient les lignes seules, entourées, polluées
par un préfixe ou un suffixe et les préfixes explicitement autorisés.

C6a reste un stub CPU : aucun chevauchement CUDA ni mémoire GPU n'est mesuré.
La porte refuse désormais les mutants hors de sa liste ; une erreur de son
témoin à lot unique rend 1, jamais 4. Les neuf mutants attendent chacun la
signature de refus correspondante ou la violation observée de publication.

Les domaines du digest des candidats et du calendrier KeyCSR sont déclarés
explicitement. Les tests exigent leurs signatures déterministes.

## Vérifications et limites courantes

Les compilations, exécutions et mutants de cet audit sont épinglés dans
le [reçu courant](receipts_20260904/validation_current.json). Les cinq portes
Release ciblées, dont les refus d'allocation et le calendrier KeyCSR,
passent ; la porte de permutation et ses quatre mutants ciblés passent aussi
sur leurs sources identifiées. Aucun résultat complet de tous les mutants
ou de toute la suite Release finale n'est déduit de cette sélection.

Le contrat d'admission des fils est décrit dans
[l'audit de résidence](AUDIT_RESIDENCE_20260904.md). La couverture globale
des sites d'injection reste à inventorier sur les CTests effectivement
exécutés, avec leurs codes attendus, y compris les refus à code 2 et les
injections composées.

GCP non utilisé par l'auditeur. C6a reste un stub CPU.
