# Clôtures et compaction du registre — 7 octobre 2026

Sur le pin `99fa83246`, 73 identifiants conservés. Les anciennes cellules de preuve sont archivées intégralement
dans `historique.json` ; leurs liens et pins restent accessibles ci-dessous. Le registre actif garde les états
courants et les preuves de clôture. Aucun nouveau test natif, GPU/GCP ni temps moteur dans cette opération.

## Clôtures vérifiées

- **0007/0019, cache** : livré en `7b7d025b3`. Les hashes du corps `5c885dbf`, du header `3c7fffc6` et du test
  `1111d433` correspondent aux [captures contrôlées](../audit_reponses_20261007/cache_equivalence/README.md).
  Le corps livré ne diffère du testé `1844a7d6` que par remplacement d'un alias local. Comptage physique et marge
  ont été contre-éprouvés ; [porte officielle](../audit_reponses_20261007/cache/README.md) : 12 contrôles, mutant
  tué causalement par le bras 300+200 Kio. Six sondes ASan sur ce corps sous Clang/GCC, garde saine et deux fautes
  détectées. Clôture de ces défauts cache, aucun transfert à FULL, performances ou qualification TSan générale.
- **0235/0236, documents** : les trois fichiers de `5682d00f5` sont identiques aux
  [captures documentaires](../audit_reponses_20261007/docs/README.md). Budget C explicitement non confirmé,
  erratum quasi-sphère publié sans altération du reçu G historique. Clôtures documentaires seulement.

**Restent en cours :** 0018 (M6 `null` et couverture du nouveau juge G), 0233/0234 (finition/feuilles),
0237 (voie large), 0238 (cohorte sans régime entièrement échoué). Les 17 sources de G publiées en `99fa83246`
sont identiques à la [capture prépublication](../audit_t2g_prepublication_20261007/README.md) : la réserve du juge
s'applique donc à cette livraison. Aucune erreur géométrique du moteur n'en est déduite.

`capture.json` conserve les hashes publiés et ceux du registre avant/après ; la transformation ne touche ni les
libellés, ni les auteurs, ni les pins d'origine, ni les témoins des six lignes révisées. Les quatre changements
d'état sont les clôtures explicitement décrites ici ; aucune ligne n'est supprimée. `check_constats.py` reste un
contrôle de structure, pas un juge des preuves.

## Anciennes cellules, historique sans autorité sur l'état courant

### CST-0007 — état précédent : en cours

[capture non commise sur `f601b36ac`](../audit_reprise_20261007/buffer/README.md) : arrondi/inactifs comptés et marge de 1/8 prouvée ; refus après admission reproduit pendant une éviction concurrente (1 Mio, deux demandes de 300 Kio), plafond physique respecté ; transition à corriger avant clôture ; dév. 7 oct. : [réponse](../developpement_20261007/cst_0007_0019_cache.md) (taille physique, inactifs sous la limite, course d'éviction corrigée, Clang) ; à contre-lire

### CST-0018 — état précédent : en cours

[Témoins initiaux M2/M3/M4](../audit_socle_microbancs_20261007/preuves/README.md), [M6](../audit_session_t1_20261007/m6/README.md), [M5](../audit_b_m5_20261007/juge_format/README.md), puis résidus [M2](../audit_cd_corrections_20261007/m2/README.md)/[M4](../audit_cd_corrections_20261007/m34/README.md) après `320db4a12` ; campagnes [A](../audit_session_t1_20261007/campagne/README.md) et [C/D](../audit_cd_corrections_20261007/campagnes/README.md) complètes dans leurs portées. `1b40c0411`, [contre-audit `1f7642e10`](../audit_juges_emst_20261007/juges/README.md) : anciens témoins corrigés (31 observations), mais M5 adopte encore identité contradictoire, livres absents ou tableaux de mesures sanitizer vides ; M6 rejuge provenance vide, isolation contradictoire ou refus explicite ignoré. [G1 `274592a30`](../audit_t2_20261007/mesures/README.md) reste distinct : route falsifiée et ordre absent admis ; six journaux réels relus, aucune falsification historique démontrée. Nouveau bras G1 `151d4b6ec` et correction d’admission `8049bcc39` livrés après le pin, à contre-auditer ; développeur, lot 2 des outils (7 octobre, après `98ca07556`) : M5, validation typée commune (identité, statut, grand livre, empreintes, séries de répétitions) et diagnostic réservé avant la boucle (porte `mhgp12_traversal_driver_selftest`, 29 cas, 21 écarts sur l'ancien pilote) ; M6, schéma strict de relecture ; 18 refus gravés en cas de porte, 81 mutants tués ; [rapport](../developpement_20261007/outils_lot2_RAPPORT.md) ; pilotes durcis à rejouer sur G4  [Contre-épreuves `f601b36ac`](../audit_reprise_20261007/juges/README.md) : anciens résidus M5/M6 corrigés ; M6 accepte encore process bool/float, jusqu’à zéro prise sans fichier, et schéma inconnu évitant les gardes v2. [G1](../audit_reprise_20261007/g1/README.md) : route forgée et ordre absent désormais refusés causalement ; clôture de cette seule portée. ; dév. : lot 3 (M6 : entiers stricts, effectif validé, schémas fermés ; [rapport](../developpement_20261007/outils_lot3_RAPPORT.md))

### CST-0019 — état précédent : en cours

[contre-lecture du socle](../audit_socle_microbancs_20261007/session/REPORT.md) : buffer v12 admet 262 145 octets mais alloue 286 720, puis les garde hors compte ; empoisonnement Clang toujours à qualifier  [Capture courante](../audit_reprise_20261007/buffer/README.md) : comptage physique et détection Clang ajoutés ; refus concurrent après admission encore présent ; [Contre-épreuve ASan ciblée](../audit_cache_poison_20261007/README.md) : neuf processus, Clang 18.1.3/GCC 13.3, deux lectures fautives détectées ; retour à l’ancienne garde Clang les rend silencieuses. Partie poison confirmée, concurrence toujours ouverte. ; dév. 7 oct. : [réponse](../developpement_20261007/cst_0007_0019_cache.md) ; à contre-lire

### CST-0235 — état précédent : en cours

dév. 7 oct. : [réponse](../developpement_20261007/reponse_audit_performance.md)

### CST-0236 — état précédent : en cours

dév. 7 oct. : [réponse](../developpement_20261007/reponse_audit_performance.md)

### CST-0238 — état précédent : en cours

dév. 7 oct. : [réponse](../developpement_20261007/reponse_audit_performance.md)

