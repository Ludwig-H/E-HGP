# Terminal : un seul comptage privé

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Prototype non intégré : seule `alive_rectangles_fused` change dans la copie candidate de `generate.hpp`. La référence est le produit déjà muni du réemploi q2, SHA `345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c`. Les autres sources compilées doivent rester identiques entre bras.

Le caractère terminal est calculé avant le comptage. Un non-terminal conserve le comptage cœur seul ; un terminal demande directement les coins, une seule fois, puis utilise ce résultat pour les morts et l'émission. Aucun helper géométrique, seuil, masque, ordre de parcours, limite ou crédit n'est modifié.

## Obligation et limite

Pour une lane donnée, la valeur écrêtée ne dépend pas des autres bits demandés : ils ne modifient que le travail partagé. L'autorité avec coins conserve tous les crédits cœur. Ainsi $C_{2,\mathrm{true}}=C_{2,\mathrm{false}}$ et $C_{q,\mathrm{true}}\geq C_{q,\mathrm{false}}$ pour $q\in\{3,4\}$. Les masques par sous-arbre empêchent les doubles crédits. Une lane fermée par l'ancien premier passage reste fermée par le passage unique ; sinon sa valeur terminale est exactement celle de l'ancien second passage. Le résultat terminal est donc identique : masque et crédits vivants, ordre des rectangles et grands-livres. Par induction sur les vagues identiques, scissions, pics et refus prospectifs sont identiques.

Cela ne prouve pas un gain de coût universel. Le nouveau passage peut effectuer des coins pour une lane que l'ancien cœur aurait fermée avant le second parcours. Il calcule aussi la séparation pour les rectangles anciennement tués avant cette étape. Le comparateur autorise donc uniquement les variations de `witness_nodes` et `corner_evals`, sans exiger leur monotonie, et rapporte séparément les économies et surcoûts. Toute autre différence est rejetée.

## Qualification bornée

Réemploi byte-exact du gate historique `front_gate.cpp` SHA `45c4736806e103b930c8ac7da1982d069d7ae115a9b310a62f4c847158fc6b2e` ; ses commentaires historiques ne sont pas les pins de ces nouveaux bras. Corpus inchangé : 174 fronts, dont 6 refus ; s=8/10/12, 7 masques, deux seuils, quatre scènes. La paire à deux points exige ici q2 seul 3→3 nœuds, et chacun des six autres masques 6→3 : 36 témoins causaux. Les deux champs de travail sont seuls projetés avant l'égalité récursive typée de toute la sortie.

Le protocole neuf réutilise uniquement les primitives de capture/processus du contrôleur historique `003e3e6c972d880e2848fd2fb2371e3e650e5397e273970665308903a8cf9ea3`, copié et importé explicitement ; son ancien `main()` n'est jamais appelé. Les globals BASE/ROOT/LOG sont redirigés vers ce dossier neuf. Deux compilations O2, --selftest=0 et --unknown=2 pour chaque bras, puis comparaison Python normale/-O. Les dépendances compilées doivent appartenir aux snapshots. Pas de qualification SAN héritée, pas de benchmark FULL, pas de contrat de performance revendiqué. Aucune campagne 8k lancée par ce protocole. GCP non utilisé.
