# Graphe statique d’ancres — preuve conditionnelle et oracle fini

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La [note complète](NOTE.md) décrit une séparation géométrie/calendrier :
résoudre les représentants stricts vers des ancres statiques, puis reconstruire
les composantes filtrées du graphe d’événements décoré. La conclusion est
**conditionnelle au census exact complet**. Dates d’activation, normalisation
pré-lot, multifusions simultanées, contributions unaires et images verticales
fermées restent indispensables.

Ce reçu porte sur un oracle rationnel indépendant et fini : sept nuages,
2 à 8 points, Kmax de 2 à 6. Ce n’est ni un résultat du moteur produit, ni
une preuve de complétude WSPD, ni un benchmark CPU/GPU. Aucun contrat 50k
ou grand nuage n’est acquis ici. Aucun moteur, fichier d’auditeur ou index
Git n’a été modifié par cette publication ; GCP non utilisé.

## Résultats et périmètre

Le modèle produit ses graphes avant tout calendrier, sans requête Gamma
(garde exécutable), puis les juge contre Gamma et le raccord rationnel
dynamique. Pour chaque politique d’intrus : 648 coupes, 5 943 contrôles
facette/coupe, 203 raccords pré-lot, 5 047 contrôles verticaux par facette.
Deux blocs changent réellement de terminal sous l’autre règle, sans changer
les partitions, couvertures ou images verticales. Sept mutants physiques
sont refusés sous Python normal et `-O` ; les quatre sorties nominales
capturées sont identiques.

Le helper géométrique énumère toutes les facettes sur ces petites entrées :
c’est une technique d’oracle, pas une proposition d’architecture produit.
Les compteurs `geometry_table_reads` ne sont pas des appels MEB C++.

La note distingue aussi le dernier intrus obtenu par balayage exhaustif
d’une DFS droite anticipée donnant le même rang Morton. Les chiffres
auditeur uniform « 311 864 → 270 662 » désignent les **requêtes d’intrus**,
pas les MEB. Cette observation n’est pas une mesure de gain physique ;
la qualification du prototype DFS est un travail distinct.

## Lecture reproductible

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/static_anchor_graph_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/static_anchor_graph_20260910/verify.py
```

Le lecteur est en lecture seule : il vérifie les manifestes, les sources
figées, les 32 commandes capturées, les codes de sortie et les raisons
exactes des sept rejets. Il reste effectif sous `-O` et ne dépend pas du
répertoire privé initial ni de l’état courant des sources du moteur.
Un contrôle non mutant peut être rejoué avec
`python3 -B morsehgp3D_v7/receipts/static_anchor_graph_20260910/capture/graph_oracle.py`.

## Provenance et conservation

[source_pins.json](source_pins.json) épingle les helpers de l’auditeur,
le seul changement mécanique privé (borne d’entrée 7 → 10, cas exécutés
≤8), les sources moteur/index inspectées et le manifeste privé d’origine
`9a88419ad2ae42ae1ba91de6d4f7bd5fd5dfe7bb944e0a37c1c517b4f49ca0f5`.

Les 75 fichiers de ce manifeste et le manifeste lui-même ont été copiés
**sans changer un octet** dans [capture/](capture/). Les commandes y gardent
leurs chemins historiques absolus ; le lecteur vérifie cette provenance,
sans essayer de lancer ces chemins. `capture/README.md.source`, `freeze.py` et
`verify.py` sont des documents/scripts historiques : utiliser le lecteur
public indiqué ci-dessus. La version consultable de la note, `NOTE.md`,
ne change que les liens Markdown relatifs de la note capturée. Le seul
nom physique adapté est `README.md` → `README.md.source`, pour que la
documentation historique à liens non rebasés ne soit pas rendue comme une
page actuelle ; les octets et le nom logique du manifeste sont préservés.

Les résultats exploratoires antérieurs non inclus dans le manifeste privé
final ne sont pas publiés ici. Aucun ELF ni autre binaire n’est inclus.
`MANIFEST.json` scelle le paquet publié ; `capture/MANIFEST.json` conserve
le sceau privé original. La lecture du paquet vérifie aussi l’identité
des captures avec ce sceau initial.
