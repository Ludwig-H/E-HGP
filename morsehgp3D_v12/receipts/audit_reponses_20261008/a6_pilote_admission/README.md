# A6 livré : fermeture du juge avant agrégation

8 octobre 2026. Pin `30a69104a697fa2ea2dadd8499bde6c7cb8d72c0`, pilote
`1afa27d097b0418c0c84c78b10cd47a254d24ec75d2be7968673f4dec4c006cc`,
lecteur FULL `15437e5f…`. Lecture de source et Python seulement ; aucun moteur, compilation,
GPU, archive de données ou coordonnées. Constat de la famille **CST-0018** : ces témoins
montrent des faux verdicts possibles du juge, pas une erreur observée d'une campagne réelle.

## Cinq admissions indues reproductibles

`check.py` construit **85 journaux JSONL synthétiques**, soit 1 306 passes FULL : identité
sur 11 clés par bras, 37 cas artificiels dont 21 au-dessus de 60 000 sites ; six tournées
grandes × trois bras × 42 passes, puis cinq tournées ng × trois trames × trois bras × dix
passes. Les noms et temps sont inventés ; aucun résultat LiDAR ne leur est attribué.
Chaque prise passe le **vrai lecteur FULL livré**, puis `juger(..., verifier=True)` rehache
et relit les journaux. Les six auto-tests officiels passent aussi, mais utilisent
`verifier=False` et seulement trois clés d'identité.

| Changement isolé depuis ce nominal | Juge livré | Proposition |
| --- | --- | --- |
| Identité réduite de 11 clés à la seule `ng00_k5` | adopté, aucun refus | refusé |
| Cinq tournées grandes présentes sur six demandées | adopté, aucun refus | refusé |
| `tour.ordre` réduit de 21 noms à un, sans changer les 42 passes brutes | adopté, aucun refus | refusé |
| SHA de `avant_bis` différent de `avant`, chacun déclaré stable | adopté, aucun refus | refusé |
| Chronos W1 et `attendu.fils=1`, au lieu du W48 commandé | adopté, aucun refus | refusé |

Le troisième cas est particulièrement concret : `logs_grandes` tire `n` du rapport et
prend `murs_ns[n:]`. Avec `n=1`, il utilise une passe froide au lieu du second tour ; le
lecteur continue pourtant d'admettre les 42 bonnes passes contre les attentes archivées.
Il manque le lien **ordre déclaré → séquence attendue → positions chaudes**, avant
l'agrégation. Réduire une liste d'identités ou de tours supprime aussi ces prises de
`prises_du_rapport`, sans inventaire externe qui les exige.

Le contrôle A/A statistique existe : la campagne de l'auto-test à A/A=1,03 est refusée.
Les seuils utilisent les bornes bootstrap en pleine précision, avant l'arrondi des tableaux.
Le défaut A/A ci-dessus porte sur l'identité des exécutables, pas sur ce veto statistique.

## Correctif proposé, non appliqué au produit

`proposition.patch` ferme la cohorte **avant relecture et statistiques**. Le plan attendu
vient des paramètres de la commande et du seul `bundle_manifest.json`, jamais des prises
à juger. Il exige les clés d'identité, bras, séquences trame/sites, K, voie, profil, fils,
passes et nombres exacts de tours, ainsi que l'égalité des SHA `avant_bis`/`avant` et leur
fermeture. Types stricts des codes et SHA sont contrôlés. Les 37 noms et leurs suffixes
de sonde doivent être distincts ; les grandes sont sélectionnées depuis ce manifeste.
Le mode de campagne documenté reste W48 ; un autre régime exige sa déclaration séparée.

La proposition n'impose **pas** six tournées universellement : cinq sont acceptables si
elles ont été commandées. Le témoin en commande six et doit donc refuser cinq. De même,
elle ne code pas en dur 21 grandes : elle les déduit du manifeste de la cohorte commandée.
Pour un rapport archivé, reconstruire ce plan depuis la commande et les métadonnées
épinglées de la Session, et le passer à `juger(..., plan=...)`. Le champ
`cohorte_demandee` que le pilote publierait sert à la traçabilité, pas à remplacer cette
autorité externe. La voie `verifier=False` reste limitée aux auto-tests statistiques.

**Interface du correctif** : l'étape `rapport` exige désormais `--archive-v12set`, y compris
quand elle est appelée seule ; elle ne lit que son manifeste. L'API de rejeu
`juger(..., verifier=True)` exige un `plan` externe. Un manifeste manquant entraîne un
refus explicite, sans reconstituer une cohorte à partir des seules prises restantes.

Le rejeu couvre aussi **26 processus CPU synthétiques en mode essai** : le manifeste
contient toujours 37 cas, puis l'identité retient les deux premiers et les grandes les
deux dernières au-dessus du seuil ; deux uniformes et `ng00_k5_1fil` à une passe restent
exigés. Les sept clés d'identité et ce plan réduit passent. Un défaut d'affichage livré
est également reproduit : `etape_rapport` écrit les tableaux avant que `main` force le
verdict « essai », donc le Markdown peut encore dire « adopté ». La proposition applique
ce marquage avant les tableaux ; le test vérifie « essai » dans le jugement et le Markdown.

Le patch s'applique à l'extraction Git dans un temporaire ; le même nominal reste adopté
et les cinq anomalies deviennent des **refus bloquants**, sans changer les rapports ni les
critères statistiques. Portée limitée : pas de certification de toute entrée malformée,
de l'immuabilité temporelle d'un binaire, ni de la provenance source→ELF. Une relecture
indépendante peut admettre une campagne réellement complète sans prétendre réparer le
juge publié ; aucune prise réelle n'est invalidée par ces seuls témoins.

## Comparabilité et contrat de temps

La comparaison Git **bdfca8fb1 → 30a69104a** ne change que dix fichiers produit A6 sous
`src/tower/`. La sonde `bench/full_probe.cpp` est identique, SHA `1068940a…`, avec cache
de blocs par défaut **8 Gio** et voie recouverte. Le pilote n'ajoute ni `--cache` ni
`--sequentiel` ; la comparaison de ces deux sources conserve donc ce régime. Ce fait
n'authentifie pas à lui seul l'archive passée en argument : la fermeture externe doit
encore vérifier que l'archive avant correspond réellement à bdf et le paquet après au pin.

L'identité FUL1 est demandée dans des processus séparés ; les chronos n'ont pas `--digest`.
La règle mesure un gain relatif sur la moyenne géométrique des grandes trames (borne
haute < 0,97), avec garde par ng (borne < 1,02), pas le maximum de latence de chaque grande
trame. **« adopté A6 » ne signifie pas « FULL ≤ 100 ms »** : il n'existe pas de seuil absolu
de 100 ms dans ce juge. Pour la cible produit, publier séparément les latences par trame
et leurs maximums dans le régime contractuel.

Enfin, `noyau K - G K` des tableaux est une différence de **fins absolues**, pas la durée
exclusive du noyau. Les fenêtres G/forêt se recouvrent : ne pas les additionner comme
des temps disjoints ni attribuer cette différence entière au seul noyau.

## Rejeu

```sh
python check.py /workspaces/E-HGP
python -O check.py /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Sorties identiques au champ `result` de `capture.json`. Les sources sont relues depuis Git,
le patch appliqué seulement dans un temporaire, et tous les journaux synthétiques supprimés
à la fin. Aucun journal natif ni copie intégrale de source n'est ajouté à ce reçu.
