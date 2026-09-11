# Terminal privé : correction de l’identité commune

Cadre `phase=exploration_v7_hors_registre`, `backend=HOST_STUB`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Révision privée issue du terminal c03, sans optimisation post-seed, aucun code actif/Git/audit/GCP modifié. O2 et SAN ROOT core/T2/guards sont fermés PASS sur cette révision. Aucun export CUDA ni kernel n’appartient à cette qualification hôte ; la préparation `cuda_trial/` est un chantier de preuve séparé.

## Défaut conservé, correction bornée

Le premier terminal est conservé intégralement dans `build/v7_gpu_static_terminal_20260911/`. Ses O2 et SAN core/T2/guards sont fermés PASS sur leurs fixtures, mais ne couvrent pas une collision réelle découverte en contre-lecture : `HostOwner` et `IndexOwner` avaient chacun un compteur démarrant à 1, tout en exposant le même type `IndexView`. Un index étranger pouvait ainsi être admis avec le catalogue d’un autre propriétaire.

Cette révision ne modifie que l’attribution de l’identité : `HostOwner` réutilise le token du **nouvel IndexOwner interne qu’il construit et possède**, jamais celui d’un index emprunté. Il n’a plus de compteur indépendant et ne remplace plus le token de sa vue d’index. L’espace monotone des IndexOwner couvre donc tous les IndexView compatibles.

Cette identité détecte le mélange de propriétaires certifiés dans le processus. Les vues POD restent empruntées en lecture seule : elle ne protège pas contre la fabrication manuelle arbitraire de pointeurs/tokens ni contre l’emploi d’une vue après destruction de son propriétaire.

`guards_trial/gate.cpp --owner-cross` est une invocation séparée en processus frais : création d’un IndexOwner étranger, puis d’un HostOwner sur une géométrie différente, vérification du cas nominal, et mélange effectif index/catalogue. Le refus attendu est `kInvalidView`, sans aucun appel MEB payé. Le recorder doit également compiler l’ancien `terminal_owner.hpp` exact conservé sous `guards_trial/terminal_owner.r2.hpp` et constater que cette même fixture échoue sur `guard.cross_owner_must_refuse_foreign_index`. L’ancien résultat ne sera ni écrasé ni réinterprété en preuve positive.

`ownerfix_import.json` épingle les 96 fichiers copiés et les sept reçus antérieurs ; `prepare_ownerfix.py` est create-only. Les règles algorithmiques, l’intégration FULL de preuve et les juges restent ceux décrits dans `originals/terminal_readme.r2.md`. Ce document historique ne décrit pas le statut courant de cette nouvelle révision.

## Qualification fermée, sans héritage

Les trois captures O2 passent : mêmes 20 851 contrôles unitaires, 256 672 contrôles FULL historiques en static1/static4, et 54 tours T2 K1..10. Les guards gardent 139 contrôles/31 refus nominaux ; `--owner-cross` ajoute cinq contrôles et un refus avant travail. L’ancien owner exact, recompilé avec le nouveau juge, échoue causalement comme attendu sur `guard.cross_owner_must_refuse_foreign_index` (exit 1). Les cinq autres mutants O2 restent causaux. Ce n’est pas une simple inspection du token : le mélange réel est présenté à `resolve`.

Les trois nouvelles captures SAN ROOT passent les mêmes contrôles core/T2 et les rejets nominaux/croisés de guards, ASan/UBSan avec détection des fuites activée. Les six mutations compilées de guards ont été exécutées en O2 ; les quatre mutants T2 intégrés au binaire sont aussi réfutés en SAN. Les invocations inconnue/absente retournent 2. Aucune ancienne réussite ne remplace ces nouvelles captures.

```bash
python3 -B build/v7_gpu_static_terminal_ownerfix_20260911/record.py --out o2_r1 --mode stub
python3 -B build/v7_gpu_static_terminal_ownerfix_20260911/t2_trial/record.py --out o2_r1 --mode stub
python3 -B build/v7_gpu_static_terminal_ownerfix_20260911/guards_trial/record.py --out o2_r1 --mode stub --mutants
```

Les recorders compilent réellement leur `source_snapshot` et conservent commandes, empreintes, stdout/stderr et codes attendus. SAN a été exécuté séparément depuis ROOT, fuites activées. Aucun benchmark n’est contenu dans ces gates : la référence CPU et les calculs supplémentaires de preuve ne sont pas le chemin produit.

Les limites restent explicites : K=2..10 pour le terminal, index géométriquement unique, catalogue de la représentation c03 avec coquille ≤12 et intérieur ≤9, aucune limite de chaîne ajoutée. Le catalogue est admis structurellement par ce propriétaire, pas certifié géométriquement. La trace diagnostique peut être bornée sans borner la recherche. Aucun wrapper CUDA complet, aucune exécution device de ce terminal, aucun contrat de tour 50k ou multi-millions ne sont acquis.
