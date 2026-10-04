# Supports : multiplicités et coupes datées — revue privée, 4 octobre 2026

Deux témoins exacts suffisent à fixer les noms des futures sorties. Les lectures du workflow
`wf_a7dbdf1a-21c` restent des propositions ; aucun moteur, build, fit ou GCP n'a été exécuté ici.
Les quatre sources Git sont épinglées à `57dd21be1fd9ce68935910a78c8fc7174a1a3a5f` ; les deux rapports
de conception sont copiés avec leurs empreintes. Les six contenus sont inchangés avant/après.
Le rapport forêt déclare une lecture au `7df199f73` ; sa capture ne lui attribue pas implicitement
une exécution au pin Git courant.

| Cas exact, K=2 | Parties comprimées I∪A | Traces strictes | Toutes les parties fermées | Nouveaux sommets réels | Cofaces | Arêtes Johnson | Unions des anciennes composantes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Triangle (0,0),(2,0),(1,2), β=25/16 | 3 | 3 | 3 | 0 | 1 | 3 | 2 |
| Ligne (0,0),(1,0),(2,0), β=1 | 2 | 2 | 3 | 1 | 1 | 3 | 1 |

Le triangle a un unique support positif ABC : une coface ABC joint trois faces et donne trois
arêtes du graphe Johnson, puis deux unions de composantes antérieures. « Une liaison » doit donc
désigner un de ces objets explicitement. Le graphe des intersections et le graphe Johnson ont les
mêmes trois arêtes dans ces deux petits cas ; cela ne prétend pas une égalité générale de leurs
adjacences, seulement de la connectivité prévue par le contrat.

Sur la ligne, I={1}, U={0,2}. Les deux parties I∪A sont strictes ; la partie {0,2} naît au plateau
et n'est pas comprimée. Ainsi C(m,K−p)−S=0 ne compte pas tous les nouveaux sommets du graphe réel.
La proposition `kparts=C(m,K−p)` doit être nommée `compressed_part_count` et accompagnée de sa
définition. Elle ne doit pas devenir un dénombrement de toutes les K-parties de P_b.

Les ancrages sont [lecture forêt, §3](sources/workflow_lecture_foret_k.md), lignes 82–101,
et [lecture supports, proposition des liaisons](sources/workflow_lecture_supports_qb.md), lignes
263–273. La formule binomiale par support y compte correctement les (K+1)-parties contenant Q,
grâce à M2. Il faut les nommer **cofaces**, pas les identifier à des arêtes simples du graphe.
Sommer ces nombres sur plusieurs Q compte des incidences (Q,G) ; un même G peut contenir plusieurs
supports. Cette multiplicité peut être voulue, mais doit être distinguée des boules/cofaces uniques.
Ce sont des corrections de vocabulaire et de contrat de sortie avant la spécification finale,
pas une preuve de défaut de la forêt native existante.

Le filtrage temporel λ_b≤a et le rattachement après fermeture du plateau sont déjà proposés dans
la lecture forêt, lignes 76–78. Le modèle vérifie une coupe fermée abstraite à égalité et montre
pourquoi l'ensemble des supports de toute la vie d'un nœud ne représente pas ses coupes antérieures.
Cet exemple algébrique vérifie la règle de conception ; il ne juge aucun export natif. Le contrat
Zoltan capturé exige déjà une identité d'instantané daté, avec `cut_side` et univers explicites.

Rejeu autonome depuis ce dossier :

```sh
python3 -B -S check_incidences.py
python3 -B -O -S check_incidences.py
```

Les deux sorties conservées sont identiques : **61 gardes explicites**, deux fixtures de trois sites
et un exemple temporel algébrique. Aucun `assert` ne porte une vérification ; `-O` ne les supprime pas.
La clôture inventorie tous les fichiers, y compris les sources et les deux manifestes de capture.
Seul `SHA256SUMS` à la racine est exclu de son propre inventaire ; `LEDGER.json` inventorie les autres
payloads et est lui-même haché dans `SHA256SUMS`.
