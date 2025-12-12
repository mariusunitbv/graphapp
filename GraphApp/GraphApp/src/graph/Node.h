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

    void setSelected(bool selected, uint32_t selectTime);
    bool isSelected() const;
    uint32_t getSelectTime() const;

   private:
    NodeIndex_t m_index{INVALID_NODE};
    QRgb m_fillColor{};

    QPoint m_position{};
    QString m_label{};

    bool m_selected{false};
    uint32_t m_selectTime{0};

   public:
    static constexpr auto k_radius{24};
};
