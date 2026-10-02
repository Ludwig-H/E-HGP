# Audit indépendant v11 — bornes numériques et contrats de port

2026-10-02 06:26:32 UTC. Documents d'ouverture ancrés à `52687f8e5`.
Audit de conception : aucun moteur v11 exécuté, aucun GCP, `not_claimed`.
Une seule note courante de cet auditeur ; sources/rejeux dans `receipts/`.

**Les fondations peuvent avancer. Corriger F3 avant de l'utiliser dans un
filtre**, puis requalifier chaque port. L'absence de code à cette lecture
ne permet pas de conclure sur son exactitude ou ses performances.

## 1. F3 : remplacer le compte d'instructions par une borne d'expression

La borne `(1+u)^m−1`, u=2^-52, échoue si une approximation est réutilisée.
Notre témoin réellement exécuté : convertir N=2^53+1 et D=2^53, diviser,
puis faire quatre carrés. Sept opérations donnent toujours 1 ; la valeur
exacte est `(N/D)^16`. Chaque erreur élémentaire respecte ≤u, mais l'erreur
finale dépasse la borne annoncée : environ 8u contre 7u. Une clé dyadique
voisine donne aussi un ordre inversé si cette borne sert à certifier F4.
Aucun filtre produit défectueux allégué. Ce résultat concorde avec le
contre-exemple dirigé de l'autre audit d'ouverture, déjà déposé localement.

Correction constructive : propager des intervalles de facteur par expression,
avec réutilisations comptées deux fois et intervalle du dénominateur inversé.
Variante simple `constexpr` : facteur dans `[(1−u)^E,(1−u)^−E]` ; conversion
E=1, produit/quotient E=Ea+Eb+1, somme de même signe E=max(Ea,Eb)+1.
Le carré vaut donc 2Ea+1. Borner aussi les transformations autorisées ; l’auto-test F5 ne remplace pas cette preuve. F2 et les budgets d'entiers ne
sont pas réfutés. [62 gardes normal/−O et proposition](../receipts/audit_independant_20261002/floating_bounds/PUBLICATION.md).

## 2. R2 : reprendre les correctifs, attribuer les preuves à leur version

Le vieux mutant Pool MR1 est **fermé dans final5** ; ne pas redemander son
correctif. MR1b est un survivant différent, déclaré équivalent sur le périmètre
contrôlé. Les 425 essais relus comprennent 387 rejets par juge, 5 signaux,
1 délai et 32 survivants déclarés : ces causes restent distinctes.

La série conservée contient 30 patches jusqu'à `210b9fc`, tandis que la
source annoncée pour le port est `865f5e6`. Les changements C++ intermédiaires
sont des commentaires, sans nouveau calcul produit identifié ; deux portes
FP ont néanmoins été ajoutées, avec compteurs 84/82 distincts de 82/80.
Le reçu de clôture est encore annoncé en rédaction. Épingler les fichiers
réellement portés et leurs portes v11 ; aucun héritage automatique.
[Recoupe de provenance et limites](../receipts/audit_independant_20261002/provenance_review/README.md).

## 3. Trois décisions d'architecture à préciser tôt

| Contrat | Décision attendue |
| --- | --- |
| Budget et propriétaires | Compter capacités, agrandissements ancien+nouveau, scratch/tri/IO et résultats simultanés. Fixer la durée de vie du budget vis-à-vis des résultats/vues et du Pool ; un vector hors boucle n'est pas forcément petit. |
| Transaction et reprise | Définir l'opération publique atomique. Segments/checkpoints privés et publication par manifeste permettent de conserver le résultat précédent sur refus ; RAM, disque et périmètre du chrono doivent être explicites. |
| Formats et profils | h exact, origine commune, IDs retours→sites, profils et unités ; PointId, indices de boules, offsets et rangs exacts sont des domaines distincts. U18 est la voie initiale à requalifier ; compiler 21/24 ne les qualifie pas. Conserver l'étape u32 demandée et déclarer la sémantique des multiplicités. |

[Revue des quatre documents](../receipts/audit_independant_20261002/architecture_review/README.md).
Pour la frontière : conserver le continuum de centres, les incidences
internes et les vrais plateaux. Une taille mûre recouvrante ne garantit pas
mcs membres exclusifs après projection. [Témoin minimal de trois points](../../morsehgp3D_v10/receipts/audit_independant_20261002/maturity_review/README.md).
Présence dans FULL, projection et compatibilité restent les premières
questions ; la sélection ne doit pas masquer leur perte.
