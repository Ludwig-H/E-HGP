# Captures FULL du 2 octobre 2026

`full3/` conserve la campagne close `v11.20261002.full3`, source
`c6ca345e0239b70191526eb0d38e516298386cea`. Le lecteur est **LIVE** : le
reçu brut local référencé dans `receipt.json` reste obligatoire. Il ne
contacte ni GCP ni un exécutable natif, et lit le tar sans extraction.
Les captures antérieures et les refus avant démarrage restent distincts.

```sh
python3 morsehgp3D_v11/receipts/full_20261002/check_full.py
python3 -O morsehgp3D_v11/receipts/full_20261002/check_full.py
python3 morsehgp3D_v11/receipts/full_20261002/check_full_selftest.py
python3 -O morsehgp3D_v11/receipts/full_20261002/check_full_selftest.py
```

Le code 0 du lecteur signifie **cohérence des preuves**, pas réussite du
calendrier : `failed_remote`, worker 1 et `conforming=false` sont conservés.
La matrice principale passe 1 971/1 971 portes et le supplément ASan/UBSan
18 bits, modules `num;index;tower`, passe 139/139. La différentielle FULL
compare à nouveau les octets archivés de 14 fixtures × 3 profils à v10
`c764e121aa52f2e5dd9b85fbe308c9c3511ff55e`, soit 42 égalités. Les témoins
incluent la coface K12 de treize sites et la coquille dont qmin exclut le
premier site. Cette comparaison bornée ne valide pas une projection de
points ni une tête de clustering.

Le banc demandait 24 essais. Il a achevé 13 essais K5, puis omis les
11 derniers pour la marge du budget de 600 s : cinq répétitions K5 et
les six essais K10. Aucun K10 n'a été exécuté. Durée de campagne :
560,738 s, décodage Python inclus ; zéro divergence parmi les empreintes
comparables, calendrier incomplet. Toutes les commandes ont leur groupe
fermé sans processus résiduel signalé. Le reçu atteste l'arrêt de la
bonne génération, la vérification des résultats, le retrait des clés
OS Login, la suppression de la clé privée et la libération du verrou.

| Trame sans sol | Sites | FULL K1..5 u21, secondes | FULL K1..5 u24, secondes |
| --- | ---: | ---: | ---: |
| 08/000000 | 39 885 | 21,207–21,294 (3 essais) | 21,644–21,725 (2 essais) |
| 08/000100 | 35 551 | 15,190–15,309 (2 essais) | 15,356–15,466 (2 essais) |
| 08/000200 | 45 845 | 18,554–18,559 (2 essais) | 18,897–18,911 (2 essais) |

Ces temps couvrent index, catalogue, recherche des boules, forêts et
verticales, à W48. Ils excluent préparation Cloud, Pool, lecture/écriture,
sérialisation, normalisation Python et segmentation du sol. Les entrées
sont les trois sous-nuages entiers figés sur grille 1 mm, dans un domaine
18 bits commun, calculés avec les profils arithmétiques 21/24. Trois
trames d'une même séquence ne constituent pas plusieurs séquences. Ce
résultat n'acquiert ni 200 ms, ni FULL sur brut float32, ni calcul GPU.

`check_full.py` épingle archive, contrat et cinq helpers exécutés dans
`source_c6/`. Il recoupe copies compactes, SHA des entrées téléversées,
JUnit/listes des portes, provenance/cache/drapeaux, inventaire ordonné,
intentions, diagnostics natifs, durées, comptes et comparaisons. Les gros
fichiers canoniques FULL ont été supprimés après mesure : leurs hashes
et résumés enregistrés sont contrôlés, leurs octets ne sont pas recalculés
par cette lecture. Les 42 petits fichiers communs v10/v11 sont présents
et réellement recomparés. L'oracle géométrique indépendant reste celui
des petites fixtures ; il n'est pas rejoué sur les trames LiDAR.

Les lectures normal/−O ont le même résultat. L'auto-test refuse
46 corruptions en mémoire avec trois témoins positifs ; il ne prétend pas
muter causalement tous les gardes des lecteurs historiques importés.
`check_full_selftest.json` conserve commandes, résultats et empreintes.

Deux incidents de préparation restent explicités : la capture initiale
cherchait à tort `package/data/manifest.json` ; `inputs.json` a été copié
du staging après égalité avec le SHA de l'upload. Le premier lecteur
réutilisait une garde exigeant le binaire catalogue dans le supplément
`num;index;tower` et l'a refusé. La garde propre à ce supplément contrôle
désormais ses cibles réelles et leurs drapeaux ASan/UBSan. Aucun reçu
natif clos n'a été modifié pour ces corrections de lecture.
