# Dialogue actif avec le constructeur

**Le lot unitaire `21b77d29` passe la qualification indépendante.** O2 et ASan/UBSan : 114 ordres, 912 sorties et 69 120 coupes par build. Les 872 sorties historiques sont identiques à `13c6`, compteurs et refus compris. Le supplément rationnel exerce la naissance simultanée absente du corpus précédent ; le mutant perdant le quatrième parent est réfuté. Les [preuves](receipts_full_singleton_20260905/README.md) séparent ce rejeu des 17+17 CTests constructeur contre-vérifiés sur captures. `public_status=not_claimed`.

**Tous les builds et moteurs d’audit sont terminés depuis le 2026-09-05 à 20:19:12 UTC ; CPU libéré.** Aucun nouveau moteur ni benchmark pendant vos mesures mono. Les avis singleton, J1, cache nul/saturé, ancres muettes, naissances et plafonds ne sont plus des demandes ouvertes.

## Suite utile : coût des normalisations

**La dernière relecture/écriture de compression est supprimable sans changer l’état final des successeurs.** Retenir le dernier nœud avant la racine lors de la première passe, puis arrêter la compression avant lui. La [preuve](CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante) donne 1 opération si d=0, 3d−1 sinon ; versionner le calendrier si ces opérations cessent d’être facturées.

Le [diagnostic](MONO_FULL_COURANT.md#diagnostic-des-successeurs-sur-les-captures-lazy-closes) vérifie 48 ordres lazy clos. À 32k/K8, cette suppression ferait passer les opérations de 119 950 564 à 106 373 946 (−11,32 %). Les clôtures des directes ne pèsent seules que 4,57 %. Ces comptes ne prédisent ni un gain de temps ni la réussite du K9 refusé à 128 millions ; son préfixe est exclu des égalités de succès.

La [contre-fixture q4](S1_COURANT.md#7-rejet-précoce-dun-bloc-q4--frontière-de-loptimisation) borne séparément le rejet du bloc profond : poursuivre le balayage et versionner les compteurs. Le développeur a retenu ces conditions. Aucun de ces deux futurs deltas n’est qualifié par le lot unitaire.

La [PASSATION constructeur](../PASSATION.md) porte les campagnes mono et leurs limites. L’export doit lier les arènes à l’entrée, aux ordres, à l’horizon, à la convention de coupe et au succès terminal. Les ancres inter-K se rattachent à l’état inférieur fermé ; le supplément pondéré reste distinct. Aucun catalogue Gamma exhaustif n’est demandé.

## Entretien et coordination

Les notes dépassées et les reprises de documentation principale restent retirées. La qualification remplace l’attente dans les notes existantes, sans nouvelle synthèse à la racine ; les preuves brutes sont conservées. Les questions secondaires tiennent dans [un seul fichier](QUESTIONS_SECONDAIRES.md), avec [registre d’entretien](ENTRETIEN.json).

Contrôles clos : fraîcheur K, documents et registre passent. L’auditeur réserve maintenant l’index vide pour publier exclusivement les fichiers de ce dossier ; réservation levée automatiquement après son commit et le retour à un index vide. Aucun fichier constructeur inclus. Le header qualifié est capturé depuis votre worktree, séparément de sa future publication Git. GCP non utilisé.
