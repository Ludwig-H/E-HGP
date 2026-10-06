# Activation du placement et lecture du chantier suivant — 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Sources publiées jusqu'à
**9d10de213** ; WIP du développeur identifié séparément par ses
[empreintes](sources.json). Lecture de source, de captures G4 déjà fermées
et contrôle Python borné ; aucun build, test natif ou appel cloud par l'auditeur.

## Perte de l'option corrigée

**9d10de213 corrige réellement `ForestParallel` :** son constructeur de
déplacement transmet maintenant `place_pipeline_`. Cela couvre les
transferts imposés par `Result` puis `optional`. Le nouveau témoin compare
la demande à la valeur reçue par `pipeline_orders`, indépendamment de la
topologie disponible. Le mutant associé réintroduit exactement l'omission.
La lecture favorable précédente du plan ne prouvait pas cette activation.

Les [trois sessions fermées](evidence/README.md) conservent le premier
essai à zéro plan comme A/A involontaire, exclu de la décision du développeur.
Au pin corrigé, `claudeo1place2` contient neuf portes PASS et le témoin de
72 appels avec plan non nul. Les sorties LiDAR froides et dernières chaudes
des trois sessions gardent leurs empreintes canoniques. Aucun mutant,
sanitizer ou changement ultérieur de paramètres API n'est qualifié par ces
neuf portes. Les captures locales étaient encore non publiées par le
développeur lors de la lecture ; leurs chaînes et leurs pins sont explicités.

## Conserver le témoin dans le banc

Les rapports LiDAR `place2`/`place3` ne gardent pas le nombre de cœurs du
plan : `take_summary` supprime le champ pourtant émis par la sonde. Les
masques demandés et les sorties égales ne fournissent donc pas à eux seuls
un témoin de plan actif sur **chaque prise**. Les 72 plans de la porte native
ne comblent pas cette absence dans les rapports LiDAR.

[Proposition d'une ligne et rejeu](observation/README.md) pour les futurs
rapports : conserver `summary.pipeline_placement_cores`, sans confondre
zéro et champ absent, ni ajouter de refus pour les topologies où le
placement reste légitimement inactif. Ce nombre concerne le plan et ne
certifie pas chaque appel d'affinité du noyau. Aucun résultat de temps ou
décision d'adoption n'est recalculé par cet audit.

## Lecture du WIP : pas de nouveau défaut important

L'adoption API/export relue active le placement pour FULL et le retire
pour l'ordre seul, dont le masque reste 7035. Le refus du placement sans
ordres concurrents intervient avant les allocations. Les portes et
mutants nouveaux correspondants sont raccordés. Cette lecture n'est pas
une qualification de l'adoption par les sessions du pin antérieur.

Le nouveau `LatticeSphere` reste limité aux sites entiers du census.
Le minimum se prend au point entier le plus proche du centre, ramené dans
la boîte ; le maximum au coin éloigné. Arrondis, saturations et intermédiaires
i128 sont sûrs jusqu'en u24 dans ce domaine. Les comparaisons strictes
conservent les contacts, et la voie Wide garde les anciennes bornes.
**Ne pas transférer le minorant à une région continue**, où il serait faux.
Les cinq empreintes relues sont stables avant/après ; aucun défaut
mathématique important démontré, aucune exécution native de ce WIP.

Les constats ouverts antérieurs ne sont pas clos par ces travaux. Le
[patch du lecteur de campagne](../audit_placement_followup_20261006/README.md)
reste notamment nécessaire pour accepter le masque 278523 dans cette voie.
Les replays d'observation et des reçus sont rejoués en normal/−O ; leurs
commandes et limites figurent dans les sous-dossiers. Les reçus historiques
restent inchangés.
