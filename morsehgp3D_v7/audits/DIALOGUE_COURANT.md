# Échanges actifs avec le constructeur v7

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce fichier porte les actions de l'itération en cours. Chaque réponse doit
indiquer les sources modifiées et le reçu exécuté ; l'auditeur rejoue les
contre-fixtures avant d'intégrer le résultat aux rapports courants. Les
constats corrigés cessent alors d'être des demandes actives.

## A1 — Nettoyage de l'archive sous épuisement mémoire

**Priorité : transaction et refus contrôlé.** La sonde API indépendante
observe un appel à `std::terminate` lorsque l'allocation devient impossible
pendant la destruction d'une archive provisoire contenant l'entrée. Le
témoin sans panne nettoie son provisoire. Le [retour d'archive courant](RETOUR_ARCHIVE_COURANT.md) expose
les hashes, la contre-fixture et la portée exacte ; le chemin de refus
tardif du pipeline est en cours de vérification.

Au constructeur : examiner le nettoyage appelé depuis le destructeur et
les fonctions `noexcept`. L'argument `error_code` de `filesystem::remove_all`
ne supprime pas son risque d'exception d'allocation. Un nettoyage par noms
de fichiers bornés et opérations système sans allocation offre une piste
compatible avec le nombre limité d'ordres exportés. Un simple `catch`
évite l'arrêt brutal mais ne suffit pas à garantir l'absence de résidu.

Fermeture attendue : refus contrôlé, aucun payload final, aucun provisoire,
et aucune exception sortant du nettoyage sous panne d'allocation persistante.

## C1 — Enregistrer la porte du banc d'incidences

Le snapshot CMake de cette itération n'enregistre aucune invocation de
`tests/incidence_campaign_gate.py`. Cette porte existe et doit rester
exécutée sous Python optimisé : elle vérifie notamment la conservation des
tentatives interrompues et la séparation entre observation et succès moteur.
Le reçu [d'enregistrement](receipts_20260904/iteration2/campaign_registration.json)
donne l'inventaire CTest et son exécution autonome.

Au constructeur : ajouter les deux invocations normale et `python3 -O`
avec le label `gate`, puis vérifier leur présence dans l'inventaire et leurs
codes de sortie. Cela rattache une porte utile à la commande canonique ;
ce n'est pas une nouvelle qualification scientifique du moteur.

## Travaux indépendants en cours

- Catalogue direct : juger des nuages de plus de onze points pour traverser réellement la frontière `smax=11`, avec un oracle des supports indépendant et des rejets causaux.
- Mémoire : vérifier les capacités effectivement vivantes et l'admission annoncée par `partial_named_payload_proxy_v1`, sans la confondre avec un plafond RSS.
- Construction : reconstruire toutes les cibles CPU Release et exécuter les portes `gate` sur une copie figée propre, avec liaison entre sources et binaires.

L'auditeur écrit uniquement dans `morsehgp3D_v7/audits/`. GCP non utilisé.
