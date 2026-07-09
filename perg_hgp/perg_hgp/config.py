import yaml

class PERGHGPConfig:
    """
    Configuration parameters for PERG-HGP.
    """
    def __init__(self, **kwargs):
        # Model parameters
        self.K = kwargs.get('K', 10)
        self.K_rho = kwargs.get('K_rho', None)
        if self.K_rho is None:
            self.K_rho = max(32, 3 * self.K)
        self.alpha = kwargs.get('alpha', 0.0)

        # Exactness Mode
        self.exactness_mode = kwargs.get('exactness_mode', 'atlas_exact') # 'soft_only', 'atlas_exact', 'global_gabriel_certified', 'cut_certified'

        # Budgets
        self.max_witnesses_per_rank = kwargs.get('max_witnesses_per_rank', 5000000)
        self.max_cofaces = kwargs.get('max_cofaces', 20000000)
        self.max_unique_facets = kwargs.get('max_unique_facets', 100000000)
        self.max_dual_edges = kwargs.get('max_dual_edges', 300000000)
        self.max_ram_facets = kwargs.get('max_ram_facets', 100000000)

        # Grid parameters
        self.m_local = kwargs.get('m_local', 128)
        self.grid_resolution = kwargs.get('grid_resolution', 64)

        # Witness parameters
        self.m_active = kwargs.get('m_active', 128)
        self.W1_budget = kwargs.get('W1_budget', 3000000)
        self.budget_per_rank = kwargs.get('budget_per_rank', 2000000)
        self.beam_per_bucket = kwargs.get('beam_per_bucket', 4)
        self.rank_eps_schedule = kwargs.get('rank_eps_schedule', [1.0, 0.5, 0.25, 0.125])
        self.gamma = kwargs.get('gamma', 0.8)
        self.fixed_point_iters_per_temp = kwargs.get('fixed_point_iters_per_temp', 4)


        # Lifting parameters
        self.L_shell = kwargs.get('L_shell', 4)
        self.support_cap = kwargs.get('support_cap', 4)

        # Certification & Audit
        self.local_gabriel = kwargs.get('local_gabriel', True)
        self.global_gabriel = kwargs.get('global_gabriel', 'selective') # 'none', 'selective', 'all'
        self.gabriel_tol = kwargs.get('gabriel_tol', 1e-6)

        # Hierarchy
        self.min_cluster_size = kwargs.get('min_cluster_size', 100)
        self.expZ = kwargs.get('expZ', 2.0)

        # Device
        self.device = kwargs.get('device', 'cpu') # 'cpu' or 'cuda'

        # Checkpointing
        self.checkpoint_dir = kwargs.get('checkpoint_dir', None)

    @classmethod
    def load_from_yaml(cls, filepath):
        with open(filepath, 'r') as f:
            data = yaml.safe_load(f)
        return cls(**data)

    def to_dict(self):
        return self.__dict__.copy()
