local imported = import 'algorithm.libsonnet';
local algorithm = imported.algorithm;

{}
+ algorithm("rn222",
            plugin = "MC_truth_algorithm",
            duration_usec = 39,
            inputs = ["id"],
            outputs = ["MCTruths"]
           )
+ algorithm("ar39",
            plugin = "MC_truth_algorithm",
            duration_usec = 12410,
            inputs = ["id"],
            outputs = ["MCTruths"]
           )
+ algorithm("cosmicgenerator",
            plugin = "MC_truth_algorithm",
            duration_usec = 4926215,
            inputs = ["id"],
            outputs = ["MCTruths"]
           )
+ algorithm("kr85",
            plugin = "MC_truth_algorithm",
            duration_usec = 1643,
            inputs = ["id"],
            outputs = ["MCTruths"]
           )
+ algorithm("generator",
            plugin = "three_tuple_algorithm",
            duration_usec = 69616,
            inputs = ["id"],
            outputs = ["MCTruths", "BeamEvents", "beamsims"]
           )
+ algorithm("ar42",
            plugin = "MC_truth_algorithm",
            duration_usec = 148,
            inputs = ["id"],
            outputs = ["MCTruths"]
           )
