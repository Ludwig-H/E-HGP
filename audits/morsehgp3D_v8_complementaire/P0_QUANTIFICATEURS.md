# Quantificateurs des crédits de blocs — preuve conservée

13 septembre 2026, première passe complémentaire. Cadre hors registre v8,
CPU, entrée u16, audit indépendant, `public_status=not_claimed`.

La fixture distingue absence de témoin universel commun et absence de
crédit par ancre. Le prédicat v8 audité emploie correctement un b0 fixe
et un maximum sur les ancres : il évite le piège décrit ici. Cette preuve
et sa fixture restent utiles après leur intégration par le développeur ;
l'[état courant](ETAT_COURANT.md) porte les demandes encore actives.

## 1. Les quantificateurs à conserver

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

## 2. Fixture u16 et issue constructive

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

## 3. Borner le raffinement sans tronquer le résultat

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


## Rejeu borné

Le [modèle exact](p0_quantifiers_gate.py) vérifie 27 combinaisons de voie
et seuil, avec 18 réfutations de modèles fautifs, succès et rejet non
vides. Modes Python normal/`-O` : code 0 ; échec de contrôle 1 ; usage 2.
Le [reçu initial](MODELS_CHECKS.json) reste inchangé. Aucun import produit,
aucune qualification FULL ou chronométrique.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/p0_quantifiers_gate.py --selftest
python3 -B -O audits/morsehgp3D_v8_complementaire/p0_quantifiers_gate.py --selftest
```

GCP non utilisé.
