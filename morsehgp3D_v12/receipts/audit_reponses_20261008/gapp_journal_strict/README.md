# GAPP — enveloppe du journal et cohorte exactes

Audit Codex, 8 octobre 2026. **Correctif proposé, pas une livraison produit.**
Au pin `c648b3857c83ec1ed174139eef36106b187d2cf0`, le pilote `291c7f3a…`
accepte plusieurs contrefaçons JSON d'une campagne favorable. Le correctif les
refuse ; sur les **15 prises réelles × 6 passes**, tout le jugement recalculé
reste **strictement identique**, avec verdict `rejete`, sans refus d'admission.
Aucun moteur, binaire natif, build, GPU, contrôleur, GCP ni payload de scène exécuté/lu.

## Causes et correction

Le lecteur écrasait chaque ancienne phase singleton par la suivante et ignorait
les phases inconnues. Le juge admettait plus de processus que commandé ; les
comparaisons Python assimilaient `False` à `0` pour le code et l'indice de passe.
Le code 1 pouvait accompagner une identité vraie sans empêcher l'adoption.
Six témoins directs reproduisent ces acceptations, puis d'autres variantes
ciblées vérifient les mêmes frontières ; voir [results.json](results.json).

[proposition.patch](proposition.patch) modifie le pilote et complète sa porte
existante avec `test_journal_strict.py` :

- journal GPU : exactement `recolte`, `appareil`, passes consécutives,
  `transferts`, `identite` ; journal hôte seul : `recolte`, passes, `identite`.
  L'émetteur `mes_g_app.cpp` aux lignes 506, 674, 726, 789 et 801 porte cet ordre ;
  transferts et appareil sont absents ensemble sans appareil. Phases inconnues,
  doublons, permutation et contenu après l'identité sont refusés ; les clés JSON
  répétées et constantes non JSON aussi. Les champs supplémentaires d'une phase
  ne sont pas fermés : ce correctif ne prétend pas valider tout le schéma.
- effectifs de campagne positifs et indices/codes entiers, hors booléens ;
  les trois trames et le nombre commandé de prises sont exacts. La cohorte n'est
  pas un réservoir permettant d'ajouter une prise favorable.
- le drapeau d'identité doit correspondre aux compteurs d'écarts et à la cohérence
  des sondes ; code 0 signifie identité vraie, code 1 identité fausse, conformément
  à `mes_g_app.cpp:798–816`. Un **vrai écart mesuré** avec code 1 reste `rejete`.
- pour les campagnes futures, la collecte du mutant utilise ce même lecteur et
  résumé. Un mutant tronqué ou à phases ambiguës ne prouve plus une erreur
  d'identité. Le rapport historique n'archive que son code/drapeau/nom de journal :
  sa relecture par `juger` conserve cette portée. Le [reçu primaire](../gapp_admission/README.md)
  contrôle séparément le journal réel du mutant.

Ni bootstrap, ni seuils census/sondes/propositions, ni fenêtre A/A ne changent.
Cette proposition ferme l'admission décrite ci-dessus ; elle n'ajoute pas un
critère de performance et ne transforme pas le rejet réel en refus.

## Preuves légères

Le lecteur [check.py](check.py) extrait les préimages Git, vérifie/applique le
patch en copie temporaire, puis exécute uniquement du Python :

- porte officielle préexistante : **9 tests**, conservés ; après patch : **10 tests**,
  dont une porte regroupant **28 cas**. Les cas positifs couvrent adoption,
  rejet chronométrique, écart d'identité mesuré, hôte seul et collecte nominale
  d'un mutant. Deux collectes remplacent intégralement `capturer`, l'observation
  GPU et le hachage par des doubles Python ; aucune sonde n'est lancée ;
- **8 suppressions de gardes** indépendantes réadmettent chacune leur témoin,
  refusé par le correctif. Ce sont des mutants Python du juge, distincts du
  mutant natif `cote_nul` de la campagne ;
- sources et inventaire de 18 fichiers retournés épinglés ; lignes embarquées
  égales aux journaux hachés ; jugement primaire avant/après strictement égal.

Les modes normal et `-O` donnent les mêmes résultats. Le rapprochement avec
le rapport worker relève exactement quatre écarts flottants déjà retrouvés
indépendamment dans le reçu primaire : deux GM des sondes et deux informations
CPUHD/produit. Leurs **chemins et valeurs exacts**, pas une tolérance générale,
sont épinglés dans [capture.json](capture.json). Aucun IC, seuil, motif ni verdict
ne diffère. Cette variation entre recalcul local et rapport ne concerne pas
l'égalité stricte avant/après correctif sur le même interpréteur.

```sh
python check.py --repo DEPOT --returned DOSSIER_RETURNED_GAPP --check
python -O check.py --repo DEPOT --returned DOSSIER_RETURNED_GAPP --check
```

La porte permanente, après intégration éventuelle, se joue comme avant :
`python -S [-O] microbancs/mes_g_appareil/test_pilote_g_appareil.py`.
Le fichier de test déjà livré est conservé ; la nouvelle porte a un nom distinct.
Proposition relue statiquement avec l'auditeur des primaires pour l'ordre des
phases et le raccord code/identité. Aucun état de registre fermé par cet audit.
