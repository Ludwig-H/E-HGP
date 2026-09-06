# Mesurer le proposeur MEB dans FULL

Cadre : `exploration_v7_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`public_status=not_claimed`. Cette sonde n'est pas la CLI industrielle.

La [sonde courante v5](../bench/full_gabriel_lazy_probe.cpp) conserve
l'entrée, le cache et les digests, mais retire les quotas arbitraires
d'opérations sur demande explicite du 6 septembre. Son profil est
`memory_guarded_no_operation_quotas_v1`. Les anciennes captures v2/v3/v4
restent attribuées à leurs propres sources et limites : aucune n'est
réétiquetée. La [politique propre à la sonde](../bench/full_gabriel_probe_limits.hpp)
ne change pas les API de budgets explicites du producteur ni leurs tests.

P reste un choix **explicite et obligatoire**, partagé entre tous les
appels MEB d'un ordre. P=0 garde F ; un P fini épuisé replie sur F.
`P=unlimited` permet la proposition sans quota de travail de sonde.
Le [contrat du raccord](CONTRAT_MEB_FULL.md) conserve son autorité propre.

## Options et compteurs

La commande ajoute `--meb-proposal-supports=P`, y compris pour P=0.
P est un entier décimal u64 ou le littéral `unlimited`. Le champ
`meb_proposal_budget_kind` distingue `disabled`, `finite` et `unlimited` :
le MAX numérique est fini déclaré, même si son plafond moteur effectif
est identique à celui du littéral. La borne 584 millions dérivée des
quatre millions d'appels n'est plus une limite d'admission.
Les doublons, valeurs signées, non entières ou hors domaine sont rejetés.
Le champ `meb_accounting` nomme
`reference_ordinal_plus_native_z_q3_q4_proposal_v2` ; il est contrôlé aussi
sur le résultat retourné par FULL. Les anciens calendriers restent nommés.

Les sept quotas de travail — facettes visitées, portails, pas de descente,
appels MEB, requêtes, supports de référence et successeurs — deviennent
la borne représentable u64, avec charges prospectives et refus avant
débordement. MAX n'est pas un entier mathématique infini. Les seize pivots
locaux restent un mécanisme algorithmique de terminaison avec repli,
pas un plafond global de benchmark à supprimer.

Les capacités des catalogues, certificats, lectures et caches sont
dérivées des octets des arènes nommées et des types, non de constantes
4M/8M/40M. Les indices spatiaux et de candidats conservent leurs limites
représentationnelles ; les gardes du fold F non appelé sont retirées.
La sonde n'est plus limitée à n=32k ni à un cache d'un million d'entrées.
Les petits contrôles d'admission ne qualifient pas pour autant un run massif.

Le budget logique nommé de 8 Gio, la limite d'espace virtuel de 26 Gio et
les refus d'allocation restent actifs dans la sonde. Les limites par
arène ne constituent pas une borne globale de RAM réelle. À la demande
suivante de l'utilisateur, les mesures directes ne passent plus par
l'admission du contrôleur ni par son délai automatique de 1 200 secondes :
l'agent suit leur progression et arrête une exécution qui tourne en rond.
Un arrêt reste un essai interrompu, jamais une hiérarchie complète.

Portée de sûreté : les charges prospectives et tests MAX qualifient les
compteurs FULL/MEB visés. Le générateur conserve des sommes diagnostiques
u64 dont la non-saturation sur tout le nouveau domaine d'entrée n'est pas
prouvée par ce delta. Le cap de candidats ne borne pas tout le travail
rejeté en amont. Ne pas appeler ce profil « calcul illimité » ni lui
attribuer une certification anti-débordement globale ou massive.

Les lignes d'ordre et le diagnostic terminal publient cinq coûts distincts :

| Champ | Ce qu'il compte |
| --- | --- |
| `meb_proposal_supports` | Essais proposés admis prospectivement, p |
| `meb_proposal_pivots` | Pivots engagés |
| `meb_proposal_certified` | Propositions munies du certificat final |
| `meb_proposal_fallback` | Décisions de repli disposant encore de marge L |
| `meb_reference_supports` | Formes réellement exécutées dans F, A |

`meb_supports` conserve l'ancien préfixe ordinal c : il ne devient pas un
compteur de formes physiques. On vérifie p≤P et A≤c≤L ; les identités
d'appels complètes portent sur les réussites, pas sur les exceptions
intermédiaires. `last_order_work` est un miroir diagnostique : ne pas le
rajouter au cumul des lignes d'ordre.
La v5 vérifie aussi les bornes indépendamment contre-lues
`p ≤ 146 × appels_FULL` et `certified ≤ c−A ≤ 550 × certified` ; sans
certificat, c=A, même si le proposeur a essayé des formes avant son repli.

## Tests et mesures directes

La [capture R1 conservée](../receipts/full_probe_no_quotas_20260906/README.md)
ferme la compilation, 52 contrôles des limites par build O2/SAN et six
CTest ciblés. La campagne micro est partielle : 36 configurations Kmax5
validées, puis échec de métadonnée du selftest first-C à la première Kmax10.
Sa correction ponctuelle ne transforme pas ce reçu en campagne complète.

Les tests portent sur le parseur, les lecteurs normal/`-O`, leurs mutants,
la conservation des compteurs et des digests sur les microcas, puis les
nouveaux domaines CLI et les frontières représentables MAX.
La sentinelle locale tétraèdre distingue P3 (repli, A=11) et P6
(certificat, A=0) à support et niveau brut identiques.

Le [runner direct](../bench/run_full_probe.py) lance un seul essai, garde
la commande, le hash du binaire, les sorties brutes, le temps et le code
de sortie. Ce n'est pas un certificat de complétude. Un défaut de format
d'un auto-test de lecteur n'empêche pas une mesure du binaire compilé ;
la qualification partielle et le défaut restent déclarés séparément.

```bash
python3 -B morsehgp3D_v7/bench/run_full_probe.py --binary build/v7_no_work_quotas_20260906_controller/build_r1/full_gabriel_lazy_probe --output build/v7_direct_example/n8000_s8 --n 8000 --s 8
```

Les neuf cellules n=8k/16k/32k et s=8/10/12 sont pré-déclarées ; chacune
compare le même binaire à P0 et à `unlimited`, en mono-thread. Un refus clos
du premier bras ne bloque pas à lui seul l'exécution du second. Une
censure ou un terminal absent ne peut être promu en mesure réussie.
Les comparaisons de préfixes nomment les seuls ordres communs réussis.

Le temps inclut le digest et la lecture provisoire ; chaque forêt est
ensuite libérée. **Ni archive FULL retenue, ni verticale inter-K, ni tour
industrielle complète ne sont mesurées.** Les contrats [50k et G4](CONTRAT_PERFORMANCE.md)
restent distincts. Aucune mesure v2/v3/v4 n'est réétiquetée en mesure du
proposeur ; aucun succès relatif de la sonde ne certifie à lui seul
la complétude géométrique du fournisseur de catalogues.

## Croissance quand n double

La consigne utilisateur du 6 septembre demande d'évaluer explicitement
la croissance sur 8 000, 16 000 et 32 000 points. On garde constants Kmax,
s, famille, graine, profil, politique de cache, P, binaire et nombre de
threads. Les ratios de temps, pic mémoire, candidats, sorties et travail
sont rapportés séparément pour les deux doublements ; l'exposant observé
est $\alpha(n)=\log_{2}(T(2n)/T(n))$. Une valeur inférieure à deux sur
ces tailles n'est pas une preuve asymptotique universelle.

Le temps complet n'est comparé que pour des tentatives complètes de même
périmètre. Si 32k refuse à K9, son temps de refus ne remplace pas le temps
de K1..10. Les compteurs des ordres communs réussis et les étapes amont
complètes restent analysables, avec ce périmètre explicitement restreint.
On sépare le nombre de minima/nœuds/parents réellement émis du travail
intermédiaire ; une borne sur Delaunay ou sur les candidats n'est pas
automatiquement une borne sur la sortie FULL. Uniforme/seed3 constitue
un premier régime, pas une qualification de toutes les géométries.
