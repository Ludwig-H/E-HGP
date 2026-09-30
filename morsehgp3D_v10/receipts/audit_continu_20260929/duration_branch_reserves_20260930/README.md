# Frontière interne et conservation des continuations

30 septembre 2026. Ce paquet complète le contrôle de durée des feuilles
sans modifier le moteur ni prétendre qualifier une nouvelle tête de
clustering. Il corrige aussi notre réserve pour K2. Le
[rapport mathématique, section 11](../../../audits/audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md)
donne la portée de conception.

- [math/](math/README.md) : preuve générale qu'à K2 chaque point distinct
  a **au moins une** incidence de feuille. Cela ne récupère pas toutes ses
  composantes couvrantes futures. Deux vrais contre-exemples 3D, à six
  sites K3 et sept sites K5, n'ont aucune incidence de feuille pour le
  point frontière étudié ; celui-ci entre sur une branche interne.
  Un jitter d'une unité à trois échelles u18 montre pourquoi conserver
  la continuation d'un arc après fusion. Calculs Fraction normal/−O,
  primitive MEB partagée : pas d'indépendance arithmétique revendiquée.
- [native/](native/README.md) : deux petits exports natifs W1, K3/K5,
  comparés aux coupes exactes Γ et au nerf géométrique complet. Seul un
  wrapper d'audit est compilé contre une bibliothèque existante ; aucun
  moteur reconstruit et aucun nouvel essai LiDAR.

Les manifestes originaux des sous-paquets restent inchangés ; un manifeste
global ferme cette agrégation. Les chemins absolus des reçus désignent
l'exécution historique et ne sont pas réécrits. Les scripts écrivains
se rejouent uniquement dans une nouvelle copie temporaire.

Les exemples imposent de conserver l'entrée frontière interne et d'éviter
de perdre ou doubler la masse des continuations. Ils ne réfutent pas toute
pondération par durée et ne démontrent aucun ARI/EOM, sous-quadratique
global ou contrat FULL/G4 de 100 ms. `public_status=not_claimed`, GCP0.
