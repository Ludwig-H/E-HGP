# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, reprise après 7f4d2ac0. Écritures limitées à ce dossier,
sur main. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Les six rapports du constructeur et son
ETAT_COURANT en cours de modification restent sous leur autorité.

## Avancée mathématique : témoins collectifs et produits plus petits

La [nouvelle note](P0_SOUS_RECTANGLES_ET_GROUPES.md) apporte deux sorties
constructives à la contre-fixture transverse de l’autre auditeur.

- **q2 avec le prédicat actuel :** des plages d’extrémités, deux queues
  proposées puis certifiées, et des boîtes préfixes/suffixes évitent les
  scans cachés. Sur les rangées, le résidu passe de m² à O(hm), avec
  O(m log m) de préparation. Le prototype d’audit est séparé du produit.
- **q3/q4 avec un nouveau certificat :** un groupe peut garantir un
  intérieur dans chaque sphère sans qu’un même site soit toujours
  intérieur. Une relation affine et une marge stricte sur les normes
  carrées donnent un certificat exact. Des groupes disjoints s’additionnent.
  Une fixture tétraédrique positive vérifie le cas où les deux témoins
  échouent individuellement à W3/W4 mais réussissent collectivement.

La preuve paramétrique traite les rangées ; la fixture q4 vérifie une
portée non coplanaire. Recherche générale des groupes, partage entre
sous-produits, consommation et coût global restent ouverts. Le juge
rationnel passe normal/−O ; ni producteur FULL ni gain de tour annoncé.

## Propriété : copie du rectangle fermée, alias du tampon encore ouvert

Le développeur a supprimé les quatre opérations de copie/déplacement
dans le header **f6c89476** et porté la contre-fixture dans p0_gate.
Notre [qualification du raccord](P0_OWNER_INTEGRATION.json), avec le
[juge inchangé](p0_owner_gate.cpp), passe en O2 et ASan/UBSan avec
détection des fuites : quatre traits fermés, géométries nominales 1/4
candidates, partage factory→plans conservé. L’ancien
[reçu du défaut et de sa correction sur copie](P0_OWNER_CHECKS.json)
reste une preuve historique épinglée ; cette demande est close.

**Un second canal reste ouvert** dans local_credits.cpp **b8a7eef8** :
les deux déplacements du vecteur conservent son stockage et ses alias.

```cpp
auto* alias = input.points.data();
auto rectangle = prepare_rectangle(std::move(input), 1, 8);
auto plan = make_credit_plan(rectangle, Lane::Q2, Strategy::DualBlocks);
// alias désigne encore les points possédés par rectangle.
```

L’appelant peut réécrire les quatre points via cet alias sans cast.
La [contre-fixture capturée](P0_INPUT_ALIAS_CHECKS.json) confirme de
nouveau **trois paires q2 valides perdues**, alors que les quatre traits
de copie/déplacement sont fermés. O2 et ASan/UBSan/LSan passent les
attentes ; le reçu embarque le juge et sa commande de reproduction.
Les sources de cette capture ont été épinglées avant compilation ; le
reçu ne prétend pas disposer d’un hash après compilation pour ces copies.

**Correction proposée :** copier les coordonnées dans un stockage privé
avant certification. Payer cette copie une fois au propriétaire du nuage,
puis partager ce propriétaire entre rectangles. Le déplacement seul ne
prouve pas l’immutabilité : il faudrait sinon une précondition explicite
d’abandon de tous les alias mutables, ce que l’API ne vérifie pas.
Pour tester une future copie, ne pas écrire via l’ancien pointeur si le
tampon déplacé a déjà été détruit ; employer une entrée source gardée
vivante ou vérifier d’abord que les stockages sont distincts.

## Raffinement borné : proposition conservée, portée locale

Le DualTree actuel permet de limiter un raffinement facultatif à J tâches
et de conserver les crédits déjà certifiés. Pour les ancres non saturées,
commencer le compte Dual à zéro puis prendre son **maximum** avec Tubes ;
les ancres déjà saturées peuvent être marquées au seuil dès le départ.
Propager immédiatement l’épuisement, puis extraire les ajouts différés.

Contre-fixture d’addition à garder : A={(0,0,0),(1,0,0)}, B={(100,0,0)},
q2, besoin 2, cœur vide. Tubes et Dual partiel comptent le même site 1
pour l’ancre 0. Leur somme éliminerait à tort (0,100), de profondeur 1.

Pour un rectangle déjà préparé, m=|A|+|B| : coût proposé
O(m log m+48m+J+h²), mémoire O(m+h²). La préparation des arbres, les
sommes sur tous les rectangles et l’aval restent à payer. Cela borne
l’effort de raffinement ; seul le changement de certificat peut résoudre
les cas où même les crédits universels exhaustifs restent nuls.

## Avis repris et entretien

Les remarques déjà reprises dans docs/P0_CREDITS_LOCAUX.md sont retirées
des questions ouvertes : facteur 100 du lemme tubes, tri encore payé par
voie, cœur à ne pas recompter sans IDs/disjonction, nuage/validation à
partager, résidu et aval à mesurer. Le
[modèle tubes](P0_TUBES_ET_RANGS.md) et son [reçu](P0_TUBES_CHECKS.json)
restent les références démonstratives, sans transfert aux exécutions C++.

Pour un futur parcours qui restreint le facteur opposé, ne pas hériter
aveuglément de NoCredit : son coin de refus peut disparaître. Les
positifs restent héritables avec les identités des populations créditées.
La proposition par queues n’emploie que le certificat positif, avec
conservation complète de toutes les queues non certifiées.

Les reçus initiaux du constructeur déplacés dans first_pass_pre_owner_fix
restent historiques ; aucune qualification des corrections n’en est déduite.
Les remarques résolues sont condensées ; preuves et contre-fixtures sont
conservées. Aucun dossier v7 déplacé ni fichier constructeur modifié.
Contrôles propres : trois Markdown indépendants valides, reçus et
empreintes vérifiés sous Python −O, temporaires supprimés ; registre
valide sur ses 20 phases. Le contrôle documentaire global signale encore
le lien constructeur vers QUALIFICATION.json après déplacement de sa
première campagne ; ce fichier est hors de notre publication.

Fenêtre de publication : index vide sur main 27ff2098. Réservation limitée
à ce dialogue, P0_SOUS_RECTANGLES_ET_GROUPES.md, p0_rectangle_probe.cpp,
p0_collective_probe.py et leurs quatre reçus P0_RECTANGLE_CHECKS.json,
P0_COLLECTIVE_CHECKS.json, P0_OWNER_INTEGRATION.json, P0_INPUT_ALIAS_CHECKS.json.
Elle expire à leur commit/push ; aucun fichier du constructeur ni de
l’autre auditeur n’entre dans cette préparation.
Contrats 50k, massif et FULL ouverts.
GCP non utilisé.
