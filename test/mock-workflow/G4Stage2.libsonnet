local g4stage1 = import 'G4Stage1.libsonnet';
local utils = import 'algorithm.libsonnet';
local algorithm = utils.algorithm;

{}
+ algorithm('IonAndScint',
            plugin='ion_and_scint',
            duration_usec=5457973,
            inputs=[f + "/SimEnergyDeposits" for f in std.objectFields(g4stage1)],
            outputs=["SimEnergyDeposits", "SimEnergyDeposits/priorSCE"]
           )
+ algorithm('PDFastSim',
            plugin='pd_fast_sim',
            duration_usec=69681950,
            inputs=['IonAndScint/SimEnergyDeposits/priorSCE'],
            outputs=['SimPhotonLites', 'OpDetBacktrackerRecords'],
           )
