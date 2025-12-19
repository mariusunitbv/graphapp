#pragma once

using NodeIndex_t = uint32_t;
constexpr auto INVALID_NODE = std::numeric_limits<NodeIndex_t>::max();

class NodeData {
   public:
    NodeData(size_t index, const QPoint& position);

    QRect getBoundingRect() const;

    void setIndex(NodeIndex_t index);
    NodeIndex_t getIndex() const;

    void setFillColor(QRgb c);
    QColor getFillColor() const;

    void setLabel(const QString& label);
    const QString& getLabel() const;

    void setPosition(const QPoint& position);
    QPoint getPosition() const;

    void select(uint32_t selectOrder);
    void deselect();
    bool isSelected() const;
    uint32_t getSelectOrder() const;

   private:
    NodeIndex_t m_index{INVALID_NODE};
    QRgb m_fillColor{};

    QPoint m_position{};
    QString m_label{};

    uint32_t m_selectOrder = -1;

   public:
    static constexpr auto k_radius{28};
};
