# Livraison Gc1/Gc2 et compatibilité D6 — 8 octobre 2026

`4df326cc8` puis `10050a96e` livrent les deux tranches Gc. Les 16 fichiers `src/tower/` du second commit sont identiques octet pour octet au prototype Git `45976be8` déjà audité. Les arbres `src/core/` et `src/catalogue/` restent identiques à l'avant-livraison `83962e5a2`. `pins.json` donne les commits complets, 24 hashes de sources, les 22 chemins du delta et les dépendances réutilisées. Le lecteur peut revérifier le prototype avec `--gc-git`.

Cette égalité de sources ne rejoue aucune porte native : les 701 succès annoncés dans le commit ne sont pas requalifiés ici. La [garde de domaine de Catalogue::find_support](../../audit_reponses_20261007/gc_support_domain_delta/README.md) reste une proposition distincte, le catalogue étant inchangé. Le skip des sites S appartient, lui, au Gc livré. Aucun nouveau temps G4, FULL ou TMVR n'est acquis par ce reçu.

Rejeux Python normal/−O identiques :
- Juge G : neuf modes par régime, dont écarts objet/travail à empreinte identique et refus `exit.order` bool/flottant. Le changement COBJ conserve le contrôle séparé des compteurs de travail.
- Pilote T2-c : 21 prises synthétiques (3 admises, 18 refusées), deux formats natifs archivés avant/après, deux variantes de forme `sites<K`, trois cohortes de dix tours. Résumé forgé et brut invalide re-haché refusés ; auto-test et garde du mode rapport vérifiés. Identité finale seulement, pas oracle ni identité de chaque passe.
- Le défaut CLI reste huit tours alors que la garde en exige dix : sans `--processus=10`, refus d'usage code2. Cette version du pilote ne collecte pas le catalogue ; ses informations portent sur G. Aucun chrono C+G intégré n'en découle.

**D6 doit être adapté avant une campagne sur Gc.** Le lecteur livré `48f40fd6` admet `before.jsonl` et refuse le vrai `after.jsonl` déjà publié (8 sites/K5/W1/P2) : ses diagnostics fermés ignorent les neuf champs Gc ajoutés. Les retirer dans un témoin causal suffit à rétablir l'admission ; ce retrait n'est pas la correction proposée. Les anciennes mesures D6 ne sont pas invalidées.

`d6_g_schemas_proposed.patch`, non appliqué au produit, admet explicitement les deux schémas complets : scalaires u64 stricts et tableaux de longueur K/K−1 demandé, même lorsque sites<K ; aucun mélange entre passes, champ manquant/inconnu ou bool admis. Les deux vrais formats passent ; 27 corruptions ciblées échouent ; K1 avec tableaux K−1 vides passe en modèle de forme. Contrôles structuraux, sans oracle géométrique ni ajout de lignes profil (build D6 normal). La proposition est contre-lue par l'auditeur mathématique. [L'admission du plan D6](../../audit_d6_20261007/pilotage/README.md) reste séparée et ouverte.

Rejeu depuis le dépôt :
```
python CHEMIN/check.py --repo . --check
python -O CHEMIN/check.py --repo . --check
```
Option `--gc-git DOSSIER_GIT` : vérifier aussi les octets du prototype. Aucun moteur natif, build, cloud, donnée LiDAR ou copie de fixture dans ce reçu.
