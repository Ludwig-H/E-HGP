# Audit complémentaire v8 — P0

13 septembre 2026. Intervenant **AUDITEUR_COMPLEMENTAIRE**, distinct du
développeur ROOT et de l'auditeur historique v7. Cadre :
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Avis utile au chantier actif

**Le premier raccord C++ reçoit un avis favorable borné sur les prédicats
et sur les rails testés.** À n2718, DualBlocks retrouve les 2 916 candidates
attendues ; Pool en laisse 1 846 881. Le choix actuel de h+1 témoins globaux,
neuf par facteur ici, distribue seulement un témoin par rail et ne rejette
aucune paire. Identités, crédits saturés et expansions physiques concordent
à s8/10/12, sans erreur UBSan.

| Contrôle indépendant exécuté | Résultat et preuve |
| --- | --- |
| Prédicats C++ sur coins et points intérieurs échantillonnés | 900 000 comparaisons ponctuelles, 7 290 000 requêtes universelles, 411 blocs positifs, 46 913 négatifs, 42 676 indécis ; deux mutations compilées réfutées ; [reçu](PREDICATES_CHECKS.json) |
| Module de crédits C++ sur rails q3/q4 | Neuf cas, comptes et paires physiques, deux mutations compilées réfutées ; [reçu](LOCAL_CREDITS_CHECKS.json) |
| Modèles exacts de quantificateurs et de rails | Modes Python normal/`-O`, fixtures positives/négatives et bornes du pool ; [reçu](MODELS_CHECKS.json) |

Les compilations C++ utilisent C++20, les avertissements stricts et UBSan,
dans des copies temporaires neuves ; aucune source du développeur n'est
mutée. Les headers et le `.cpp` du module sont épinglés par SHA256.
Ce sont ses octets locaux en construction, pas une qualification héritée
d'un commit v7 ou d'une future révision v8. Les contrôles de blocs portent
sur des échantillons bornés ; la preuve des bornes continues reste distincte.
Ce dossier n'annonce aucun résultat CTest du développeur.

**Comparaison des variantes :** la [contre-fixture des rails](P0_RAILS.md)
montre qu'avec 2 718 sites u16, tout pool global fixe d'au plus huit témoins
par facteur laisse au moins 1 436 463 paires q4 à besoin 8, contre 2 916
avec les crédits locaux exacts. Des crédits répartis par rail traitent ce
cas ; la proposition tubes/rangs de l'autre auditeur mérite ainsi un bras
apparié. Il ne s'agit ni d'un temps moteur ni d'une taille de sortie FULL.

Le parcours conjoint ancres×témoins doit distinguer **l'absence de témoin
commun** de **l'absence de crédits pour chaque ancre**. Réemployer directement
la borne négative universelle v7 pour couper une tâche de crédits locaux
peut rendre toute la préparation inutile. Sur la fixture exacte ci-dessous,
il reste alors 4 096 paires au lieu de 55 à seuil 10. Il s'agit d'un piège
d'intégration réfuté par un modèle indépendant, pas d'un défaut v8 constaté :
aucun source moteur v8 n'était présent à l'ouverture de cette passe.

**Actualisation après apparition des headers :** `classify_witness_block`
emploie bien un b0 fixe et maximise H sur toutes les ancres et tous les
témoins. Il évite donc le piège décrit ci-dessous. Avis favorable sur cette
distinction, sans attribuer au code une erreur seulement hypothétique.

### 1. Les quantificateurs à conserver

Avec $H(a,b,z)=(z-a)\cdot(b-z)$, le helper historique
[`hmax4_boxes`](../../morsehgp3D_v7/src/spindle/spindle.hpp) borne, à un
facteur quatre près, une expression de type :

$$U=\min_{a\in\mathrm{corners}(A),\ b\in\mathrm{corners}(B)}\max_{z\in Z}H(a,b,z).$$

Le calcul v7 est séparable par axe. Si cette borne est non positive,
chaque site de Z échoue pour au moins un couple d'extrémités : il n'est
pas universel sur A×B. Pour le front universel, c'est précisément utile.
Pour calculer les crédits **de chaque** a, ce n'est pas une preuve que
tous les z échouent pour cet a. Dans un parcours conjoint, cette réponse
doit donc signifier « pas de crédit commun ; raffiner les ancres ou laisser
le crédit inconnu ». Elle ne signifie pas « tous les crédits locaux sont
exactement nuls ».

Une borne supérieure de H sur **tout** A×B×Z, non positive, autoriserait
ce dernier rejet ; elle est plus conservatrice. Une implémentation peut
aussi employer une borne plus fine, à condition de prouver ses propres
quantificateurs. Pour les voies q3/q4, refuser H suffit au rejet, tandis
que le crédit exige aussi la marge stricte de leur fuseau.

### 2. Fixture u16 et issue constructive

Prendre $A=\lbrace(i,0,0):0\leq i<m\rbrace$ et
$B=\lbrace(D+j,0,0):0\leq j<m\rbrace$, avec D=4096 et m=3,16,64.
Ces facteurs sont séparés même à s=12. Le nuage est A∪B, donc le cœur
extérieur est vide. Les deux tâches racines ancres×témoins ont chacune
une borne négative universelle égale à zéro : choisir l'ancre d'A la plus
à droite, ou celle de B la plus à gauche, suffit.

Pourtant les crédits individuels exacts valent :

$$h_a(i)=m-1-i,\qquad h_b(D+j)=j.$$

En collinéaire, Ξ=0 ; les trois fuseaux donnent les mêmes crédits stricts.
Avec un besoin h≤m, le nombre de paires résiduelles est :

$$M=\sum_{r=0}^{h-1}(h-r)=\frac{h(h+1)}{2}.$$

À m=64 et h=10 : **55 contre 4 096** si les deux racines sont abandonnées
avec des crédits nuls. Le préfiltre reste sûr avec ces minorants nuls,
mais il a déplacé le travail vers son résidu. Ce sont des paires candidates,
pas une taille de sortie FULL ou une mesure q3/q4 aval. La collinéarité
est volontaire ; aucun contrat FULL régulier n'est qualifié ici.

Une solution légère réussit sur cette famille : choisir les h sites d'A
les plus à droite et les h sites de B les plus à gauche. Elle produit
exactement les comptes saturés à h et les mêmes 55 paires. Cette fixture
offre ainsi **un contrôle positif pour le proposeur directionnel** et
**un contrôle négatif pour le mauvais raccord de la borne universelle**.
Elle n'établit pas que ce choix suffit hors de cette famille.

### 3. Borner le raffinement sans tronquer le résultat

Une limite de travail du **préfiltre facultatif** est compatible avec la
complétude : conserver les crédits déjà prouvés et envoyer tous les cas
indécis vers le résidu. On arrête de chercher des certificats ; on ne
supprime aucune candidate faute de budget. Cette distinction permet de
comparer un parcours conjoint borné à une sélection de petits ensembles,
sans exiger de finir des histogrammes exacts pour poursuivre.

Pour annoncer une préparation O(h(|A|+|B|)), il faut imputer dans ce budget
aussi les propositions, allocations, tris, mises à jour de lignes et
visites effectivement exécutées. Borner seulement le nombre de couples
de blocs ne borne pas les mises à jour de toutes leurs ancres. La borne
ne couvre pas les candidates ni leur aval ; **P0 reste ouverte** tant que
leur travail total n'est pas mesuré. Ce mécanisme est une proposition,
pas une qualification du futur parcours v8.

## Preuve reproductible et entretien

Rejeu des contrôles C++ (compilation dans des copies temporaires neuves
incluse) :

```bash
python3 -B audits/morsehgp3D_v8_complementaire/predicates_checks.py
python3 -B audits/morsehgp3D_v8_complementaire/local_credits_checks.py --selftest
```

Le premier runner refuse des headers différents des pins audités ; le
second capture un nouvel instantané et ses empreintes. Une nouvelle sortie
ne réécrit pas les reçus clos. Les mutants compilés changent la frontière
stricte, remplacent un maximum par un minimum, répètent une colonne à
l'expansion et doublent un crédit de feuille ; tous sont réfutés par le
contrôle attendu, pas par un échec de compilation ou de sanitizer.

Prochaine couture utile : comparer les familles de crédits sur les mêmes
entrées et mesurer la consommation du résidu. Les contraintes de réemploi
des IDs du cœur et de propriétaire de nuage sont déjà suivies dans le
dialogue de l'autre auditeur ; aucun avis redondant n'est ajouté.

[`p0_quantifiers_gate.py`](p0_quantifiers_gate.py) est un modèle exact borné,
sans import du produit ou de l'oracle v7/v8. Il énumère les témoins sur les
sites opposés, vérifie la borne continue indépendamment, les comptes
saturés, les paires physiques et la frontière ouverte. Modes normal et
`-O` exigent chacun 27 comparaisons de lane/seuil et 18 réfutations des
deux modèles fautifs, avec succès et rejets non vides. Code de succès 0,
échec de contrôle 1, arguments incorrects 2.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/p0_quantifiers_gate.py --selftest
python3 -B -O audits/morsehgp3D_v8_complementaire/p0_quantifiers_gate.py --selftest
```

Le [journal de coordination](../COORDINATION_MORSEHGP3D_V8.md) reçoit les
constats utiles et les fenêtres d'index. Ce dossier garde une seule entrée
courante ; les preuves closes restent à leur emplacement pour ne pas casser
leurs chemins ou hashes. À cette date les six rapports constructeur v8
sont tous récents : aucune suppression ni archive n'est justifiée.
Les reçus v7 et les fichiers en cours des autres intervenants sont préservés.

La porte documentaire générale exclut volontairement les audits indépendants :
valider aussi explicitement ces Markdown avec `tools/check_docs.py:validate`.
Lectures ciblées : entrées v8, plan P0, verrous B1–B5, rapport WSPD et
primitives historiques utiles. Cette passe ne réaudite pas le manuscrit
intégral, le moteur v7, une tour FULL v8 ou un backend GPU. GCP non utilisé.
