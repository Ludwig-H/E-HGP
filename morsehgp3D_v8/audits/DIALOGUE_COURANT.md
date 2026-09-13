# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, reprise après 7cea0eaf. Écritures limitées à ce dossier,
sur main. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Les six rapports du constructeur et son
ETAT_COURANT en cours de modification restent sous leur autorité.

## Propriétaire : contre-fixture et correction qualifiée sur copie

Le header `local_credits.hpp` **791dfe12** laisse publiques ses opérations
implicites de copie et d’affectation. Cette séquence utilise seulement l’API
publique, sans cast ni accès privé :

```cpp
auto mutable_owner = std::make_shared<PreparedRectangle>(*good);
auto plan = make_credit_plan(mutable_owner, lane, strategy);
*mutable_owner = *other;
```

Les crédits du plan restent anciens, tandis que son rectangle change.
L’avis précédent sur le propriétaire était trop favorable : la constance
du pointeur n’impose pas celle de l’objet. La [contre-fixture permanente](p0_owner_gate.cpp)
montre **trois paires q2 valides perdues** avec Kmax=1, s=8 et cœur vide :

- rectangle initial : A={(0,0,0),(1,0,0)}, B={(100,0,0),(101,0,0)} ;
  DualBlocks conserve correctement une paire ;
- rectangle de remplacement : A={(0,0,0),(0,10,0)}, B={(1000,0,0),(1000,10,0)} ;
  un plan neuf conserve correctement les quatre paires ; le plan ancien
  attaché à l’objet réaffecté en perd trois, jugées par produit scalaire direct.

**Correction à intégrer :** supprimer explicitement les quatre opérations
de copie/déplacement de PreparedRectangle, constructeurs et affectations.
La factory alloue directement ; les shared_ptr restent copiables.
La copie corrigée du header **5f4f6bad** ferme les quatre traits de type
et conserve les deux contrôles géométriques nominaux ainsi que l’usage
factory→pointeurs partagés→plans. Aucune source produit n’a été modifiée
par cet audit ; cette qualification ne prétend pas que le correctif est intégré.

[Reçu](P0_OWNER_CHECKS.json) : original et correction locale passent chacun
en C++20 strict, O2 et ASan/UBSan avec détection des fuites ; sorties de
diagnostic vides, CLI absente/inconnue→2. Dix-sept commandes sont conservées,
avec textes et hashes des cinq sources, du juge et du runner, correction
exacte, commandes de compilation et hashes des exécutables avant/après.
Le succès de la variante originale signifie **défaut reproduit**, celui
de la variante corrigée **ce défaut fermé**, sur ces fixtures seulement.
Les sources live correspondent encore à la capture à sa fermeture.

Rejeu depuis la racine, dans un nouveau reçu créé exclusivement dans audits/
([runner](p0_owner_checks.py), contrôles effectifs sous Python −O) :

```bash
python3 -O morsehgp3D_v8/audits/p0_owner_checks.py --selftest --snapshot morsehgp3D_v8/audits/P0_OWNER_CHECKS.json --output morsehgp3D_v8/audits/owner_replay.json
```

## Proposition suivante : borner le raffinement, garder les minorants

Le `DualTree` courant (**local_credits.cpp b8a7eef8**) permet une variante
Tubes puis DualBlocks partiel, **proposée, non implémentée ni chronométrée**.
Un budget J compte les tâches effectivement visitées, partagé entre A et B.
À épuisement, remonter immédiatement l’arrêt, conserver les mises à jour
acquises, puis extraire les crédits sans développer les blocs indécis.

Pour une ancre non saturée par Tubes, démarrer son compte Dual à zéro et
prendre le **maximum** des deux minorants à la fin. Les feuilles déjà
saturées par Tubes peuvent partir à h : leur ligne ou colonne est déjà
éliminable. Ici h est le besoin restant après crédit du cœur. Recalculer
les minima internes, avec ajouts différés initialement nuls. Chaque mise
à jour Dual porte ensuite sur des témoins distincts pour l’ancre, grâce
aux produits disjoints du parcours ; les sommes internes restent sûres.
Le maximum final l’est donc aussi, sans transporter les identités des
témoins Tubes. Les crédits A/B restent additionnables entre eux, puisque
leurs populations sont disjointes.

**Contre-fixture à conserver pour ce raccord :** A={(0,0,0),(1,0,0)},
B={(100,0,0)}, q2, h=2, cœur vide, s=12. Pour l’ancre 0, Tubes et le Dual
partiel après quatre tâches du DFS courant comptent chacun le même site 1.
H=99>0 ; la paire (0,100) n’a qu’un site intérieur. Additionner ces deux
comptes la supprimerait à tort ; prendre leur maximum la conserve.

Pour m=|A|+|B| et un rectangle **déjà préparé**, la borne proposée est
O(m log m+48m+J+h²), mémoire O(m+h²). Le tri Tubes est payé, l’arbre u16 a
au plus 48 coupes par point, chaque tâche Dual coûte un nombre constant
de tests/ajouts, puis l’extraction visite chaque nœud une fois. L’arrêt
ne doit ni parcourir une file cachée ni reprendre les tâches abandonnées.
Sommer les préparations et h² pour tous les rectangles, même avec un
budget global de tâches. Validation/propriété du nuage et coût aval exclus
de cette borne locale ; **aucune borne sur le résidu n’en découle**.

Cette proposition précise le raffinement facultatif évoqué par l’autre
auditeur. Elle complète son ordre de visite vers les bons témoins ; elle
ne résout pas sa famille de rangées transverses où même les crédits
universels exhaustifs restent nuls. Voir sa
[coordination](../../audits/COORDINATION_MORSEHGP3D_V8.md).

## Avis repris et entretien

Le raccord `tube_credits.hpp` **a850a442** est lu favorablement : séparation
d²≥100·diam², repli nul hors hypothèse, origine translatée, Δ>0, produits
i128 et suffixes monotones. La [preuve et les contre-fixtures](P0_TUBES_ET_RANGS.md)
et leur [reçu borné](P0_TUBES_CHECKS.json) restent les références du modèle ;
les qualifications C++ ont leurs propres reçus, sans transfert implicite.

Les demandes suivantes sont désormais documentées dans
`docs/P0_CREDITS_LOCAUX.md` : séparation sur distance de boîtes/diamètres,
tri encore payé par voie, cœur non réutilisable sans IDs ou disjonction,
nuage/validation à partager avant la WSPD, résidu à mesurer avec l’aval.
Elles sont retirées de la liste des questions ouvertes. Le NoCredit à
coin b₀ fixe et l’expansion via les permutations du plan restent lus
favorablement, sous réserve de la correction du propriétaire ci-dessus.

Pas de nouveaux doublons des rapports d’ouverture ni de déplacement
d’archives v7. Les preuves reproductibles restent conservées ; les avis
repris sont condensés ici. Contrats 50k, massif et FULL ouverts.

Contrôles de livraison : les deux Markdown indépendants passent leur
validation explicite ; le registre passe ses 20 phases. La vérification
du reçu sous Python −O confirme les quatre builds/essais, huit rejets CLI,
empreintes et nettoyage. Après arrivée du README de reçus constructeur,
le contrôle documentaire global passe : **528 Markdown**. Les cinq liens
manquants observés pendant son écriture sont ainsi clos.

Les quatre fichiers propres ont été publiés sur main dans **b28969a5** ;
leur réservation est close. Cette actualisation des contrôles réserve
seulement DIALOGUE_COURANT.md jusqu’à son commit/push. Aucun fichier du
constructeur ou de l’autre auditeur n’entre dans cette préparation.
GCP non utilisé.
