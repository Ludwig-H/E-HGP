# Question à l'auditeur — le census q3 est le mur, chiffré

Date : 17 août 2026 UTC. Cadre : `exploration_v4_hors_registre`,
`public_status=not_claimed`. Fait suite à votre ETAT_COURANT (ordre de
travail, étape 5) et au reçu `receipts/q3_events_20260817/`.

## Le fait

L'instruction q3 est **exacte** (juge par identités 0/0 sur uniform et
eight_clusters n=400, coquilles refusées transactionnellement) mais son coût
est dominé par les ancres à grande lentille : à n=2000, eight_clusters paie
115,5 M de tests de porteurs et 475 s là où uniform en paie 7,3 M et 36 s.
Chaque porteur retenu paie en outre une descente de profondeur de
circum-boule (early-exit à h_3, élagage/crédit par blocs, mais depuis la
racine à chaque fois). La v3 avait déclaré ce poste non résolu (« c'est le
census, et non l'énumération des aigus, qui reste à rendre
sous-quadratique ») ; la v4 le confirme et le chiffre.

## Questions

**Q9 — analogue axial pour q3.** Pour une ancre (a,b) fixée, les
circumcentres des triangles {a,b,x} vivent sur le plan médiateur de ab
(2 degrés de liberté, contre 1 pour l'axe q4). Voyez-vous une structure
« racines extrémales » à la Q4SeedAxisTopR4 sur ce plan — par exemple en
factorisant par droite du plan médiateur (le « pied » v3 § 5.3), qui
rendrait le census q3 par ancre en O(candidats retenus) plutôt qu'en
O(porteurs × descente) ? Ou faut-il partager les descentes entre porteurs
d'une même ancre (les circum-boules d'une ancre sont toutes incluses dans
B(m, 0,966·D...) — une seule descente de lentille pourrait servir tous les
porteurs si l'on sait borner P_x(z) uniformément en x) ?

**Q10 — ordonnancement output-sensitive.** Les événements se concentrent
sur les ancres courtes ; les ancres longues inter-amas coûtent le plus et
rendent le moins. Un contrat « instruire par taille de lentille croissante,
continuation `resource_exhausted` au-delà d'un budget déclaré » est-il
acceptable comme contrat public v4 (le § 5.6 v3 prouve qu'une garantie
sous-quadratique universelle est impossible : sortie q3 quadratique
réelle) ? Ou voyez-vous un certificat de vacuité de lentille par bloc
d'ancres (dual-tree ancres × porteurs) qui éviterait l'énumération des
grandes lentilles vides ?

**Q11 — indépendance du juge.** Le juge d'identités partage le prédicat de
profondeur du sujet. Priorité à l'oracle à arithmétique indépendante
(rationnels/BigInt réécrits, selftest du juge) maintenant, ou après le
census q4 ?
