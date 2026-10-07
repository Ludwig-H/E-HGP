# Audit Codex — état courant v12

7 octobre 2026. Dernière base publiée **`91b1a7ee2`** ; contre-épreuves des corrections
en cours conservées séparément, avec sources et empreintes. Aucun chrono nouveau.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`.
[Contre-épreuves courantes](../receipts/audit_reprise_20261007/README.md) ;
états au [registre unique](CONSTATS.md).

**À corriger pendant le développement.**

- **Cache, `0007/0019`** : [correction concurrente et porte complétées](../receipts/audit_reponses_20261007/cache/README.md)
  sur `buffer.cpp` SHA `1844a7d6…`. Deux bras de 300+300 et 300+200 Kio : 12 contrôles
  officiels passent ; retirer la relecture de `held` cause trois échecs dans le second bras.
  ASan rejoué sur ce corps sous Clang/GCC : garde saine, restitution et dépassement de taille
  détectés. Comptage physique et marge de 1/8 déjà contre-éprouvés ; clôture formelle après
  publication du correctif, sans transfert à FULL, aux performances ni à TSan.
- **Juges, `0018`** : contre-lecture M5/M6 du lot `2b2113264`. M6 laisse encore sortir
  `mes_m6_ok` avec moins de prises que demandé, voire zéro : indices `false`/`0.0`
  égaux aux entiers dans une première garde puis ignorés par la suivante. Un schéma inconnu
  emprunte aussi le chemin historique et évite les nouvelles gardes. Validation entière
  stricte, effectif réellement validé et liste fermée de schémas nécessaires.
- **Petits nuages, `0238`** : [séparation K/fils/famille corrigée](../receipts/audit_mes_p_corrections_20261007/README.md),
  pentes témoins 100/60/10 µs/site retrouvées. Résidu : un régime entièrement en échec
  disparaît de la cohorte commune ; si tous échouent, la table disparaît aussi. Reconstruire
  K et les régimes depuis **toutes** les prises, puis intersecter leurs succès :
  un régime sans succès impose une cohorte vide. Cinq témoins JSON, normal/−O identiques ;
  temps inventés, aucune mesure HGP. Pilote vérifié par doubles de processus seulement.
- **EMST, `0232` clos** : le témoin historique aux PointId 17/17 est désormais refusé
  avec et sans référence. 107 appels indépendants par mode, 141 contrôles officiels,
  six permutations d'identités et 24 nuages géométriques inchangés. Portée : ordre un,
  pas les ordres supérieurs ni les verticales.
- **Découpes, `0218` clos dans la portée tracée** : huit manifestes et journal retrouvés,
  69 découpes/24 scènes cohérentes, 321 fichiers source aux bonnes tailles, 277 fichiers
  de paquets liés aux mêmes sources ; raccord neuf découpes / onze prises E confirmé.
  Aucun rehash des payloads ni recalcul géométrique réel indépendant ; l'outil avait
  déjà passé 32 sélections exactes.


**Priorité performance maintenue.** Catalogue CPU produit : 451,6 / 372,7 / 469,5 ms à
K5 sur ng00/01/02, 48 fils, médiane de neuf passes chaudes. Référence historique v11 :
200 / 163 / 195 ms, rapport descriptif ≈2,3, pas A/B apparié. Aucun chrono nouveau.
La voie GPU intégrée et FULL v12 restent à mesurer.

Les propositions exactes sont dans le [reçu performance](../receipts/audit_performance_20261007/README.md) :

- **`0233`** : finition CPU remise en série ; assemblage+table 122,5–171,3 ms à K5.
  Scan parallèle par blocs prouvé, premier représentant rationnel conservé, table exacte.
  Modèle : 3 537 confrontations, quatre mutants, 13 468 requêtes ; natif encore à qualifier.
- **`0234`** : paires calculées deux fois et census CPU poursuivi après rejet logique.
  Prototype d'arrêt : mêmes émissions et quinze compteurs sur sept feuilles ; gain G4 à mesurer.
- **`0235`** : M2+M5 ne confirment pas le budget catalogue complet. À aval CPU inchangé,
  rendre parcours+feuilles gratuits laisserait 145,1–201,7 ms à K5 ; scénario conditionnel,
  pas borne d'une autre implantation GPU. Juger préparation, émission, finition et transferts.
- **`0236/0237`** : `synth_sphere` arrondie n'est pas exactement cosphérique ; distinguer
  liste candidate, coquille et présentations. Capacités 256/64 encore incompatibles avec
  l'objectif sans refus de largeur ; q_min≤4 ne borne aucune de ces tailles.
  [Réponse documentaire en cours](../receipts/audit_reponses_20261007/docs/README.md) :
  PLAN et erratum corrigent le budget et la sphère ; clôtures `0235/0236` après publication.
  `0237` reste en cours : T1-c est prévu, la voie large manque encore. Préciser que les
  transferts encore non mesurés sont ceux du raccord complet, M5 comptant déjà les siens.

Mathématiques pour la suite : empreinte FULL par valeurs rationnelles exactes justifiée ;
clés de supports par rang XYZ prouvées ; caches par coquille/population/partie complètes
sous propriétaire et contexte, jamais par niveau, cardinal ou hash seul. G-L3 reste rejeté
(+3–4 % malgré 81–83 % de censuses évités) ; priorité aux sondes de table puis à la proposition.

Prochaine contre-épreuve : corrections des résidus ci-dessus, finition parallèle et voie
appareil dès livraison, puis ablations appariées et FULL multi-séquences. Les preuves locales
ne qualifient ni un catalogue accéléré ni les 100 ms. Canal maintenu à quatre fichiers ;
détails et captures dans `receipts/`, aucune donnée sous licence ni nouveau recours GCP.
