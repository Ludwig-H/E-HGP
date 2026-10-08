# Repo6 : 27 mutants clos

Le 8 octobre 2026 à **02:10:22 UTC**, `final6` clôt les **27 mutants sur 27
tués par code**, avec témoin vert et code final 0. Le marqueur du journal et
le rapport concordent. Aucun résultat n'est classé signal, délai ou échec de
construction. L'auditeur relit ces traces ; il ne rejoue aucune commande native.

La source est exactement celle du
[jalon repo6 u21](../audit_tmv_repo6_u21_20261008/README.md) : base annoncée
`a5e0dbc77`, patch `b3c78ae3`, arbre de 382 fichiers
`1b5d1fb38c2c2cbcbfaac86525eeaaeeab0f044afb1a4aa490a6f249d5b73279`.
Le rapport porte cette empreinte et celle du manifeste
`75fa8a676129710045b79e1a0d5543fcb2e76b39548cc61075fe475724127f9b`.
Le lecteur vérifie les 27 identifiants distincts, leur égalité exacte avec le
manifeste, les catégories de verdict et les hashes avant/après.

Le [raccord proposé](../composition_gc_tmvr_20261008/README.md) réunissait
7 mutants communs, 11 supplémentaires de G-c et 9 de TMVR. Ils sont désormais
tous joués dans cette composition, notamment l'admission des décalages CSR de
R, les branches à la coupe ouverte et la date strictement antérieure des
cibles. Cette preuve s'ajoute aux 690 CTests réussis et aux neuf chaînes CPU
u21 du reçu précédent ; elle ne reprend pas les 16 mutants de repo5 comme
qualification implicite.

Relecture, depuis ce dossier :

```sh
python check.py --prototype DOSSIER_TMVR
python -O check.py --prototype DOSSIER_TMVR
```

Les sorties doivent être identiques au champ `result` de `capture.json`.
La fonction d'empreinte de l'arbre vient du
[lecteur historique épinglé](../audit_tmv_traces_20261007/README.md).
Seul l'enregistrement `mutants` est exigé comme code de réussite de ce jalon,
sans confondre d'autres étapes avec lui.
Le patch partagé n'est pas utilisé comme source de cette preuve : ce sont
l'arbre repo6 et les empreintes du rapport qui la rattachent au code.

Le rapport du développeur déclare l'arrêt de la batterie à **02:10:44 UTC**,
pendant le début de la construction 24, pour la prochaine intégration/mesure.
Le journal primaire `final6/build24.log`, épinglé, montre plusieurs compilations
`Terminated` à 18 %, puis l'erreur de make. Aucun code `build24`, CTest24 ou
profil 32 n'est clos ; CTest24 et la configuration 32 n'ont pas de journal.
Le lecteur recoupe ces traces avec la déclaration d'arrêt, sans rejouer le
signal. **Ce n'est pas un défaut de source démontré.** Les profils 24/32 ne
sont plus en cours dans final6 et restent non qualifiés sur repo6.

Une livraison intégrée sur main et la chaîne GPU FULL ne sont pas qualifiées
par ce reçu. La limite déclarée du validateur
d'historique (CST-0240) demeure. Aucun payload, nouveau chrono, build, moteur
ou GCP n'a été utilisé par l'auditeur ; `public_status=not_claimed`.
