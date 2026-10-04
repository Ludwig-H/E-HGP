# Façade S5 : relecture et réponses D.1/D.3

4 octobre 2026. Sources WIP base `f98aeed67`, capturées dans `source/` et
`cli_source/` avant leurs lectures respectives. Les fichiers de tests ajoutés
ensuite sont listés dans `AFTER.json`, sans être prétendument relus. Aucun
build, natif, CUDA, fit ou GCP ; le seul code produit exécuté est une copie
du nouvel oracle Python sur trois sites distincts, K2.

## D.1 — Agrégats du manifeste

Conserver `cofaces` **par boule** suffit pour le besoin choisi. Ces cofaces
ont b pour boule minimale ; deux boules différentes ne comptent donc pas
la même coface. La portée reste les événements W_K exportés : ce n'est pas
nécessairement le nombre de toutes les liaisons de Γ_K. L'agrégat par support
est une incidence (b,Q,G), déjà calculable ; il peut rester optionnel.

La somme de `kparties_reliees` est une autre incidence, (b,F). La ligne
X={0,1,2}, K2, donne trois boules et les comptes1/1/3 : somme5, mais
seulement trois paires distinctes. Cela ne change pas la formule choisie
C(p+m,K) par boule ; préciser cette portée au lecteur et au manifeste.

L'invariance par rapport à Q_b vaut à p,m,K fixés. Elle n'est pas une
stabilité générale sous perturbations : une troisième position passant
de la coquille à l'extérieur d'une boule diamétrale fixe fait passer
C(p+m,2) de3 à1 alors que Q_b reste le même diamètre. La fixture entière
utilise un déplacement d'une unité, pas une limite de déplacements
arbitrairement petits dans le domaine fini u21.

## D.3 — Publication et erreurs après renommage

Déclarer un état de publication distinct. La visibilité d'un dossier
complet et la réussite du transport/synchronisation sont deux faits.
Après renommage, une synchronisation puis un retrait peuvent tous deux
échouer ; un code2 ne doit pas signifier implicitement « rien publié ».
`committed()` connaît déjà ce fait. Le CLI essaie maintenant `retract()`
sur tout échec après publication et avertit si le retrait échoue.

Conseil minimal : rendre aussi l'état `published_complete` dans le résultat
API et dans la ligne de refus lorsque celle-ci peut être écrite ; conserver
l'empreinte du manifeste fermé même si la synchronisation du parent échoue.
Son affectation actuelle, après `publish()` dans le S4 publié, laisse zéro
en cas de publication persistante en erreur. Ne pas présenter ces cas comme
succès de durabilité ou comme sortie partielle. Synchronisation et retrait
doivent avoir leurs portes de faute sur G4. Aucune faute de système de
fichiers n'a été provoquée dans cet audit.

## Relecture favorable bornée

La façade garde les paramètres du masque16379 ; le writer FULL reprend
l'ordre et les types du dump existant. La portée reste une lecture de source,
sans nouvelle identité native démontrée. La garde des options précède les
effets ; les entrées et le Product du CLI sortent de portée avant `close()`.
Le coût de la ligne standard reste distinct du manifeste déterministe.

`check.py` vérifie aussi les quinze témoins F5 : résultats exacts F2,
résultats F3 encadrés par deux voisins binary64 adjacents, conversion i64
négative et témoin d'excès de précision. Les bornes du numérateur global
du centre sont96/111/126 bits pour u18/u21/u24, sous i128 signé. Ceci juge
l'arithmétique des constantes et la borne existante, jamais le FENV natif
ou les instructions du compilateur.

**92 gardes**, normal/−O identiques, stderr vides. `ORACLE_BEFORE/AFTER.json`
fixent les copies du nouvel oracle et ses dépendances ; l'exécution se limite
à la ligne de trois points, et au contact diamétral de trois points.
`BEFORE/AFTER.json` fixent les neuf sources API. Le premier libellé de HEAD
(celui de root au lieu de l'acteur) est conservé dans `attempts/` ; il a été
corrigé **avant** lecture des sources. Les références supplémentaires sont
copiées, avec leurs clôtures, sans inventer de capture Git publiée du WIP.

```sh
python3 -B -S check.py
python3 -B -O -S check.py
sha256sum -c SHA256SUMS
```
