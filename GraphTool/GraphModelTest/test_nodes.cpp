#include "pch.h"

import graph_model;

TEST(GraphModel, AddNode) {
    GraphModel graph;
    graph.addNode({100.f, 200.f});
    graph.addNode({300.f, 400.f});

    const auto* node1 = graph.getNode(0);
    const auto* node2 = graph.getNode(1);

    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);

    EXPECT_EQ(node1->getWorldPos().m_x, 100.f);
    EXPECT_EQ(node1->getWorldPos().m_y, 200.f);
    EXPECT_EQ(node2->getWorldPos().m_x, 300.f);
    EXPECT_EQ(node2->getWorldPos().m_y, 400.f);

    EXPECT_THROW(graph.addNode({150.f, WORLD_BOUNDS_FIXED_SIZE}), std::runtime_error);

    const auto graphBounds = graph.getGraphBounds();
    EXPECT_EQ(graphBounds.valid(), true);
}
