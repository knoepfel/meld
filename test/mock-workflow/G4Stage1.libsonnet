local generators = import 'SinglesGen.libsonnet';
local utils = import 'algorithm.libsonnet';

utils.algorithm('largeant',
                plugin='largeant',
                duration_usec=15662051,
                inputs=[f + "/MCTruths" for f in std.objectFields(generators)],
                outputs=["ParticleAncestryMap", "Assns", "SimEnergyDeposits", "AuxDetHits", "MCParticles"]
               )
