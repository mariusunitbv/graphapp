#pragma once

struct AlgorithmLabel {
    AlgorithmLabel() = default;
    explicit AlgorithmLabel(QGraphicsScene* scene, QPointF pos,
                            std::function<void(QGraphicsSimpleTextItem*)> computeFunc);
    ~AlgorithmLabel();

    AlgorithmLabel(const AlgorithmLabel&) = delete;
    AlgorithmLabel& operator=(const AlgorithmLabel&) = delete;

    AlgorithmLabel(AlgorithmLabel&& other);
    AlgorithmLabel& operator=(AlgorithmLabel&& other);

    void compute() const;

   private:
    QGraphicsScene* m_scene;
    QGraphicsSimpleTextItem* m_label;
    std::function<void(QGraphicsSimpleTextItem*)> m_compute;
};
