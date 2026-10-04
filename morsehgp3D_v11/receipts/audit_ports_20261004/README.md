# Suivi des ports et réponse J3 — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Sources Git figées : q3 **56216392e**,
E1 **723cf6e43**, lemme R **9b9244a00**, banc **54c167bb6**, question **49831e9e9** ; compteurs **0c358261c** relus ensuite.
Trois notes actives actualisées en place ; aucune nouvelle note dans audits/.
Six capsules sont copiées sans mutation. La capsule catalogue sélectionne
les mêmes sources et preuves sans ses deux caches Python générés ; son
inventaire initial est conservé dans `SHA256SUMS.original`. La capture
privée close reste intacte. Les commandes de rejeu utilisent `-B -S`.

## Ports q3 et R

[Catalogue q3](catalogue/README.md) : candidat possédé, tag3/certificats,
contacts, S* et niveau brut de degré six conservés ; matérialisation après
admission géométrique, avant un éventuel refus du Collector. **1 983 gardes**.
[MEB q3](meb/README.md) : inclusion fermée avant Level, mêmes sept compteurs
et support canonique ; quatre niveaux q3 rejetés évités sur le tétraèdre
du témoin. **1 344 gardes Fraction/Gram/Gauss**. Le différentiel eager partage
la factory réécrite : garder le juge arithmétique indépendant en complément.

[Lemme R](lemma_r/README.md) : fermeture Q, centre dans Q et générateurs de
coquille impliquent intérieur/extérieur stricts par dominance ; aucune
coquille supplémentaire perdue. **103 200 gardes**, douze cas, passages
63/64 et 127/128, ordres et saturations. Le code budgète bien les deux
tableaux ; `CATALOGUE.md` conserve encore le terme 8C⌈C/64⌉ au lieu de
16C⌈C/64⌉. Aucun défaut d'admission déduit de cette formule documentaire.
Les 43 % annoncés par le développeur concernent des classifications par
masque ; aucun gain chronométrique n'est établi par notre modèle.

Le port tardif **0c358261c** applique R1 : quinze champs initialisés,
vidés chacun par `checked_add` après succès, réduction des tâches inchangée.
La borne conservatrice 140464088678400<2^49 couvre m≤1024. Les refus
remontent sans publication partielle ; portes/mutants relus, non exécutés.
`late_counters/` conserve les sources et la recoupe, sans nouveau gain acquis.

## E1 et portée statistique

[Recoupe E1](head/README.md) : H_L2 applique désormais Holm ET IC basse
strictement >−0,02 ; **94 gardes AST** et **10 contrôles du selftest stdlib**.
La porte inclut réellement z2 et une fixture distinctive ; son résultat
géométrique et sa nouvelle qualification G4 ne sont pas joués ici.
[Complément du préenregistrement](head_prereg_delta/README.md) : **six
recoupes** confirment que les phrases formelles attribution/PS2 restent
inchangées, malgré la correction de `SORTIE_PLATE.md`. Un corrigendum
d'interprétation doit dire « contribution supplémentaire non établie » ;
conserver seuils, prédictions historiques et incertitudes. L'oracle supervisé
`oracle_m05` des campagnes est distinct de l'oracle de correction EOM.

## Banc et question D

[Sonde des ordres](ab_order/check.py) : **482 gardes AST**, sans invoquer
le banc. Rotation rep%N puis inversion à chaque rep fixe base→new à N=2 ;
pour N pair chaque variante garde la parité de sa position. Inverser après
un cycle complet de N rotations équilibre positions et précédences sur
2N prises ; les cycles partiels ont un écart de position au plus un.
L'ancien protocole à deux variantes alternait correctement. Le plan
annoncé base/q3/q3+R utilise N=3 : ce témoin ne l'invalide pas. Il mesure
q3 puis le gain de R conditionnel à q3, sans estimer leur interaction.
L'échec initial de liaison `len` du modèle est conservé ; aucun produit
n'avait été exécuté.

[Réponse J3 épinglée](j3_counter/README.md) : accord pour une voie explicite,
sans reproduire les hits du cache ; `fallback` désigne le cache demandé
mais indisponible, pas un repli numérique. Demandes logiques et préparations/
calculs physiques H sont séparés. Le fail-fast sur trois faces ordonnées
f1,f2,f3 donne 1+f1+f1f2 demandes et 1−f1f2f3 rejets, conditionnellement
aux portes précédentes : comptages agrégés possibles sans rejouer la géométrie.

## Rejeu et limites

`REPLAYS.json` conserve les sorties identiques normal/−O des sept scripts
bornés (selftest inclus). `check.py` vérifie inventaires enfants, résultats
et identité des deux modes. Aucun fit, bootstrap réel, test/build natif,
workflow ou GCP exécuté par cet audit ; aucun contrat 100 ms/GPU nouveau.
Les annonces locales du développeur et la session prévue claudeab8 restent
distinctes de ces contrôles. Les anciens reçus clos restent immuables.
