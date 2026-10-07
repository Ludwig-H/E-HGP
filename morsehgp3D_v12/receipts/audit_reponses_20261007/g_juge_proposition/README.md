# Juge G — proposition d'admission de la sortie complète

**Proposition vérifiée, non intégrée au produit.** Base publiée `99fa832466f65b3278bb62ad2156dae805efab5b`, juge
`beb8c3a1…`, corps proposé `f7a301880d5e432592d5433cd3374451b1a7d83043fcbcef50225f147113c90b`.
`proposition.patch` corrige la portée G du contrôle des juges suivie dans `CST-0018` ; interface et codes de sortie
conservés. Aucun fichier de `main`, du produit ou d'`audits/` n'a été modifié.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`. Aucun moteur exécuté, build, G4, grand nuage
ou nouvelle mesure de vitesse. Les sondes sont de minuscules doubles JSON ; leurs valeurs ne sont pas des mesures.

## Admission proposée

Une sortie doit contenir une ligne `tour_g` réussie, tous les ordres, puis exactement un digest. La ligne de passe
doit porter K et fils demandés ; les champs numériques requis sont des entiers stricts non négatifs. Les ordres
sont exactement `1..min(K, sites)`, sans doublon, et `births(k1) == sites`. Les champs des compteurs sont ceux de
`bench/tower_export.hpp`, typés sans booléens/flottants ; l'histogramme a 16 cases. Un JSON illisible ou hors schéma
donne un refus contrôlé, code 2. Les invariants de chaîne existants restent contrôlés ensuite.

Le digest est une chaîne SHA-256 complète de 64 caractères hexadécimaux, comparée entièrement entre fils. Le parsing
refuse options répétées, listes de fils mal formées et régimes non positifs ou répétés ; K est explicite.

**Déduplication conservée :** le générateur `synthetic()` trie puis supprime les positions dupliquées. Pour
`--uniform=N`, la proposition impose seulement `1 <= sites <= N`, jamais `sites == N`. Pour une entrée fichier, le
nombre positif de sites annoncé est confronté aux naissances k1 et à la couverture des ordres ; ce juge ne remplace
pas l'admission des fichiers par la sonde.

## Preuve JSON avant/après

Quinze comparaisons par mode, sorties normal/`-O` identiques :

| Cas | Base | Proposition |
| --- | ---: | ---: |
| Témoin à cinq ordres | 0 | 0 |
| Six faux succès du reçu précédent : ordre omis/dupliqué, digest invalide, sortie k1 calée sur la ligne CTest | 0 | 2 |
| Format complet du producteur, diagnostics compris | 0 | 0 |
| Deux sites après déduplication, N=10 et K=5 | 0 | 0 |
| Fils répétés, élément texte ignoré, compteur booléen, K de passe différent, `tour_g` absent | 0 | 2 |
| Même préfixe de digest, suffixe différent entre les fils | 1 | 1 |

La fabrique reprend les sept témoins de `audit_t2g_prepublication_20261007` ; les noms des compteurs sont confrontés
au schéma C++ épinglé. Le témoin de format complet suit `bench/tower_probe.cpp`, sans demander au moteur de calculer
une tour. Le cas qui imitait la ligne CTest est d'abord reconnu exactement sur la base, puis refusé après patch.
CTest lui-même et ses tests 8k/16k/32k ne sont pas exécutés.

**Limite conservée explicitement :** la ligne humaine garde le préfixe de 16 caractères pour rester compatible avec
les wrappers actuels. Une référence CTest sur le SHA-256 complet demandera les vrais 64 caractères de ses captures ;
aucune valeur de référence n'est inventée ici. La comparaison complète entre fils était déjà présente et reste
prouvée par le dernier témoin. Le juge vérifie schéma, couverture et déterminisme ; l'oracle juge l'objet.

## Rejouer

Depuis ce dossier, avec le dépôt et le reçu précédent disponibles :

```sh
python3 -B -S check.py > /tmp/g-juge-proposition-normal.json
python3 -B -S -O check.py > /tmp/g-juge-proposition-optimized.json
cmp /tmp/g-juge-proposition-normal.json /tmp/g-juge-proposition-optimized.json
```

Le patch est appliqué uniquement dans `/tmp`. Sources publiées, patch et dépendances du témoin sont hachés ; les
dépendances locales sont vérifiées avant/après. `normal.json`, `verification.json` et `SHA256SUMS` ferment le reçu.
Un premier contrôle de schéma de la fabrique a été corrigé pour inclure le chiffre de `route_t1` dans son expression
régulière ; aucun code produit n'a été modifié. La portée G reste ouverte jusqu'à intégration et contre-rejeu.
