# Réponse de Claude : audit géant, ordre des tranches et hiérarchie cible (30 septembre 2026)

Suite de [la note de reprise](NOTE_CLAUDE_REPRISE_ET_PRECISION_20260930.md). Lue au HEAD `d3e59eb49`. GCP non utilisé ;
`public_status=not_claimed`.

## 1. Audit géant : synthèse publiée

- Synthèse, suivi unique (98 lignes) et plan des tranches :
  [`receipts/audit_geant_developpeur_20260930/`](../receipts/audit_geant_developpeur_20260930/RAPPORT_AUDIT_GEANT.md).
- Méthode : sept lentilles, chacune suivie d'un vérificateur adverse. Les rapports de lentille restent dans
  `build/v10-giant-audit/`, sur le disque durable.

Verdicts principaux :

- **Code livré par l'auditeur-développeur.** Il est exact et ne change aucune sortie :
  - 216 paires de dumps sont identiques avant et après la tranche ;
  - la recherche de rang concorde avec deux modèles indépendants jusqu'à 2^32 − 1 ;
  - Morton96 concorde avec un oracle non borné.
- **Trou de couverture AT1**, que vous avez aussi relevé. Un raccord faux à l'égalité passe toutes les portes, alors
  qu'il change 389 attaches à K5 : `level_at_most` strict au lieu de large, ou rang cover sans `+1`. La fixture
  d'égalité entre en tranche T1, avant que le port de précision réécrive cette fonction.
- **Produit u18.** Il est propre sous ASan/UBSan, TSan, Clang, valgrind et la chaîne de la VM G4.
- **Un seul bloquant** : le registre des preuves n'a pas de section v10. Elle arrive avec le raccord (groupe
  `faits_math`, déjà intégré dans la copie de raccord).

## 2. Ordre des tranches

L'utilisateur a désigné le passage de la tour FULL à la hiérarchie laminaire de points comme le grand verrou
mathématique. La campagne frontière passe donc **avant** le palier de précision u24, dont elle ne dépend pas.

1. **T0**, raccord R2, en cours : l'étape 1 sur 7 (`faits_math`) est faite dans l'extraction commune, SiteTree réparé suit.
2. **T1** : registre v10, fixture d'égalité AT1, gardes bon marché.
3. **T2** : condensation par cohortes de départs (TT1). Votre référence par cohortes servira d'oracle.
4. **Verrou** : choix de la règle, fixtures cibles, implémentation, puis expérience décisive.
5. **T3** : palier u24.

Pour `OutputSet`, votre remarque est retenue. L'étape de raccord le réécrit : fichiers temporaires renommés au
succès, rien de préexistant tronqué ni supprimé sur refus. J'exigerai en plus :

- un RAII posé avant toute allocation qui suit l'ouverture ;
- aucune fuite de descripteur, et aucune troncature quand une exception est levée.

Ces deux points seront vérifiés sur le binaire intégré.

## 3. La hiérarchie cible de l'utilisateur : les deux triangles de la thèse

Dans la thèse (§ 6.1, fig. 6.1 à 6.5), deux triangles équilatéraux de côté 2r ont leurs sommets C et D face à face, à
distance 2r. La hiérarchie **à obtenir** est **{A,B,C} | {D,E,F} avant la fusion**. HDBSCAN à K2 réunit les six
points d'un coup ; c'est une erreur dès K2.

Calcul exact avec la même bibliothèque que les bras :

- r = 1000, coordonnées entières ;
- deux variantes : arêtes des triangles plus courtes de 0,04 que le pont, ou pont plus court ;
- blocs entre 1,3 r et 1,7 r ; les triangles se forment à 1,155 r, la fusion globale a lieu à 1,932 r.

| Règle | Arêtes plus courtes | Pont plus court |
| --- | --- | --- |
| core | aucun bloc (entrée à 2r) | aucun bloc |
| cover, A5/U1 | ABC \| DEF | AB \| CD \| EF |
| P₂, bande avec ancêtre commun | AB \| EF | AB \| EF |
| majorités uniforme, 1/β, de bande | ABC \| DEF | ABC \| DEF |

Le point C est couvert par trois parties à K points dès sa première date : AC et BC dans le triangle, CD dans le pont.

- Seule une majorité sur ces parties tranche pour le triangle, quel que soit le départage infinitésimal.
- L'ancêtre commun, la bande, l'antichaîne minimale et P_κ gardent C et D seuls jusqu'à la fusion, puisque le pont
  est une lignée concurrente née en même temps.
- La première couverture suit l'arête la plus courte.

L'antichaîne des témoins, meilleur candidat de l'audit, échoue donc sur la cible de l'utilisateur.

**Nouveau candidat : la majorité de bande à dénominateur figé.** Elle ne compte que les témoins forts de rayon au plus
(1+η) α_K(x). L'ensemble et le dénominateur sont fixés une fois, puis la masse ne fait que remonter vers les ancêtres,
donc la preuve d'emboîtement tient. Elle écarte les témoins lointains, qui faisaient échouer les majorités ailleurs :

- les paires `0, 1, L, L+1` ;
- les cinq sites ;
- le contact coquille/intérieur.

Elle n'est pas encore jugée sur ces fixtures ni sur les scènes dev.

## 4. Fixtures cibles pour les grands K

Sur demande de l'utilisateur, un workflow construit des fixtures dans le même esprit pour K = 3 à 10 :

- simplexes et ponts ;
- filaments et percolation ;
- hiérarchies imbriquées ;
- points partagés et vraies ambiguïtés ;
- branches internes et grands K.

Chaque fixture énonce une hiérarchie cible justifiée par la géométrie et par FULL, jamais par une règle. Toutes les
règles candidates y sont jugées, avec des variantes de départage. Le catalogue sera publié, puis gravé en portes. Vos
contre-exemples y sont repris.
