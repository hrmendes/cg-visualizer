#include "utils/generators/graph.hpp"
#include "utils/generators/tree.hpp"
#include "utils/AlgorithmRecorder.hpp"

int main() {
    AlgorithmRecorder recorder(-100, 100, -100, 100, 1300, 1300);

    {
        auto adj = random_graph(20, 25, false);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::CIRCULAR
        );
        recorder.record_log("Random undirected graph - Circular");
        recorder.record_unweighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_graph(20, 25, false);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random undirected graph - Force Directed");
        recorder.record_unweighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_graph(15, 20, true);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::CIRCULAR
        );
        recorder.record_log("Random directed graph");
        recorder.record_unweighted_graph(adj, layout, radius, true, false);
        recorder.commit_step();
    }
    {
        auto adj = random_graph_with_cycles(15, 20, true);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random directed graph with cycles");
        recorder.record_unweighted_graph(adj, layout, radius, true, true);
        recorder.commit_step();
    }
    {
        auto adj = random_dag(15, 25);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random DAG");
        recorder.record_unweighted_graph(adj, layout, radius, true, false);
        recorder.commit_step();
    }
    {
        auto adj = random_bipartite_graph(16, 20);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::BIPARTITE
        );
        recorder.record_log("Random bipartite graph");
        recorder.record_unweighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_functional_graph(15);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random functional graph");
        recorder.record_unweighted_graph(adj, layout, radius, true, true);
        recorder.commit_step();
    }
    {
        auto adj = random_grid_graph(4, 5);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Grid graph 4x5");
        recorder.record_unweighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_weighted_graph(12, 20, false, 1, 50);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random weighted graph");
        recorder.record_weighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_weighted_graph_with_cycles(12, 20, true, 1, 50);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED
        );
        recorder.record_log("Random weighted graph with cycles");
        recorder.record_weighted_graph(adj, layout, radius, true, true);
        recorder.commit_step();
    }
    {
        auto adj = random_tree(15);
        auto [radius, layout] = recorder.compute_tree_layout(adj, 0);
        recorder.record_log("Random tree");
        recorder.record_unweighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }
    {
        auto adj = random_weighted_tree(15, 1, 30);
        auto [radius, layout] = recorder.compute_tree_layout(adj, 0);
        recorder.record_log("Random weighted tree");
        recorder.record_weighted_graph(adj, layout, radius, false, false);
        recorder.commit_step();
    }    
    {
        auto adj = random_flow_network(12,18,1,20,true);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FLOW_NETWORK
        );
        recorder.record_log("Bipartite flow network");
        recorder.record_weighted_graph(adj, layout, radius, true, false);
        recorder.commit_step();
    }
    {
        auto adj = random_flow_network(15, 25, 1, 20, false);
        auto [radius, layout] = recorder.compute_graph_layout(
            adj,
            AlgorithmRecorder::GraphLayoutType::FLOW_NETWORK
        );
        recorder.record_log("General flow network");
        recorder.record_weighted_graph(adj, layout, radius, true, false);
        recorder.commit_step();
    }

    recorder.run();

    return 0;
}
