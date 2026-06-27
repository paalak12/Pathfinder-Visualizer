#include "graph.hpp"
#include "pathfinder.hpp"
#include "renderer.hpp"
#include <unordered_map>
#include <iomanip>
#include <chrono>

const int width = 1200;
const int height = 800;

struct NodePosition {
    Vector2 pos;
    Color color;
};

int main(){
    SetTraceLogLevel(LOG_NONE);
    
    Renderer renderer(width, height, "Jaipur Pathfinder - Dijkstra vs A*");
    renderer.LoadMap("./maps/map.png");
    
    Graph graph("./maps/map_graph.json");
    auto G = graph.getGraph();
    
    std::ifstream f("./maps/map_bounds.json");
    json bounds; f >> bounds; f.close();
    
    double lat_max = bounds["lat_max"].get<double>();
    double lat_min = bounds["lat_min"].get<double>();
    double lon_max = bounds["lon_max"].get<double>();
    double lon_min = bounds["lon_min"].get<double>();
    
    double actual_map_width = static_cast<double>(renderer.MapWidth());
    double actual_map_height = static_cast<double>(renderer.MapHeight());
    double lat_range = lat_max - lat_min;
    double lon_range = lon_max - lon_min;

    // Pre-calculate all node positions
    std::unordered_map<std::string, NodePosition> nodePositions;
    for (const auto& [id, node] : G) {
        double x = ((node.lon - lon_min) / lon_range) * actual_map_width;
        double y = ((lat_max - node.lat) / lat_range) * actual_map_height;
        nodePositions[id] = {{static_cast<float>(x), static_cast<float>(y)}, GREEN};
    }

    // State
    bool pathReady = false;
    bool selectingStart = true;
    std::string start_node, end_node;
    std::vector<Vector2> dijkstraPath, astarPath;
    int dijkstraNodes = 0, astarNodes = 0;
    double dijkstraDist = 0, astarDist = 0;
    double dijkstraTime = 0, astarTime = 0;
    bool showDijkstra = true;

    // Instructions
    std::cout << "Click on map to select START point, then END point." << std::endl;
    std::cout << "SPACE to toggle Dijkstra/A*. R to reset." << std::endl;

    while (renderer.Running()){
        renderer.HandleInput();

        // Handle mouse click for node selection
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !pathReady) {
            Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), renderer.GetCamera());

            // Find nearest node to click
            std::string nearest;
            double minDist = std::numeric_limits<double>::max();
            for (const auto& [id, np] : nodePositions) {
                double d = pow(mousePos.x - np.pos.x, 2) + pow(mousePos.y - np.pos.y, 2);
                if (d < minDist) { minDist = d; nearest = id; }
            }

            if (selectingStart) {
                start_node = nearest;
                nodePositions[start_node].color = BLUE;
                std::cout << "Start set! Now click END point." << std::endl;
                selectingStart = false;
            } else {
                end_node = nearest;
                nodePositions[end_node].color = RED;

                // Run Dijkstra
                Pathfinder dijkstra(G, start_node, end_node, "dijkstra");
                dijkstraNodes = dijkstra.GetNodesExplored();
                dijkstraDist = dijkstra.GetDistances().at(end_node);
                dijkstraTime = dijkstra.GetTimeTaken();
                auto dp = dijkstra.GetPath();
                for (auto& nid : dp) dijkstraPath.push_back(nodePositions[nid].pos);

                // Run A*
                Pathfinder astar(G, start_node, end_node, "astar");
                astarNodes = astar.GetNodesExplored();
                astarDist = astar.GetDistances().at(end_node);
                astarTime = astar.GetTimeTaken();
                auto ap = astar.GetPath();
                for (auto& nid : ap) astarPath.push_back(nodePositions[nid].pos);

                pathReady = true;
                std::cout << "Path found!" << std::endl;
            }
        }

        // Toggle algorithm
        if (IsKeyPressed(KEY_SPACE) && pathReady)
            showDijkstra = !showDijkstra;

        // Reset
        if (IsKeyPressed(KEY_R)) {
            pathReady = false;
            selectingStart = true;
            dijkstraPath.clear();
            astarPath.clear();
            start_node.clear();
            end_node.clear();
            for (auto& [id, np] : nodePositions) np.color = GREEN;
            std::cout << "Reset! Click START point." << std::endl;
        }

        BeginDrawing();
        ClearBackground(WHITE);

        BeginMode2D(renderer.GetCamera());
        renderer.DrawMap();

        // Draw nodes
        for (const auto& [id, nodePos] : nodePositions)
            DrawCircleV(nodePos.pos, 18.0f, nodePos.color);

        // Draw path
        auto& currentPath = showDijkstra ? dijkstraPath : astarPath;
        Color pathColor = showDijkstra ? ORANGE : SKYBLUE;
        if (currentPath.size() > 1) {
            for (size_t i = 0; i < currentPath.size() - 1; i++)
                DrawLineEx(currentPath[i], currentPath[i+1], 20.0f, pathColor);
        }

        EndMode2D();

        // Stats panel
        DrawRectangle(0, 0, 340, 160, Fade(BLACK, 0.7f));
        std::string algoName = showDijkstra ? "DIJKSTRA" : "A*";
        Color algoColor = showDijkstra ? ORANGE : SKYBLUE;
        DrawText(algoName.c_str(), 10, 10, 28, algoColor);

        if (pathReady) {
            int nodes = showDijkstra ? dijkstraNodes : astarNodes;
            double dist = showDijkstra ? dijkstraDist : astarDist;
            double time = showDijkstra ? dijkstraTime : astarTime;

            DrawText(TextFormat("Distance: %.0f m", dist), 10, 50, 20, WHITE);
            DrawText(TextFormat("Nodes explored: %d", nodes), 10, 78, 20, WHITE);
            DrawText(TextFormat("Time: %.2f ms", time), 10, 106, 20, WHITE);
        }

        // Instructions
        if (!pathReady) {
            std::string msg = selectingStart ? "Click to set START" : "Click to set END";
            DrawText(msg.c_str(), 10, height - 35, 22, YELLOW);
        } else {
            DrawText("SPACE: toggle algo | R: reset", 10, height - 35, 20, GRAY);
        }

        EndDrawing();
    }
    
    return 0;
}