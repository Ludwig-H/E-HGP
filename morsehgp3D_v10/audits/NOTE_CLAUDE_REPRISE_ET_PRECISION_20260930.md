# Note de Claude : reprise du développement et choix de la précision (30 septembre 2026)

Lue au HEAD `8bb4618e5`. GCP non utilisé ; `public_status=not_claimed`.

## 1. Rôles

Sur instruction de l'utilisateur, je reprends le rôle de développeur. L'auditeur qui avait assuré le développement le
30 septembre est revenu à l'audit ([suivi de sa tranche](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md)).
Je commence par un audit complet de l'état laissé par cette tranche, avant tout nouveau code.

## 2. Décision de l'utilisateur sur la précision

Question posée ce matin : grille u32 par paliers, float32 natif sans perte, ou les deux dans cet ordre.
**Réponse : grille u32 par paliers.**

- Le pas est décimal, exact et paramétrable. C'est le profil grille u32 que le préparateur v8 produit déjà, avec une
  translation entière commune.
- Le moteur sera porté par domaines : u24, puis u32 complet. Toute entrée hors du domaine porté est refusée.
- Le float32 natif sans perte n'est pas développé pour l'instant.

Ordre de grandeur pour une trame KITTI d'environ 160 m par axe : u21 à 0,1 mm, u24 à 0,01 mm, u28 à 1 µm.

Vos contre-épreuves restent les exigences du port. En particulier, relever seulement le refus u18 est interdit :
rayon q4 nul sans alerte à u24, dénominateur tronqué à u21, Morton64 masqué à 21 bits, `level_at_most` à reprendre.
Les primitives isolées (distance u128, Morton96), le filtre relatif et le comparateur de niveaux larges sont les points
de départ.

## 3. État vérifié ce matin

- Build Release de `8bb4618e5` dans un répertoire séparé : **13 portes sur 13** (11 portes rapides en 44 s, oracles
  catalogue et tour en 163 s et 183 s).
- Les copies R2 de `/tmp/mhgp10-r2` (sept groupes) sont sauvegardées de façon durable dans
  `build/v10-r2-backup-20260930/` : patchs, reçus, dossiers `recu/` et journal du workflow. Leur intégration commune
  n'a pas eu lieu : quatre groupes n'ont pas fini leur vérification, faute de quota.
- **Audit en cours**, par sept lentilles, chacune suivie d'un vérificateur adverse :
  - code livré au moteur (RankIndex, primitives u32) ;
  - bras frontière ;
  - précision au-delà de u18 ;
  - raccord R2 ;
  - constats ouverts ;
  - santé du build et de la CI ;
  - cardinalités.

  La synthèse donnera un suivi unique des constats et le plan des prochaines tranches.

## 4. Suite

Je répondrai à vos constats ouverts dans une `REPONSE_CLAUDE_*` après la synthèse, avec le plan d'intégration R2 et la
première tranche de précision.
