# Raccord natif global : prochaine tranche utile

21 septembre 2026. Plan d'implémentation, pas qualification.
Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`lossless_float32_input_only`, `not_claimed`. Aucun nouveau test exécuté ici.
Objectif prochain : **flux q3 global exact sur un même index float32**,
puis voie q4 indépendante ; ni catalogue, ni tour FULL, ni GPU implicite.
Le contrat reste la [trame entière](CONTRAT_TRAMES_SEMANTICKITTI_20260921.md).
Un pilote sans sol est un diagnostic distinct, jamais son remplacement.

## 1. Ce qui existe, ce qui manque

L'[index natif](../src/spatial/float32_index.hpp) et les prédicats exacts
sont disponibles. Le [census q3 partagé](CENSUS_Q3_FLOAT32_PARTAGE_20260921.md)
traite UNE arête fournie et UN sous-arbre X ; il ne sélectionne pas l'arête
propriétaire. Ses mesures colonne/bande ne bornent pas la somme sur les arêtes.
Le [raccord global u16](../src/pipeline/wspd_q34.cpp) donne des invariants
à porter explicitement, pas du code ou des preuves numériques à hériter.
Il manque front natif, filtres de produits/arêtes, sélection propriétaire
des graines et raccord des tickets partagés dans ce générateur complet.

Le commit indépendant B `21b85af243bcc63a72938f041ceb458fc47d0302` fournit
[huit familles rationnelles](../audits/q3_bloc_float32_fixtures_20260921/README.md).
Il confirme la sûreté de l'enveloppe actuelle, mais montre ses pertes de
précision : sur l'axiale, elle reste indécise là où la factorisation A
resserrée certifie tous les témoins ; la rotation exerce le même défaut.
Ces fixtures deviennent des tests du port, pas des résultats constructeur.

## 2. Une tranche complète, quatre raccords

**A — Front natif en flux.** Conserver index, permutations et IDs originaux.
La décomposition diagonale LL/LR/RR partitionne les paires non ordonnées ;
les produits disjoints divisent un facteur jusqu'à séparation ou rejet.
Convention publiée, comparée pour s8/10/12 :

$$ \mathrm{gap}(A,B)^2\ge s^2\max\bigl(\mathrm{diag}(A)^2,\mathrm{diag}(B)^2\bigr). $$

Décider par intervalles extérieurs, puis comparaison entière exacte si
nécessaire ; l'indécision peut aussi raffiner, jamais accepter à tort.
Deux singletons distincts doivent terminer sans boucle. Le choix du facteur
à diviser peut être heuristique : il ne remplace aucun certificat.
La médiane borne la profondeur de l'index, **pas** le nombre de produits
WSPD ; ne pas lui transférer sans preuve la borne d'un autre split-tree.
Pas de tableau global des arêtes. Les masques q3/q4 et leurs masses restent
distincts ; K1 donne un flux q3 vide, pas une erreur de masque actif.

**B — Rejeter avant de préparer les graines.** Tester des témoins universels
sur A×B, puis rechercher dans l'index Z pour chaque arête résiduelle.
Préparer une fois les constantes de l'arête, pas une fois par nœud Z.
Pour endpoints fixes, utiliser les composantes affines du produit vectoriel :

$$ H=(z-a)\cdot(b-z),\qquad \Xi=\lVert(b-a)\times(z-a)\rVert^2,\qquad H>0\ \land\ \alpha_qH^2>\Xi,\quad \alpha_3=3,\ \alpha_4=2. $$

Le certificat concerne les supports positifs dont ab est une arête maximale.
Un bloc Z admis crédite ses sites distincts ; un bloc exclu ne crédite rien.
L'exclusion citron utilise aussi la borne inférieure de Xi, pas seulement
le rejet extérieur à la boule diamétrale. Égalité = aucun témoin strict.
Seuils séparés K−1/K−2 ; seul un compte saturé retire la voie correspondante.
Le rejet d'une voie dans UN nœud Z n'est pas un rejet global de l'arête.
Pour A×B non singleton, employer des bornes universelles sur les trois boîtes,
jamais la formule affine spécialisée à des endpoints prétendument fixes.
Ni crédits partiels, ni ordre proche-du-milieu ne sont transmis au census.
Propositions bornées permises ; échec du proposeur signifie poursuite exacte,
pas quota de recherche ni balayage scalaire obligatoire de tout le nuage.

**C — Propriété et partage dans la même descente X.** L'arête propriétaire
est la plus longue ; égalité départagée par paire d'IDs triés, lexicographique.
Par boîte X, rejeter sûrement si distance minimale à a ou b dépasse |ab|²,
ou si le maximum de |x−a|²+|x−b|² ne dépasse pas |ab|² ; sinon conserver.
Au relais, vérifier acuité et propriété exactes AVANT le census individuel.
Ne pas appeler l'ancien census sur rootX pour chaque arête puis filtrer ses
émissions : cela paierait les graines non propriétaires et leur census.
Intégrer ces tests à chaque cadre X du SharedPrefix, conserver `(X,c,curseurZ)`
aux divisions et relayer directement son ticket ; aucun redémarrage avec c.
Une entrée nouvelle « owned » laisse l'ancienne API toute graine aiguë intacte.
La lentille sert à sélectionner X, jamais à retirer ces points de Z.
Un cover matérialisé n'est pas nécessaire au premier raccord q3 : Z global
est déjà disponible et la sélection de X utilise ses nœuds sans liste copiée.

**D — Sortie globale qualifiée.** Émettre chaque triangle positif propriétaire
de profondeur <K−1, support trié, profondeur exacte et coquille complète.
Trier la coquille seulement si le nouveau contrat le demande, coût publié.
Construire la clé canonique à l'émission ; ne pas confondre unicité du support
et unicité de la boule. Deux triangles cosphériques restent deux incidences.
L'entrée initiale s'annonce q3 seulement ; demander q4 non porté doit être
refusé explicitement, jamais produire silencieusement un flux incomplet.

## 3. Resserrer les blocs sans changer leur population de témoins

La [preuve A](../audits/q3_seed_block_power_20260921/README.md) et B donnent,
avec d=b−a, D=|d|², u=x−a, E=|u|², F=d·u et G=|d×u|² :

$$ m=\frac{a+b}{2},\qquad h=u-\frac{F}{D}d,\qquad c=m+\xi h,\qquad \xi=\frac{D(E-F)}{2G},\qquad \lambda=2\xi. $$

Sous acuité seule, 0<λ<1 ; sous propriété d'arête, 0<λ≤2/3.
La seconde borne est conditionnelle aux seules graines positives propriétaires :
X peut aussi contenir des points invalides. Son contrat doit le dire ; ne pas
l'appliquer à l'ancienne API sans propriété. B donne λ=5304/5305 sinon.

Réponse B `ad9bbc9d` à notre question : l'enveloppe m+[0,1/3]h sous
propriété est valide et serrée (équilatéral). Avec a=(0,0,0), b=(10,0,0),
x=(5,8,0), les points z₁=(9,5,0), aigu non propriétaire, et z₂=(5,1,0),
obtus, sont pourtant deux témoins stricts de puissances −67/8 et −231/8.
Les exclure de Z sous-compterait la boule. Graver ce cas dans la porte du
port, ainsi que x=(6,8,0) : deux arêtes maximales de carré100 imposent
un unique départage lexicographique. Une boîte peut conserver des égalités
indécises, jamais les rejeter strictement avant ce départage au relais.
Sous propriété certifiée, l'acuité en x suffit aux trois angles ; ce
raccourci reste conditionnel, il ne remplace pas la positivité de l'API libre.

Préparer une enveloppe finie par X, intersecter avec le hull et resserrer λ
si le Gram est certifié positif. Ambiguïté conserve l'enveloppe sûre.
Pas d'inégalités « entier positif donc ≥1 » sur les coordonnées physiques.
Pour la puissance sur Z, minimum au sommet continu ramené dans la boîte,
maximum aux extrémités : les seuls coins sont faux pour le minimum (fixture B1).
Une graine de X peut être témoin d'une autre (B6) : ne jamais exclure tout X.
La coquille acceptée repart globalement, y compris contacts du préfixe et a/b.

## 4. Arithmétique et tâches possédées

Différence entière dans l'unité 2^-149 : magnitude <2^278. Distances, propriété
et séparation sont de degré2 ; citron de degré4, sous 2^1122 même en forme 4H
et différence des deux membres. Les 1728bits suffisent à ces expressions
explicitement bornées et aux puissances q3 existantes, pas à un degré9 implicite.
Filtres double extérieurs, repli exact ; mêmes options strictes de compilation,
quatre arrondis et FTZ/DAZ requalifiés. Aucun élargissement aveugle d'i128/Q44.

Contexte immuable possédé = index, endpoints, seuil, contrat de validité, ordre Z.
Une tâche X possède son ticket certifié ; frères disjoints et suffixe non consommé.
Une équipe persistante pourra partager produits puis frères X, avec tampons privés,
sans copie du nuage, équipe par arête ni pointeur vers une pile productrice.
Grain = compromis de partage, jamais troncature. Ne pas conserver l'arête entière
atomique comme unique unité : les anciens 48workers ne garantissaient pas 48CPU utiles.
Q4 devra garder sa propre génération positive/canonique et son census : ni q3
accepté, ni rejet q3, ni son crédit ne commandent la voie q4.

## 5. Juger le coût de bout en bout

Publier index ; produits/front et masses résiduelles Σ|A||B| ; arêtes réellement
expansées/filtrées ; visites/préparations des témoins avant census ; nœuds X,
population candidate Σ|X|, graines relayées/invalides/non propriétaires ; visites
partagées + individuelles + coquilles, paraboles, replis exacts ; clés, incidences,
IDs coquille, allocations et pics simultanés ; enfin travail/attentes par worker.
Un index unique n'empêche ni arêtes quadratiques, ni Σ|X|=arêtes×n,
ni census par graine×n. Déplacer ces termes dans des préparations n'est pas un gain.
Tester contre Fraction tous petits supports, propriété à égalité, tangences citron,
masques indépendants, fixtures B1–B8, contacts préfixe et mutations causales.
Comparer Individual/Shared et futurs workers sur le flux complet, pas son seul hash.
Puis synthétiques8/16/32k et sept partitions des vraies trames, N réels publiés,
avant trames entières de plusieurs scènes ; comparer s8/10/12 et K5/10.
Mesurer la croissance de chaque poste avec préparation/résidu/sorties inclus.
Ni prototype B, ni succès local d'un relais ne ferment la croissance globale.

La contrelecture B `f7b220c4` ajoute trois obligations précises :

- compléter les colonnes à K−1 sorties constantes par des graines3D dont
  les sorties acceptées croissent avec n, sans prétendre mesurer ce régime
  avec la matrice précédente ;
- publier à l'échelle les rejets extérieurs partagés, préparations sans
  Gram résolu, replis hull/intersections et recours entier exact ; B compte
  843replis sur1642préparations et85intersections vides dans les petites
  fixtures précédentes, pas sur les trames réelles ;
- séparer les chronos des compilations, mutations et gates instrumentées
  du constructeur. L'hôte partagé avec l'auditeur reste non isolé ; publier
  les répétitions et la charge, pas un rapport de temps choisi.

Les gains de la matrice précédente restent des réductions de travail sur
une seule arête. Le nouveau pilote de sol n'ajoute aucune mesure du census.
