# Audit Codex — état courant v12

7 octobre 2026. Reprise au pin **`f601b36ac`** : correctifs des juges, identités EMST,
découpes LiDAR ; capture séparée du correctif mémoire non commis, identifiée par hashes.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`.
[Contre-épreuves courantes](../receipts/audit_reprise_20261007/README.md) ;
états au [registre unique](CONSTATS.md).

**À corriger pendant le développement.**

- **Cache, `0007/0019`** : le correctif en cours compte l'arrondi et les blocs inactifs,
  mais une éviction concurrente peut encore refuser une allocation admise. Budget/cache
  1 Mio, deux demandes de 300 Kio : pendant la libération d'un ancien bloc, la liste est
  vide et `held` encore plein ; le second fil refuse puis réussit après libération.
  Plafond physique respecté, promesse d'admission indépendante de l'entrelacement rompue.
  Synchroniser les transitions et recontrôler l'état avant refus ; ne pas rendre le crédit
  physique avant la libération. Constat sur capture de travail, pas sur un commit livré.
  [Poison ASan confirmé](../receipts/audit_cache_poison_20261007/README.md) : neuf petits
  processus sous Clang/GCC ; ancienne détection Clang prise en défaut causalement.
  Cet acquis ne clôt pas le refus concurrent ; aucun chrono produit nouveau.
- **Juges, `0018`** : contre-lecture M5/M6 du lot `2b2113264`. M6 laisse encore sortir
  `mes_m6_ok` avec moins de prises que demandé, voire zéro : indices `false`/`0.0`
  égaux aux entiers dans une première garde puis ignorés par la suivante. Un schéma inconnu
  emprunte aussi le chemin historique et évite les nouvelles gardes. Validation entière
  stricte, effectif réellement validé et liste fermée de schémas nécessaires.
- **Petits nuages, `0238`** : l’analyseur fusionne 1/4/48 fils en une seule droite.
  Témoin : 100/60/10 µs/site séparés, 56,67 réunis. Grouper par K, fils et famille,
  afficher les échecs et comparer des cohortes communes avant de choisir le seuil CPU/GPU.
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

Mathématiques pour la suite : empreinte FULL par valeurs rationnelles exactes justifiée ;
clés de supports par rang XYZ prouvées ; caches par coquille/population/partie complètes
sous propriétaire et contexte, jamais par niveau, cardinal ou hash seul. G-L3 reste rejeté
(+3–4 % malgré 81–83 % de censuses évités) ; priorité aux sondes de table puis à la proposition.

Prochaine contre-épreuve : corrections des résidus ci-dessus, finition parallèle et voie
appareil dès livraison, puis ablations appariées et FULL multi-séquences. Les preuves locales
ne qualifient ni un catalogue accéléré ni les 100 ms. Canal maintenu à quatre fichiers ;
détails et captures dans `receipts/`, aucune donnée sous licence ni nouveau recours GCP.
