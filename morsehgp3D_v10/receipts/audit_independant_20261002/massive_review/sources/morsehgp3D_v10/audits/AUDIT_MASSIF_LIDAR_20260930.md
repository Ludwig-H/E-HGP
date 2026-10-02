# LiDAR massif et précision — contrat proposé au développeur

30 septembre 2026. Demandes utilisateur : dizaines de millions et précision paramétrable. Moteur publié inchangé depuis 4b7d70422 ; preuves jusqu'à 33fcb53a0, raccord privé R2 commencé. Primitives larges/filtre isolés. public_status=not_claimed. Aucun GCP, allocation massive ou moteur modifié par cet audit. [Massif](../receipts/audit_independant_20260930/massif/README.md), [précision](../receipts/audit_independant_20260930/precision_grille/representation/README.md).

**Décision utilisateur confirmée : grille u32 par paliers u24 puis u32 complet ; float32 natif hors chantier courant.** Exposer le pas physique h, publier le domaine exact certifié et conserver un repère commun. Pour le massif : segments depuis les boîtes de centres certifiées, fusion externe exacte. Cela traite la capacité du catalogue ; atlas, verticales, incidences et reprise restent à concevoir.

## Précision : pas et largeur distincts

Étendue de grille = h·(2^b−1). Cloud/Morton actuels acceptent b≤21 ; générateur et FULL restent **u18**. Distance u128 et Morton96 sont isolés, non raccordés. Notre [complément](../receipts/audit_independant_20260930/grid32_followup/README.md) passe 7 157 contrôles normal/UBSan ; Morton change d'ordre sous translation : jamais un ID persistant ou une clé commune à des origines différentes.

| Pas | Étendue u18 par axe |
| --- | ---: |
| 10 mm | 2 621,43 m |
| 1 mm, défaut courant | 262,143 m |
| 0,1 mm | 26,2143 m |
| 0,001 mm | 0,262143 m |

Une carte de 1 km à 0,1 mm demande 24 bits : dimensionnement, pas qualification. Retirer la garde u18 serait faux : tétraèdre (0,0,0),(m,m,0),(m,0,m),(0,m,m), m=2²¹−1, centre intérieur mais D² de 130 bits, formé en u128 par [level4](../src/arith/geometry.cpp#L88). [Calcul symbolique](../receipts/audit_independant_20260930/precision_grille/representation/README.md), aucune exécution hors domaine.

Contrat minimal proposé :

1. Paramètre precision_mm décimal positif exact ; pas actif 1 mm, proposition large 0,1 mm à déclarer explicitement. Poser h=precision_mm/1000 en mètres pour l'export physique. Profil parmi les voies certifiées ; pas de bits libre supposant la preuve acquise. Calcul large avant conversion, contrôle d'étendue et refus si aucun profil disponible ne convient.
2. Grille, origine et repère communs, arrondi/ex æquo déclarés, puis translation entière commune. Pas d'adaptation silencieuse du pas, écrêtage ou origines indépendantes par tuile.
3. Manifest versionné lié au hash des coordonnées : h rationnel, origine/traduction, repère, profil, unités, IDs, correspondance retours→sites et fusions. Le u32le nu/CLI ne portent pas h/origine et recréent les IDs par ligne.
4. Voie large qualifiée ensemble : Morton/identité, centres, prédicats, niveaux et filtres. Un pas fin ne récupère pas une précision déjà perdue par le capteur.

Niveaux exacts en cellules² : β_phys=h²β_grille, r_phys=h·r_grille, λ_phys=h^(−z)λ_grille. EOM idéal invariant pour le même arbre, masses/z et conventions de zéro. **Requantifier change le nuage.** Garder unité interne et dates exactes. Le [filtre relatif certifié](../receipts/audit_continu_20260929/relative_filter_20260930/README.md) conserve les contacts en prototype ; READY ne certifie pas la positivité du support. Signe exact : borne q3 de 201 bits, protocole générique sur 192 bits jusqu'à 258 bits. Repli, nearest natif et port commun restent à faire ; aucun FULL large ou gain acquis. [Complément ordre/KNN](../receipts/audit_independant_20260930/wide_order_followup/README.md) : deux boules q2 K3 de niveaux E et E+1/4 coïncident en double u32, même physiquement ; coupe exacte au seuil E distincte. Prévalider les niveaux avant tri, jamais traiter un refus comme une égalité. Distances/q2 u24 exacts en double ne couvrent pas q3/q4.

Borne utile au dispatch : MEB certifiée dans une boîte de largeurs Δ donne 4β≤ΣΔ² ; ainsi β u32<2^64, même si les K-NN demandent 66 bits. Le raccourci fermé 4e≥ΣΔ² exige ce certificat et le même profil ; équidistance ou READY ne suffisent pas. [Preuve, neuf MEB exactes et contre-cas obtus](../receipts/audit_independant_20260930/wide_order_followup_bound/dispatch_meb_bound.md), normal/−O. Une valeur β bornée ne dispense pas des numérateur/dénominateur larges ; aucun constructeur ou gain de débit acquis.

## Mesures et dimensionnement

Pas de délai ni enveloppe RAM/disque/sortie massif v10 fixé ; cadre historique GCP G4. Anciens délais/plafonds [historisés](../../docs/PERFORMANCE_MORSEHGP3D.md#L245). Aucun transfert du jalon de 100 ms. Hypothèse : FULL 1..10, repli explicite 1..5 ; attaches, tête, retours/export mesurés séparément puis dans le total.

Session5 CPU : 169 configurations réussies, 7 non lancées sur budget ; une graine/exécution ; 67 synthétiques et 21 morceaux de trois trames de la seule séquence 08. [Extraction](../receipts/audit_independant_20260930/massif/dimensionnement/README.md).

| Même amas synthétique, 1 024 000 sites | K5 | K10 |
| --- | ---: | ---: |
| Boules | 87 574 708 | 478 482 791 |
| Catalogue + tour | 20,8461 s | 124,3633 s |
| Mur du processus | 21,51 s | 126,82 s |
| Pic RSS | 23,405 Gio | 135,284 Gio |

Commandes sans points : tous ordres/verticales, aucune attache, tête ou export durable complet. Lecture/préparation hors catalogue+tour ; masque/quantification hors ligne. B provient d'un processus distinct : conserver comptes/digest du processus réellement chronométré. Aucune capacité de 10–50 M LiDAR ni FULL GPU qualifiée.

303,6–314,9 octets/boule dans les grands cas K10 sont empiriques. **Scénarios seulement** à 30 M sites : 120 boules/site → 3,6 milliards et 1,09–1,13 To ; 460 → 13,8 milliards et 4,19–4,35 To. Ratios/coûts variables. VM capturée : 176 Gio, 79 G disque libres : aucun spool massif dimensionné.

## Verrous avant montée en taille

| Verrou | Correction |
| --- | --- |
| Boules / forêt | B→u32 sans garde au catalogue ; garde proposée en copie R2. Atlas/représentants protègent déjà la [forêt/CSR](../receipts/audit_independant_20260930/forest_cardinality/README.md) valide actuelle. Promouvoir les réserves ; conserver ces invariants globaux dans le futur port segmenté. |
| RankIndex | Milieu sûr et produit élargi intégrés dans 4b7d70422 ; [helper réel, 36 220 cas normal/UBSan](../receipts/audit_independant_20260930/rank_search_preintegration/README.md) passent. Raccord observé dans le binaire u18 ; cardinalité avant cast de level.size() distincte. |
| Atlas | Refus cellules/représentants≥kNone présent, après catalogue résident ; B représentable ne garantit pas l'atlas. |
| Arène ExtCell | rep_first et son addition restent u32 sur l'arène commune à tous K. Les gardes par K donnent Σsr[k], compatible u64 ; garde globale append/span recommandée. [392 contrôles scalaires normal/UBSan](../receipts/audit_independant_20260930/developer_rebound/catalogue/README.md), sans corruption de catalogue géométrique exécutée revendiquée. |
| Mémoire | Budget Buffer partiel, grands vecteurs et budget par défaut illimité. Réserver états simultanés/disque et fermer workers sur refus. |
| Retours | Cloud conserve poids/IDs, mais FULL refuse les multiplicités. Déduplication des scans exige modèle déclaré et correspondance complète. |

N retours, n sites, B boules, P occurrences I/U, L niveaux : catalogue : 42B+4P+56L ; tri : 168B+4P ; assemblage : 179B+8P+56L. Deux derniers pics distincts, hors capacités/socle/tour. Socle≈160n si N=n ; attaches jusqu'à 16n/ordre, ball_nodes : 4B/ordre. [Formules](../receipts/audit_independant_20260930/massif/representation/FORMULES.json).

Linéaire en B n'est pas linéaire en n. [Famille rationnelle v7](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md) : n²/4 naissances FULL dès K2, précision croissante, pas asymptotique infinie dans u18 fixe. Streaming/GPU ne suppriment pas la sortie explicite.

## Segments exacts et prochaine livraison

[GEN §§3.1–3.5](../docs/conception/GEN_v2.md#L110) : N_Kmax(c) inclus dans L(Q), voisins ex æquo compris, émission unique par feuille demi-ouverte. Kmax suffit : p+q_min≤Kmax+1 implique p≤Kmax−1. Conserver I/U complets.

Kmax témoins S donnent R_Q²=max des ||s−v||² sur sommets v de Q. Un bloc global Z avec dist(Q,Z)²>R_Q² peut être rejeté ; égalité conservée. [Piste Q×Z](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md), sans gain ni borne globale acquis ; listes parfois larges.

[Protocole de moments sur vraies feuilles](../receipts/audit_independant_20260930/developer_rebound/catalogue/MOMENTS_PROTOCOL.md) : capturer S/candidats du générateur, boîtes potentiellement très allongées, ancres hors Q fermé. Groupes fixes préparés une fois, credits recouvrants pris par maximum ; mesurer taux éligible, préparation, Dom et coût aval. Petit témoin cubique ne prouve ni feuille atteignable ni gain natif. La réduction Euler des attaches ajoute aussi O(P) résumés de workers ; éviter une table dense n×W supposée gratuite.

Chaîne : index global → boîtes certifiées → segments triés (niveau exact,S*) → fusion externe → plateaux globaux → atlas/verticales → incidences/points → tête/retours. IDs globaux, rangs exacts uniques, accès disque unions/descentes et réservations par phase à définir. [Tri externe](https://www.ittc.ku.edu/~jsv/Papers/AgV88.IO.pdf), [graphes externes](https://www.ittc.ku.edu/~jsv/Papers/CGG95.external_graph.pdf) : références d'E/S, aucune borne HGP complète.

Point partagé/halo fixe ne suffisent pas : K2 {0,1,2} couvre 1 deux fois à β=1/4, fusionne à β=1 ; {0,1,10,11} naît dans le vide à 81/4, fusionne à 25 avec trois parents. [Calculs exacts](../receipts/audit_independant_20260930/massif/semantique/receipt.json).

**Prochaine livraison développeur :** pas/manifeste et port u24 complet, gardes de cardinalité ; résidence RAM/disque. Puis différentiel résident/segments : plateaux transverses, verticales fermées, incidences internes, segments vides et reprise. Sceller segments et publier seulement les plateaux validés. Hiérarchie de points : un K fixé conformément au choix courant ; le [croisement inter-K](../receipts/audit_independant_20260930/cover_band_followup/README.md) ne bloque pas cette cible.
