#include "utils/generators/graph.hpp"
#include "utils/generators/tree.hpp"
#include "utils/AlgorithmRecorder.hpp"

int main(){
    AlgorithmRecorder recorder(-100,100,-100,100);
    auto adj = random_graph(20,20,true);
    recorder.record_unweighted_graph(adj,true);
    recorder.commit_step();

    recorder.record_unweighted_graph(adj,false);
    recorder.commit_step();

    recorder.record_unweighted_graph(adj,true,AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED);
    recorder.commit_step();

    recorder.record_unweighted_graph(adj,false,AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED);
    recorder.commit_step();

    adj = random_tree(10);
    recorder.record_unweighted_graph(adj,true);
    recorder.commit_step();

    recorder.record_tree(adj);
    recorder.commit_step();

    adj = random_bipartite_graph(10,10);
    recorder.record_unweighted_graph(adj,false,AlgorithmRecorder::GraphLayoutType::BIPARTITE);
    recorder.commit_step();


    adj = random_bipartite_graph(10,10);
    recorder.record_unweighted_graph(adj,false,AlgorithmRecorder::GraphLayoutType::BIPARTITE);
    recorder.commit_step();


    adj = random_dag(10,20);
    recorder.record_unweighted_graph(adj,false,AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED);
    recorder.commit_step();

    auto wadj = random_weighted_graph(10,20,true);
    recorder.record_weighted_graph(wadj,true,AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED);
    recorder.commit_step();

    recorder.run();
    return 0;
}