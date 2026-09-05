# Réponse courante : composition horizontale sous fermeture de fenêtre

4 septembre 2026, conclusions actualisées le 5 septembre. Cadre : `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le raccord d'ancrage du § 5 est justifiable sous S.** Il ne nécessite
pas de resolver top-K supplémentaire. La preuve gagne à séparer trois
faits ci-dessous pour éviter de supposer la bijection au moment de la
construire. Cette réponse examine le [texte du constructeur](../docs/PREUVE_HORIZONTALE_COMPOSITION.md)
et ses [deux questions](QUESTION_COMPOSITION_CONSTRUCTEUR_20260904.md).
La preuve de S1 et sa qualification compilée sont portées séparément par
les notes réunies dans le [certificat horizontal CPU E](CERTIFICAT_HORIZONTAL_COURANT.md).

Texte examiné : SHA-256
`5aba4f18a90e4fabf05503ee2e420afbb079b72956ef7ec6bfdafd6ea3422e70`.
On fixe ici $2\leq K\leq K_{\mathrm{eff}}$ ; l'exception K=1 conserve
ses racines initiales normatives.

## 1. Toute composante retenue possède un ancrage direct déjà actif

Considérons une coupe ouverte ou fermée et une coface retenue active.
Si elle est directe, elle contient immédiatement des facettes du cœur.
Sinon, elle appartient à une chaîne dont le suffixe entier est retenu,
avec niveaux strictement décroissants jusqu'à une coface directe. Deux
maillons consécutifs partagent une facette. Le suffixe est donc déjà
actif à cette même coupe et relie la coface à son ancrage direct.

Un terminal en cache ne change pas l'argument : il renvoie à un suffixe
déjà certifié. Le code alimente `completed` seulement après l'atteinte
du terminal, conserve les maillons et les trie à leurs vrais niveaux.
Un succès garantit cette fermeture par suffixes ; une sortie partielle
sur refus n'entre pas dans le théorème.

Ainsi aucune composante non triviale du sous-flot n'est une composante
flottante formée uniquement de facettes hors cœur. Ce fait est interne
au sous-flot et ne suppose aucune bijection préalable avec Gamma.

## 2. Toute composante non triviale de Gamma possède un ancrage direct

Dans la généalogie finie d'une composante, choisir une naissance non
triviale. Une coface non-Gabriel régulière possède un apex strict non
trivial : les remplacements essentiels donnent des cofaces antérieures.
Un bloc irrégulier hors fenêtre possède également un apex strict non
trivial par le théorème d'inertie. Aucun des deux ne peut donc constituer
seul cette naissance. Sous S et le refus des shells pertinents, elle
contient une coface directe régulière du catalogue fourni.

Cette coface persiste dans les composantes ultérieures. L'argument prouve
l'existence de l'ancrage dans Gamma ; il ne demande pas au produit de
parcourir cette généalogie ni de construire Gamma.

## 3. Construire la bijection par inclusion des facettes

Chaque coface retenue étant une vraie coface Gamma à son vrai niveau,
l'inclusion définit une application des composantes retenues vers les
composantes non triviales de Gamma. Cette application est définie par
les facettes, **pas par l'égalité des ensembles de points couverts** :
ces derniers peuvent se recouvrir à ordre supérieur.

Supposons, à une coupe donnée, la même activation incidente et les mêmes
classes sur le cœur global. Le point 1 donne un ancrage du cœur à chaque
composante retenue. Si deux composantes retenues avaient la même image
Gamma, leurs ancrages seraient équivalents sur le cœur, donc elles seraient
déjà une seule composante retenue. L'application est injective. Le point 2
et la présence de toutes les cofaces directes donnent sa surjectivité.

Il suffit donc à l'induction par niveaux de préserver les classes du cœur,
avec les contacts égaux et stricts distingués. Au premier niveau d'incidence
d'une facette du cœur, sa chaîne la relie au bon ancrage antérieur ; la
confluence garantit que le choix d'un seul minimiseur ne choisit pas un
autre apex. Les contacts stricts ont déjà été installés. Le lemme de contact
du § 3 écarte une première incidence cachée dans un bloc irrégulier omis.

Le raccord vaut aussi pour les pièces retenues hors cœur. Chaque maillon
publié a une miniball globalement régulière : la requête termine le shell
avant son émission. Un contact égal avec un bloc irrégulier imposerait
donc une même miniball à la fois régulière et irrégulière. Un contact égal
avec une coface non-Gabriel régulière relève de la confluence. Pour un
contact strict, les apex Gamma antérieurs coïncident, et le suffixe retenu
localise le maillon dans leur unique composante retenue par induction.
La facette commune hors cœur n'a pas besoin d'un jeton artificiel tiré
de Gamma. Si la pièce retenue est directe, sa facette de contact appartient
au cœur et l'invariant fournit déjà sa première incidence.

Les parties non-Gabriel régulières et les blocs saturés omis couvrent leurs
points dans leur apex strict. Ils n'apportent donc aucun point nouveau au
lot. Les cofaces directes, présentes des deux côtés, apportent les mêmes
points aux groupes correspondants. La couverture se conserve par induction.
L'application par inclusion commute avec les inclusions de coupes : les
naissances réduites, parents abstraits et nombres de parents sont préservés.
Les identifiants exhaustifs et les premières matérialisations du sous-flot
restent des objets distincts.

## 4. Précision utile pour le lemme de contact du § 3

Noter séparément le cardinal minimal positif du support et le cardinal du
shell : une égalité de ces deux nombres ne découle pas du seul rayon.
Écrivons $q=\lvert U\rvert$, $p=\lvert X\cap B^{\circ}\rvert$ et $e=\lvert X\cap\partial B\rvert$.
La fenêtre utilise $p+q$, et non $p+e$.

Pour un bloc hors fenêtre, $\lvert S_B\rvert\geq p+q\geq r_{\max}+1\geq K+2$.
Le graphe strict connecté qui couvre tous les points de ce saturé ne
peut donc se réduire à une seule facette de cardinal K. Toute facette
stricte y possède un voisin, donc une coface incidente de niveau strictement
inférieur. Cette étape justifie précisément le passage de la connexité
stricte à la première incidence antérieure, utilisé par le constructeur.

Le [test de fenêtre exécuté](RETOUR_MATH_COURANT.md) oppose une boule avec extra-shell à
$p+q=11$ à une boule à $p+q=12$ : la première doit être reçue puis refusée,
la seconde peut être omise par inertie. Les 88 contacts stricts du cœur
possèdent chacun une coface incidente antérieure dans l'oracle. Ces fixtures bornées complètent
le raccord mathématique ; elles ne prouvent pas seules S1 pour tout nuage.

## 5. Ce que ce raccord permet de fermer

La composition peut être présentée comme un théorème horizontal
conditionnel, avec S1–S4 et les garanties exactes locales nommées.
La régularité géométrique globale et un resolver top-K implicite n'ont
pas à être ajoutés comme exigences générales de cette route.

Le raccord S1 est porté par le [théorème géométrique conditionnel](S1_COURANT.md),
qui suit le parcours jusqu'au RLE. Ses primitives, le [domaine CPU](DOMAINE_CPU_COURANT.md)
et les [frontières compilées](receipts_front_compiled_20260905/README.md)
sont désormais raccordés à la route E qualifiée. Le
[certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) ferme leur
composition avec la complétion et le lecteur des deltas, dans son domaine
local accepté et après succès terminal. La qualification des primitives et
l'assemblage horizontal ne restent donc pas des demandes ouvertes sur E ;
les sources F concurrentes conservent une attribution distincte.
La verticale, les poids de rendu, les identités publiques du quotient,
la reprise et les coûts d'échelle gardent leurs propres critères de qualification.

GCP non utilisé.
