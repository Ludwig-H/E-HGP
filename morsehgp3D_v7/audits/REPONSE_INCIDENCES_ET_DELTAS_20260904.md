# Incidences et deltas : état courant

Date : 4 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le verdict et la reproduction maintenus figurent dans
[l'audit mathématique courant](AUDIT_MATHEMATIQUE_20260904.md) et son
[reçu exécuté](receipts_20260904/math_current_repro.json). Il n'existe pas
d'addendum séparé à appliquer.

Le lecteur de deltas fige les jetons de racines avant chaque lot, consomme
chaque parent une seule fois, matérialise les `born` et exige la sortie
canonique. Gamma fournit la référence des coupes sans suppléer une
transition absente. Les suppressions de matérialisation ou de continuation,
ainsi que le remplacement d'une sortie par une facette non canonique de
la même composante, sont rejetés par les contre-fixtures actuelles.

La confluence des premières incidences fournit un argument conditionnel
pour la sélection d'une chaîne. La preuve globale doit encore composer
les prémisses de catalogue, régularité et rang avec la réduction. La
verticale, les identités publiques et la résidence industrielle possèdent
leurs contrats distincts.

Le [banc incidence](../bench/incidence_campaign.py) distingue
`engine_completed`, `engine_refused`, `censored` et `invalid`. Son statut
d'observation ne vaut ni réussite du moteur ni qualification d'exactitude.
Les coûts de la route complétée devront être mesurés sur des sources et
binaires stabilisés avec les reçus de campagne correspondants.

GCP non utilisé par l'auditeur.
