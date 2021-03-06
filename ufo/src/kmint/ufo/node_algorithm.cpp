#include <algorithm>
#include <cassert>
#include <numeric>
#include <utility>
#include <cfloat>
#include <iostream>
#include <set>
#include <queue>
#include <stack>
#include "kmint/ufo/node_algorithm.hpp"
#include "kmint/random.hpp"

namespace kmint::ufo {

    map::map_node const &random_adjacent_node(map::map_node const &node) {
        int r = random_int(0, node.num_edges());
        return node[r].to();
    }

    map::map_node &find_node_of_kind(map::map_graph const &graph, char kind) {
        auto i = std::find_if(graph.begin(), graph.end(), [kind](auto const &node) {
            return node.node_info().kind == kind;
        });
        return const_cast<map::map_node &>(*i);
    }

    map::map_node &random_node_of_kind(map::map &map, char kind) {
        auto b = map.begin_of_kind(kind);
        auto e = map.end_of_kind(kind);
        int n = e - b;
        assert(n > 0);
        return *b[random_int(0, n)];
    }

    map::map_node const &random_node_of_kind(map::map const &map, char kind) {
        auto b = map.begin_of_kind(kind);
        auto e = map.end_of_kind(kind);
        int n = e - b;
        assert(n > 0);
        return *b[random_int(0, n)];
    }

    map::map_node const &find_closest_node_to(map::map_graph const &graph,
                                              math::vector2d location) {
        auto i = std::min_element(graph.begin(), graph.end(),
                                  [location](auto const &l, auto const &r) {
                                      return distance2(l.location(), location) <
                                             distance2(r.location(), location);
                                  });
        return *i;
    }

    double waiting_time(map::map_node const &node) { return node[0].weight(); }

    void moveNodeToClosed(
            NodeWrapper *node,
            std::vector<NodeWrapper *> &openList,
            std::vector<NodeWrapper *> &closedList) {
        auto it = openList.begin();
        for (auto i : openList) {
            if (i->getNode()->node_id() == node->getNode()->node_id()) {
                closedList.emplace_back(i);
                openList.erase(it);
                return;
            } else {
                it++;
            }
        }
    }

    NodeWrapper *findNode(NodeWrapper *node, std::vector<NodeWrapper *> &openList) {
        for (auto i : openList) {
            if (i->getNode()->node_id() == node->getNode()->node_id()) {
                return i;
            }
        }
        return nullptr;
    }

    NodeWrapper *getMinNode(std::vector<NodeWrapper *> &openList) {
        NodeWrapper *min = nullptr;
        for (auto node : openList) {
            //update min node when smaller fCost is found
            //update min node when equal but lower hCost
            if (min == nullptr || min->fCost() > node->fCost()
                || (min->fCost() == node->fCost() && node->hCost < min->hCost)) {
                min = node;
            }
        }
        return min;
    }

    float calculate_heuristic(Heuristic h, const float x, const float y, const float goal_x, const float goal_y) {
        switch (h) {
            case MANHATTAN: {
                return std::abs(x - goal_x) + std::abs(y - goal_y);
            }
            case DIAGONAL: {
                if (goal_x < 0 || goal_y < 0) {
                    return -1;
                }
                return std::max(std::abs(x - goal_x), std::abs(y - goal_y));
            }
            case EUCLIDEAN: {
                if (goal_x < 0 || goal_y < 0) {
                    return -1;
                }
                return std::sqrt(std::abs(x - goal_x) * 2 + std::abs(y - goal_y) * 2);
            }
            case DIJKSTRA: {
                return 0;
            }
            default: {
                return -1;
            }
        }
    }

    float calculate_heuristic(Heuristic h, map::map_node const &loc, map::map_node const &destLoc) {
        return calculate_heuristic(h, loc.location().x(), loc.location().y(), destLoc.location().x(),
                                   destLoc.location().y());
    }

    void
    CreatePath(NodeWrapper *current, NodeWrapper *origin, std::vector<NodeWrapper *> &path, map::map_graph &graph) {
        if (current == origin) {
            return;
        }
        graph[current->getNode()->node_id()].tag(graph::node_tag::path);
        path.emplace_back(current);
        CreatePath(current->getParent(), origin, path, graph);
    }

    /// tag the shortest path from actorLoc to goalLoc using our own implementation of A*
    /// \param heuristic used to calculate distance between the current position and the goal position.
    /// \param actorLoc starting map_node of search.
    /// \param goalLoc destination map_node of search
    PathWrapper tag_shortest_path_astar(Heuristic heuristic, map::map_node const &actorLoc, map::map_node
    const &goalLoc, map::map_graph &graph) {
        graph.untag_all();
        std::vector<NodeWrapper *> openList{};
        std::vector<NodeWrapper *> closedList{};
        PathWrapper pathWrapper{};
        //! <rant>I dislike the way you have to get a non-const reference (and I have to since I want to use tag) from the kmint framework
        //! This is my workaround. I know I break the holy const code but idc cause not undefined behaviour checkmate.</rant>
        //! What it does: takes the node_id from the const ref and pull it from the graph non-const with [] operator.
        auto *mutableActorLoc = &graph[actorLoc.node_id()];
        auto *actorPtr = new NodeWrapper(0.0, 0.0, mutableActorLoc, nullptr);
        openList.emplace_back(actorPtr);
        while (!openList.empty()) {
            NodeWrapper *topPtr = getMinNode(openList);
            //! Same workaround as above to get mutableRef
            auto &topNodeRef = graph[topPtr->getNode()->node_id()];
            topNodeRef.tag(graph::node_tag::visited);
            float prevf = topPtr->fCost();
            for (auto &edge : topNodeRef) {
                map::map_node &successor = edge.to();
                //! Calculate new distance for successor
                float hCost = calculate_heuristic(heuristic, successor, goalLoc);
                if (hCost < 0) { return pathWrapper; }
                float gCost = prevf + edge.weight();
                auto *successorPtr = new NodeWrapper{gCost, hCost, &successor, topPtr};
                //! Check if this edge leads to the goal node.
                if (successor.node_id() == goalLoc.node_id()) {
                    std::vector<NodeWrapper *> path{};
                    CreatePath(successorPtr, actorPtr, path, graph);
                    pathWrapper = PathWrapper{path};

                    for (auto c : openList) {
                        //std::cout << *c << std::endl;
                        if (c->getNode()->tag() != graph::node_tag::visited &&
                            c->getNode()->tag() != graph::node_tag::path) {
                            delete c;
                        }
                    }
                    for (auto d : closedList) {
                        if (d->getNode()->tag() != graph::node_tag::path) {
                            delete d;
                        }
                    }
                    return pathWrapper;
                }
                //! Check if the successor node has been visited before
                if (successor.tag() == graph::node_tag::visited) {
                    continue;
                }
                //! Find successor on openList
                NodeWrapper *found = findNode(successorPtr, openList);
                if (found == nullptr) {
                    openList.emplace_back(successorPtr);
                } else {
                    //! check the distance to the previous we have processed before
                    //! adjust distance if the previous was higher
                    if (found->fCost() > successorPtr->fCost()) {
                        found->gCost = successorPtr->gCost;
                        found->hCost = successorPtr->hCost;
                        found->setParent(topPtr);
                    }
                }
            }
            //! Remove node from openList and add to closedList.
            moveNodeToClosed(topPtr, openList, closedList);
        }
        //! Failed to find goal
        std::cout << "Fail: " << std::endl;
        for (auto c : openList) {
            std::cout << *c << std::endl;
            if (c->getNode()->tag() != graph::node_tag::visited) {
                delete c;
            }
        }
        for (auto d : closedList) {
            delete d;
        }
        return pathWrapper;
    }
}// namespace kmint
