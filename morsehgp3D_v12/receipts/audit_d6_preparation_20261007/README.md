# D6 — inclure la préparation avant de choisir le profil produit

Lecture statique au pin `60c041aef`, 7 octobre 2026 ; sources produit inchangées depuis `9b2747eff`.
Suite du [contre-audit du pilote](../audit_d6_20261007/README.md), `CST-0207`.
Exploration v12 hors registre, CPU de référence, FULL pi0, u21 ; `not_claimed`.

Le pilote mesure explicitement C et G, ce qui convient à un diagnostic. **Ces deux rapports seuls ne qualifient
pas le choix D6 du profil produit à moins de 3 % de u21.** Dans les deux sondes, `prepare_cloud` précède le premier
`Stopwatch`. Or son coût dépend du profil, même à coordonnées et objet identiques :

| Profil | Bits Morton | `sizeof(Record)` imposé | Chiffres Morton / chiffres totaux |
| --- | ---: | ---: | ---: |
| u21 | 63 | 16 octets | 6 / 9 |
| u24 | 72 | 32 octets | 7 / 10 |
| u32 | 96 | 32 octets | 9 / 12 |

Source : `src/cloud/cloud.cpp:29–36`, `src/cloud/morton.hpp`. `fill_records` compte tous ces chiffres ;
`sort_passes` saute les chiffres constants. **Ne pas convertir le tableau en nombre de passes effectives ni
en facteur temporel** : aux mêmes petites coordonnées, les chiffres hauts peuvent être constants. Le doublement
du record est une formule de stockage et de mouvement, pas un ralentissement mesuré.

Contre-modèle arithmétique, valeurs inventées : préparation 10→20, autres étages 90→90. Les rapports sur les
autres étages sont tous 1 ; celui de la latence totale est 110/100 = 1,10. L'absence de régression sur C/G ne borne
donc pas celle d'une trame depuis son entrée. La préparation se paie à chaque nouvelle trame d'une session
résidente ; réutiliser le même nuage préparé entre passes ne la fait pas disparaître du contrat D1–D3.

Suite conseillée : conserver les mesures C/G comme diagnostics, ajouter préparation hôte/appareil et latence
intégrée par trame aux profils candidats, avec processus et trames appariés. Ne pas additionner les médianes
des deux sondes indépendantes pour inventer ce total. Juger la non-infériorité de D6 avec la borne haute de
l'intervalle du rapport, selon le protocole préenregistré ; afficher les résultats par trame et les échecs.

Aucun défaut de géométrie ni mensonge du pilote établi par cette limite : son README déclare déjà les deux
étages et l'absence d'adoption. Aucun test moteur ni temps nouveau ; lecture et calcul des constantes seulement.
