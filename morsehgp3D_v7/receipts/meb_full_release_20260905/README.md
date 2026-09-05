# Qualification Release complète après MEB différée

Verdict : **323/323 portes `gate` passées**, exécutées à neuf le 5 septembre
2026. `public_status=not_claimed`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`. GCP non utilisé.

Le [résumé terminal](full_release/summary.json) exige quatre commandes à
code 0, les 323 noms exacts uniques dans l'inventaire et le JUnit, tous à
status=run, sans failure, error ni skipped. Les selftests du juge passent
en Python normal et sous `-O` : un positif et huit rejets chacun.
Une contre-vérification indépendante en lecture seule retrouve les mêmes
323 noms, dont les sept nouvelles portes MEB, et les mêmes pins terminaux.

## Exécution et périmètre

La campagne commence à 00:13:53 UTC, après la fin de la comparaison C/D.
Le [build](full_release/build_incremental.result.json) est explicitement
**incrémental**, dans `build/v7_meb_qualification`, parallel 2, en 232,823 s.
Les portes sont toutes exécutées à neuf : aucune somme de résultats
historiques, aucun build hermétique annoncé.

[CTest](full_release/ctest.stdout) rapporte 574,05 s ; le
[runner](full_release/ctest.result.json) mesure 574,116821607 s pour sa phase
CTest. Configure prend 0,166941116 s, l'inventaire 0,066855806 s ; les quatre
codes sont 0. Ces durées décrivent une qualification sur hôte partagé,
pas une mesure de vitesse du pipeline ni un résultat SLO.

Les **140 sources** et les **37 binaires testés** restent identiques aux
frontières contrôlées. Les deux CLI C restent à 25c9bf8e et le CLI D à
127c5f92 ; les SHA complets sont dans les
[pins protégés](full_release/protected_after.json). Les métadonnées de
build, les autorités du runner et son inventaire attendu restent stables.
HEAD, worktree et environnement sont [déclarés](full_release/environment.json).

## Provenance et projections

Le [runner archivé](run_full_release.historical.py), SHA 1ad9089c, conserve
ses octets et chemins d'exécution originaux dans `build/v7_meb_full_release`.
Il n'est pas annoncé directement exécutable depuis cette archive : une
reproduction doit restaurer ce contexte, ses autorités et un dossier de
résultat neuf. La [préparation historique](preparation.historical.txt)
reste au statut qu'elle avait avant le GO, distinct du présent verdict.

[copy_map.json](copy_map.json) explicite les 40 ports texte. Trente-neuf
sont byte-identiques. L'unique projection ajoute un LF terminal à
`full_release/inventory.stdout` : 339 445 vers 339 446 octets, SHA original
89ff545e vers SHA publié 3380a49f. Le contenu JSON et les 323 noms ne changent
pas. Le [manifeste historique](full_release/receipt_manifest.historical.json)
reste inchangé et décrit les fichiers originaux ; il n'est pas le sceau
du présent dossier. Le nouveau `SHA256SUMS` est relatif à ce dossier.

## Journal intégral sans perte

[LastTest.stdout.gz](full_release/LastTest.stdout.gz) contient tout le
journal CTest : **7 679 311 octets, 146 517 lignes** après décompression.
Le flux décompressé est identique à l'original conservé dans le build,
SHA 42007892ba39f3627f103002fbe19963f0051f2ba0ee8c6e05a7439b105f6f58.
Ce n'est ni un résumé ni un exécutable distribué.

La compression mécanique utilise `gzip 1.12`, `gzip -n --keep` : pas de
nom ni d'horodatage stocké. Le fichier gzip fait 83 168 octets, SHA
021ca773e155266caee6e91239c832b81bc23770bc054e6d6bfc495e0c685c5a.
La copie emploie `cp --no-clobber`, destination vérifiée absente ; son
avertissement de portabilité n'empêche pas le code 0 et l'identité vérifiée.
[Les métadonnées de compression](LastTest.compression.json) consignent
ces vérifications. Une copie texte partielle de travail a été retirée avant
le sceau ; aucun journal original n'a été supprimé.

Vérifications depuis ce dossier :

```bash
sha256sum -c SHA256SUMS
gzip -cd full_release/LastTest.stdout.gz | sha256sum
```

## Limites conservées

Ce reçu qualifie les portes CPU enregistrées sur ce delta. Il ne démontre
ni exactitude horizontale globale, ni performance 50 000 points sous
1 seconde puis 100 ms, ni passage massif, ni exécution GPU. La preuve locale,
la qualification ciblée ASAN/UBSAN et la comparaison C/D ont leurs reçus
distincts ; aucun de leurs temps n'est transféré ici.
