# Nettoyage du Codespace — 3 octobre 2026

Anciens builds, toolchains, caches, CLI remplacés, sorties volumineuses dérivées et worktrees périmés retirés. Les sources, modifications, scripts et mesures uniques utiles sont sauvegardés et dédupliqués avant retrait. Les binaires régénérables sont inventoriés par hash ; leurs octets ne sont pas tous conservés. HGP-old reste intact.

Les données actives, worktrees courants, référence R2, captures G4 et modifications locales préexistantes sont préservés. Les anciens fichiers versionnés restent dans Git. Les 51 dossiers de build conservés contiennent du travail actuel, des entrées/fixtures ou des preuves encore utiles ; leurs motifs sont dans le reçu local `remaining-old-builds/SELECTION.json`.

[Résumé](summary.json) et [contrôle final](checks.json) : sept sauvegardes fermées, 520 fichiers inventoriés, zéro absent, supplémentaire ou hash erroné ; lecteurs Python normal/−O identiques. Environ24Go disponibles après nettoyage, contre moins de0,4Go au début ; les opérations concurrentes empêchent d’attribuer chaque variation à une seule suppression.

Les sauvegardes complètes restent **hors Git**, dans `/workspaces/.ehgp-backups/cleanup-20261003/` (environ250Mo). Ce reçu versionné en conserve les empreintes et la portée ; il ne copie pas ces archives. `verify_local.py` rejoue leurs inventaires si ce dossier local est présent. Chaque sous-dossier fournit la restauration de son contenu dédupliqué, avec ses dépendances Git/archives déclarées.

Le nettoyage ne constitue aucune qualification native ni preuve de performance. Les modifications du moteur et campagnes G4 ont leurs reçus séparés.
