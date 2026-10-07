# CST-0224 : correction partielle et conflit de chemins

Pin `c3de9d73d8999f2f1e31a0f592b829efc6e7a4da`, correctif `df9140b5d`.
Le [pilote](check.py) appelle le vrai `main` de `recu_session.py` sur des
répertoires temporaires synthétiques. [Résultats](normal.json) identiques
en normal et `-O`, source et témoin hachés avant/après. Aucun reçu réel touché.

**Correction confirmée du témoin initial.** Les deux noms de fichiers qui
deviennent identiques après anonymisation sont désormais refusés avec le
code 3. Cela vaut dans un même dossier et dans deux dossiers dont les noms
se confondent. Les fichiers déjà écrits sont retirés ; la destination
absente au départ disparaît, celle initialement vide reste vide. Les cas
positifs et l'expurgation du manifeste passent, les destinations non vides
restent intactes avec code 2 : `CST-0219/0221` ne régressent pas dans ce lot.

**Résidu de CST-0224 : un fichier peut devenir l'ancêtre d'un autre fichier.**
La session contient le fichier `audit@example.invalid` et le fichier
`<compte>/child.json`. Après anonymisation, le premier doit devenir
`resultats/<compte>`, et le second a besoin de ce même chemin comme dossier.
Le dictionnaire `targets` ne contrôle que l'égalité des chemins de fichiers ;
il ne voit pas cette incompatibilité.

Le vrai appel lève **`FileExistsError`**, sans retour contrôlé ni nettoyage.
Il laisse `receipt.json` et `resultats/<compte>`, sans manifeste. Le défaut
se reproduit pour une destination absente et pour une destination vide.
Aucun ancien fichier ni fichier source n'est perdu : c'est une publication
partielle après exception, distincte de l'écrasement initial désormais corrigé.
Le constat reste **en cours**, sous le même identifiant de collision.

Correction attendue : prévalider tout l'arbre des destinations anonymisées
(unicité et incompatibilité fichier/dossier) avant de publier, ou garantir
le nettoyage transactionnel de ces exceptions. L'existence d'un manifeste
cohérent demeure nécessaire mais ne remplace pas cette validation.

Dix appels de publication/refus sur destinations absentes ou vides et cinq
contrôles de destination occupée ; uniquement `audit@example.invalid`, adresse
synthétique. Rejeu : `python3 -B check.py`, puis `python3 -B -O check.py`.
