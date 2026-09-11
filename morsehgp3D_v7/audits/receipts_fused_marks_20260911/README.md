# Résoudre les marques dans le premier parcours des fusions

11 septembre 2026, après **93f4d110**. Proposition et prototype d’audit
sur le calendrier structurel scellé **1cf438cb**, consommé par le raccord
[CPU parallèle](../../receipts/parallel_birth_streaming_20260911/README.md).
Aucun changement du moteur actif. GCP non utilisé.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

**Le premier union-find contient déjà la réponse fermée de chaque marque.**
La calculer à sa date d’admission retire le second union-find et son rejeu
final. L’histoire et toutes les marques gardent les mêmes champs physiques.
Cela complète le réemploi des marques à l’export : on les calcule une fois,
puis on consomme leurs réponses et leurs dates d’activation.

## 1. Le travail redondant dans le calendrier actuel

`reconstruct` produit les naissances et multifusions avec un DSU sur les L
naissances. Après construction, il trie les marques par admission et recrée
un DSU `marked`, un tableau `active_segment` et un curseur sur tous les nœuds
admis. Ce second parcours rejoue leurs parents avant d’attacher les marques.

Ce parcours évite déjà les remontées individuelles de hauteur potentiellement
grande. La présente proposition retire un rejeu et ses tableaux ; elle garde
la normalisation des entrées, les tris et le traitement de chaque marque.
Le graphe sur naissances, les arêtes datées, les marques et la forêt FULL
restent les mêmes objets. Aucun simplexe ou complexe global supplémentaire
n’est matérialisé.

## 2. Fusionner deux suites de dates

Trier les marques par `(admission,id)` comme auparavant. Fusionner ce curseur
avec celui des arêtes déjà triées, sans construire un tableau global
d’événements. À chaque date t :

1. Émettre toutes les naissances encore absentes de date ≤t, dans l’ordre
   `(date,id)` conservé.
2. Si un plateau d’arêtes commence à t, traiter **tout** ce plateau avec le
   code original : racines pré-lot, unions, groupes et multifusions atomiques.
3. Résoudre toutes les marques de date t dans l’état ainsi fermé.

Après les derniers événements, émettre encore les naissances restantes.
Conserver les tris finaux de `out.births` et `out.marks` par identifiant.
Les marques silencieuses sont résolues elles aussi ; elles alimentent les
ancres et les minima requis par l’[encodage historique](../receipts_historical_export_20260911/README.md).

## 3. Invariant et identité physique

Après fermeture de t, les sommets natifs admis d’une même composante ont
la même racine DSU, et `segment[root]` désigne le dernier nœud atomique de
cette composante à t. C’est l’invariant du premier reconstructeur après
chaque plateau ; une naissance isolée l’étend sans union. Une date de marque
sans arête laisse la partition inchangée.

Pour une marque m admise, sa réponse est donc :

```cpp
segment[dsu.find(index_of(forest.births, m.representative))]
```

Elle égale le segment que le second sweep obtenait en rejouant les mêmes
fusions jusqu’à cette coupe fermée. Conserver `m.admission` dans la sortie,
même si le segment obtenu est né plus tôt.

Les marques n’émettent aucun nœud et ne réalisent aucune union. Leurs find
supplémentaires compriment les chemins sans changer racines ni tailles.
Les décisions et départages futurs du DSU restent donc identiques. Entre
deux plateaux, leurs dates ne font que découper la même suite de naissances.
L’ordre de création des nœuds, les parents, successeurs, racines, least_birth
et offsets des parents restent identiques à la référence.

Deux précautions de représentation sont nécessaires :

- `out.births` est encore en ordre d’émission pendant le sweep. La recherche
  par identifiant utilise **forest.births normalisé**, déjà trié par id.
- À égalité exacte entre marque et arête, la date brute d’une multifusion
  reste celle de la **première arête** du plateau. Le curseur marque ne doit
  pas lui substituer un autre représentant équivalent du même niveau.

La preuve suppose les mêmes contrôles d’entrée, un certificat acyclique et
un préordre exact cohérent des dates. Elle admet plusieurs composantes,
l’absence de marques ou d’arêtes et les plateaux. Elle ne dépend pas d’une
régularité géométrique ou de la connexité finale d’une tour particulière.
La validité géométrique et la complétude du graphe fourni restent des prémisses
séparées.

## 4. Travail et résidence

Le nouveau parcours effectue une union par arête du certificat et une
résolution de marque dans le premier DSU. Le second DSU, son tableau
`active_segment` et le rejeu des nœuds/parents disparaissent. Pour un ordre
contenant L naissances, les tableaux retirés représentent logiquement :

$$L\,(2\,\mathrm{sizeof}(\mathrm{size\_t})+\mathrm{sizeof}(\mathrm{Id})).$$

Cela vaut 24L octets lorsque les deux types occupent huit octets. Une somme
sur tous les K est un volume cumulé, et ces tailles logiques ne donnent pas
un gain RSS. Les marques de sortie, leurs tris, les copies de normalisation,
les groupes de plateau et les autres tableaux restent à compter.

Le prototype demeure séquentiel. Le contrat de tableaux ne change pas pour
une future construction parallèle des histoires ; la réutilisation des
réponses à l’export reste applicable une fois ces réponses calculées.
Aucune accélération de la tour, mesure GPU ou cible 50k/1 s n’est déduite.

## 5. Qualification bornée O2/SAN

Le [prototype](fused_marks.hpp), dérivé du corps du calendrier scellé, passe
C++20 strict O2 et ASan/UBSan/LSan : **30 essais**, 184 nœuds, 186 marques et
**6 828 comparaisons** des composantes aux coupes ouvertes/fermées par parcours
de graphe. Les [captures O2](o2.json) et [SAN](san.json) ont les mêmes sorties.
La [gate](gate.cpp) compare tous les champs physiques à `fc::reconstruct` ;
son oracle de connexité parcourt le graphe initial, avec le prédicat de coupe
exact partagé. Les dates portent un rang et un tag brut ignoré par l’ordre ;
les deux essais à représentants équivalents préservent leurs tags.

Les quinze fixtures sont rejouées avec listes d’entrée directes et inversées :
empty, naissance tardive, composantes disjointes, multifusion ternaire,
continuation tardive, dates brutes équivalentes, queue non marquée et huit
recettes fixes. Les IDs 5, 2, 9 sont volontairement émis dans cet ordre
temporel. Une forêt non vide entièrement dépourvue de marques est couverte
par la preuve, mais n’a pas de fixture dédiée dans ce corpus.

| Faute ou entrée invalide | Cause observée |
| --- | --- |
| Marques avant fermeture du plateau | `history.mark_fields`, segment erroné réellement exercé |
| Admission remplacée par celle de la naissance | `history.mark_fields`, activation anticipée réellement exercée |
| Représentant de marque inconnu | `filtered.unknown_identity` |
| Marque avant son représentant | `filtered.mark_before_representative` |
| Arête à la date de naissance d’une extrémité | `filtered.edge_not_strictly_after_birth` |
| Cycle | `filtered.certificate_cycle` |
| Identifiant de marque dupliqué | `filtered.duplicate_or_reserved_identity` |

Les deux mutants sont comparés à la sortie nominale dans le selftest ; les
cinq entrées invalides sont refusées par les deux reconstructeurs. Ce sont
des contrôles internes du selftest, pas sept commandes CLI distinctes.
Arguments absent et inconnu : code 2, stdout/stderr vides.

Le corpus paye 70 unions dans le premier sweep ; l’ancien rejeu aurait
payé 124 tentatives supplémentaires, déduites des parents admis avant la
dernière marque de chaque essai. Les tableaux retirés totalisent 2 928
octets logiques cumulés sur les essais, sans mesure de pic ni de temps.
Les entrées restent structurelles synthétiques, sans réalisation 3D revendiquée.

## 6. Lecture et reproduction

```bash
python3 -B morsehgp3D_v7/audits/receipts_fused_marks_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_fused_marks_20260911/verify.py
python3 -B morsehgp3D_v7/audits/receipts_fused_marks_20260911/record.py --out .work_replay_o2
python3 -B morsehgp3D_v7/audits/receipts_fused_marks_20260911/record.py --out .work_replay_san --san
```

Le lecteur vérifie le sceau, les commandes, leurs sorties et les liaisons
de source, compilateur et ELF ; il n’exécute aucun binaire. Le recorder
exige un nouveau sous-dossier, garde ses temporaires sous ce paquet et
borne compilation/tests par timeout, avec captures conservées en échec.
Les deux captures r2 comprennent cinq commandes chacune ; les sources,
le compilateur et les ELF sont fermés avant/après leurs consommateurs.
Les ELF sont omis du dépôt. Les headers système, le lien et les runtimes
ne constituent pas une fermeture hermétique.

Les premiers essais r1 réussis précédaient le durcissement du recorder et
ne sont pas promus ; la qualification publiée porte sur r2, code C++ inchangé.
Aucun run C++ en échec n’a été supprimé. Le calendrier emprunté n’est pas
recopié : son objet scellé est lié par hash. Variantes moteur D–Q inchangées,
aucune nouvelle qualification globale ni archive industrielle. GCP non utilisé.
