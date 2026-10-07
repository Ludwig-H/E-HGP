# Contre-lecture de la livraison des trois corrections

7 octobre 2026, produit publié `28cf75cd1861cd3f8ce9d038a7aac24874c2f2fa`.
Exploration v12 hors registre, CPU de référence, FULL pi0, u21 ; `not_claimed`.

- **M6 : résidu `null` corrigé.** Le juge publié `f90b56a7…` et les trois autres sources du lot sont identiques
  à la [capture contre-jugée](../m6_null_cloture/README.md). 71 cas officiels, quinze anciens témoins et reçu
  historique v1 conformes, normal/−O. Cette vérification de publication compare les hashes, sans répéter les tests
  sur des sources identiques. Les 9 prises/585 lignes historiques restent des mesures anciennes.
- **MES-P : CST-0238 clos** dans sa portée de séparation des régimes et de cohorte commune. Le corps publié est
  exactement le patch `aa64cdbf…` proposé ; [contre-rejeu de la livraison](../mes_p_integration/README.md).
  Les régimes entièrement en échec participent désormais à l'intersection et la rendent vide. Les anciens temps
  de la session G à 48 fils ne sont pas remplacés par les sentinelles des tests.
- **G : ordres manquants, métadonnées et digests corrigés** ; [contre-rejeu et erratum](../g_juge_integration/README.md).
  Notre proposition précédente `f7a30188…` omettait la ligne finale `exit` du vrai producteur : l'affirmation
  « format complet du producteur » dans son reçu était incorrecte. Le développeur a rectifié la proposition avant
  intégration. Le nouveau témoin prouve causalement ce défaut de notre double ; l'ancienne archive est conservée.
  Résidu étroit : `exit.order=False` ou `0.0` passe encore via l'égalité Python avec zéro ; garde de type proposée.
  La comparaison entre fils porte bien sur 64 caractères, l'affichage de référence CTest en conserve 16.

**CST-0018 reste ouvert**, en particulier pour le [pilote D6](../../audit_d6_20261007/README.md), dont la source
est inchangée par cette livraison. Aucun contrat de tour, performance ou GPU ne découle de ces corrections de
juges. Les portes natives déclarées par le développeur dans sa
[réponse](../../developpement_20261007/reponse_audit_reponses.md) ne sont pas rejouées ici.

`publication.json` ferme la correspondance entre la capture M6 et les sources publiées. Les autres reçus épinglent
directement cette livraison. `historique_registre.json` conserve les cellules de preuve avant leur synthèse :
[historique des juges](../../audit_clotures_20261007/README.md),
[M6 avant correction](../m6_integration/README.md),
[G avant correction](../../audit_t2g_prepublication_20261007/README.md),
[MES-P, mélange initial](../../audit_reprise_20261007/mes_p_provisoire/README.md),
[MES-P, régime disparu](../../audit_mes_p_corrections_20261007/README.md),
[proposition MES-P](../mes_p_proposition/README.md).

Aucune falsification historique alléguée, aucun moteur ou GCP exécuté par ce lot. Les temps artificiels du JSON
servent uniquement aux portes. Le registre conserve tous ses identifiants et leurs preuves ; seul CST-0238 change
ici d'état, les clôtures partielles de CST-0018 étant explicitement bornées.
