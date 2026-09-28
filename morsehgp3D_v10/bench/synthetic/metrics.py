"""Metriques du banc v10. Convention principale ARI_s : le bruit vrai (-1) et le bruit predit (-1) deviennent
chacun des singletons ; l'ARI ne recompense donc ni la couverture ni l'abstention (EVAL_v2, D1)."""
import numpy as np
from sklearn.metrics import adjusted_mutual_info_score, adjusted_rand_score


def singletons(labels):
    labels = np.asarray(labels).copy()
    noise = labels < 0
    base = labels.max() + 1 if (~noise).any() else 0
    labels[noise] = base + np.arange(noise.sum())
    return labels


def scores(truth, pred):
    truth = np.asarray(truth)
    pred = np.asarray(pred)
    keep = truth != -2  # lignes ignorees (ponts)
    t, p = truth[keep], pred[keep]
    return dict(ari_s=float(adjusted_rand_score(singletons(t), singletons(p))),
                ari_nc=float(adjusted_rand_score(t, p)),
                ami_nc=float(adjusted_mutual_info_score(t, p)),
                coverage=float((p >= 0).mean()),
                clusters=int(len(set(p.tolist()) - {-1})))
