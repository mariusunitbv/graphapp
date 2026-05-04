module;
#include <pch.h>

module node_viewer;

import graph_common;

void NodeViewer::render(const GraphModel* model, GraphViewModel* viewModel) {
    m_model = model;
    m_viewModel = viewModel;

    drawNodeViewer();
    drawAddEdgePopup();
    drawChangeIDPopup();
    drawChangeWeightPopup();
}

void NodeViewer::openAddEdgePopup() { m_addEdgePopupOpen = true; }

bool& NodeViewer::isOpen() { return m_isOpen; }

bool& NodeViewer::followNodeStateChangeAlgorithm() { return m_followNodeStateChangeAlgorithm; }

void NodeViewer::onNodeSelected(NodeIndex_t nodeIndex) {
    m_shouldScrollToNode = nodeIndex;
    m_selectedNode = nodeIndex;
}

void NodeViewer::onNodeAdded(NodeIndex_t nodeIndex) {
    m_shouldScrollToNode = nodeIndex;
    m_selectedNode = nodeIndex;
}

void NodeViewer::onNodeStateChange(NodeIndex_t nodeIndex) {
    if (!m_followNodeStateChangeAlgorithm) {
        return;
    }

    m_viewModel->centerOnNode(nodeIndex);
}

void NodeViewer::drawNodeViewer() {
    if (!m_isOpen) {
        return;
    }

    const auto nodeCount = m_model->getNodeCount();
    static bool addOnlyVisibleNodes = false;

    ImGui::Begin("Node Viewer", &m_isOpen);

    if (ImGui::BeginTable("node_viewer_settings", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        drawTextCentered("Only visible");

        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##only_visible", &addOnlyVisibleNodes);

        ImGui::EndTable();
    }

    ImGui::BeginChild("##node_viewer_child", ImVec2(0, 250), true);
    if (ImGui::BeginTable("nodes", 3,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                              ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Out Degree", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        if (addOnlyVisibleNodes) {
            const auto& visibleNodes = m_viewModel->getVisibleNodes();
            const auto& nodesPositions = m_viewModel->getVisibleNodesPositions();

            clipper.Begin((int)visibleNodes.size());

            if (m_shouldScrollToNode != INVALID_NODE) {
                const auto lookupIndexOpt = m_viewModel->getLookupIndex(m_shouldScrollToNode);
                if (lookupIndexOpt) {
                    clipper.IncludeItemByIndex(static_cast<int>(*lookupIndexOpt));
                }
            }

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto nodeIndex = visibleNodes[i];
                    if (nodeIndex == m_shouldScrollToNode) {
                        ImGui::SetScrollHereY(0.5f);
                        m_shouldScrollToNode = INVALID_NODE;
                    }

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    char label[32];
                    snprintf(label, sizeof(label), "%u", nodeIndex);

                    if (ImGui::Selectable(label, m_selectedNode == nodeIndex,
                                          ImGuiSelectableFlags_SpanAllColumns)) {
                        if (m_selectedNode == nodeIndex) {
                            m_selectedNode = INVALID_NODE;
                        } else {
                            m_selectedNode = nodeIndex;
                            m_viewModel->centerOnNode(nodeIndex);
                        }
                    }

                    ImGui::TableSetColumnIndex(1);
                    const auto degree = m_model->getNodeDegree(nodeIndex);
                    ImGui::Text("%u", degree);

                    ImGui::TableSetColumnIndex(2);
                    const auto position = nodesPositions[i];
                    ImGui::Text("(%.0f, %.0f)", position.m_x, position.m_y);
                }
            }
        } else {
            clipper.Begin((int)nodeCount);

            if (m_shouldScrollToNode != INVALID_NODE) {
                clipper.IncludeItemByIndex(static_cast<int>(m_shouldScrollToNode));
            }

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto nodeIndex = i;

                    if (nodeIndex == m_shouldScrollToNode) {
                        ImGui::SetScrollHereY(0.5f);
                        m_shouldScrollToNode = INVALID_NODE;
                    }

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    char label[32];
                    snprintf(label, sizeof(label), "%u", nodeIndex);

                    if (ImGui::Selectable(label, m_selectedNode == nodeIndex,
                                          ImGuiSelectableFlags_SpanAllColumns)) {
                        if (m_selectedNode == nodeIndex) {
                            m_selectedNode = INVALID_NODE;
                        } else {
                            m_selectedNode = nodeIndex;
                            m_viewModel->centerOnNode(nodeIndex);
                        }
                    }

                    ImGui::TableSetColumnIndex(1);
                    const auto degree = m_model->getNodeDegree(nodeIndex);
                    ImGui::Text("%u", degree);

                    ImGui::TableSetColumnIndex(2);
                    const auto position = m_model->getNode(nodeIndex)->getWorldPos();
                    ImGui::Text("(%.0f, %.0f)", position.m_x, position.m_y);
                }
            }
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();

    const auto isDisabled = (m_selectedNode == INVALID_NODE || m_selectedNode >= nodeCount);
    ImGui::BeginDisabled(isDisabled);
    ImGui::SeparatorText("Node Connections");
    ImGui::BeginChild("##node_connections_child", ImVec2(0, 0), true);

    if (ImGui::Button("Add Edge", ImVec2(-FLT_MIN, 0))) {
        m_addEdgePopupOpen = true;
    }

    if (ImGui::BeginTable(
            "edges", 2,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        if (!isDisabled) {
            const auto& edges = m_model->getNodeEdges(m_selectedNode);

            ImGuiListClipper edgeClipper;
            edgeClipper.Begin((int)edges.size());

            while (edgeClipper.Step()) {
                for (int i = edgeClipper.DisplayStart; i < edgeClipper.DisplayEnd; i++) {
                    const auto& edge = edges[i];

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    char label[32];
                    snprintf(label, sizeof(label), "%u", edge.first);

                    bool isSelected = false;
                    if (ImGui::Selectable(label, &isSelected,
                                          ImGuiSelectableFlags_SpanAllColumns)) {
                        m_viewModel->centerOnNode(edge.first);
                    }

                    if (ImGui::BeginPopupContextItem(label)) {
#if 0
                        if (ImGui::MenuItem("Change ID")) {
                            m_changeIDPopupOpen = true;
                        }

                        if (ImGui::MenuItem("Change Weight")) {
                            m_changeWeightPopupOpen = true;
                        }
#endif

                        if (ImGui::MenuItem("Remove Edge")) {
                            m_viewModel->removeEdge(m_selectedNode, edge.first);
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", edge.second);
                }
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::EndDisabled();

    ImGui::End();
}

void NodeViewer::drawAddEdgePopup() {
    if (!m_addEdgePopupOpen) {
        return;
    }

    if (m_selectedNode == INVALID_NODE || m_selectedNode >= m_model->getNodeCount()) {
        common::Logger::get().warning(
            "Attempted to open Add Edge popup with invalid selected node index.");

        m_addEdgePopupOpen = false;
        return;
    }

    ImGui::OpenPopup("Confirmation");

    const auto centerPos = ImGui::GetIO().DisplaySize * 0.5f;
    ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirmation", &m_addEdgePopupOpen)) {
        static int targetNodeId = 0;
        static int edgeWeight = 0;

        if (ImGui::IsWindowAppearing()) {
            targetNodeId =
                std::clamp(targetNodeId, 0, static_cast<int>(m_model->getLastNodeIndex()));
        }

        if (ImGui::BeginTable("AddEdgeTable", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            drawTextCentered("Target Node ID");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##targetNodeId", &targetNodeId, 0, 1000)) {
                targetNodeId =
                    std::clamp(targetNodeId, 0, static_cast<int>(m_model->getLastNodeIndex()));
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            drawTextCentered("Edge Weight");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputInt("##edgeWeight", &edgeWeight, 1, 100);

            ImGui::EndTable();
        }

        ImGui::Separator();

        const auto availableWidth = ImGui::GetContentRegionAvail().x;
        const auto itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        const auto buttonWidth = (availableWidth - itemSpacing) * 0.5f;

        if (ImGui::Button("Add", ImVec2(buttonWidth, 0))) {
            m_viewModel->addEdge(m_selectedNode, static_cast<unsigned int>(targetNodeId),
                                 edgeWeight);
            m_addEdgePopupOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetNavCursorVisible(true);
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_addEdgePopupOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void NodeViewer::drawChangeIDPopup() {
    if (!m_changeIDPopupOpen) {
        return;
    }
}

void NodeViewer::drawChangeWeightPopup() {
    if (!m_changeWeightPopupOpen) {
        return;
    }
}

void NodeViewer::drawTextCentered(const char* fmt, ...) {
    float textHeight = ImGui::GetTextLineHeight();
    float sliderHeight = ImGui::GetFrameHeight();
    float offsetY = (sliderHeight - textHeight) * 0.5f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

    char text[256];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    ImGui::TextWrapped("%s", text);
}
