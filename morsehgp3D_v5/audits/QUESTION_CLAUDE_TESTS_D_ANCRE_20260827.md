# Question de Claude aux auditeurs — tests d'ancre (W_q exact + témoins sectoriels), contrat 50 k et priorité GPU

- **Date :** 27 août 2026
- **Pin :** `a9a2f509` (code, fixtures, mutants, reçus de mesure) ; analyse : `../docs/analyses/seeds_20260827/` (rapport mathématique, mesure instrumentée, conception device, contradicteur) ; théorèmes : `../docs/MATHEMATIQUES.md` § 10
- **Cadre :** `phase=exploration_v5_hors_registre`, `public_status=not_claimed`, GCP non utilisé pour cette question

## Ce qui a été établi et ce qui est demandé

Le coût des lanes q3/q4 sur les familles denses (18,2 G seeds q3 à `eight_clusters` 50 k pour quelques millions de candidats) vient d'ancres dont **chaque** boule est profonde sans témoin commun : le test d'ancre existant (histogramme de coin, et $W_4$ en q4) ne les voit pas. Deux tests suffisants, exacts en entiers, ont été prouvés, mesurés et intégrés dans les corps partagés `scan_anchor_q3` / `process_anchor_q4` (donc dans les lanes hôte, par lots et device) :

1. **$W_3$ exact** (`anchor_universal_kill` = `in_spindle(kQ3)` sur le cover, sortie à $h_3$) — absent de la lane q3 jusqu'ici ;
2. **témoins sectoriels** (`anchor_sector_kill`, théorème 10.3 : polygone convexe à sommets entiers contenant le disque des centres, minimum d'une forme affine aux sommets).

Les deux sont **incomparables** (fixtures F1 et F3) et cumulés. Invariance de l'objet : une ancre tuée n'émettait rien (chaque seed avait $\ge h$ intérieurs stricts dans le cover, donc était tué à la génération) ; les conformités v4 restent égales ; seuls les compteurs changent.

**Verrous (par importance, d'après le contradicteur) :**

- **V7 — objet et contrat.** Recevez-vous les deux preuves de suffisance (§ 10, lemmes 10.1–10.3) et le placement dans les corps partagés ? Quelle liste de compteurs contractuels retenez-vous pour les portes appariées (`seeds`, `depth_killed`, `q3_cert`, `anchors_killed_w3`, `anchors_killed_sectors`, `q4_completions`, rejets), sachant que les reçus 8 k / 16 k / 32 k / 50 k antérieurs ont des compteurs périmés et des digests inchangés ? Faut-il refaire ces reçus avant toute autre mesure ?
- **V8 — priorité GPU.** La base « lane q3 CPU = 94 s » est périmée. Proposition : la prochaine session G4 mesure la **nouvelle lane CPU** à 16 k / 32 k / 50 k sur les quatre familles **avant** toute écriture de kernel par rectangle ; si q3 tombe sous ~20 s, le device n'a plus de gain démontré sur q3 et le point 2 se recentre sur q4 (ou se ferme comme voie de gain). Êtes-vous d'accord pour subordonner la livraison 7 à cette mesure ?
- **V9 — routage $W_3$.** $W_3$ avant le cover (`cover_query` coef 1, cover seulement si survie) ou après (sur le cover trié, classes 0..10) : les deux formes sont égales post-RLE ; le choix est une mesure G4, une seule forme par défaut, sans sélection par famille. Objection ?
- **V10 — test cellulaire.** Le contradicteur reçoit la mathématique du « $W_3$ par cellule » ($k_{cell} \le \text{depth} \le c_{cell}$, strictness exacte) mais **refuse** son emploi comme lane (les évaluations ne sont pas le coût) ; il ne l'envisage qu'en raffinement du polygone (deux anneaux, $K > 8$) sur mesure de temps. Confirmez-vous ?
- **V11 — q4.** Recevez-vous $J = G(D^2 - 8 \left\vert v_3 \right\vert^2) \ge G D^2/3 > 0$ (branche $J < 0$ inatteignable, gardée), l'identité de signe $\text{sign}\, q4\_power(f_4, z) = \text{sign}(B(y)) \cdot \text{sign}(P(z) B(y) - P(y) B(z))$ pour $y$ non coplanaire, et le refus d'ouvrir le chantier « tri des rationnels par seed » sans modèle de coût (8,8 % des complétions atteignent la profondeur, à ~12 sites) ?
- **V12 — nécessité.** Confirmez-vous qu'aucun test sur le disque **fermé** n'est nécessaire (exemple 2.4 : 28 sites sur la sphère diamétrale, $\min$ sur le disque $= 0$ mais tout seed mort) et qu'aucun juge $O(m^2 \log m)$ n'est requis (fixtures + égalité de digest suffisent, l'énumération des seeds étant le juge d'échantillon) ?
- **V13 — mou de l'histogramme.** Le facteur 2,33 entre ancres survivantes et ancres vraiment $W_3$-vivantes est-il structurel (cônes de 60° hors des boîtes $A$, $B$) et non une faute de `h_coeur` ? Le contradicteur le classe structurel.
- **V14 — hygiène de mesure.** Tous les temps cités viennent de 8 vCPU partagés avec des sondes concurrentes (dérives de 42 % observées entre deux runs du même code) : ils ne sont donnés que comme ratios dans un même run ; aucun temps ne sera cité sans reçu G4. Le `LISEZMOI` du reçu `mesures_secteurs_20260827` a été corrigé en ce sens.

## Ce que je fais en attendant

Session G4 gardée mesurant la nouvelle lane CPU (et les contrats `--gpu` inchangés, pour l'égalité des deux digests) ; aucune écriture de kernel par rectangle avant votre réponse sur V8.
