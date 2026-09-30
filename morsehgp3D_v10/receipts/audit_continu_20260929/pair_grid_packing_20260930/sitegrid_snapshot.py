class SiteGrid:
    """Grille entiere uniforme sur les sites : requetes exactes de boules fermees (elagage par boites entieres,
    decision par comparaison entiere exacte ; aucun flottant)."""

    def __init__(self, sites, per_cell=2):
        self.sites = sites
        n = len(sites)
        lo = [min(p[i] for p in sites) for i in range(3)]
        hi = [max(p[i] for p in sites) for i in range(3)]
        vol = 1
        for i in range(3):
            vol *= hi[i] - lo[i] + 1
        cells = max(1, n // per_cell)
        h = 1
        while h ** 3 * cells < vol:
            h *= 2
        self.h = h
        self.lo = lo
        self.hi = hi
        self.cells = {}
        for s, p in enumerate(sites):
            self.cells.setdefault(self._cell(p), []).append(s)

    def _cell(self, p):
        return tuple((p[i] - self.lo[i]) // self.h for i in range(3))

    def candidates(self, box_lo, box_hi):
        """Sites dont la case coupe la boite entiere [box_lo, box_hi] (sur-ensemble exact)."""
        blo = [max(box_lo[i], self.lo[i]) for i in range(3)]
        bhi = [min(box_hi[i], self.hi[i]) for i in range(3)]
        if any(blo[i] > bhi[i] for i in range(3)):
            return []
        clo = [(blo[i] - self.lo[i]) // self.h for i in range(3)]
        chi = [(bhi[i] - self.lo[i]) // self.h for i in range(3)]
        span = (chi[0] - clo[0] + 1) * (chi[1] - clo[1] + 1) * (chi[2] - clo[2] + 1)
        if span > len(self.cells):
            return [s for s, p in enumerate(self.sites)
                    if all(blo[i] <= p[i] <= bhi[i] for i in range(3))]
        out = []
        for cx in range(clo[0], chi[0] + 1):
            for cy in range(clo[1], chi[1] + 1):
                for cz in range(clo[2], chi[2] + 1):
                    out.extend(self.cells.get((cx, cy, cz), ()))
        return out
