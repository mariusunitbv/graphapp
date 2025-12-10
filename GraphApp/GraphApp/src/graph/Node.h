#pragma once

using NodeIndex_t = uint32_t;
constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max();

class NodeData {
   public:
    friend class GraphManager;

    NodeData(size_t index, const QPoint& position);

    const QRect& getBoundingRect() const;

    void setIndex(NodeIndex_t index);
    NodeIndex_t getIndex() const;

    void setFillColor(QRgb c);
    QColor getFillColor() const;

    void setPosition(const QPoint& position);
    QPoint getPosition() const;

   private:
    NodeIndex_t m_index{INVALID_NODE};
    QRgb m_fillColor{qRgb(255, 255, 255)};

    QRect m_boundingRect{};
    QString m_label{};

    bool m_selected{false};

   public:
    static constexpr float k_radius{24.f};
};
