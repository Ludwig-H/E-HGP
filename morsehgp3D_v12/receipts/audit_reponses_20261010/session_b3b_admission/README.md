# B3b sur A6c : admission des primaires closes

Session `v12.20261010.t2db3b`, candidat `81b0883d1`, base `aa6338ee8`.
Relecture locale Python uniquement : sources publiques, métadonnées, archives de
résultats, journaux JSONL. Aucun moteur, compilateur, test natif, appel cloud ni
payload de coordonnées/IDs/forêt.

**Le rejeu confirme clés adopté, lot B3 adopté, balayage isolé rejeté.** Ce
dernier échoue sur ng02 et `08/001176`. Le contrôle A/A est admis ; aucun seuil,
échantillon ou exclusion n'est changé. Le calcul du juge est reproduit, avec une
seule différence d'un ULP sur la borne basse des clés pour `08/001176`
(0,9690147747578327 relu, 0,9690147747578328 enregistré), sans incidence sur la
décision. Voir aussi le [calcul indépendant des statistiques](../b3b_stats/README.md).

Le lecteur `admit.py` est une adaptation explicite de
`audit_reponses_20261008/session_t2db3_admission/admit.py` : nouvelles sources et
base, hashes de l'archive avant, 755 portes au lieu de 747, 74 mutants de tour au
lieu de 57 ; verdict dérivé individuellement des bornes, sans exiger le rejet
historique de tous les leviers. Le parseur strict de G reste celui de la
proposition **d9910490…**, appliquée en copie temporaire au pilote historique
545ed987e ; ses dépendances sont identiques à celles de B3b. Cette proposition
n'est pas intégrée au pilote mesuré, dont le juge v1 conserve la lacune décrite
dans `b3b_prelecture`.

**Enveloppe fermée.** DONE 0, worker `completed`, trois commandes de code 0,
aucune erreur ni avertissement du contrôleur, archive reçue et vérifiée,
transition d'arrêt certifiée RUNNING → TERMINATED. Archive **1 027 723 octets**,
**457 membres manifestés**, hash
`3e447ced79576fc3167cf7f9da5e912343abd4b8c23f53160c1cdb9277f93a42`.
Source, paquet, plan, script worker et archive sont liés ; les **495 sources de
chaque bras** sont exactement celles des deux commits, inventaires compris.
Les substitutions des quatre bras sont rejouées et le bras après est exactement
le produit du paquet. Les cinq constructions FULL ont les mêmes options
Release/u21/CUDA ; le sixième bras A/A est le même binaire que l'avant.

**Journaux.** Les **387 processus** sont tous relus : 373 FULL, soit 2 816 passes ;
10 de résolution G ; quatre profils de G, soit 16 passes. Campagne décisive :
300 processus, 2 400 passes, dont **2 100 chaudes**. Les codes natifs FULL et
profils sont enregistrés. Les 25 journaux FULL d'identité (50 passes) sont relus
avec leurs hashes et codes ; identité avant/après/ablation obtenue, et référence
FUL1 externe connue pour ng00. Les dix journaux G donnent une identité concordante
sous la réserve de code ci-dessous.

**Portes.** 755 CTests rapides passés, aucun sauté ; deux campagnes CTest de
mutants passées. Manifeste catalogue 38, tour 74 ; aucun rapport individuel de
mutant retourné. Leur issue et cause individuelles ne sont pas rejouées. Les
copies de mutants ne reçoivent pas le drapeau CUDA et restent au défaut CPU.
Ces résultats ne ferment ni CST-0244 ni CST-0245, dont les fichiers pertinents
sont inchangés dans B3b.

**Limites de preuve conservées.**

- Aucun code natif G de résolution n'est archivé indépendamment. Le parseur
  strict est appelé avec 0 **conditionnellement** au `valide=True` produit par le
  lanceur épinglé ; cela ne reconstitue pas un code perdu.
- Aucun stderr natif individuel n'est retourné. Les stderr des trois commandes
  englobantes, eux, sont présents et vides.
- Les hashes des sondes FULL sont enregistrés avant/après la campagne décisive,
  mais pas après les mesures informatives suivantes. Les hashes G et profil sont
  initiaux seulement. Aucun fichier ELF physique n'est retourné.
- Pas de nouvelle qualification CPU seule, massif, contrat des 37 trames ou
  profil u24/u32 ; les mesures informatives restent distinctes du jugement.

Le reçu distingue donc un verdict statistique confirmé et des sorties
concordantes d'une admission indépendante complète de toutes les preuves.

Rejeu normal ou `-O`, session locale close requise :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261010/session_b3b_admission/check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261010.t2db3b
```

`capture.json` conserve seulement pins, configurations, comptes et états, sans
identité de compte ou cible cloud. `results.json` contient les projections
auditées des journaux, sans données brutes.
