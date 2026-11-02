import { listEdges, listNodes } from '../api/campus'
import { useAsync } from './useAsync'

/** The real campus graph: nodes and corridors (with blocked flags) from the C++ CampusGraph. */
export function useCampus() {
  return useAsync(async () => {
    const [nodes, edges] = await Promise.all([listNodes(), listEdges()])
    return { nodes: nodes.nodes, edges: edges.edges, blockedCount: edges.blockedCount }
  }, [])
}
